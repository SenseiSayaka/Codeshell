#include "vga_font.h"
#include "util.h"
#define VGA_FONT_BUFFER 0xA0000
void vga_load_font(const uint8_t* font) {
    uint8_t seq2,seq4,gc4,gc5,gc6;
    //save current values of registers
    outPortB(0x3C4,0x02);seq2=inPortB(0x3C5);
    outPortB(0x3C4,0x04);seq4=inPortB(0x3C5);
    outPortB(0x3CE,0x04);gc4=inPortB(0x3CF);
    outPortB(0x3CE,0x05);gc5=inPortB(0x3CF);
    outPortB(0x3CE,0x06);gc6=inPortB(0x3CF);

    //switch to plane 2
    outPortB(0x3C4,0x02);outPortB(0x3C5,0x04);
    outPortB(0x3C4,0x04);outPortB(0x3C5,0x06);
    outPortB(0x3CE,0x04);outPortB(0x3CF,0x02);
    outPortB(0x3CE,0x05);outPortB(0x3CF,0x00);
    outPortB(0x3CE,0x06);outPortB(0x3CF,0x04);
    uint8_t* dst=(uint8_t*)VGA_FONT_BUFFER;
    for (int i=0;i<256;i++) {
        for (int j=0;j<16;j++) {
            dst[i*32+j]=font[i*16+j];
        }
    }//restore registers
    outPortB(0x3C4,0x02);outPortB(0x3C5,seq2);
    outPortB(0x3C4,0x04);outPortB(0x3C5,seq4);
    outPortB(0x3CE,0x04);outPortB(0x3CF,gc4);
    outPortB(0x3CE,0x05);outPortB(0x3CF,gc5);
    outPortB(0x3CE,0x06);outPortB(0x3CF,gc6);
}
