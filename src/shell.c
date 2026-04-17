#include "shell.h"
#include "vga.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "keyboard.h"

static const char* help_text =
	"Commands:\n"
	"  help		- show this message\n"
	"  clr		- clear the screen\n"
	"  echo[text]   - print text\n";

static const char* skip_spaces(const char* s)
{
	while(*s == ' ') s++;
	return s;
}
static uint32_t word_len(const char* s)
{
	uint32_t n = 0;
	while(*s && *s != ' ')
	{
		s++;
		n++;
	}
	return n;
}

void shell_exec(const char* input)
{
	input = skip_spaces(input);

	if(*input == '\0')
		return;

	uint32_t cmd_len = word_len(input);
	
	if(k_strncmp(input, "help", cmd_len) == 0 && cmd_len == 4)
	{
		print("\n");
		print(help_text);
	}
	else if (k_strncmp(input, "clr", cmd_len) == 0 && cmd_len == 3)
	{
		Reset();
	}
	else if(k_strncmp(input, "echo", cmd_len) == 0 && cmd_len == 4)
	{
		const char* arg = skip_spaces(input + 4);
		print("\n");
		if(*arg != '\0')
			print(arg);
		print("\n");
	}
	else
	{
		print("\nunknown command: ");
		print(input);
		print("\n");
	}
}
