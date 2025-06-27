#include "vga.h"

void kernel_main(void);

void kernel_main(void)
{
    set_screen_color(0x0F);
    print("---------------codeshell----------------\n");
    print("login:\n");

}
