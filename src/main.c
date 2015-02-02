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
#define UPDATE_MS 30 // Refresh rate in milliseconds
#define ZOOM 5       // Number of pixels per map square
#define MAP_W 28
#define MAP_H 31
#define BOARD_X 2
#define BOARD_Y 13
//Cardinal Directions = facing >> 15... maybe.
#define East 0  // +x
#define South 1 // +y
#define West 2  // -x
#define North 3 // -y
  
struct XYStruct {
  int32_t x;
  int32_t y;
};

struct PlayerStruct {
// 	uint16_t x, y; // in pixels, not tile -- center pixel
  struct XYStruct speed;
  struct XYStruct pos;
  //uint8_t face;  // 0=Left, 1=Up, 2=Right, 3=Down
  int16_t facing;             // Player Direction Facing (from 0 - TRIG_MAX_ANGLE)
  uint8_t frame; // Animation frame. Mouth: 0=Closed, 1=Open, 2=Wide, 3=Open
  //uint8_t speed;
  uint8_t score;
};

struct PlayerStruct player[1];
//struct Spectre *spectre[4];

static Window *game_window;
static Layer *game_layer;
GBitmap *background;
static uint16_t totalpellets=0;
GBitmap *playerspritesheet, *playersprite[4][4];
//static TextLayer *label_text_layer;
uint8_t dotflashing,dotflashingtimer=0;
uint8_t speed;
uint8_t currentplayer=0;

  static bool up_button_depressed = false; // Whether Pebble's   Up   button is held
  static bool dn_button_depressed = false; // Whether Pebble's  Down  button is held
  static bool sl_button_depressed = false; // Whether Pebble's Select button is held
//static bool bk_button_depressed = false; // Whether Pebble's  Back  button is held

static int8_t map[MAP_W * MAP_H];  // int8 means cells can be from -127 to 128

static int8_t level[MAP_W * MAP_H] =
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 2,-1, 0, 0,-1, 1,-1, 0, 0, 0,-1, 1,-1,-1, 1,-1, 0, 0, 0,-1, 1,-1, 0, 0,-1, 2,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1,-1,
 -1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 0,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,
  0, 0, 0, 0, 0,-1, 1,-1,-1,-1,-1,-1, 0,-1,-1, 0,-1,-1,-1,-1,-1, 1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
  0, 0, 0, 0, 0, 0, 1, 0, 0, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1, 0, 0, 0, 0, 0, 0,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1, 0, 0, 0, 0, 0,
 -1,-1,-1,-1,-1,-1, 1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1,-1, 1,-1,-1,-1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 1,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1, 1,-1,-1,-1,-1, 1,-1,
 -1, 2, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 2,-1,
 -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,
 -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1, 1,-1,-1,-1,
 -1, 1, 1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1,-1,-1, 1, 1, 1, 1, 1, 1,-1,
 -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,
 -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,-1,
 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};
//-1 = Impassable
// 0 = Blank
// 1 = Pellet
// 2 = Power Pellet



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
  for(uint16_t i=0; i<MAP_W*MAP_H; i++) map[i] = level[i];
    

  //for(uint16_t y=0, i=0; y<MAP_H; y++){
  //  for(uint16_t x=0; x<(MAP_W/2); x++,i++) map[(y*MAP_W)+x] = level[i];
  //  for(uint16_t x=(MAP_W/2); x<MAP_W; x++, i--) *MAP_H; i++) map[(y*MAP_W)+x] = level[i];
  //}
  speed=19;
}

static void init_player() {
  player[currentplayer].pos.x = (14<<6);//(1<<6)+32;//1<<6;//(13<<6) + 32; // start halfway between 13&14//69;
  player[currentplayer].pos.y = (23<<6)+32;//3<<6;//14<<6;//23<<6; // start in middle of 23 //127;
  player[currentplayer].facing = 2;//1<<14;
  player[currentplayer].frame = 0;
  player[currentplayer].speed.x=0;//19;
  player[currentplayer].speed.y=0;
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

// ------------------------------------------------------------------------ //
//  Timer Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  struct XYStruct testspeed, pos;
  int16_t testfacing, tempspeed;
  pos.x=player[currentplayer].pos.x;
  pos.y=player[currentplayer].pos.y;
  tempspeed=speed;
  
  //************************************//
  // Eat Dots, Get Score, Slow Speed
  //************************************//
  if(getmap(player[currentplayer].pos.x, player[currentplayer].pos.y)==1) {  // if eat any sort of dot
    tempspeed -= 2;
    player[currentplayer].score += 10;
    setmap(player[currentplayer].pos.x, player[currentplayer].pos.y, 0);
  }
  if(getmap(player[currentplayer].pos.x, player[currentplayer].pos.y)==2) {  // if eat any sort of dot
    tempspeed -= 6;
    player[currentplayer].score += 50;
    setmap(player[currentplayer].pos.x, player[currentplayer].pos.y, 0);
  }

  
  /* TODO:
         Use ARCTAN2 to figure X&Y Joystick by XZ and YZ accelerometer
         Maybe subtle movement = centered joystick position
*/
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel); // Read accelerometer
  if(abs32(accel.x)<abs32(accel.y)) {
    testspeed.x = 0;
    if(accel.y<0) {testspeed.y = 1; testfacing = 3;} // Down
    else          {testspeed.y =-1; testfacing = 1;} // Up
  } else {
    testspeed.y = 0;
    if(accel.x<0) {testspeed.x =-1; testfacing = 2;} // Left
    else          {testspeed.x = 1; testfacing = 0;} // Right
  }

  if(getmap(pos.x+(64*testspeed.x), pos.y+(64*testspeed.y))>=0) {  // if trying to turn and you can turn
    player[currentplayer].speed.x=testspeed.x;
    player[currentplayer].speed.y=testspeed.y;
    player[currentplayer].facing = testfacing;
    player[currentplayer].pos.x += (speed*player[currentplayer].speed.x);
    player[currentplayer].pos.y += (speed*player[currentplayer].speed.y);
    
    // Wrap around for tunnel
    if(player[currentplayer].pos.x<-63) player[currentplayer].pos.x += (32<<6);
    if(player[currentplayer].pos.x>(32<<6)) player[currentplayer].pos.x -= (32<<6);
    
    player[currentplayer].frame++;

    // Center in square (vert or horz, whichever not currently traveling)
    if(player[currentplayer].speed.x==0) player[currentplayer].pos.x = ((player[currentplayer].pos.x>>6)<<6)+32;
    if(player[currentplayer].speed.y==0) player[currentplayer].pos.y = ((player[currentplayer].pos.y>>6)<<6)+32;

  } else {  // else can't turn, keep going the same direction
    if(getmap(player[currentplayer].pos.x+(64*player[currentplayer].speed.x), player[currentplayer].pos.y+(64*player[currentplayer].speed.y))>=0) { // if not running into a wall
      player[currentplayer].pos.x += (speed*player[currentplayer].speed.x);
      player[currentplayer].pos.y += (speed*player[currentplayer].speed.y);
      
      // Wrap around for tunnel
      if(player[currentplayer].pos.x<-63) player[currentplayer].pos.x += (32<<6);
      if(player[currentplayer].pos.x>(32<<6)) player[currentplayer].pos.x -= (32<<6);
      
      player[currentplayer].frame++;
      
      if(player[currentplayer].speed.x==0) player[currentplayer].pos.x = ((player[currentplayer].pos.x>>6)<<6)+32;
      if(player[currentplayer].speed.y==0) player[currentplayer].pos.y = ((player[currentplayer].pos.y>>6)<<6)+32;
    } else {
      // else ran into wall, center sprite

    }
  }
  
  //TODO: (insert here) tend toward the middle
  
  dotflashingtimer++;
  dotflashing=(dotflashingtimer>>2)&1;
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
  graphics_context_set_fill_color(ctx, dotflashing);
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
  int32_t x, y;  // player's center dot on the screen
  x = ((player[currentplayer].pos.x >> 6) * 5) + (((player[currentplayer].pos.x & 63) * 5) / 64);
  y = ((player[currentplayer].pos.y >> 6) * 5) + (((player[currentplayer].pos.y & 63) * 5) / 64);
  graphics_draw_bitmap_in_rect(ctx, playersprite[player[currentplayer].facing][(player[currentplayer].frame>>1)&1], GRect(x+BOARD_X-3, y+BOARD_Y-3, 7, 7));

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
  //snprintf(text, sizeof(text), "Score:%d Dots:%d", player[currentplayer].score, totalpellets);  // What text to draw
  snprintf(text, sizeof(text), "(%ld, %ld) %d", player[currentplayer].pos.x, player[currentplayer].pos.y, getmap(player[currentplayer].pos.x, player[currentplayer].pos.y));  // What text to draw
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
  
  /*
  static char hello[] = "hello";
  
  int i = 1;
  GRect textframe = GRect(0, 123, 143, 20);  // Text Box Position and Size: x, y, w, h
  
  graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  graphics_context_set_text_color(ctx, 1);  // Text Color
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
  */
  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
}
  
// ------------------------------------------------------------------------ //
//  Button Pushing
// ------------------------------------------------------------------------ //
void up_push_in_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = true;  player[currentplayer].pos.x+=13;}
void up_release_handler(ClickRecognizerRef recognizer, void *context) {up_button_depressed = false; }
void dn_push_in_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = true;  player[currentplayer].pos.x-=13;}
void dn_release_handler(ClickRecognizerRef recognizer, void *context) {dn_button_depressed = false; }
void sl_push_in_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = true;  }  // SELECT button was pushed in
void sl_release_handler(ClickRecognizerRef recognizer, void *context) {sl_button_depressed = false; }

static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_UP, up_push_in_handler, up_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_DOWN, dn_push_in_handler, dn_release_handler, context);
  window_raw_click_subscribe(BUTTON_ID_SELECT, sl_push_in_handler, sl_release_handler, context);
  //window_single_click_subscribe(BUTTON_ID_SELECT, sl_push_in_handler);
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