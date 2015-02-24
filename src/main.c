//56b37c6f-792a-480f-b962-9a0db8c32aa4
//b00bface-effd-480f-b962-c0ded4c0ffee
/************************************************
  Notes
  ************************************************
  Use JavaScript for Phone Storage for level layout?
  If no phone connection, use default level
  Persistent Storage for Hi-Score
  Future versions might have different game modes:
    Classic Arcade
    Item Collection (Use buttons to use items)
      - Bomb ("Eats" all ghosts)
      - Save Power Pellet for use anytime
      - Each dot = $1?
    Race the clock
    Dozens of ghosts
    multiple pebmen (Multiball)
      all controlled the same, eat each other
    Goal to eat specific tile, then board resets
  
  Program Flow
  ------------
Version 1:
  1. Init
    A. Map initialized by static array
    B. Player/Ghost init positions hardcoded
    C. Init score, 1 player only
  2. Main Menu
    A. Begin Game
    B. Options
      - All Vibrations on/off
      - Death Vibration
      - "Jaws" vibration music
      - Siren vibration music
      - Sound Effects vibration
      - One/Two Player
      - Speed: 1/4x, 1/2x, 1x, 2x, 4x
    C. 
  3. Begin Game was selected
    A. Init
      - Create 2nd window
      - Init Map
      - Init Ghosts
      - Init Players
      


Version 2: (Level Editor)
  1. Init
    A. Load Map
      - Create Array
      - Read ROM, save into array
      - Draw into Bitmap (Without Dots)
    B. Init Player/Ghost positions
      - Read Map ROM for Player/Ghost positions (default facing left)
      
  
*/

#include "pebble.h"
#define UPDATE_MS 30 // Refresh rate in milliseconds (about 32fps)
#define ZOOM 5       // Number of pixels per map square
#define MAP_W 28
#define MAP_H 31
#define BOARD_X 2
#define BOARD_Y 13
//Cardinal Directions = facing >> 15... maybe.
#define East  0 // +x
#define South 1 // +y
#define West  2 // -x
#define North 3 // -y
  
static Window *game_window;
static Layer *game_layer;
GBitmap *background;
GBitmap *playerspritesheet,   *playersprite[4][4];
GBitmap *specturespritesheet, *spectresprite[4][4];

GBitmap *fruitspritesheet,    *bonussprite[1];
#define Cherries   0
#define Strawberry 1
#define Peach      2
#define Apple      3
#define Grapes     4
#define Galaxian   5
#define Bell       6
#define Key        7
typedef struct BonusStruct {
  GBitmap *sprite;
  uint16_t points;
} BonusStruct;
//BonusStruct bonus[8] = (BonusStruct){{NULL,100},};
//Ya know what?  No.  Gonna load the bonus sprite at the beginning of each level.
  
/*
============================
           Bonus
============================
 Index   Points     Name
   0       100    Cherries  
   1       300    Strawberry
   2       500    Peach
   3       700    Apple
   4      1000    Grapes
   5      2000    Galaxian
   6      3000    Bell
   7      5000    Key
============================
*/

typedef struct XYStruct {
  int32_t x;
  int32_t y;
} XYStruct;
// =========================================================================================================== //
typedef struct PlayerStruct {
// 	uint16_t x, y; // in pixels, not tile -- center pixel
  XYStruct speed;
  XYStruct pos;
  //uint8_t face;  // 0=Left, 1=Up, 2=Right, 3=Down
  int16_t facing;             // Player Direction Facing (from 0 - TRIG_MAX_ANGLE)
  uint32_t frame; // Animation frame. Mouth: 0=Closed, 1=Open, 2=Wide, 3=Open
  //uint8_t speed;
  uint32_t score;
} PlayerStruct;
PlayerStruct player[1];
uint8_t currentplayer = 0;

//TODO:
//      Eater is sprite on screen. Has temporary variables:
//        speed
//        pos
//        facing
//        frame
//        
//      Player should be human controlling Eater and has:
//        score
//        lives
//        current level
//        which dots remain
// =========================================================================================================== //
typedef struct SpectreStruct {
  struct XYStruct speed;
  struct XYStruct pos;
  
} SpectreStruct;
SpectreStruct spectre[4];
// =========================================================================================================== //

AccelData accel;
uint16_t totalpellets;
 uint8_t dotflashing=0;
 uint8_t speed;         // probably replace with level[currentlevel].playerspeed

// =========================================================================================================== //
//  Control Input Options
// ======================= //
static bool up_button_depressed = false; // Whether Pebble's   Up   button is being held
static bool dn_button_depressed = false; // Whether Pebble's  Down  button is being held
static bool sl_button_depressed = false; // Whether Pebble's Select button is being held
static bool bk_button_depressed = false; // Whether Pebble's  Back  button is being held
#define AccelerometerControl   0 // Accelerometer Movement (default)
#define ULDRButtonControl      1 // Up/Down/Back/Select for Up/Down/Left/Right
#define LRButtonControl        2 // Up/Down for CounterClockwise/Clockwise rotation, Select to reverse direction
uint8_t control_mode = AccelerometerControl;

// =========================================================================================================== //
//  Map and Level Variables
// ======================= //
// NOTE: Probably should change this to a function which figures the data instead of a lookup table
typedef struct LevelStruct {
  uint16_t bonus;    //
  uint8_t eaterspeed;
  uint8_t spectrespeed;
  
} LevelStruct;
LevelStruct level[21];
LevelStruct currentlevel;

static int8_t map[MAP_W * MAP_H];  // int8 means cells can be from -127 to 128

// static int8_t boardlayout[MAP_W * MAP_H] =
// {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 2,-1, 0, 0,-1, 1,-1, 0, 0, 0,-1, 1,-1,-1, 1,-1, 0, 0, 0,-1, 1,-1, 0, 0,-1, 2,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1,-1,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 0,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,
//   0, 0, 0, 0, 0,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 0,-1,-1,-1,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
//   0, 0, 0, 0, 0, 0, 1, 0, 0, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//   0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
//  -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
//  -1, 2, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 2,-1,
//  -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,
//  -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,
//  -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1,-1,
//  -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
//  -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
//  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
// };
static int8_t boardlayout[MAP_W * MAP_H / 2] =
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
 -1, 2,-1, 0, 0,-1, 1,-1, 0, 0, 0,-1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,
 -1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,
  0, 0, 0, 0, 0,-1, 1,-1,-1,-1,-1,-1, 0,-1,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 0, 0,-1, 0, 0, 0,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,
 -1, 2, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0,
 -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
 -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
//-1 = Impassable
// 0 = Blank
// 1 = Pellet
// 2 = Power Pellet

/*  Updated Map Data:
Map is now folded in half
bit 76543210
    xxxxxxxx = uint8_t
bit 0: 
bit 1: 
bit 2: 
bit 3: pellet
bit 4: power pellet
bit 5: spectre can't go up
bit 6: spectre goes slowly
bit 7: impassable (>127 means impassable)

Maybe bits 012 will be for 3D floor/ceiling squaretype

When bit 7 is on, the other bits change meaning:
bits 0-6: squaretype [0-127] (for 3D or 2D)

*/


//Timer
//#define UPDATE_MS 50 // Refresh rate in milliseconds
//static AppTimer *timer;
//timer = app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback, save handle in timer

// ------------------------------------------------------------------------ //
//  Helper Functions
// ------------------------------------------------------------------------ //
static int32_t abs32(int32_t x) {return (x ^ (x >> 31)) - (x >> 31);}

// ------------------------------------------------------------------------ //
//  Init Functions
// ------------------------------------------------------------------------ //
static void init_board() {
//   for(uint16_t i=0; i<MAP_W*MAP_H; i++) map[i] = boardlayout[i];

 for(uint16_t y=0; y<MAP_H*MAP_W; y+=MAP_W)
  for(uint16_t x=0; x<(MAP_W/2); x++) {
    map[y+x] = boardlayout[(y/2)+x];
    map[y + MAP_W - 1 - x] = boardlayout[(y/2)+x];
  } 
  speed=19;  // level speed will replace this
}

static void init_player() {
  player[currentplayer].pos.x   = (14<<6);    // start halfway between 13&14;
  player[currentplayer].pos.y   = (23<<6)+32; // start in middle of 23;
  player[currentplayer].facing  = 2;          // Facing Left. if using angles, then TRIG_MAX_ANGLE/2, or 1<<14;
  player[currentplayer].frame   = 0;          // Full Circle Sprite
  player[currentplayer].speed.x = 0;
  player[currentplayer].speed.y = 0;
}

static void init_spectres() {
  //Enemy Init:
	//Start Positions
	//Target Coordinates for every mode (attack, regroup, scare)
  
}

// ------------------------------------------------------------------------ //
//  Game Functions
// ------------------------------------------------------------------------ //
int8_t getmap(int32_t x, int32_t y) {
  x>>=6; y>>=6;
  if(y==14 && (x<0 || x>=MAP_W)) return 0;
  return (x<0 || x>=MAP_W || y<0 || y>=MAP_H) ? -1 : map[(y * MAP_W) + x];
}

void setmap(int32_t x, int32_t y, int8_t data) {
  x>>=6; y>>=6;
  if(x>=0 && x<MAP_W && y>=0 && y<MAP_H)
    map[(y * MAP_W) + x]=data;
}

//AccelData accel;// = (AccelData) { .x = 0, .y = 0, .z = 0 };
// ------------------------------------------------------------------------ //
//  Main Loop Functions
// ------------------------------------------------------------------------ //
static void gameloop(void *data) {
  XYStruct testspeed, pos, testfacing;
  int16_t currentspeed;
  pos.x=player[currentplayer].pos.x;
  pos.y=player[currentplayer].pos.y;
  currentspeed=speed;
  
  //************************************//
  // Eat Dots, Update Score, Slow Speed
  //************************************//
  if(getmap(player[currentplayer].pos.x, player[currentplayer].pos.y)==1) {  // if on a regular dot
    currentspeed -= 2;
    player[currentplayer].score += 10;
    setmap(player[currentplayer].pos.x, player[currentplayer].pos.y, 0);
  }
  if(getmap(player[currentplayer].pos.x, player[currentplayer].pos.y)==2) {  // if on a super dot
    currentspeed -= 6;
    player[currentplayer].score += 50;
    setmap(player[currentplayer].pos.x, player[currentplayer].pos.y, 0);
  }

  
  //************************************//
  // Get Joystick, Move Player
  //************************************//
/* TODO:
         Use ARCTAN2 to figure X&Y Joystick by XZ and YZ accelerometer
         Maybe subtle movement = centered joystick position
*/
  
  
//     if(abs32(accel.x)>abs32(accel.y)) {
//       if(accel.x<0) {testspeed.x =-1; testfacing = 2;} // Left
//       else          {testspeed.x = 1; testfacing = 0;} // Right
//     } else {
//       if(accel.y<0) {testspeed.y = 1; testfacing = 3;} // Down
//       else          {testspeed.y =-1; testfacing = 1;} // Up
//     }

  testspeed.x = 0; testspeed.y = 0;  testfacing.x = 0;  testfacing.y = 0;
  switch(control_mode) {
    case AccelerometerControl:
      accel_service_peek(&accel); // Read accelerometer
      accel.x>>=3; accel.y>>=3;
           if(accel.x<-10) {if(getmap(pos.x+(64*-1), pos.y+(64* 0))>=0) {testspeed.x =-1; testfacing.x = 2;}} // Left
      else if(accel.x> 10) {if(getmap(pos.x+(64* 1), pos.y+(64* 0))>=0) {testspeed.x = 1; testfacing.x = 0;}} // Right
           if(accel.y<-10) {if(getmap(pos.x+(64* 0), pos.y+(64* 1))>=0) {testspeed.y = 1; testfacing.y = 3;}} // Down
      else if(accel.y> 10) {if(getmap(pos.x+(64* 0), pos.y+(64*-1))>=0) {testspeed.y =-1; testfacing.y = 1;}} // Up
    
      if((abs32(accel.x)>abs32(accel.y) || testspeed.y==0) && testspeed.x!=0) {
        player[currentplayer].speed.x = testspeed.x;
        player[currentplayer].speed.y = 0;
        player[currentplayer].facing  = testfacing.x;
      } else if(testspeed.y!=0) {
        player[currentplayer].speed.x = 0;
        player[currentplayer].speed.y = testspeed.y;
        player[currentplayer].facing  = testfacing.y;
      }
      break;
    case ULDRButtonControl:
      if(bk_button_depressed && !sl_button_depressed) {if(getmap(pos.x+(64*-1), pos.y+(64* 0))>=0) {testspeed.x =-1; testfacing.x = 2;}} // Left
      if(sl_button_depressed && !bk_button_depressed) {if(getmap(pos.x+(64* 1), pos.y+(64* 0))>=0) {testspeed.x = 1; testfacing.x = 0;}} // Right
      if(dn_button_depressed && !up_button_depressed) {if(getmap(pos.x+(64* 0), pos.y+(64* 1))>=0) {testspeed.y = 1; testfacing.y = 3;}} // Down
      if(up_button_depressed && !dn_button_depressed) {if(getmap(pos.x+(64* 0), pos.y+(64*-1))>=0) {testspeed.y =-1; testfacing.y = 1;}} // Up
    
    break;
    case LRButtonControl:
      if(up_button_depressed) {if(getmap(pos.x+(64* 0), pos.y+(64*-1))>=0) {testspeed.y =-1; testfacing.y = 1;}} // Left Turn  
      if(sl_button_depressed) {if(getmap(pos.x+(64* 1), pos.y+(64* 0))>=0) {testspeed.x = 1; testfacing.x = 0;}} // Reverse
      if(dn_button_depressed) {if(getmap(pos.x+(64* 0), pos.y+(64* 1))>=0) {testspeed.y = 1; testfacing.y = 3;}} // Right Turn
      
    
    break;
  }
    
//   testspeed.x = 0; testspeed.y = 0;
//   if(joystickmode) {
//     if(abs32(accel.x)>abs32(accel.y)) {
//       if(accel.x<0) {testspeed.x =-1; testfacing = 2;} // Left
//       else          {testspeed.x = 1; testfacing = 0;} // Right
//     } else {
//       if(accel.y<0) {testspeed.y = 1; testfacing = 3;} // Down
//       else          {testspeed.y =-1; testfacing = 1;} // Up
//     }
//   } else { // button mode
//     if(sl_button_depressed && player[currentplayer].facing != 0) {testspeed.x = 1; testfacing = 0;} // Right
//     if(up_button_depressed && player[currentplayer].facing != 1) {testspeed.y =-1; testfacing = 1;} // Up
//     if(bk_button_depressed && player[currentplayer].facing != 2) {testspeed.x =-1; testfacing = 2;} // Left
//     if(dn_button_depressed && player[currentplayer].facing != 3) {testspeed.y = 1; testfacing = 3;} // Down
//   }
  
//   if(getmap(pos.x+(64*testspeed.x), pos.y+(64*testspeed.y))>=0) {  // if trying to turn and you can turn
//     player[currentplayer].speed.x=testspeed.x;
//     player[currentplayer].speed.y=testspeed.y;
//     player[currentplayer].facing = testfacing;
//   }
  
  //TODO: tend toward the middle based on speed
  if(player[currentplayer].speed.x != 0) {     // if moving horizontally
    if(getmap(player[currentplayer].pos.x+(32*player[currentplayer].speed.x), player[currentplayer].pos.y)>=0) { // if not running into a wall  
      player[currentplayer].pos.x += (currentspeed*player[currentplayer].speed.x);
      if(player[currentplayer].pos.x<-63) player[currentplayer].pos.x += (32<<6);     // tunnel left wraparound
      if(player[currentplayer].pos.x>(32<<6)) player[currentplayer].pos.x -= (32<<6); // tunnel right wraparound
      player[currentplayer].frame++;
      player[currentplayer].pos.y = ((player[currentplayer].pos.y>>6)<<6)+32; // tend toward tile center
    } else { // will hit a wall
      player[currentplayer].pos.x = ((player[currentplayer].pos.x>>6)<<6)+32; // finish moving toward wall, stop at tile center
      player[currentplayer].speed.x = 0;
    }
  } else if(player[currentplayer].speed.y !=0) {  // (not moving horizontally) if moving vertically
    if(getmap(player[currentplayer].pos.x, player[currentplayer].pos.y+(32*player[currentplayer].speed.y))>=0) { // if not running into a wall  
      player[currentplayer].pos.y += (currentspeed*player[currentplayer].speed.y);
      player[currentplayer].frame++;
      player[currentplayer].pos.x = ((player[currentplayer].pos.x>>6)<<6)+32; // tend toward tile center
    } else { // hit a wall
      player[currentplayer].pos.y = ((player[currentplayer].pos.y>>6)<<6)+32; // finish moving toward wall, stop at tile center
      player[currentplayer].speed.y = 0;
    }
  } else { } // (not moving horizontally, not moving vertically)
  
  dotflashing++;
  
  layer_mark_dirty(game_layer);  // Schedule redraw of screen
}



// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
//************************************//
// Background
//************************************//
void draw_background(GBitmap *fb) {
  //*** BG Version 1
  //graphics_draw_bitmap_in_rect(ctx, background, GRect(0,13,144,155));

  for (uint16_t i=0; i<BOARD_Y*5; i++) ((uint32_t*)(fb->addr))[i] = 0; // Top 13 rows of black -- equivilant to: graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, GRect(0,0,144,12), 0, GCornerNone);
  for (uint16_t i=0,j=BOARD_Y*5; i<155*5; i++,j++) ((uint32_t*)(fb->addr))[j] = ((uint32_t*)background->addr)[i]; // Draw Background Image -- equivilant to: graphics_draw_bitmap_in_rect(ctx, background, GRect(0,13,144,155));
}

//************************************//
// Dots
//************************************//
void draw_dots(GContext *ctx) {
  totalpellets=0; 
  graphics_context_set_stroke_color(ctx, 1);
  graphics_context_set_fill_color(ctx, (dotflashing>>2)&1);
  for(uint16_t y=0, i=0; y<MAP_H; y++)
    for(uint16_t x=0; x<MAP_W; x++, i++)
      if(map[i]==1) {
        totalpellets++; 
        graphics_draw_pixel(ctx, GPoint(x*5+BOARD_X+2, y*5+BOARD_Y+2));
      } else if(map[i]==2) {
        totalpellets++; 
        graphics_fill_rect(ctx, GRect(x*5+BOARD_X+1,y*5+BOARD_Y+1,3,3), 0, GCornerNone);
        graphics_fill_rect(ctx, GRect(x*5+BOARD_X+2,y*5+BOARD_Y,1,5), 0, GCornerNone);
        graphics_fill_rect(ctx, GRect(x*5+BOARD_X,y*5+BOARD_Y+2,5,1), 0, GCornerNone);
        //graphics_draw_pixel(ctx, GPoint(x*5+BOARD_X+2, y*5+BOARD_Y+2));
      }
}

//************************************//
// Player
//************************************//
void draw_player(GContext *ctx) {
  uint32_t *target;
  int32_t x, y;  // player's center dot on the screen
  graphics_context_set_stroke_color(ctx, 1);
  x = ((player[currentplayer].pos.x >> 6) * 5) + (((player[currentplayer].pos.x & 63) * 5) / 64);
  y = ((player[currentplayer].pos.y >> 6) * 5) + (((player[currentplayer].pos.y & 63) * 5) / 64);
  target=(uint32_t*)playerspritesheet->addr + (((player[currentplayer].frame>>0)&3)*8);
  for(uint32_t i=0; i<7; i++) {
    for(uint32_t j=0; j<7; j++) {
        if((((*(target+i)) >> ((player[currentplayer].facing * 8) + j)) & 1) == 1)
          graphics_draw_pixel(ctx, GPoint(j+x+BOARD_X-3, i+y+BOARD_Y-3));
    }
  }

  //graphics_draw_bitmap_in_rect(ctx, playersprite[player[currentplayer].facing][(player[currentplayer].frame>>1)&1], GRect(x+BOARD_X-3, y+BOARD_Y-3, 7, 7));

//   graphics_context_set_stroke_color(ctx, dotflashing);
//   graphics_draw_pixel(ctx, GPoint(x+BOARD_X, y+BOARD_Y));
}

//************************************//
// Spectres
//************************************//
void draw_spectres(GBitmap *fb) {
  
}

//************************************//
// Top Bar
//************************************//
void draw_top(GContext *ctx) {
  static char text[40];  //Buffer to hold text
  graphics_context_set_text_color(ctx, 1);  // Text Color
//   snprintf(text, sizeof(text), "Score:%ld  Dots:%d", player[currentplayer].score, totalpellets);  // What text to draw
  snprintf(text, sizeof(text), "%d %d %ld %ld", accel.x, accel.y, atan2_lookup(accel.z, accel.x), atan2_lookup(-1 * accel.y, accel.z));  // What text to draw
  
  //snprintf(text, sizeof(text), "(%ld, %ld) %d", player[currentplayer].pos.x, player[currentplayer].pos.y, getmap(player[currentplayer].pos.x, player[currentplayer].pos.y));  // What text to draw
  //snprintf(text, sizeof(text), "123456789012345678901234");  // Max for "RESOURCE_ID_GOTHIC_14" and "RESOURCE_ID_GOTHIC_14_BOLD"
  graphics_draw_text(ctx, text, fonts_get_system_font("RESOURCE_ID_GOTHIC_14_BOLD"), GRect(0, -4, 144, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
  // draw score (6 digits max -- 7 digits technically possible on level 100)
}

//************************************//
static void game_layer_update(Layer *me, GContext *ctx) {
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {
    draw_background(framebuffer);
    graphics_release_frame_buffer(ctx, framebuffer);
  }
  draw_dots(ctx);
  draw_player(ctx);
  draw_top(ctx);

  app_timer_register(UPDATE_MS, gameloop, NULL); // Finished. Wait UPDATE_MS then loop
}
  
// ------------------------------------------------------------------------ //
//  Button Pushing
// ------------------------------------------------------------------------ //
void up_push_in_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = true;  }  //   UP   button was pushed in
void up_release_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = false; }  //   UP   button was released
void dn_push_in_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = true;  }  //  DOWN  button was pushed in
void dn_release_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = false; }  //  DOWN  button was released
void sl_push_in_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = true;  }  // SELECT button was pushed in
void sl_release_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = false; }  // SELECT button was released
void bk_push_in_handler(ClickRecognizerRef recognizer, void *context) {bk_button_depressed = true;  }  //  BACK  button was pushed in
void bk_release_handler(ClickRecognizerRef recognizer, void *context) {bk_button_depressed = false; }  //  BACK  button was released

static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_UP, up_push_in_handler, up_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_DOWN, dn_push_in_handler, dn_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_SELECT, sl_push_in_handler, sl_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_BACK, bk_push_in_handler, bk_release_handler, context);

}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void game_window_load(Window *window) {
  game_layer = window_get_root_layer(window);
  layer_set_update_proc(game_layer, game_layer_update);
  background = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
// Load game sprites
  playerspritesheet = gbitmap_create_with_resource(RESOURCE_ID_PLAYER_SPRITESHEET);
//   for (uint16_t i=0,j=0; i<9; i++,j+=7) playersprite[i] = gbitmap_create_as_sub_bitmap(playerspritesheet, GRect(j,0,7,7));
  for (uint8_t i=0; i<4; i++)
    for(uint8_t j=0; j<4;j++)
      playersprite[i][j] = gbitmap_create_as_sub_bitmap(playerspritesheet, GRect(i*7,j*7,7,7));

  init_board();
  init_player();
  currentplayer=0;
  player[currentplayer].score=0;
}

static void game_window_unload(Window *window) {
//   for (uint16_t i=0; i<9; i++) gbitmap_destroy(playersprite[i]);
  for (uint8_t i=0; i<4; i++) for (uint8_t j=0; j<4; j++) gbitmap_destroy(playersprite[i][j]);
  gbitmap_destroy(background);
}

static void game_window_appear(Window *window) {}
static void game_window_disappear(Window *window) {}

static void init(void) {
  // Set up and push main window
  game_window = window_create();
  window_set_click_config_provider(game_window, click_config_provider);
  window_set_window_handlers(game_window, (WindowHandlers) {
    .load = game_window_load,
    .unload = game_window_unload,
    .appear = game_window_appear,
    .disappear = game_window_disappear
  });
  window_set_fullscreen(game_window, true);

  //Set up other stuff
  srand(time(NULL));  // Seed randomizer
  accel_data_service_subscribe(0, NULL);  // We will be using the accelerometer
  //font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_8));
  
  //Begin
  window_stack_push(game_window, true /* Animated */); // Display window (layer is now dirty).  Timer callback will be scheduled after dirty layer is written.
}
  
static void deinit(void) {
  //fonts_unload_custom_font(font);
  accel_data_service_unsubscribe();
  window_destroy(game_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

// ------------------------------------------------------------------------ //
//  Notes
// ------------------------------------------------------------------------ //
/*
#define FONT_KEY_FONT_FALLBACK "RESOURCE_ID_FONT_FALLBACK"
#define FONT_KEY_GOTHIC_18 "RESOURCE_ID_GOTHIC_18"
#define FONT_KEY_GOTHIC_18_BOLD "RESOURCE_ID_GOTHIC_18_BOLD"
#define FONT_KEY_GOTHIC_24 "RESOURCE_ID_GOTHIC_24"
#define FONT_KEY_GOTHIC_24_BOLD "RESOURCE_ID_GOTHIC_24_BOLD"
#define FONT_KEY_GOTHIC_28 "RESOURCE_ID_GOTHIC_28"
#define FONT_KEY_GOTHIC_28_BOLD "RESOURCE_ID_GOTHIC_28_BOLD"
#define FONT_KEY_BITHAM_30_BLACK "RESOURCE_ID_BITHAM_30_BLACK"
#define FONT_KEY_BITHAM_42_BOLD "RESOURCE_ID_BITHAM_42_BOLD"
#define FONT_KEY_BITHAM_42_LIGHT "RESOURCE_ID_BITHAM_42_LIGHT"
#define FONT_KEY_BITHAM_42_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_42_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_MEDIUM_NUMBERS "RESOURCE_ID_BITHAM_34_MEDIUM_NUMBERS"
#define FONT_KEY_BITHAM_34_LIGHT_SUBSET "RESOURCE_ID_BITHAM_34_LIGHT_SUBSET"
#define FONT_KEY_BITHAM_18_LIGHT_SUBSET "RESOURCE_ID_BITHAM_18_LIGHT_SUBSET"
#define FONT_KEY_ROBOTO_CONDENSED_21 "RESOURCE_ID_ROBOTO_CONDENSED_21"
#define FONT_KEY_ROBOTO_BOLD_SUBSET_49 "RESOURCE_ID_ROBOTO_BOLD_SUBSET_49"
#define FONT_KEY_DROID_SERIF_28_BOLD "RESOURCE_ID_DROID_SERIF_28_BOLD"





*/