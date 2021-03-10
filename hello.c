#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"


// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"

///// METASPRITES

#define TILE 0xd8
#define ATTR 0x01
#define ATTR2 0x61

// define a 2x2 metasprite
const unsigned char right[]={
        0,      0,      TILE+0,   ATTR, 
        0,      8,      TILE+1,   ATTR, 
        8,      0,      TILE+2,   ATTR, 
        8,      8,      TILE+3,   ATTR, 
        128};

const unsigned char left[]={
        8,      0,      TILE+0,   ATTR2, 
        8,      8,      TILE+1,   ATTR2, 
        0,      0,      TILE+2,   ATTR2, 
        0,      8,      TILE+3,   ATTR2, 
        128};

const unsigned char door[]={
        0,      0,      0xc4+0,   ATTR, 
        0,      8,      0xc4+1,   ATTR, 
        8,      0,      0xc4+2,   ATTR, 
        8,      8,      0xc4+3,   ATTR, 
        128};

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
  vram_adr(NTADR_A(1,1));		// set address
  vram_write("This is", 7);	// write bytes to video RAM
  vram_adr(NTADR_A(1,2));
  vram_write("Nick Bennett's", 14);
  vram_adr(NTADR_A(1,3));
  vram_write("first NES 'Game'!", 17);
  
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
    if (x >= 232) {
      dir -= 2;
      attrib |= 0x60;	// makes bit 6 one to flip
    } else if (x <= 8) {
      dir += 2;
      attrib &= 0x00;	// makes bit 6 zero to flip
    }
    
    if (dir > 0) {
      cur_oam = oam_meta_spr(x, 174, cur_oam, right);
    } else {
      cur_oam = oam_meta_spr(x, 174, cur_oam, left);
    }
    cur_oam = oam_meta_spr(232, 174, cur_oam, door);
    if (x > 216) {
      
    }
    
    ppu_wait_frame();
  }
}
