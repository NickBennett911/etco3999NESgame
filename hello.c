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
  
  //bullet vars
  bool b_in_use = false;
  int b_x = 0, b_y = 0;
  int lifetime = 25;
  int cur_lifetime = 0;
  int b_dir = 0;
  
  // jumping
  int min_accel = 5;
  int accel = -min_accel;
  bool jumping;
  bool accel_this_frame = true;
  
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
  
  init_bullet_list();
  // infinite loop
  while (1) 
  {
      char cur_oam = 0;
      char pad_result = pad_poll(0);
	
      // INPUT
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
      if (pad_result&(0x01<<0)) {	// space bar
        if (b_in_use == false) {
          b_x = p_x;
          b_y = p_y;
          b_in_use = true;
          b_dir = idle_dir;
        }
      }
      if (pad_result&(0x01<<4)){	// jump
        jumping = true;
      }
      // UPDATE
      if (p_x >= 232) {
          speed = 1;
          p_x = 232;
      } else if (p_x <= 8) {
          speed = 1;
          p_x = 8;
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

      // BULLET UPDATE
      if (b_in_use) {
        b_x += b_dir * 4;
        cur_lifetime += 1;
        if (cur_lifetime > lifetime) {
          cur_lifetime = 0;
          b_y = -50;
          b_in_use = false;
        } else if ( b_x >= 256 || b_x <= 0)
          b_in_use = false;
      }
    
      if (jumping) {
        p_y += accel;
        if (accel_this_frame){
          accel++;
          if (accel >= min_accel) {
            accel = -min_accel;
            jumping = false;
            p_y = 160;
          } 
          accel_this_frame = false;
        } else {
          accel_this_frame = true;
        }
      }

      // DRAW
      if (dir == 1) {		// moving right
        if (jumping) 
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_right);
         else
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right[anim_state%5]);
      } else  if (dir == -1) {	// moving left
        if (jumping)
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_left);
          else
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_left[anim_state%5]);

      } else{				// idling
          if (idle_dir == 1) {		// facing right
            if (jumping) 
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_right);
            else
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right[anim_state%5]);
          } else if (idle_dir == -1) {	// facing left
            if (jumping)
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_left);
            else
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_left[anim_state%5]);
          }
      }
      if (b_in_use) {
        cur_oam = oam_spr(b_x+4, b_y+4, 0xb6, 0x01, cur_oam);
      }
      oam_hide_rest(cur_oam);
      scroll(screen_x, screen_y);
      vrambuf_flush();
    
  }
}

