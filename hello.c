#define NES_MIRRORING 1

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"
#include "apu.h"

int seed = 1;

#include "power_up.h"
#include "config.h"
#include "enemy.h"

#include "time.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"
//#link "vrambuf.c"
//#link "apu.c"
/*{pal:"nes", layout:"nes"}*/

unsigned char ground_info[4];
int random;


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

void increment_score(char* score, char* enemy_speed) {
  //int i; 
  char temp[] = {0x30, 0x30, 0x30};
  if (score[2]+1 <= 0x39) {
    score[2] += 1;
    vrambuf_put(NTADR_A(9, 2), score, 3);
    vrambuf_flush();
    return;
  }
  score[2] = 0x30;
  if (score[1]+1 <= 0x39) {
    score[1] += 1;
    vrambuf_put(NTADR_A(9, 2), score, 3);
    vrambuf_flush();
    *enemy_speed += 1;
    return;
  }
  score[1] = 0x30;
  if (score[0]+1 <= 0x39) {
    vrambuf_put(NTADR_A(9, 2), score, 3);
    vrambuf_flush();
    score[0] += 1;
    return;
  }
}

void increment_bomb_count(char* bomb_list, char* num_bombs) {
  *num_bombs++;
  if (bomb_list[1]+1 <= 0x39){
    bomb_list[1] += 1;
    vrambuf_put(NTADR_A(27, 2), bomb_list, 2);
    vrambuf_flush();
    return;
  }
  bomb_list[1] = 0x30;
  if (bomb_list[0]+1 <= 0x39) {
    bomb_list[0] += 1;
    vrambuf_put(NTADR_A(27, 2), bomb_list, 2);
    vrambuf_flush();
    return;
  }
}

void decrement_bomb_count(char* bomb_list, char* num_bombs) {
  *num_bombs--;
  if (bomb_list[0] > 0x30){		// mulitple of ten
    if (bomb_list[1] == 0x30) {
      bomb_list[0] -= 1;
      bomb_list[1] = 0x39;
      vrambuf_put(NTADR_A(27, 2), bomb_list, 2);
      vrambuf_flush();
      return;
    }
    else if (bomb_list[1] > 0x30) {
      bomb_list[1] -= 1;
      vrambuf_put(NTADR_A(27, 2), bomb_list, 2);
      vrambuf_flush();
      return;
    }
  }
  if (bomb_list[1] > 0x30) {
    bomb_list[1] -= 1;
    vrambuf_put(NTADR_A(27, 2), bomb_list, 2);
    vrambuf_flush();
    return;
  }
}

bool has_bomb(char* bomb_list) {
  if (bomb_list[0] > 0x30) {
    return true;
  }
  if (bomb_list[1] > 0x30) {
    return true;
  }
  
}

void reset_score_hearts_bombs(char* score, char* hearts, char* bombs, char* num_bombs) {
  score[0] = 0x30; score[1] = 0x30; score[2] = 0x30;
  vrambuf_put(NTADR_A(9, 2), score, 3);
  vrambuf_flush();
  
  hearts[0] = 0x15; hearts[1] = 0x15; hearts[2] = 0x15;
  vrambuf_put(NTADR_A(20, 2), hearts, 3);
  vrambuf_flush();
  
  bombs[0] = 0x30; bombs[1] = 0x31;
  vrambuf_put(NTADR_A(27, 2), bombs, 2);
  vrambuf_flush();
  *num_bombs = 0;
}


bool decrement_health(char* hearts) {
  if (hearts[2] == 0x15) {
    hearts[2] = 0x00;
    vrambuf_put(NTADR_A(20, 2), hearts, 3);
    vrambuf_flush();
    return false;
  }
  if (hearts[1] == 0x15) {
    hearts[1] = 0x00;
    vrambuf_put(NTADR_A(20, 2), hearts, 3);
    vrambuf_flush();
    return false;
  }
  if (hearts[0] == 0x15) {
    hearts[0] = 0x00;
    vrambuf_put(NTADR_A(20, 2), hearts, 3);
    vrambuf_flush();
    return true;
  }
}

bool increment_health(char* hearts, char* score, char* enemy_speed) {
  if (hearts[1] == 0x00) {
    hearts[1] = 0x15;
    vrambuf_put(NTADR_A(20, 2), hearts, 3);
    vrambuf_flush();
    return false;
  }
  if (hearts[2] == 0x00) {
    hearts[2] = 0x15;
    vrambuf_put(NTADR_A(20, 2), hearts, 3);
    vrambuf_flush();
    return true;
  }
  increment_score(score, enemy_speed);
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
  char num_bombs[] = {0x30, 0x30};
  char bomb_count = 0;
  int invincibility_cooldown = 100;
  int cur_invincibility_cooldown = invincibility_cooldown;
  
  bool is_invinciple = false;
  int count_up = true;
  int frames_timer = 0;
  bool draw_player;
  int frame_timer_peak = 4;
  
  
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
  bool can_flap = true;
  bool accel_this_frame = true;
  int flap_cooldown = 30;
  int cur_flap_cooldown = 0;
  
  //enemy vars
  int enemy_spawn_cooldown = 15;
  int cur_enemy_spawn_cooldown = enemy_spawn_cooldown;
  char enemy_speed = 3;
  
  //bomb powerup var
  int bomb_spawn_cooldown = 30;
  int cur_bomb_cooldown = bomb_spawn_cooldown;
  
  
  bool in_start_screen = true;
  
  
  // set palette colors
  pal_all(PALETTE); // generally before game loop (in main)
  
  
  vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(StartScreen); 
  
  vram_adr(NTADR_B(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
  vram_unrle(mapB); 
  
  music_ptr = 0;
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  vrambuf_clear();
  set_vram_update((byte*)0x100); // updbuf = 0x100 -- start of stack RAM
  
  init_bullet_list();
  init_enemies();
  init_powerups();
  // infinite loop
  while (1) 
  {
      char cur_oam = 0;
      char pad_result = pad_poll(0);
    
      cur_oam = oam_spr(1, 32, 0x2e, 0x00, 0);
    
      srand(seed);
      seed += (rand()%100)+2;
    
      if (!music_ptr) 
        start_music(music1);
    
      // INPUT
      if (!is_dead) {
        if(pad_result&(0x01<<4)){	// up
          p_y -= 2;
          seed += (rand()%100)+5;
        } else if (pad_result&(0x01<<5)) { 	// down
          p_y += 2;
          seed += (rand()%100)+5;
        } 
      }
      if (pad_result&(0x01<<0) && !in_start_screen) {	// space bar
          seed += (rand()%100)+5;
          if( can_fire && !is_dead ) {
            spawn_bullet(p_x, p_y, idle_dir);
            cur_cooldown = fire_cooldown;
            can_fire = false;    
            shoot_sound();
          }  
          if (is_dead) {
            is_dead = false;
            p_y = 100;
            fade_to_black();
            ppu_off();
            vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
            vram_unrle(mapA);
            ppu_on_all();
            enemy_speed = 0;
            reset_score_hearts_bombs(score, hearts, num_bombs, &bomb_count);
            fade_to_color();
          }
      }		
      if (pad_result&(0x01<<1)) {	// B button (shift)
        seed += (rand()%100)+5;
        if (has_bomb(num_bombs)) {
          decrement_bomb_count(num_bombs, &bomb_count);
          i = num_enemies_on_screen();
          bomb_sound();
          for ( ; i >= 0; i--) {
            pal_bright(7);
            ppu_wait_frame();
            increment_score(score, &enemy_speed);
            pal_bright(4);
            ppu_wait_frame();
          }
          enemy_reset();
        }
      }
      // start menu
      if (in_start_screen) {
        if (pad_result&(0x01<<3)) {
          fade_to_black();
          ppu_off();
          vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
          vram_unrle(mapA); 
          ppu_on_all();
          enemy_reset();
          reset_bullets();
          reset_score_hearts_bombs(score, hearts, num_bombs, &bomb_count);
          in_start_screen = false;
          fade_to_color();
        }
      }
      // UPDATE
      cur_flap_cooldown -= 1;
      if (cur_flap_cooldown <= 0){
      	can_flap = true;
      }

      // cooldown for enemy spawn and powerup cooldown
      if (!in_start_screen) {
        if (!is_dead) {
          cur_enemy_spawn_cooldown -= 1;
          if (cur_enemy_spawn_cooldown <= 0) {
            spawn_enemy(enemy_speed);
            cur_enemy_spawn_cooldown = enemy_spawn_cooldown;
          }

          cur_bomb_cooldown -= 1;
          if (cur_bomb_cooldown <= 0) {
            random = (rand() % (2 + 1));

            if (random == 1)
              spawn_powerup();
            cur_bomb_cooldown = bomb_spawn_cooldown;
          }
        }
      }
      if (!is_invinciple) {
        //bird collision with player
        for (i = 0; i < NUM_ENEMIES; i++ ) {
          if (enemies[i].xpos < 38 && enemies[i].xpos > 22) {
            if (enemies[i].in_use) {
              if (enemies[i].ypos < (p_y+16)) {
                if (enemies[i].ypos > (p_y-8)){
                  enemies[i].in_use = false;
                  is_dead = decrement_health(hearts);
                  if (is_dead) {
                    fade_to_black();
                    ppu_off();
                    vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
                    vram_unrle(DeathTable); 
                    ppu_on_all();
                    enemy_reset();
                    reset_bullets();
                    powerup_reset();
                    fade_to_color();
                  }
                  take_damage_sound();
                  break;
                }
              }
            }
          }
        }
      }
      // invincibility logic
      if (is_invinciple) {
        cur_invincibility_cooldown -= 1;
        if (count_up) {
          frames_timer += 1;
          if (frames_timer > frame_timer_peak) {
	    count_up = false;
          }
        }
        if (!count_up) {
          frames_timer -= 1;
          if (frames_timer < 0 ) {
            count_up = true;
          }
        }
        if (cur_invincibility_cooldown < 0) {
          is_invinciple = false;
          cur_invincibility_cooldown = invincibility_cooldown;
          frames_timer = 0;
          count_up = true;
        }
        
      }
      
      // bullet fire cooldown
      if (!can_fire && !is_dead) {
        cur_cooldown -= 1;
        if (cur_cooldown <= 0) {
          can_fire = true;
        }
      }
      
      // touching top and bottom logic
      if (p_y >= 178) {
        p_y = 177;
        if (!is_invinciple) {
          is_dead = true;
          take_damage_sound();
          fade_to_black();
          ppu_off();
          vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
          vram_unrle(DeathTable); 
          ppu_on_all();
          enemy_reset();
          reset_bullets();
          powerup_reset();
          fade_to_color();
        }
      } else if (p_y < 38) {
        p_y = 39;
        if (!is_invinciple) {
          is_dead = true;
          take_damage_sound();
          fade_to_black();
          ppu_off();
          vram_adr(NTADR_A(0,0)); // Zelda probably started at 0x28d0 (8 rows below stats area)
          vram_unrle(DeathTable); 
          ppu_on_all();
          enemy_reset();
          reset_bullets();
          powerup_reset();
          fade_to_color();
        }
      }

      // DRAW
      if (!in_start_screen) {
        // draws the player
        if (dir == 1) {		// moving right
          if (is_invinciple) {
            draw_player = frames_timer > (frame_timer_peak/2);
            if (draw_player) {
              cur_oam = oam_meta_spr(p_x, p_y, cur_oam, inv1);
            }
          }
          else
            cur_oam = oam_meta_spr(p_x, p_y, cur_oam, anim_right);
        } 
        // draw bomb powerups
        if (active_powerup() && !is_dead) {
          for (i = 0; i < POSSIBLE_POWERUPS; i++ ) {
            if (powerups[i].in_use){
              if (!is_dead)
                update_powerup(i);
              if (powerups[i].xpos+4 < 256) {
                if (powerups[i].is_bomb) {
                  cur_oam = oam_spr(powerups[i].xpos+4, powerups[i].ypos+4, 0x19, 0x01, cur_oam);
                }
                else if (powerups[i].is_heart) {
                  cur_oam = oam_spr(powerups[i].xpos+4, powerups[i].ypos+4, 0x15, 0x01, cur_oam);
                }
                else if (powerups[i].is_invinciple) {
                  cur_oam = oam_spr(powerups[i].xpos+4, powerups[i].ypos+4, 0x18, 0x01, cur_oam);
                }
              }
	      // powerup collision with players logic
              if (powerups[i].ypos < (p_y+16)) {
                if (powerups[i].ypos > (p_y-8)){
                  if (powerups[i].xpos < 38 && powerups[i].xpos > 22) {
                    powerups[i].in_use = false;
                    powerup_pickup();
                    if (powerups[i].is_bomb) {
                      increment_bomb_count(num_bombs, &bomb_count);
                    }
                    else if (powerups[i].is_heart) {
                      increment_health(hearts, score, &enemy_speed);
                    }
                    else if (powerups[i].is_invinciple) {
                      is_invinciple = true;
                    }
                  }
                }
              }
            }
          }
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
              update_enemy(i);
            for (j = 0; j < NUM_BULLETS; j++) {
              if (bullets[j].in_use) {
                if (is_colliding(enemies[i].xpos, enemies[i].ypos, bullets[j].xpos, bullets[j].ypos)) {
                  bullets[j].in_use = false;
                  can_fire = true;
                  enemies[i].in_use = false;
                  increment_score(score, &enemy_speed);
                  deal_damage_sound();
                }
              }
            }
            if (enemies[i].xpos+8 < 256) {
              cur_oam = oam_spr(enemies[i].xpos, enemies[i].ypos, 0xb6, 0x00, cur_oam);
            }
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