
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"


// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"

int test_var = 69;

const char PALETTE[32] =
{
  0x03, // screen color
  0x24, 0x16, 0x20, 0x0, // background palette 0
  0x1c, 0x20, 0x2c, 0x0, // background palette 1
  0x00, 0x1a, 0x20, 0x0, // background palette 2
  0x06, 0x03, 0x06, 0x0, // background palette 3
  0x23, 0x31, 0x06, 0x0, // sprite palette 0
  0x00, 0x37, 0x25, 0x0, // sprite palette 1
  0x36, 0x21, 0x19, 0x0, // sprite palette 2
  0x1d, 0x37, 0x2b, // sprite palette 3
};

// main function, run after console reset
void main(void) {

  unsigned char x = 30;
  int i;
  int dir = 1;
  unsigned char attrib = 0x00;
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)

  // write text to name table
  vram_adr(NTADR_A(2,2));		// set address
  vram_write("HELLO, W\x19RLD!", 13);	// write bytes to video RAM
  
  vram_adr(NTADR_A(1,24));
  for (i = 0; i < 30; i++) {
    vram_put(0x0f);
  }
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  vrambuf_clear();
  set_vram_update((byte*)0x100); // updbuf = 0x100 -- start of stack RAM
  
  // infinite loop
  while (1) 
  {
    char cur_oam = 0;
    x += dir;
    if (x >= 240) {
      dir -= 2;
      attrib |= 0x60;	// makes bit 6 one to flip
    } else if (x <= 8) {
      dir += 2;
      attrib &= 0x00;	// makes bit 6 zero to flip
    }
    cur_oam = oam_spr(x, 180, 0x19, attrib, cur_oam);
    ppu_wait_frame();
  }
}
