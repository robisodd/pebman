#include "pebble.h"
#define UPDATE_MS 50 // Refresh rate in ms
#define UPDATE_MAP_MS 1000 // Update map rate in ms  
#define MAP_W 28
#define MAP_H 31
#define ZOOM 5       // Number of pixels per map square
#define MAP_X 2      // Upper left corner of map
#define MAP_Y 13     // Upper left corner of map

/*
Byte: ULVHCTT
 bit - Sign Bit (0 and negative = can pass)
 bit - [U]p Horz wall (T7)
 bit - [L]eft Vert Wall (T6)
 bit - [V]ert Flip -- 1 = Flip tile vertically
 bit - [H]orz Flip -- 1 = Flip tile horizontally
 bit - [C]ategory (Corner or Not -- explained below)
2bit - [T]ype (Also below)

Category / Type
  CTT
  0   Other Category
  000 Blank (T0)
  001 Ghost Door (T9)
  010 Vertical Wall (T1)
  011 Horizontal Wall (T2)

  1   Corner Category
  100 Regular Type (T3)
  101 W/Outer Type (T4)
  110 w/Inner Type (T5)
  111 Squared Type (Ghost House) (T9)
  
Examples of the 5px X 5px tiles:
------- ------- ------- ------- ------- ------- ------- ------- ------- -------
|█████| |██ ██| |█████| |█████| |██   | |█████| | ████| |     | |█████| |█████|
|█████| |██ ██| |█████| |█████| |█ ███| |█████| | ████| |█████| |█████| |█████|
|█████| |██ ██| |     | |███  | | ██  | |███  | | ████| |█████| |██   | |█████|
|█████| |██ ██| |█████| |██ ██| | █ ██| |██ ██| | ████| |█████| |██ ██| |     |
|█████| |██ ██| |█████| |██ ██| | █ ██| |██ █ | | ████| |█████| |██ ██| |█████|
------- ------- ------- ------- ------- ------- ------- ------- ------- -------
   T0     T1      T2      T3      T4      T5      T6      T7      T8      T9
   
   

Pellets
█xxx█  ██x██  █████  █████  █████  █████
xxxxx  █xxx█  █XXX█  █████  █████  █████
xxxxx  xxxxx  █XXX█  ██X██  █████  █████
xxxxx  █xxx█  █XXX█  █████  █████  █████
█xxx█  ██x██  █████  █████  █████  █████

Ready!
█xx██  █XXX█  ██X██  █xx██  x███x  ███xx
x██x█  █X███  █X█X█  █x█x█  █x█x█  ██xxx
xxx██  █XX██  X███X  █x█x█  ██x██  ██xx█
x█x██  █X███  XXXXX  █x█x█  ██x██  █████
x██x█  █XXX█  X███X  █xx██  ██x██  x████

Sprite Positioning
 11111222223333344444555556666677777
1██ █████████ ████ █████████ ███████
1██ █████████ ████ █████████ ███████
1  ███████████    ███████████       
1███████████████████████████████████
1███████████████████████████████████
2███████████████████████████████████
2███████████████████████████████████
2███████████████████████████████████
2███████████████████████████████████
2███████████████████████████████████
3███████████████████████████████████
3███████████████████████████████████
3                    
3███████████████████████████████████
3███████████████████████████████████

1██ █████████ ████ █████████ ███████
1██ █████████ ████ █████████ ███████
1  ███████████    ███████████       
1███████████████████████████████████
1█████████████    ██████████████████
2████████████       ████████████████
2██████████████      ███████████████
2████████████████    ███████████████
2██████████████      ███████████████
2████████████       ████████████████
3█████████████    ██████████████████
3███████████████████████████████████
3                                   
3███████████████████████████████████
3███████████████████████████████████
*/
    
#define TD  -1 // Tile Dot
#define TP  -2 // Tile Power Pellet
#define TB  0b0000000 // Tile Blank

   // Byte:   ULVHCTT = Upper wall, Left wall, Vertical flip, Horizontal flip, Category/Corner, 2bit Type
#define TV  0b0000010 // Tile Vertical |
#define TH  0b0000011 // Tile Horizontal -

#define TVL 0b0100010 // Tile Vertical with Left Vertical  ||    (L wall)
#define TVR 0b0101010 // Tile Vertical with Right Vertical  ||   (L wall & H flip)
#define THU 0b1000011 // Tile Horizontal with Up Horizontal =    (U wall)
#define THD 0b1010011 // Tile Horizontal with Down Horizontal == (U wall & V flip)
  
#define TDR 0b0000100 // Tile Corner Down Right (normal)
#define TDL 0b0001100 // Tile Corner Down Left  (h flip)
#define TUR 0b0010100 // Tile Corner Up Right   (v flip)
#define TUL 0b0011100 // Tile Corner Up Left    (v&h flip)
  
#define ODR 0b0000101 // Tile Outer Down Right  (normal)
#define ODL 0b0001101 // Tile Outer Down Left   (h flip)
#define OUR 0b0010101 // Tile Outer Up Right    (v flip)
#define OUL 0b0011101 // Tile Outer Up Left     (v&h flip)

#define IDR 0b0000110 // Tile Inner Down Right  (normal)
#define IDL 0b0001110 // Tile Inner Down Left   (h flip)
#define IUR 0b0010110 // Tile Inner Up Right    (v flip)
#define IUL 0b0011110 // Tile Inner Up Left     (v&h flip)
  
#define GDR 0b0000111 // Ghost Down Right       (normal)
#define GDL 0b0001111 // Ghost Corner Down Left (h flip)
#define GUR 0b0010111 // Ghost Corner Up Right  (v flip)
#define GUL 0b0011111 // Ghost Corner Up Left   (v&h flip)

#define UDR 0b1000100 // Tile Up Wall Down Right  (normal)
#define UDL 0b1001100 // Tile Up Wall Down Left   (h flip)
#define UUR 0b1010100 // Tile Down Wall Up Right  (v flip)
#define UUL 0b1011100 // Tile Down Wall Up Left   (v&h flip)
  
#define LDR 0b0100100 // Tile Left Wall Down Right  (normal)
#define LDL 0b0101100 // Tile Left Wall Down Left   (h flip)
#define LUR 0b0110100 // Tile Left Wall Up Right  (v flip)
#define LUL 0b0111100 // Tile Left Wall Up Left   (v&h flip)
  
#define GOL 0b0000001 // Ghost Opening Left
#define GOR 0b0001001 // Ghost Opening Right
int GOStatus=0; // Ghost Opening Status (Door Position)
  

  
static Window *window;
static GRect window_frame;
static Layer *graphics_layer;
static AppTimer *timer;
static AppTimer *timer2;

// ------------------------------------------------------------------------ //
//  Map Functions
// ------------------------------------------------------------------------ //
static int8_t map[MAP_W * MAP_H];  // int8 means cells can be from -127 to 128


static int8_t map[MAP_W * MAP_H] =
{ODR,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,UDL,UDR,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,THU,ODL,
 TVL, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD,TVR,
 TVL, TD,TDR, TH, TH,TDL, TD,TDR, TH, TH, TH,TDL, TD, TV, TV, TD,TDR, TH, TH, TH,TDL, TD,TDR, TH, TH,TDL, TD,TVR,
 TVL, TP, TV, TB, TB, TV, TD, TV, TB, TB, TB, TV, TD, TV, TV, TD, TV, TB, TB, TB, TV, TD, TV, TB, TB, TV, TP,TVR,
 TVL, TD,TUR, TH, TH,TUL, TD,TUR, TH, TH, TH,TUL, TD,TUR,TUL, TD,TUR, TH, TH, TH,TUL, TD,TUR, TH, TH,TUL, TD,TVR,
 TVL, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD,TVR,
 TVL, TD,TDR, TH, TH,TDL, TD,TDR,TDL, TD,TDR, TH, TH, TH, TH, TH, TH,TDL, TD,TDR,TDL, TD,TDR, TH, TH,TDL, TD,TVR,
 TVL, TD,TUR, TH, TH,TUL, TD, TV, TV, TD,TUR, TH, TH,TDL,TDR, TH, TH,TUL, TD, TV, TV, TD,TUR, TH, TH,TUL, TD,TVR,
 TVL, TD, TD, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TD, TD,TVR,
 OUR,THD,THD,THD,THD,IDL, TD, TV,TUR, TH, TH,TDL, TB, TV, TV, TB,TDR, TH, TH,TUL, TV, TD,IDR,THD,THD,THD,THD,OUL,
  TB, TB, TB, TB, TB,TVL, TD, TV,TDR, TH, TH,TUL, TB,TUR,TUL, TB,TUR, TH, TH,TDL, TV, TD,TVR, TB, TB, TB, TB, TB,
  TB, TB, TB, TB, TB,TVL, TD, TV, TV, TB, TB, TB, TB, TB, TB, TB, TB, TB, TB, TV, TV, TD,TVR, TB, TB, TB, TB, TB,
  TB, TB, TB, TB, TB,TVL, TD, TV, TV, TB,GDR,THD,THD,GOL,GOR,THD,THD,GDL, TB, TV, TV, TD,TVR, TB, TB, TB, TB, TB,
 THU,THU,THU,THU,THU,IUL, TD,TUR,TUL, TB,TVR, TB, TB, TB, TB, TB, TB,TVL, TB,TUR,TUL, TD,IUR,THU,THU,THU,THU,THU,
  TB, TB, TB, TB, TB, TB, TD, TB, TB, TB,TVR, TB, TB, TB, TB, TB, TB,TVL, TB, TB, TB, TD, TB, TB, TB, TB, TB, TB,
 THD,THD,THD,THD,THD,IDL, TD,TDR,TDL, TB,TVR, TB, TB, TB, TB, TB, TB,TVL, TB,TDR,TDL, TD,IDR,THD,THD,THD,THD,THD,
  TB, TB, TB, TB, TB,TVL, TD, TV, TV, TB,GUR,THU,THU,THU,THU,THU,THU,GUL, TB, TV, TV, TD,TVR, TB, TB, TB, TB, TB,
  TB, TB, TB, TB, TB,TVL, TD, TV, TV, TB, TB, TB, TB, TB, TB, TB, TB, TB, TB, TV, TV, TD,TVR, TB, TB, TB, TB, TB,
  TB, TB, TB, TB, TB,TVL, TD, TV, TV, TB,TDR, TH, TH, TH, TH, TH, TH,TDL, TB, TV, TV, TD,TVR, TB, TB, TB, TB, TB, 
 ODR,THU,THU,THU,THU,IUL, TD,TUR,TUL, TB,TUR, TH, TH,TDL,TDR, TH, TH,TUL, TB,TUR,TUL, TD,IUR,THU,THU,THU,THU,ODL,
 TVL, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD,TVR,
 TVL, TD,TDR, TH, TH,TDL, TD,TDR, TH, TH, TH,TDL, TD, TV, TV, TD,TDR, TH, TH, TH,TDL, TD,TDR, TH, TH,TDL, TD,TVR,
 TVL, TD,TUR, TH,TDL, TV, TD,TUR, TH, TH, TH,TUL, TD,TUR,TUL, TD,TUR, TH, TH, TH,TUL, TD, TV,TDR, TH,TUL, TD,TVR,
 TVL, TP, TD, TD, TV, TV, TD, TD, TD, TD, TD, TD, TD, TB, TB, TD, TD, TD, TD, TD, TD, TD, TV, TV, TD, TD, TP,TVR,
 LUR, TH,TDL, TD, TV, TV, TD,TDR,TDL, TD,TDR, TH, TH, TH, TH, TH, TH,TDL, TD,TDR,TDL, TD, TV, TV, TD,TDR, TH,LUL,
 LDR, TH,TUL, TD,TUR,TUL, TD, TV, TV, TD,TUR, TH, TH,TDL,TDR, TH, TH,TUL, TD, TV, TV, TD,TUR,TUL, TD,TUR, TH,LDL,
 TVL, TD, TD, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TV, TV, TD, TD, TD, TD, TD, TD,TVR,
 TVL, TD,TDR, TH, TH, TH, TH,TUL,TUR, TH, TH,TDL, TD, TV, TV, TD,TDR, TH, TH,TUL,TUR, TH, TH, TH, TH,TDL, TD,TVR,
 TVL, TD,TUR, TH, TH, TH, TH, TH, TH, TH, TH,TUL, TD,TUR,TUL, TD,TUR, TH, TH, TH, TH, TH, TH, TH, TH,TUL, TD,TVR,
 TVL, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD, TD,TVR,
 OUR,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,THD,OUL
};
  

void GenerateMap() {

}
/*
typedef struct Actor {
  int32_t x;             // Player's X Position (Center of Sprite)
  int32_t y;             // Player's Y Position (Center of Sprite)
  int32_t f;             // Player Direction Facing (from 0 - TRIG_MAX_ANGLE)
} Actor;
static Actor player;
static Actor ghost[4];
void init_actors() {
  player = (Actor){.x=20, .y=20, .f=0};
  ghost[1].x=40; ghost[1].y=50; ghost[1].f=0;
  ghost[2].x=60; ghost[2].y=50; ghost[2].f=0;
  ghost[3].x=80; ghost[3].y=50; ghost[3].f=0;
}

void draw_actors() {
  
}
*/
int8_t getmap(uint16_t x, uint16_t y) {
  if (x>=MAP_W || y>=MAP_H) return TD; 
  return map[(int)y * MAP_W + (int)x];
}

void setmap(int16_t x, int16_t y, int8_t value) {
  if ((x >= 0) && (x < MAP_W) && (y >= 0) && (y < MAP_H))
    map[y * MAP_W + x] = value;
}

static void draw_tile(GContext *ctx, uint8_t x, uint8_t y, int8_t tile) {
  if(tile<=0) {  // Non-rotating, non-flippable tiles, dots and blanks
    if(tile==-1) graphics_draw_pixel(ctx, GPoint(x,y)); // dot
    if(tile==-2) {                                      // Power Pellet
      graphics_fill_rect(ctx, GRect(x-1, y-1, 3, 3), 0, GCornerNone);
      graphics_draw_pixel(ctx, GPoint(x-2,y));
      graphics_draw_pixel(ctx, GPoint(x+2,y));
      graphics_draw_pixel(ctx, GPoint(x,y-2));
      graphics_draw_pixel(ctx, GPoint(x,y+2));
    }
    return;
  }
  
     // Byte: 0bULVHCTT
  int8_t hflip=-1 * ((((tile>>3)&1)*2) - 1);  // -1 or 1
  int8_t vflip=-1 * ((((tile>>4)&1)*2) - 1);  // -1 or 1
  int8_t lwall=(tile>>5)&1;
  int8_t uwall=(tile>>6)&1;
  
  // Draw horizontal or vertical double walls with flips
  if(uwall) graphics_draw_line(ctx, GPoint(x-2,y-(2*vflip)), GPoint(x+2, y-(2*vflip)));
  if(lwall) graphics_draw_line(ctx, GPoint(x-(2*hflip),y-2), GPoint(x-(2*hflip), y+2));
  
  if(((tile>>2)&1)==0) { // Category Other
    switch(tile&7) {
      case 1: graphics_draw_line(ctx, GPoint(x-3*hflip,y+1), GPoint(x+(2-GOStatus)*hflip, y+1)); break;// Ghost House Door
      case 2: graphics_draw_line(ctx, GPoint(x,y-2), GPoint(x, y+2)); break;       // Vertical Wall
      case 3: graphics_draw_line(ctx, GPoint(x-2,y), GPoint(x+2, y)); break; // Horizontal Wall
    }
  } else {             // Category Corner
    
    if((tile&3)!=2) { // Draw normal corner with v&h flip
      graphics_draw_pixel(ctx, GPoint(x+2*hflip,y));
      graphics_draw_pixel(ctx, GPoint(x+hflip,y));
      graphics_draw_pixel(ctx, GPoint(x,y+vflip));
      graphics_draw_pixel(ctx, GPoint(x,y+2*vflip));
    }
    switch(tile&3){ // Special corners
      case 1: // Outer lines with v&h flip
        graphics_draw_line(ctx, GPoint(x,y-2*vflip), GPoint(x+(2*hflip), y-2*vflip));
        graphics_draw_pixel(ctx, GPoint(x-hflip,y-vflip)); 
        graphics_draw_line(ctx, GPoint(x-(2*hflip),y), GPoint(x-(2*hflip), y+2*vflip));
      break;
      case 2: // Inner Rounded Corner with v&h flip
        graphics_draw_pixel(ctx, GPoint(x+(2*hflip),y));
        graphics_draw_pixel(ctx, GPoint(x+hflip,y+vflip));
        graphics_draw_pixel(ctx, GPoint(x,y+2*vflip));
      break; 
      case 3:// Corner Dot and Center Dot
        graphics_draw_pixel(ctx, GPoint(x+2*hflip,y+2*vflip));
        graphics_draw_pixel(ctx, GPoint(x,y)); 
    }
    
  }
  
 /*
  static char text[40];
  graphics_context_set_text_color(ctx, GColorWhite);
  snprintf(text, sizeof(text), "(x%d,y%d) s(%d, %d)",(int)disc->pos.x,(int)disc->pos.y, (int)(disc->vel.x*100),(int)(disc->vel.y*100));
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(1, 1, 143, 143), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  snprintf(text, sizeof(text), "[%d, %d] (%d, %d)",(int)(disc->radius - disc->pos.x), (int)(disc->radius - disc->pos.y), (int)(proport1*10000),(int)(proport2*10000));
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(1, 17, 143, 143), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  
  graphics_fill_circle(ctx, GPoint(disc->pos.x, disc->pos.y), disc->radius);
*/
}


// ------------------------------------------------------------------------ //
//  Drawing Functions
// ------------------------------------------------------------------------ //
static void graphics_layer_update(Layer *me, GContext *ctx) {
  /*
  //for (int i = 0; i < NUM_DISCS; i++) disc_draw(ctx, &discs[i]); graphics_context_set_stroke_color(ctx, GColorWhite);
  static char text[40];  //Buffer to hold text
  GRect textframe = GRect(0, 0, 143, 20);  // Text Box Position and Size
  snprintf(text, sizeof(text), " x:%d y:%d", (int)(mouse.pos.x), (int)(mouse.pos.y));  // What text to draw
  graphics_context_set_fill_color(ctx, 0); graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Filled Rectangle
  graphics_context_set_stroke_color(ctx, 1); graphics_draw_rect(ctx, textframe);                //White Rectangle Border
  graphics_context_set_text_color(ctx, 1);  // White Text
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  //Write Text
*/
  graphics_context_set_stroke_color(ctx, 1);
  graphics_context_set_fill_color(ctx, GColorWhite);
  //graphics_fill_circle(ctx, GPoint(10,10), 160);
  for (uint16_t x = 0; x < MAP_W; x++)
    for (uint16_t y = 0; y < MAP_H; y++) {
    //graphics_context_set_fill_color(ctx, rand() % 2);
      draw_tile(ctx,(x*ZOOM)+MAP_X+2, (y*ZOOM)+MAP_Y+2, getmap(x,y));
      //graphics_context_set_fill_color(ctx, getmap(x,y));
      //graphics_fill_rect(ctx, GRect((x*ZOOM)+MAP_X, (y*ZOOM)+MAP_Y, ZOOM, ZOOM), 0, GCornerNone);
    }
    //uint16_t millis = time_ms(NULL, NULL);
    //graphics_context_set_fill_color(ctx, (time_ms(NULL, NULL) % 500)>250?0:1 );
    //graphics_fill_rect(ctx, GRect((player.x*ZOOM)+MAP_X, (player.y*ZOOM)+MAP_Y, ZOOM, ZOOM), 0, GCornerNone);



  
}

// ------------------------------------------------------------------------ //
//  Main Functions
// ------------------------------------------------------------------------ //
static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);

  //accel.x = (accel.x - 500) / 10;
  //accel.y = (accel.y - 500) / 10;

  layer_mark_dirty(graphics_layer);

  timer = app_timer_register(UPDATE_MS, timer_callback, NULL);
}

static void update_map(void *data) {
  GenerateMap();
  timer2 = app_timer_register(UPDATE_MAP_MS, update_map, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = window_frame = layer_get_frame(window_layer);

  graphics_layer = layer_create(frame);
  layer_set_update_proc(graphics_layer, graphics_layer_update);
  layer_add_child(window_layer, graphics_layer);

}

static void window_unload(Window *window) {
  layer_destroy(graphics_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_set_fullscreen(window, true);
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);
  srand(time(NULL));  // Seed randomizer
  accel_data_service_subscribe(0, NULL);

  //GRect frame = window_frame;
  //mouse.pos.x = frame.size.w/2;
  //mouse.pos.y = frame.size.h/2;
  GenerateMap();
  //init_actors();
  timer = app_timer_register(UPDATE_MS, timer_callback, NULL);
  timer2 = app_timer_register(UPDATE_MAP_MS, update_map, NULL);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
