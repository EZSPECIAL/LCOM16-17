#ifndef LOLCOM_H
#define LOLCOM_H

//Legend of LCOM useful macros/constants

#define BIT(n) 			(0x01<<(n))

//Constants for image loading
#define COMPONENTS		3		//Number of components to read from image, 3 is RGB, 4 would be RGBA

//Constants for the Logic Module

#define MAPSIZE			176
#define TILESIZE		32				//Pixels used for square tiles (16x16, 32x32, etc)
#define ENTITY_N		8
#define STATUSBAR_H		TILESIZE * 4 	//Status bar height
#define MWIDTH			16
#define MHEIGHT			11
#define PC_CENTER_X		7.5 * TILESIZE
#define PC_CENTER_Y 	5 * TILESIZE
#define INIT_X			7
#define INIT_Y			7

#define PLAYER2			1
#define GAME			0

//Constants for menu

#define MENU_FRAMES			15
#define TRIFORCE_X			95
#define TRIFORCE_Y			134
#define TRIFORCE_ADJUST		30
#define MENU_MAX			1

//Constants for Game Over state

#define OVER_ANIM_FRAMES	8
#define FADE_FRAMES			20

//Constants for entities

#define LINK_I				0
#define SWORD_I				7
#define IFRAME_ROW			2
#define KNOCK_FRAMES		15
#define I_FRAMES			60
#define WALK_ANIM_FRAMES	15
#define MAX_SPEED			4
#define SPAWN_RTC			0
#define SPAWN_SERIAL		1
#define SPAWN_RATE			5
#define SWORD_W				14
#define SWORD_H				24
#define SWORD_FRAMES		30

//File paths for initialization functions

#define INITIAL_MAP		((const unsigned char*)"/tmp/resources/overworld_map/7_7.csv")
#define MAP_PATH		((const unsigned char*)"/tmp/resources/overworld_map/")
#define ENTITY_PATH		((const unsigned char*)"/tmp/resources/entity_data/")
#define FONT_PATH		((const unsigned char*)"/tmp/resources/tilesets/Font18x14.png")
#define IMG_PATH		((const unsigned char*)"/tmp/resources/images/")
#define OVER_PATH		((const unsigned char*)"/tmp/resources/tilesets/Overworld32d")
#define MUSIC_PATH		((const unsigned char*)"/tmp/resources/music/")

//Flags for loading map files

#define TILESET			BIT(0)	//Tells map loading function that tileset filename is coming next
#define	TILESPERLINE 	BIT(1)
#define NTILES			BIT(2)
#define MAPDATA			BIT(3)
#define COLLISION		BIT(4)
#define SPRITESHEET		BIT(5)
#define	HITPOINTS		BIT(6)
#define SPEED			BIT(7)

#define SCROLL_FRAMES	6

//Constants for graphics

#define VMODE			0x112			//Video mode used by the game
#define DOUBLEBUFFER	0
#define PAGEFLIP		1

#define BLACK			0			//Any RGB color is black when 0
#define TRANSPARENT		0xFF00FF 	//Color ignored when drawing (hot pink)

//Constants for font

#define FONT_H			14
#define FONT_W			18
#define FONT_TILES_LINE 10
#define FONT_START		48
#define FONT_Y_ADJUST	16
#define FONT_X_ADJUST	72

//Constants for mouse

#define MOUSE_TOL		3
#define MOUSE_SCROLL	3			//Mouse id returned when 4th packet with scroll wheel only is activated
#define MOUSE_SCROLL_EX	4			//Mouse id returned when 4th packet with extra buttons is activated

//Legend of LCOM event types

typedef enum {KBD_INT, MOUSE_INT, SERIAL_INT, RTC_INT, TIMER_INT} origin_t;
typedef enum {MOVE_DOWN, MOVE_LEFT, MOVE_UP, MOVE_RIGHT, MOVE_NONE} event_t;
typedef enum {NORMAL, ATTACKING, KNOCKBACK_DMG, IFRAMES} entity_state_t;
typedef enum {MENU, PLAYER1, GAMEOVER, END} state_t;
typedef enum {NA, MENUOPTION, PLAYER1_QUIT, EXITING, DIED} game_event_t;

//Legend of LCOM data structs

typedef struct {
	int16_t x;
	int16_t y;
} point_t;

typedef struct {
	int16_t x;
	int16_t y;
} vector_t;

typedef struct {
	uint16_t knockback;
	uint16_t attack;
	uint16_t iframes;
	uint16_t walk_anim;
	uint16_t move;
} cooldown_t;

typedef struct {
	point_t currmap;
	uint8_t menu_frame;
	uint8_t menu_countdown;
	uint8_t menu_choice;
	uint8_t death_anim_count;
	uint8_t death_fade_count;
	uint8_t death_f;
	uint8_t changemap_f;
	event_t changemap_dir;
	state_t state;
} game_state_t;

typedef struct {
	unsigned char* tileset;
	uint16_t tileset_width;
	uint8_t tilesperline;
	uint8_t ntiles;
	uint8_t map[MAPSIZE];
	uint8_t collision[MAPSIZE];
} map_t;

typedef struct {
	unsigned char* spritesheet;
	uint16_t spritesheet_width;
	uint8_t tilesperline;
	uint8_t ntiles;
	uint8_t currsprite;
	uint8_t walk_anim_f;
	point_t coords;				//Player character coords relative to play area, not full window
	uint8_t hitpoints;
	uint8_t isPC;				//Player character or enemy flag
	uint8_t speed;
	vector_t speed_vect;
	cooldown_t cooldown;
	event_t movement;
	entity_state_t state;
} entity_t;

typedef struct {
	unsigned char* fontdata;
	unsigned char word[64];
	uint8_t word_size;
	uint16_t number;
	uint16_t fontdata_width;
	uint8_t tilesperline;
	point_t coords;
} font_t;

typedef struct {
	unsigned char* image;
	uint16_t image_width;
	uint16_t image_height;
} png_t;

#endif //LOLCOM_H
