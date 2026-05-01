#pragma once
void initKeyboard();
void keyboardHandler(struct InterruptRegisters *regs);
void append(char *part);
void setLineStart();
int kbd_read_line(char* buf, uint32_t max_len);
