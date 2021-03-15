#define NES_MIRRORING 1

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"
#include "config.h"


// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"









// main function, run after console reset
void main(void) {

  unsigned char p_x = 30, p_y = 160;
  int i;
  int dir = 0;
  int speed = 1;
  int idle_dir = 1;
  unsigned char attrib = 0x00;
  
  int screen_x = -8, screen_y = 0;
  int right_side = 0, left_side = 1;
  int is_player_control = 1;
  
  int anim_state = 0;
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)
  
  vram_adr(NTADR_A(1, 1));
  vram_write("HEALTH: ", 8);
  for ( i = 0; i < 10; i++) {
  	vram_put(0x15);
  }
  
  vram_adr(NTADR_A(0,4)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapA); // my map01 is an array of 274 unsigned char's
  
  vram_adr(NTADR_B(0,4)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapB); // my map01 is an array of 274 unsigned char's
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  vrambuf_clear();
  set_vram_update((byte*)0x100); // updbuf = 0x100 -- start of stack RAM
  
  // infinite loop
  while (1) 
  {
    char cur_oam = 0;
    char pad_result = pad_poll(0);
    
    
    if(pad_result&(0x01<<6)){	// left
    	dir = -1;
    	idle_dir = -1;
      	anim_state++;
    } else if (pad_result&(0x01<<7)) { 	// right
    	dir = 1;
      	idle_dir = 1;
        anim_state++;
    } else {
      dir = 0;
      anim_state = 0;
    }
    
    if (p_x >= 232) {
        speed = 1;
      	p_x = 232;
      //attrib |= 0x60;	// makes bit 6 one to flip
    } else if (p_x <= 8) {
        speed = 1;
      	p_x = 8;
      //attrib &= 0x00;	// makes bit 6 zero to flip
    }
    if (is_player_control) {
    	p_x += speed * dir;
       
      	if (p_x >= 128 && left_side) {
        	is_player_control = 0;
          	left_side = 0;
        }
        if (p_x <= 128 && right_side) {
        	is_player_control = 0;
          	right_side = 0;
        }
    } else {
    	screen_x += speed * dir;
      	if (screen_x <= -8) {
        	is_player_control = 1;
          	left_side = 1;
          	right_side = 0;
        } else if (screen_x >= 264) {
        	is_player_control = 1;
          	right_side = 1;
          	left_side = 0;
        }
    }
    
    
    
    if (dir == 1) {
      cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right[anim_state%5]);
    } else  if (dir == -1) {
      cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_left[anim_state%5]);

    } else{
    	if (idle_dir == 1) {
          cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right[anim_state%5]);
        } else if (idle_dir == -1) {
          cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_left[anim_state%5]);
        }
    }
    scroll(screen_x, screen_y);
    vrambuf_flush();
    
  }
}
