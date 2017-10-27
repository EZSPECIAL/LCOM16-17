#ifndef LOGIC_H
#define LOGIC_H

#include "LoLCOM.h"

//Initializes the game for Player 1 mode, called between transitions from menu to game
//Returns 0 upon success, -1 otherwise
int8_t logic_player1_init();

//Frees memory used by the game in Player 1 mode
void logic_player1_free();

//Initializes the main menu for Player 1 mode, called at the start of Player 1 mode
//Game over and exiting the game transitions to this state
//Returns 0 upon success, -1 otherwise
int8_t logic_menu_init();

//Frees memory used by the game on the main menu of Player 1 mode
void logic_menu_free();

//State machine, changes states according to latest event and current state
void logic_change_state(game_event_t event);

//Main gameloop, updates cooldowns, entities and display
//Returns 0 upon success
int8_t logic_gameloop();

//Main dispatcher of Player 1 mode, receives interrupts from driver_receive
//and sends them to the correct handlers according to the current game state
//Returns 0 upon success
int8_t logic_handler(uint32_t data, uint8_t* pnumber, uint8_t* sync, uint8_t mode, origin_t origin);

//Loads a map .csv file to struct map_t
//Returns 0 upon success, -1 otherwise
int8_t logic_lmap(const unsigned char* filename, map_t* map);

//Given an entities coords determines the coords of the tile the entities' standing on
//Used for collision detection with the map
//param entity - coords of the entity to find current tile of
//Returns point_t of the tile
point_t logic_currtile(point_t entity);

//Counts down all cooldowns for the current game state
void logic_tick();

//Resets entity cooldowns, speed, spritesheet and walk animation flag
//param entity - entity to reset
void logic_reset_monster(entity_t* entity);

//Changes the sprite used by entity, updates it to a suitable orientation sprite
//param entity - entity to be updated
void logic_currsprite(entity_t* entity);

void logic_update_sword();

void logic_update_movement(entity_t* entity);

int8_t logic_playercol();

void logic_enemy_move(entity_t* entity);

int8_t logic_entitycol();

uint8_t logic_tilecolcycle(entity_t entity);

uint8_t logic_tilecollision(point_t entity, uint8_t isPC);

uint8_t logic_check_end();

int8_t logic_changemap(event_t direction);

uint8_t logic_scrollmap(event_t direction);

int8_t logic_updatedisplay();

int8_t logic_serial_free();

uint8_t logic_swordcol(entity_t entity);

uint8_t logic_aabbcol(entity_t entity);

int8_t logic_clear_enemies();

int8_t logic_rtc_handler();

int8_t logic_rtc_spawn(unsigned char* enemy_type);

int8_t logic_kbd_input(uint32_t scancode, uint8_t origin);

int8_t logic_game_over_fade(uint8_t stage);

int8_t logic_mouse_input(uint32_t data, uint8_t* pnumber, uint8_t* sync, uint8_t mode);

int8_t logic_mouse_handler(uint8_t mode);

int8_t logic_serial_handler(uint32_t serial_rcv);

int8_t logic_serial_spawn(unsigned char* enemy_type);

int8_t logic_serial_tick();

int8_t logic_serial_init();

int8_t logic_display_serial();

int8_t logic_lpng(png_t* png, const unsigned char* filename);

//-----------------------------------------------------
//font_t functions
//-----------------------------------------------------

int8_t logic_lfont(font_t* font);

int8_t logic_font_number(font_t* font);

//-----------------------------------------------------
//map_t functions
//-----------------------------------------------------

map_t map_get();

//-----------------------------------------------------
//entity_t functions
//-----------------------------------------------------

int8_t logic_lentity(const unsigned char* entity_name, entity_t* entity, uint8_t isPC);

#endif //LOGIC_H
