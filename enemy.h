

typedef struct Enemy
{
  int xpos, ypos;
  int health;
  int dir;
  int move_radius;
};

const int enemy_spawns[2][2]={
  {110, 160},
  {120, 135}
};

const unsigned char enemy_left[] = {
        8,      0,      0xf8+0,   ATTR2, 
        8,      8,      0xf8+1,   ATTR2, 
        0,      0,      0xf8+2,   ATTR2, 
        0,      8,      0xf8+3,   ATTR2, 
        128
};




