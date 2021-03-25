#define NES_MIRRORING 1

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"

#include "config.h"
#include "enemy.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"
unsigned char ground_info[4];

bool on_ground(px, py, cx, cy) {
  
  row = (py + 16 + cy)>>3;
  col = (px + cx)>>3;
  byte_col = col>>3;
  bit = (col&0x07);
  if (collisions[row<<2 + byte_col] & (0x80>>bit)) {
    return true;
  }
  return false;
}

// main function, run after console reset
void main(void) {
  
  // player vars
  unsigned char p_x = 30, p_y = 160;
  int i;
  int dir = 0;
  int speed = 1;
  int idle_dir = 1;
  unsigned char attrib = 0x00;
  int health = 10;
  
  // camera vars
  int screen_x = -8, screen_y = 0;
  int right_side = 0, left_side = 1;
  int is_player_control = 1;
  
  int anim_state = 0;
  
  // bullet vars
  int fire_cooldown = 20;
  int cur_cooldown = 0;
  bool can_fire = true;
  
  // jumping vars
  int min_accel = 5;
  int accel = -min_accel;
  bool jumping;
  bool accel_this_frame = true;
  
  //unsigned char row, col, byte_col, bit;
  
  
  // enemies
  int num_enemies = 2;  
  struct Enemy enemies[2];
  
  for (i = 0; i < num_enemies; i++) {
    enemies[i].xpos = enemy_spawns[i][0];
    enemies[i].ypos = enemy_spawns[i][1];
    enemies[i].health = 10;
    enemies[i].dir = -1;
    enemies[i].move_radius = 30;
  }
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)
  
  vram_adr(NTADR_A(1, 1));
  vram_write("HEALTH: ", 8);
  for ( i = 0; i < health; i++) {
  	vram_put(0x15);
  }
  
  vram_adr(NTADR_A(0,4)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapA); 
  
  vram_adr(NTADR_B(0,4)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapB); 
  
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
    
      cur_oam = oam_spr(1, 32, 0x2e, 0x00, 0);
	
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
          if( can_fire ) {
            spawn_bullet(p_x, p_y, idle_dir);
            cur_cooldown = fire_cooldown;
            can_fire = false;
            // this is to cause damage
            health -= 1;
            vram_adr(NTADR_A(9+health, 1));
            vram_put(0x00);
            
          }  
      }
      if (pad_result&(0x01<<4)){	// jump
        jumping = true;
      }
      // UPDATE
      if (p_x >= 232) {
          speed = 1;
          p_x = 232;
      } else if (p_x <= 9) {
          speed = 1;
          p_x = 9;
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
          if (screen_x <= -7) {
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
      if (active_bullet()) {
        for (i = 0; i < NUM_BULLETS; i++) {
          if (bullets[i].in_use) {
            bullets[i].xpos += bullets[i].dir * bullets[i].speed;
            bullets[i].lifetime -= 1;
            if (bullets[i].lifetime <= 0){
              bullets[i].in_use = false;
              break;
            }
          }
        }
      }
      
      // bullet fire cooldown
      if (!can_fire) {
        cur_cooldown -= 1;
        if (cur_cooldown <= 0) {
          can_fire = true;
        }
      }
      // updates the enemies
      for (i = 0; i < num_enemies; i++) {
        enemies[i].xpos += enemies[i].dir;
        if (enemies[i].xpos < enemy_spawns[i][0] - enemies[i].move_radius){
          enemies[i].dir  = 1;
        } else if (enemies[i].xpos > enemy_spawns[i][0] + enemies[i].move_radius) {
          enemies[i].dir = -1;
        }
      }
    
      // JUMPING UPDATE	
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
      
      // player falling update
      ground_info[0] = (p_y + 16 - 48 + screen_y)>>3;	//divide world 
      ground_info[1] = (p_x + screen_x)>>3;
      ground_info[2] = ground_info[1]>>3;
      ground_info[3] = (ground_info[1]&0x07);
      if (collisions[ground_info[0]<<2 + ground_info[2]] & (0x80>>ground_info[3])) {
      //if (collisions[90] & (0x80)) {
        //do nothing;
      } else {
        p_y += 1;
      }
      //if (!on_ground(p_x, p_y, screen_x, screen_y)) {
      //  p_y += 1;
      //}

      // DRAW
      // draws the player
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
      // draws the bullets
      if (active_bullet()) {
        for (i = 0; i < NUM_BULLETS; i++) {
          if (bullets[i].in_use) {
            cur_oam = oam_spr(bullets[i].xpos+4, bullets[i].ypos+4, 0xb6, 0x01, cur_oam);
          }
        }
      }
      // draws the enemies
      for (i = 0; i < num_enemies; i++) {	// display enemies
        if (enemies[i].xpos-screen_x+4 > 0 && enemies[i].xpos-screen_x-4 < 256)
          cur_oam = oam_meta_spr(enemies[i].xpos-screen_x, enemies[i].ypos, cur_oam, enemy_left);
      }
      oam_hide_rest(cur_oam);
      split(screen_x, screen_y);
      //vrambuf_flush();
  }
}

