#include "stdio.h"
#include "stdlib.h"
#include "time.h"

#define POSSIBLE_POWERUPS 1
#define UPPER 15
#define LOWER 150

typedef struct 
{
  int speed;
  bool in_use;
  int xpos;
  int ypos;
  int dir;
}PowerUP;

PowerUP powerups[POSSIBLE_POWERUPS];

void init_powerups() {
  int i;
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    powerups[i].in_use = false;
    powerups[i].xpos = 270;
    powerups[i].ypos = (rand() % (LOWER - UPPER + 1)) + UPPER;
    powerups[i].speed = 4;
    powerups[i].dir = -1;
  }
}

void spawn_powerup() {
  int i;
  for (i = 0; i < POSSIBLE_POWERUPS; i++) {
    if (!powerups[i].in_use) {
      powerups[i].in_use = true;
      powerups[i].xpos = 270;
      powerups[i].ypos = (rand() % (LOWER - UPPER + 1)) + UPPER;
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
    if (powerups[i].in_use) {
      powerups[i].in_use = false;
    }
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
  