#define NES_MIRRORING 1

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"
#include "apu.h"

#include "config.h"
#include "enemy.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"
//#link "apu.c"
unsigned char ground_info[4];

bool is_colliding(Bullet* b, Enemy* e) {
  int bx = b->xpos;
  int by = b->ypos;
  int ex = e->xpos;
  int ey = e->ypos;
  int xdiff = ex - bx;
  int ydiff = ey - by;
  if (xdiff > -8 && xdiff < 8) {
    if (ydiff > -8 && ydiff < 8) {
      return true;
    }
  }
  return false;
}

// main function, run after console reset
void main(void) {
  
  // player vars
  unsigned char p_x = 30, p_y = 100;
  int i, j;
  int dir = 1;
  int speed = 1;
  int idle_dir = 1;
  unsigned char attrib = 0x00;
  int health = 10;
  int fall_accel_timer = 5;	
  int max_accel = 5;
  
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
  bool flapping;
  bool can_flap = true;
  bool accel_this_frame = true;
  int flap_cooldown = 30;
  int cur_flap_cooldown = 0;
  
  //enemy vars
  int enemy_spawn_cooldown = 25;
  int cur_enemy_spawn_cooldown = enemy_spawn_cooldown;
  
  //unsigned char row, col, byte_col, bit;
  
  //apu_state |= ENABLE_PULSE1; // ENABLE_PULSE0, ENABLE_TRIANGLE, ENABLE_NOISE, ENABLE_DMC
  APU_ENABLE(ENABLE_PULSE0);
  // enemies
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)
  
  vram_adr(NTADR_A(1, 1));
  vram_write("HEALTH: ", 8);
  for ( i = 0; i < health; i++) {
  	vram_put(0x15);
  }
  
  vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapA); 
  
  vram_adr(NTADR_B(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapB); 
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  vrambuf_clear();
  set_vram_update((byte*)0x100); // updbuf = 0x100 -- start of stack RAM
  
  init_bullet_list();
  init_enemies();
  // infinite loop
  while (1) 
  {
      char cur_oam = 0;
      char pad_result = pad_poll(0);
    
      cur_oam = oam_spr(1, 32, 0x2e, 0x00, 0);
	
      // INPUT
      //if(pad_result&(0x01<<6)){	// left
      //    dir = -1;
      //    idle_dir = -1;
      //    anim_state++;
      //} else if (pad_result&(0x01<<7)) { 	// right
      //    dir = 1;
      //    idle_dir = 1;
      //    anim_state++;
      //} else {
      //  dir = 0;
      //  anim_state = 0;
      //}
      if (pad_result&(0x01<<0)) {	// space bar
          if( can_fire ) {
            spawn_bullet(p_x, p_y, idle_dir);
            cur_cooldown = fire_cooldown;
            can_fire = false;
            // Plays a pulse which lasts a finite amt of time and stops
            //unsigned char channel = PULSE_CH0; // or PULSE_CH1
            //int period = 2000; // pitch, high value = lower pitch (0-2047)
            //unsigned char duty = DUTY_25; // or DUTY_12, DUTY_50, DUTY_75
            //unsigned char fade_time = 15; // fade out: 0=fast, 15=slow (in 240Hz frames?)
            //unsigned char length = 30; // time of sound (0-31)
            APU_PULSE_DECAY(PULSE_CH0, 2000, DUTY_25, 15, 30);
          }  
      }
      if (pad_result&(0x01<<4)){	// jump
        if (can_flap) {
          flapping = true;
          can_flap = false;
          cur_flap_cooldown = flap_cooldown;
          accel = -min_accel;
          spawn_enemy();
        }
      }
      // UPDATE
      cur_flap_cooldown -= 1;
      if (cur_flap_cooldown <= 0){
      	can_flap = true;
      }

      update_enemies();
    
      // BULLET COLLISION WITH ENEMIES
      if (active_bullet() && active_enemie()) {
      	for (i = 0; i<NUM_BULLETS; i++) {
          if (bullets[i].in_use ) {
            for (j = 0; j<NUM_ENEMIES; j++) {
              if (enemies[j].in_use ) {
            	if (is_colliding(&bullets[i], &enemies[j])) {
                  bullets[i].in_use = false;
                  enemies[j].in_use = false;
                }
              }
            }
          }
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
      
    
      // flapping UPDATE	
      if (flapping) {
        p_y += accel;
        if (accel_this_frame){
          accel++;
          if (accel >= 1) {
            flapping = false;
            fall_accel_timer = 5;	
            max_accel = 5;
          } 
          accel_this_frame = false;
        } else {
          accel_this_frame = true;
        }
      }    
      
      if (!flapping) {
        fall_accel_timer -= 1;
        if (fall_accel_timer <= 0) {
          p_y += 1;
          max_accel -= 1;
          if (max_accel <= 1) 
            max_accel = 1;
          fall_accel_timer = max_accel;

        }	
      }
      if (p_y >= 168) {
        p_y = 168;
      } else if (p_y < 8) {
        p_y = 8;
      }

      // DRAW
      // draws the player
      if (dir == 1) {		// moving right
        if (flapping) 
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_right);
         else
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right[anim_state%5]);
      } else  if (dir == -1) {	// moving left
        if (flapping)
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, jump_left);
          else
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_left[anim_state%5]);

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
      if (active_enemie()) {
        for (i = 0; i < NUM_ENEMIES; i++) {	// display enemies
          if (enemies[i].in_use) {
            if (enemies[i].xpos+8 < 256)
              cur_oam = oam_spr(enemies[i].xpos, enemies[i].ypos, 0xb6, 0x01, cur_oam);
            }
        }
      }
      oam_hide_rest(cur_oam);
      scroll(screen_x, screen_y);
      //vrambuf_flush();
  }
}

