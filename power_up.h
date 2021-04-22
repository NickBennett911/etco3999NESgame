#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define POSSIBLE_POWERUPS 1
#define POWERUPPER 44
#define LOWER 175

int choice;

typedef struct 
{
  int speed;
  bool in_use;
  int xpos;
  int ypos;
  int dir;
  bool is_bomb;
  bool is_heart;
  bool is_invinciple;
}PowerUP;

PowerUP powerups[POSSIBLE_POWERUPS];

void init_powerups() {
  int i;
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    powerups[i].in_use = false;
    powerups[i].xpos = 270;
    powerups[i].ypos = (rand() % (LOWER - POWERUPPER + 1)) + POWERUPPER;
    powerups[i].speed = 4;
    powerups[i].dir = -1;
    powerups[i].is_bomb = false;
    powerups[i].is_heart = false;
    powerups[i].is_invinciple = false;
  }
}

void spawn_powerup() {
  int i;
  //int choice;
  
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    if (!powerups[i].in_use) {
      srand(seed);
      powerups[i].in_use = true;
      powerups[i].xpos = 270;
      powerups[i].ypos = (rand() % (LOWER - POWERUPPER + 1)) + POWERUPPER;
      powerups[i].is_bomb = false;
      powerups[i].is_heart = false;
      powerups[i].is_invinciple = false;
      choice  = (rand() % (3));
      if (choice == 0) 
        powerups[i].is_bomb = true;
      else if (choice == 1)
        powerups[i].is_heart = true;
      else if (choice == 2)
        powerups[i].is_invinciple = true;
      break;
    }
  }
}

bool active_powerup() {
  int i;
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    if (powerups[i].in_use) {
      return true;
    }
  }
  return false;
}

void powerup_reset() {
  int i;
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    powerups[i].in_use = false;
    powerups[i].is_bomb = false;
    powerups[i].is_heart = false;
    powerups[i].is_invinciple = false;
  }
}

void update_powerup(int i) {
  if (powerups[i].in_use) {
    powerups[i].xpos += powerups[i].speed * powerups[i].dir;
    if (powerups[i].xpos <= 0) {
      powerups[i].in_use = false;
    }
  }
  
}
  