//56b37c6f-792a-480f-b962-9a0db8c32aa4
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
#define UPDATE_MS 200 // Refresh rate in milliseconds
#define ZOOM 5       // Number of pixels per map square
#define MAP_W 28
#define MAP_H 31

struct PlayerStruct {
	uint16_t x, y; // in pixels, not tile -- center pixel
	//uint16_t y;
  uint8_t face;  // 0=Left, 1=Up, 2=Right, 3=Down
  uint8_t mouth; // 0=Closed, 1=Open, 2=Wide, 3=Open
  uint8_t speed; 
};

struct PlayerStruct player;
//struct Spectre *spectre[4];

static Window *game_window;
static Layer *game_layer;
GBitmap *background;
static uint16_t totalpellets=0;
GBitmap *playerspritesheet, *playersprite[9];
//static TextLayer *label_text_layer;



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
//  Timer Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel); // Read accelerometer
  /*
  if(abs32(accel.x)<abs32(accel.y)){
    if(accel.y<0) JoyStick = JoyUp;
    else          JoyStick = JoyDown;
  } else {
    if(accel.x<0) JoyStick = JoyLeft;
    else          JoyStick = JoyRight;
  }
  */
  layer_mark_dirty(game_layer);  // Schedule redraw of screen
}

// ------------------------------------------------------------------------ //
//  Game Functions
// ------------------------------------------------------------------------ //
static void init_board() {
  for(uint16_t i=0; i<MAP_W*MAP_H; i++) map[i] = level[i];
    
  totalpellets=0; for(uint16_t i=0; i<MAP_W*MAP_H; i++) if(map[i]>0) totalpellets++;  // Count Pellets (not Power Pellets)

  //for(uint16_t y=0, i=0; y<MAP_H; y++){
  //  for(uint16_t x=0; x<(MAP_W/2); x++,i++) map[(y*MAP_W)+x] = level[i];
  //  for(uint16_t x=(MAP_W/2); x<MAP_W; x++, i--) *MAP_H; i++) map[(y*MAP_W)+x] = level[i];
  //}
  
}

static void init_player() {
  player.x = 69;
  player.y = 127;
  player.face = 0;
  player.mouth = 1;
  player.speed = 5;//; 80%
}
// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //

//#define DRAW_PIXEL( framebuffer, x, y, white )  framebuffer[((y*160 + x) / 8)] = ( framebuffer[((y*160 + x) / 8)] & (0xFF ^ (0x01 << (x%8))) ) | (white << (x%8))

static void game_layer_update(Layer *me, GContext *ctx) {
  /*
  static char hello[] = "hello";
  
  int i = 1;
  GRect textframe = GRect(0, 123, 143, 20);  // Text Box Position and Size: x, y, w, h
  
  graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  graphics_context_set_text_color(ctx, 1);  // Text Color
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
  */


//************************************//
// Background
//************************************//
  //*** BG Version 1
  //graphics_draw_bitmap_in_rect(ctx, background, GRect(0,13,144,155));

  //*** BG Version 2
  //GBitmap* layerbitmap = (GBitmap*)ctx;
  //uint32_t* framebuffer = (uint32_t*)layerbitmap->addr;
  //for (uint16_t i=0; i<13*5; i++) framebuffer[i] = 0; // Top 13 rows of black
  //for (uint16_t i=0,j=13*5; i<155*5; i++,j++) framebuffer[j] = background_frame[i]; // Draw Background Image.  5 32bit words per row, 155 rows, starting on row 13

  //*** BG Version 3
  //uint32_t* framebuffer = (uint32_t*)(((GBitmap*)ctx)->addr);
  //uint32_t* background_frame = (uint32_t*)background->addr;  
  //for (uint16_t i=0; i<13*5; i++) framebuffer[i] = 0; // Top 13 rows of black
  //for (uint16_t i=0,j=13*5; i<155*5; i++,j++) framebuffer[j] = background_frame[i]; // Draw Background Image

  //*** BG Version 4
  for (uint16_t i=0; i<13*5; i++) ((uint32_t*)(((GBitmap*)ctx)->addr))[i] = 0; // Top 13 rows of black -- equivilant to: graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, GRect(0,0,144,12), 0, GCornerNone);
  for (uint16_t i=0,j=13*5; i<155*5; i++,j++) ((uint32_t*)(((GBitmap*)ctx)->addr))[j] = ((uint32_t*)background->addr)[i]; // Draw Background Image -- equivilant to: graphics_draw_bitmap_in_rect(ctx, background, GRect(0,13,144,155));

//************************************//
// Dots
//************************************//
  graphics_context_set_stroke_color(ctx, 1);
  graphics_context_set_fill_color(ctx, 1);
  for(uint16_t y=0, i=0; y<MAP_H; y++)
    for(uint16_t x=0; x<MAP_W; x++, i++)
      if(map[i]==1)
        graphics_draw_pixel(ctx, GPoint(x*5+4, y*5+15));
      else if(map[i]==2) {
        graphics_fill_rect(ctx, GRect(x*5+3,y*5+14,3,3), 0, GCornerNone);
        //graphics_draw_pixel(ctx, GPoint(x*5+4, y*5+15));
      }
//************************************//
// Player
//************************************//
  graphics_draw_bitmap_in_rect(ctx, playersprite[player.mouth], GRect(player.x, player.y, 7, 7));
  
//************************************//
// Spectres
//************************************//

//************************************//
// Top Bar
//************************************//
  static char text[40];  //Buffer to hold text
  graphics_context_set_text_color(ctx, 1);  // Text Color
  snprintf(text, sizeof(text), "Total: %d    %x", totalpellets, (int)ctx);  // What text to draw
  //snprintf(text, sizeof(text), "123456789012345678901234");  // Max for "RESOURCE_ID_GOTHIC_14" and "RESOURCE_ID_GOTHIC_14_BOLD"
  //snprintf(text, sizeof(text), "ox10 000000 HI:000000  o");
  graphics_draw_text(ctx, text, fonts_get_system_font("RESOURCE_ID_GOTHIC_14_BOLD"), GRect(0, -4, 144, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
// ************************************ //

  

  // draw score (6 digits max -- 7 digits technically possible on level 100)
  
  app_timer_register(UPDATE_MS, timer_callback, NULL); // Schedule a callback
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
  for (uint16_t i=0,j=0; i<9; i++,j+=7) playersprite[i] = gbitmap_create_as_sub_bitmap(playerspritesheet, GRect(j,0,7,7));
  init_board();
  init_player();
}

static void game_window_unload(Window *window) {
  for (uint16_t i=0; i<9; i++) gbitmap_destroy(playersprite[i]);
  gbitmap_destroy(background);
}

static void game_window_appear(Window *window) {}
static void game_window_disappear(Window *window) {}

static void init(void) {
  // Set up and push main window
  game_window = window_create();
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