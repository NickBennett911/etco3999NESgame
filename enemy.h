#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ENEMIES 7
#define ENEMYUPPER 44
#define ELOWER 175

typedef struct
{
  int speed;
  bool in_use;
  int xpos, ypos;
  int dir;
}Enemy;

Enemy enemies[NUM_ENEMIES];

void init_enemies() {
  int i;
  for (i = 0; i < NUM_ENEMIES; i++) {
    enemies[i].in_use = false;
    enemies[i].xpos = 270;
    enemies[i].ypos = (rand() % (ELOWER - ENEMYUPPER + 1)) + ENEMYUPPER;
    enemies[i].speed = 3;
    enemies[i].dir = -1;
  }
}

void spawn_enemy(int speed) {
  int i;
  for (i = 0; i < NUM_ENEMIES; i++) {
    if (!enemies[i].in_use) {
      enemies[i].in_use = true;
      enemies[i].xpos = 270;
      enemies[i].ypos = (rand() % (ELOWER - ENEMYUPPER + 1)) + ENEMYUPPER;
      enemies[i].speed = speed;
      break;
    }
  }
}

bool active_enemie() {
  int i;
  for (i = 0; i < NUM_ENEMIES; i++) {
    if (enemies[i].in_use) {
      return true;
    }
  }
  return false;
}

void enemy_reset() {
  int i;
  for (i = 0; i < NUM_ENEMIES; i++) {
    if (enemies[i].in_use) {
      enemies[i].in_use = false;
    }
  }
}

void update_enemy(int i) {
  if (enemies[i].in_use) {
    enemies[i].xpos += enemies[i].speed * enemies[i].dir;
    if (enemies[i].xpos <= 0) {
      enemies[i].in_use = false;
    }
  }
  
}

int num_enemies_on_screen() {
  int count;
  int i;
  for (i = 0; i < NUM_ENEMIES; i++) {
    if (enemies[i].in_use) {
      count++;
    }
  }
  return count;
}

const unsigned char enemy_left[] = {
        8,      0,      0xf8+0,   ATTR2, 
        8,      8,      0xf8+1,   ATTR2, 
        0,      0,      0xf8+2,   ATTR2, 
        0,      8,      0xf8+3,   ATTR2, 
        128
};




