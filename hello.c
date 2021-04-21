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
/*{pal:"nes", layout:"nes"}*/

unsigned char ground_info[4];


bool is_colliding(int x1, int y1, int x2, int y2) {
  int xdiff = x1 - x2;
  int ydiff = y1 - y2 - 4;

  if (xdiff > -8 && xdiff < 8) {
    if (ydiff > -8 && ydiff < 8) {
      return true;
    }
  }
  return false;
}

void increment_score(char* score) {
  //int i; 
  char temp[] = {0x30, 0x30, 0x30};
  if (score[2]+1 <= 0x39) {
    score[2] += 1;
    vrambuf_put(NTADR_A(8, 1), score, 3);
    vrambuf_flush();
    return;
  }
  score[2] = 0x30;
  if (score[1]+1 <= 0x39) {
    score[1] += 1;
    vrambuf_put(NTADR_A(8, 1), score, 3);
    vrambuf_flush();
    return;
  }
  score[1] = 0x30;
  if (score[0]+1 <= 0x39) {
    vrambuf_put(NTADR_A(8, 1), score, 3);
    vrambuf_flush();
    score[0] += 1;
    return;
  }
}

void reset_score_hearts(char* score, char* hearts) {
  score[0] = 0x30; score[1] = 0x30; score[2] = 0x30;
  vrambuf_put(NTADR_A(8, 1), score, 3);
  vrambuf_flush();
  
  hearts[0] = 0x15; hearts[1] = 0x15; hearts[2] = 0x15;
  vrambuf_put(NTADR_A(19, 1), hearts, 3);
  vrambuf_flush();
}


bool decrement_health(char* hearts) {
  if (hearts[2] == 0x15) {
    hearts[2] = 0x00;
    vrambuf_put(NTADR_A(19, 1), hearts, 3);
    vrambuf_flush();
    return false;
  }
  if (hearts[1] == 0x15) {
    hearts[1] = 0x00;
    vrambuf_put(NTADR_A(19, 1), hearts, 3);
    vrambuf_flush();
    return false;
  }
  if (hearts[0] == 0x15) {
    hearts[0] = 0x00;
    vrambuf_put(NTADR_A(19, 1), hearts, 3);
    vrambuf_flush();
    return true;
  }
}

// main function, run after console reset
void main(void) {
  
  // player vars
  unsigned char p_x = 30, p_y = 100;
  int i;
  int j;
  int dir = 1;
  int speed = 1;
  int idle_dir = 1;
  unsigned char attrib = 0x00;
  int health = 3;
  char hearts[] = {0x15, 0x15, 0x15};
  int fall_accel_timer = 5;	
  int max_accel = 5;
  bool is_dead = false;
  char score[] = { 0x30, 0x30, 0x30 };
  
  // camera vars
  int screen_x = -8, screen_y = 0;
  int right_side = 0, left_side = 1;
  int is_player_control = 1;
  
  int anim_state = 0;
  
  int spirte_table = 0xFF;
  
  // bullet vars
  int fire_cooldown = 20;
  int cur_cooldown = 0;
  bool can_fire = true;
  
  // jumping vars
  int min_accel = 7;
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
  
  
  //APU_ENABLE(ENABLE_NOISE);
  // enemies
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)
  
  //vram_adr(NTADR_A(1, 1));
  //vram_write("HEALTH: ", 8);
  //for ( i = 0; i < health; i++) {
  //	vram_put(0x15);
  //}
  
  vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapA); 
  
  vram_adr(NTADR_B(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapB); 
  
  vrambuf_put(NTADR_A(8, 1), score, 3);
  vrambuf_flush();
  
  vram_adr(NTADR_A(8, 1));
  for (i = 0; i < 3; i++) {
    vram_put(score[i]);
  }
  
  vram_adr(NTADR_A(19, 1));
  for (i = 0; i < health; i++) {
    vram_put(0x15);
  }
  
  
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
      if (!is_dead) {
        if(pad_result&(0x01<<4)){	// up
          p_y -= 2;
            //dir = -1;
            //idle_dir = -1;
            //anim_state++;
        } else if (pad_result&(0x01<<5)) { 	// down
          p_y += 2;
            //dir = 1;
            //idle_dir = 1;
           // anim_state++;
        } 
      }
      if (pad_result&(0x01<<0)) {	// space bar
          if( can_fire && !is_dead ) {
            spawn_bullet(p_x, p_y, idle_dir);
            cur_cooldown = fire_cooldown;
            can_fire = false;    
            shoot_sound();
          }  
          if (is_dead) {
            is_dead = false;
            p_y = 100;
            //enemy_reset();
            reset_bullets();
            reset_score_hearts(score, hearts);
          }
      }
      /*if (pad_result&(0x01<<4)){	// jump
        if (can_flap) {
          flapping = true;
          can_flap = false;
          cur_flap_cooldown = flap_cooldown;
          accel = -min_accel;
          //spawn_enemy();
          APU_PULSE_DECAY(PULSE_CH0, 940, 192, 5, 4);
          APU_PULSE_SWEEP(PULSE_CH0, 2, 4, 1);
          //APU_NOISE_DECAY((15 + 10) & 0xF, 10, 7);
        }
      }*/
      // UPDATE
      cur_flap_cooldown -= 1;
      if (cur_flap_cooldown <= 0){
      	can_flap = true;
      }

      //if (!is_dead) {
      //  update_enemies();
      //}
        
      cur_enemy_spawn_cooldown -= 1;
      if (cur_enemy_spawn_cooldown <= 0) {
        spawn_enemy();
        cur_enemy_spawn_cooldown = enemy_spawn_cooldown;
      }
    
      //bird collision with player
      for (i = 0; i < NUM_ENEMIES; i++ ) {
        if (enemies[i].xpos < 38 && enemies[i].xpos > 22) {
          if (enemies[i].in_use) {
            if (enemies[i].ypos < (p_y+16)) {
              if (enemies[i].ypos > (p_y-8)){
                enemies[i].in_use = false;
                is_dead = decrement_health(hearts);
                take_damage_sound();
                break;
              }
            }
          }
        }
      }
      
      // bullet fire cooldown
      if (!can_fire && !is_dead) {
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
      
      /*if (!flapping) {
        fall_accel_timer -= 1;
        if (fall_accel_timer <= 0) {
          p_y += 1;
          max_accel -= 1;
          if (max_accel <= 1) 
            max_accel = 1;
          fall_accel_timer = max_accel;

        }	
      }*/
      if (p_y >= 168) {
        p_y = 167;
        is_dead = true;
        take_damage_sound();
      } else if (p_y < 8) {
        p_y = 9;
        is_dead = true;
        take_damage_sound();
        
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
      //if (active_bullet()) {
        for (i = 0; i < NUM_BULLETS; i++) {
          if (bullets[i].in_use) {
            if (!is_dead)
              update_bullet(i);
            cur_oam = oam_spr(bullets[i].xpos+4, bullets[i].ypos+4, 0xb6, 0x01, cur_oam);
          }
        }
        //}
        // draws the enemies
        //if (active_enemie()) {
        for (i = 0; i < NUM_ENEMIES; i++) {	// display enemies
          if (enemies[i].in_use) {
            if (!is_dead)
              //update_enemy(i);
            for (j = 0; j < NUM_BULLETS; j++) {
                if (bullets[j].in_use) {
                  if (is_colliding(enemies[i].xpos, enemies[i].ypos, bullets[j].xpos, bullets[j].ypos)) {
                    bullets[j].in_use = false;
                    enemies[i].in_use = false;
                    increment_score(score);
                    deal_damage_sound();
                  }
                }
              }
            if (enemies[i].xpos+8 < 256) {
              cur_oam = oam_spr(enemies[i].xpos, enemies[i].ypos, 0xb6, 0x00, cur_oam);
            }
          }
        }
      waitvsync();
      play_music();
      // draws the hearts
      //vram_adr(NTADR_A(19, 1));
      //for (i = 0; i < health; i++) {
      //  vram_put(0x15);
      //}
      //}
      oam_hide_rest(cur_oam);
      ppu_wait_frame();
      //scroll(screen_x, screen_y);
      //vrambuf_flush();
  }
}