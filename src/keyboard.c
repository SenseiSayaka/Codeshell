#include "stdint.h"
#include "util.h"
#include "idt.h"
#include "stdlib/stdio.h"
#include "keyboard.h"
#include "vga.h"
#include "shell.h"
#include "fat12.h"
bool capsOn;
bool capsLock;
static bool extendedKey = false;

// --- buffer of the current input  ---
#define INPUT_BUF_SIZE 256
static char     inputBuf[INPUT_BUF_SIZE];
static uint16_t inputLen = 0;   
static uint16_t inputPos = 0;   

// pos on the screen after "csh>"
static uint16_t startCol  = 0;
static uint16_t startLine = 0;
void setLineStart() {
    
    extern uint16_t column;
    extern uint16_t line;
    startCol  = column;
    startLine = line;
    inputLen  = 0;
    inputPos  = 0;
}
// command history
#define HISTORY_SIZE 20
static char history[HISTORY_SIZE][INPUT_BUF_SIZE];
static int history_count = 0;
static int history_pos = -1;
static char saved_input[INPUT_BUF_SIZE];
static uint16_t saved_len = 0;


static void bufToScreen(uint16_t pos, uint16_t *col, uint16_t *ln) {
    uint16_t abs = startCol + pos;
    *ln  = startLine + abs / width;
    *col = abs % width;
}

// перерисовать буфер начиная с позиции from и стереть символ после конца
static void redrawFrom(uint16_t from) {
    uint16_t col, ln;
    for (uint16_t i = from; i < inputLen; i++) {
        bufToScreen(i, &col, &ln);
        putCharAt(inputBuf[i], col, ln);
    }
    // стереть символ, который был в конце (после удаления)
    bufToScreen(inputLen, &col, &ln);
    putCharAt(' ', col, ln);
}
static void history_push(const char* cmd)
{
	if(!cmd || cmd[0] == '\0') return;

	if (history_count > 0 &&
			k_strcmp(history[(history_count - 1) % HISTORY_SIZE], cmd) == 0)
			return;
	uint32_t idx = history_count % HISTORY_SIZE;
	uint32_t i = 0;
	while(cmd[i] && i < INPUT_BUF_SIZE - 1)
	{
		history[idx][i] = cmd[i];
		i++;
	}
	history[idx][i] = '\0';
	history_count++;
}
static void load_history_entry(const char* entry)
{
	uint16_t col, ln;
	uint16_t clear_pos = inputLen;
	// erase current line on the screen
	while(clear_pos > 0)
	{
		clear_pos--;
		bufToScreen(clear_pos, &col, &ln);
		putCharAt(' ', col, ln);
	}

	uint32_t i = 0;
	while(entry[i] && i < INPUT_BUF_SIZE - 1)
	{
		inputBuf[i] = entry[i];
		i++;
	}
	inputLen = i;
	inputPos = i;
	redrawFrom(0);
	bufToScreen(inputPos, &col, &ln);
	setCursorPos(col, ln);
}
static const char* commands[]={
	"info", "clr", "echo", "meminfo", "fsinfo", "ls", "cat", 0
};

static int find_completions(const char* prefix, uint32_t prefix_len,
		char matches[][64], int max_matches){
	int count = 0;
	for(int i = 0; commands[i] != 0 && count < max_matches; i++){
		if(k_strncmp(commands[i], prefix, prefix_len) == 0){
			uint32_t j = 0;
			while(commands[i][j]){
				matches[count][j] = commands[i][j];
				j++;
			}
			matches[count][j] = '\0';
			count++;
		}
	}
	Fat12File* files = (Fat12File*) k_malloc(sizeof(Fat12File) * FAT12_MAX_FILES);
	if(files)
	{
		int file_count = fat12_ls(files, FAT12_MAX_FILES);
		for(int i = 0; i<file_count && count < max_matches; i++){
			if(k_strncmp(files[i].name, prefix, prefix_len) == 0){
				uint32_t j = 0;
				while(files[i].name[j]){
					matches[count][j] = files[i].name[j];
					j++;
				}
				matches[count][j] = '\0';
				count++;
			}
		}
		k_free(files);
	}
	return count;
}

static void handle_tab(){
	if (inputLen == 0) return;

	int word_start = 0;
	for (int i = 0; i < (int)inputPos; i++){
		if(inputBuf[i] == ' ') word_start = i + 1;
	}
	uint32_t prefix_len = inputPos - word_start;
	char prefix[INPUT_BUF_SIZE];
	for(uint32_t i = 0; i < prefix_len; i++)
		prefix[i] = inputBuf[word_start + i];
	prefix[prefix_len] = '\0';

	char matches[16][64];
	int count = find_completions(prefix, prefix_len, matches, 16);
	if(count == 0){
		// no matches - nothing to do
		return;
	}else if (count == 1){
		uint16_t c, ln;
		while(inputPos > (uint16_t)word_start){
			inputPos--;
			bufToScreen(inputPos, &c, &ln);
			putCharAt(' ', c, ln);
		}
		inputLen = word_start;

		// put matches
		uint32_t i = 0;
		while(matches[0][i] && inputLen < INPUT_BUF_SIZE - 1){
			inputBuf[inputLen++] = matches[0][i++];
		}
		if(inputLen < INPUT_BUF_SIZE - 1)
			inputBuf[inputLen++] = ' ';
		inputPos = inputLen;
		redrawFrom(word_start);
		bufToScreen(inputPos, &c, &ln);
		setCursorPos(c, ln);
	}else{
		print("\n");
		for(int i = 0; i < count; i++){
			print(" ");
			print(matches[i]);
		}
		print("\ncsh>");
		uint16_t save_len = inputLen;
		uint16_t save_pos = inputPos;
		inputBuf[inputLen] = '\0';
		setLineStart();
		inputLen = save_len;
		inputPos = save_pos;
		redrawFrom(0);
		uint16_t c, ln;
		bufToScreen(inputPos, &c, &ln);
		setCursorPos(c, ln);
	}
}

// --- scancodes / keymaps ---

const uint32_t UNKNOWN = 0xFFFFFFFF;
const uint32_t ESC     = 0xFFFFFFFF - 1;
const uint32_t CTRL    = 0xFFFFFFFF - 2;
const uint32_t LSHFT   = 0xFFFFFFFF - 3;
const uint32_t RSHFT   = 0xFFFFFFFF - 4;
const uint32_t ALT     = 0xFFFFFFFF - 5;
const uint32_t F1      = 0xFFFFFFFF - 6;
const uint32_t F2      = 0xFFFFFFFF - 7;
const uint32_t F3      = 0xFFFFFFFF - 8;
const uint32_t F4      = 0xFFFFFFFF - 9;
const uint32_t F5      = 0xFFFFFFFF - 10;
const uint32_t F6      = 0xFFFFFFFF - 11;
const uint32_t F7      = 0xFFFFFFFF - 12;
const uint32_t F8      = 0xFFFFFFFF - 13;
const uint32_t F9      = 0xFFFFFFFF - 14;
const uint32_t F10     = 0xFFFFFFFF - 15;
const uint32_t F11     = 0xFFFFFFFF - 16;
const uint32_t F12     = 0xFFFFFFFF - 17;
const uint32_t SCRLCK  = 0xFFFFFFFF - 18;
const uint32_t HOME    = 0xFFFFFFFF - 19;
const uint32_t UP      = 0xFFFFFFFF - 20;
const uint32_t LEFT    = 0xFFFFFFFF - 21;
const uint32_t RIGHT   = 0xFFFFFFFF - 22;
const uint32_t DOWN    = 0xFFFFFFFF - 23;
const uint32_t PGUP    = 0xFFFFFFFF - 24;
const uint32_t PGDOWN  = 0xFFFFFFFF - 25;
const uint32_t END     = 0xFFFFFFFF - 26;
const uint32_t INS     = 0xFFFFFFFF - 27;
const uint32_t DEL     = 0xFFFFFFFF - 28;
const uint32_t CAPS    = 0xFFFFFFFF - 29;
const uint32_t NONE    = 0xFFFFFFFF - 30;
const uint32_t ALTGR   = 0xFFFFFFFF - 31;
const uint32_t NUMLCK  = 0xFFFFFFFF - 32;

const uint32_t lowercase[128] = {
    UNKNOWN,ESC,'1','2','3','4','5','6','7','8',
    '9','0','-','=','\b','\t','q','w','e','r',
    't','y','u','i','o','p','[',']','\n',CTRL,
    'a','s','d','f','g','h','j','k','l',';',
    '\'','`',LSHFT,'\\','z','x','c','v','b','n','m',',',
    '.','/',RSHFT,'*',ALT,' ',CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,
    NUMLCK,SCRLCK,HOME,UP,PGUP,'-',LEFT,UNKNOWN,RIGHT,'+',
    END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

const uint32_t uppercase[128] = {
    UNKNOWN,ESC,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',CTRL,
    'A','S','D','F','G','H','J','K','L',':','"','~',LSHFT,'|',
    'Z','X','C','V','B','N','M','<','>','?',RSHFT,'*',ALT,' ',
    CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,NUMLCK,SCRLCK,HOME,UP,PGUP,'-',
    LEFT,UNKNOWN,RIGHT,'+',END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
    UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

// --- handler ---

void keyboardHandler(struct InterruptRegisters *regs){
    uint8_t raw = inPortB(0x60);

    if (raw == 0xE0) {
        extendedKey = true;
        return;
    }

    char     press    = raw & 0x80;
    char     scanCode = raw & 0x7F;
    uint16_t col, ln;

    if (extendedKey) {
        extendedKey = false;
        if (press == 0) {
            switch (scanCode) {
                case 0x4B:  // left arrow
                    if (inputPos > 0) {
                        inputPos--;
                        bufToScreen(inputPos, &col, &ln);
                        setCursorPos(col, ln);
                    }
                    return;
                case 0x4D:  // right arrow
                    if (inputPos < inputLen) {
                        inputPos++;
                        bufToScreen(inputPos, &col, &ln);
                        setCursorPos(col, ln);
                    }
                    return;
			case 0x48:
				if (history_pos == -1)
				{
					uint32_t i = 0;
					while(i < inputLen) {saved_input[i] = inputBuf[i]; i++;}
					saved_input[i] = '\0';
					saved_len = inputLen;
					history_pos = history_count;
				}

				if (history_pos > 0 && history_pos > (int) history_count - HISTORY_SIZE)
				{
					history_pos--;
					load_history_entry(history[history_pos % HISTORY_SIZE]);
				}
				return;
			case 0x50:
			    if (history_pos == -1) return;

	            history_pos++;
	            if (history_pos >= history_count) {
	            
	            history_pos = -1;
	            load_history_entry(saved_input);
	            } else {
	            	load_history_entry(history[history_pos % HISTORY_SIZE]);
	            }
	            return;
			}
		}				              
        return;
    }

    switch (scanCode) {
        case 1:
        case 29:
        case 56:
        case 59: case 60: case 61: case 62: case 63:
        case 64: case 65: case 66: case 67: case 68:
        case 87: case 88:
            break;
        case 42:
            capsOn = (press == 0);
            break;
        case 58:
            if (press == 0) capsLock = !capsLock;
            break;
        default:
            if (press != 0) break;
            uint32_t key = (capsOn || capsLock) ? uppercase[scanCode] : lowercase[scanCode];
	    if(key=='\t'){
		    handle_tab();
		    break;
	    }
            if (key == '\n') {
                // drop buffer and print new prompt
                inputBuf[inputLen] = '\0';
                history_push(inputBuf);
		history_pos = -1;
		bool is_clear = (k_strcmp(inputBuf, "clr") == 0);
		shell_exec(inputBuf);
		
		
		inputLen = 0;
                inputPos = 0;
              	if(is_clear)
		{
			print("csh>");
		}
		else
		{
			print("csh>");
		}
                setLineStart();

            } else if (key == '\b') {
                // backspace — delete char right 
                if (inputPos > 0) {
                    for (uint16_t i = inputPos - 1; i < inputLen - 1; i++)
                        inputBuf[i] = inputBuf[i + 1];
                    inputLen--;
                    inputPos--;
                    redrawFrom(inputPos);
                    bufToScreen(inputPos, &col, &ln);
                    setCursorPos(col, ln);
                }

            } else if (key != UNKNOWN && key <= 0x7F) {
                
                if (inputLen < INPUT_BUF_SIZE - 1) {
                    for (uint16_t i = inputLen; i > inputPos; i--)
                        inputBuf[i] = inputBuf[i - 1];
                    inputBuf[inputPos] = (char)key;
                    inputLen++;
                    inputPos++;
                    redrawFrom(inputPos - 1);
                    bufToScreen(inputPos, &col, &ln);
                    setCursorPos(col, ln);
                }
            }
    }
}


void initKeyboard(){
    capsOn      = false;
    capsLock    = false;
    extendedKey = false;
    irq_install_handler(1, &keyboardHandler);
}
