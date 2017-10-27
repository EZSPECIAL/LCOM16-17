#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h" //library for reading image files, namely PNG
#include "LoLCOM.h"
#include "video_gr.h"
#include "logic.h"
#include "helper.h"
#include "i8042.h"
#include "RTC.h"
#include "mouse.h"
#include "UART.h"

//Game state
static game_state_t game = {{INIT_X, INIT_Y}, 0, 0, 0, 0, 0, FALSE, FALSE, MOVE_NONE, MENU};	//Game state
static game_event_t latest_event = NA;
static const game_state_t game_base = {{INIT_X, INIT_Y}, 0, 0, 0, 0, 0, FALSE, FALSE, MOVE_NONE, MENU};

//Map data
static map_t currentmap = {0};	//Current map
static map_t nextmap = {0};		//Map that will be the next currentmap
static map_t currentcopy = {0};	//Used for scrolling, contains a copy of the old currentmap
static const map_t map_base = {0};

//Entity data
static entity_t entities[ENTITY_N] = {0};
static const entity_t entity_base = {0};

//Text data
static font_t score = {0};
static font_t serial_cooldown = {0};
static font_t link_hp = {0};
static const font_t font_base = {0};

//Image data
static png_t serial_image = {0};
static png_t menu[3] = {0};
static png_t triforce = {0};
static png_t game_over_screen = {0};
static const png_t png_base = {0};

//Other data
static uint16_t scroll_line = 0; //Used for scrolling the map, current line being scrolled
static uint8_t scroll_delay = 0; //Adds delay to scrolling without interfering with timer, still runs at 60hz
static uint32_t last_keypress = 0; //Last keypress by the player
static uint8_t packet[4]; //Mouse packets
static uint8_t game_over_stage = 0;

void logic_change_state(game_event_t event) {

	if(event == NA) {
		return;
	}

	switch(game.state) {
	case MENU:
		if(event == MENUOPTION) {
			game.state = PLAYER1;
			logic_menu_free();
			vg_change_buffering(game.menu_choice);
			if(logic_player1_init() != 0) {
				panic("LoLCOM: player1_init: failed\n");
			}
			latest_event = NA;
		} else if(event == EXITING) {
			game.state = END;
			logic_menu_free();
			latest_event = NA;
		}
		break;
	case PLAYER1:
		if(event == PLAYER1_QUIT) {
			game.state = MENU;
			logic_player1_free();
			if(logic_menu_init() != 0) {
				panic("LoLCOM: menu_init: failed\n");
			}
			latest_event = NA;
		} else if(event == DIED) {
			game.state = GAMEOVER;
			game.death_f = TRUE;
			game_over_stage = 0;
			latest_event = NA;
		}
		break;
	case GAMEOVER:
		if(event == EXITING) {
			game.state = MENU;
			logic_player1_free();
			if(logic_menu_init() != 0) {
				panic("LoLCOM: menu_init: failed\n");
			}
			latest_event = NA;
		}
	default:
		break;
	}
}


int8_t logic_handler(uint32_t data, uint8_t* pnumber, uint8_t* sync, uint8_t mode, origin_t origin) {

	logic_change_state(latest_event);

	if(game.state == END) {
		return 0;
	}

	switch(origin) {
	case TIMER_INT:
		logic_gameloop();
		break;
	case KBD_INT:
		logic_kbd_input(data, mode);
		break;
	case MOUSE_INT:
		logic_mouse_input(data, pnumber, sync, mode);
		break;
	case RTC_INT:
		logic_rtc_handler();
		break;
	case SERIAL_INT:
		if(game.state == PLAYER1) {
			logic_serial_handler(data);
		}
		break;
	default:
		break;
	}

	return 0;
}


int8_t logic_player1_init() {

	game = game_base;
	game.state = PLAYER1;

	scroll_line = 0;
	scroll_delay = 0;
	last_keypress = 0;

	currentmap = map_base;
	nextmap = map_base;
	currentcopy = map_base;

	score = font_base;
	link_hp = font_base;

	size_t i;
	for(i = 0; i < ENTITY_N; i++) {
		entities[i] = entity_base;
	}

	//Load initial map (7, 7)
	if(logic_lmap(INITIAL_MAP, &currentmap) != 0) {
		return -1;
	}

	if(logic_lentity("link", &entities[LINK_I], TRUE) != 0) {
		return -1;
	}

	if(logic_lentity("sword", &entities[SWORD_I], FALSE) != 0) {
		return -1;
	}

	entities[SWORD_I].coords = (point_t){entities[LINK_I].coords.x, entities[LINK_I].coords.y - TILESIZE};

	if(logic_lfont(&score) != 0) {
		return -1;
	}

	if(logic_lfont(&link_hp) != 0) {
		return -1;
	}

	strcpy(score.word, "SCORE:");
	score.word_size = strlen("SCORE:");
	logic_font_number(&score);

	strcpy(link_hp.word, "HP:");
	link_hp.word_size = strlen("HP:");
	link_hp.number = entities[LINK_I].hitpoints;
	logic_font_number(&link_hp);

	rtc_read_register(RTC_STATUS_C); //Make sure nothing is stopping RTC interrupts
	rtc_setalarm_s(SPAWN_RATE);

	return 0;
}


void logic_player1_free() {

	stbi_image_free(currentmap.tileset);

	if(game.changemap_f == FALSE) {
		stbi_image_free(currentcopy.tileset);
	}

	stbi_image_free(score.fontdata);
	stbi_image_free(link_hp.fontdata);

	size_t i;
	for(i = 0; i < ENTITY_N; i++) {
		stbi_image_free(entities[i].spritesheet);
	}
}


int8_t logic_menu_init() {

	game = game_base;
	game.state = MENU;
	game.menu_countdown = MENU_FRAMES;

	menu[0] = png_base;
	menu[1] = png_base;
	menu[2] = png_base;
	triforce = png_base;

	if(logic_lpng(&menu[0], "Menu1.png") != 0) {
		return -1;
	}

	if(logic_lpng(&menu[1], "Menu2.png") != 0) {
		return -1;
	}

	if(logic_lpng(&menu[2], "Menu3.png") != 0) {
		return -1;
	}

	if(logic_lpng(&triforce, "Triforce.png") != 0) {
		return -1;
	}

	return 0;
}


void logic_menu_free() {
	stbi_image_free(menu[0].image);
	stbi_image_free(menu[1].image);
	stbi_image_free(menu[2].image);
	stbi_image_free(triforce.image);
}


int8_t logic_gameloop() {

	if(game.state == MENU || game.state == GAMEOVER) {
		logic_tick();
		logic_updatedisplay();
	} else if(game.state == PLAYER1) {
		logic_tick();
		logic_playercol();
		logic_entitycol();
		logic_update_sword();
		logic_updatedisplay();
	}

	return 0;
}


void logic_tick() {

	if(game.state == MENU) {
		game.menu_countdown--;

		if(game.menu_countdown == 0) {
			game.menu_frame++;
			game.menu_countdown = MENU_FRAMES;
		}

		if(game.menu_frame > 2) {
			game.menu_frame = 0;
		}
	} else if(game.state == PLAYER1) {

		logic_font_number(&score);
		logic_font_number(&link_hp);

		size_t i;
		for(i = 0; i < ENTITY_N - 1; i++) {

			if(entities[i].hitpoints != 0) {

				switch(entities[i].state) {
				case KNOCKBACK_DMG:
					entities[i].cooldown.knockback--;
					break;
				case IFRAMES:
					entities[i].cooldown.iframes--;
					break;
				default:
					break;
				}

				if(entities[i].cooldown.walk_anim == 0) {
					entities[i].cooldown.walk_anim = WALK_ANIM_FRAMES;
					entities[i].walk_anim_f ^= BIT(0);
				} else {
					entities[i].cooldown.walk_anim--;
				}

				if(entities[i].cooldown.move == 0 && entities[i].isPC == FALSE) {
					entities[i].cooldown.move = 60;
				} else if(entities[i].isPC == FALSE) {
					entities[i].cooldown.move--;
				}

				if(entities[i].cooldown.knockback == 0 && entities[i].state == KNOCKBACK_DMG) {

					entities[i].state = IFRAMES;
					entities[i].movement = MOVE_NONE;
					entities[i].speed_vect = (vector_t){0, 0};

					if(entities[i].isPC == TRUE) {
						logic_kbd_input(last_keypress, GAME);
					}

				} else if(entities[i].cooldown.iframes == 0 && entities[i].state == IFRAMES) {
					entities[i].state = NORMAL;
					entities[i].currsprite -= entities[i].tilesperline * IFRAME_ROW;
				}
			}
		}

		if(entities[SWORD_I].cooldown.attack != 0) {
			entities[SWORD_I].cooldown.attack--;
			if(entities[SWORD_I].cooldown.attack == SWORD_FRAMES / 2) {
				entities[SWORD_I].hitpoints = 0;
				entities[LINK_I].currsprite -= entities[LINK_I].tilesperline * 4;
			}
		}
	} else if(game.state == GAMEOVER) {
		entities[LINK_I].speed_vect = (vector_t) {0, 0};

		if(game.death_fade_count == 0) {
			if(game_over_stage < 5) {
				logic_game_over_fade(game_over_stage);
				game_over_stage++;
			} else game.death_f = FALSE;
			game.death_fade_count = FADE_FRAMES;
		} else game.death_fade_count--;

		if(game.death_anim_count == 0) {

			if(entities[LINK_I].currsprite >= 3) {
				entities[LINK_I].currsprite = 0;
			} else entities[LINK_I].currsprite++;
			game.death_anim_count = OVER_ANIM_FRAMES;
		} else game.death_anim_count--;

	}
}


void logic_currsprite(entity_t* entity) {

	//Entity is against a wall and knockback just happened
	if(entity->speed_vect.x == 0 && entity->speed_vect.y == 0 && entity->cooldown.knockback == KNOCK_FRAMES - 1) {
		entity->currsprite += entity->tilesperline * IFRAME_ROW;
	}

	//Entity is in knockback or invincibility state
	else if(entity->state == IFRAMES || entity->state == KNOCKBACK_DMG) {
		if(entity->speed_vect.x > 0) {
			entity->currsprite = (uint8_t) MOVE_RIGHT + entity->tilesperline * IFRAME_ROW + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.x < 0) {
			entity->currsprite = (uint8_t) MOVE_LEFT + entity->tilesperline * IFRAME_ROW + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.y > 0) {
			entity->currsprite = (uint8_t) MOVE_DOWN + entity->tilesperline * IFRAME_ROW + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.y < 0) {
			entity->currsprite = (uint8_t) MOVE_UP + entity->tilesperline * IFRAME_ROW + entity->tilesperline * entity->walk_anim_f;
		}
	}

	else if(entity->isPC == TRUE && entities[SWORD_I].hitpoints != 0) {
		entity->currsprite = entities[SWORD_I].currsprite + entity->tilesperline * 4;
	}

	//Entity is in normal state
	else {
		if(entity->speed_vect.x > 0) {
			entity->currsprite = (uint8_t) MOVE_RIGHT + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.x < 0) {
			entity->currsprite = (uint8_t) MOVE_LEFT + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.y > 0) {
			entity->currsprite = (uint8_t) MOVE_DOWN + entity->tilesperline * entity->walk_anim_f;
		} else if(entity->speed_vect.y < 0) {
			entity->currsprite = (uint8_t) MOVE_UP + entity->tilesperline * entity->walk_anim_f;
		}
	}
}


void logic_update_sword() {
	if(entities[LINK_I].movement != MOVE_NONE) {
		entities[SWORD_I].currsprite = (uint8_t) entities[LINK_I].movement;
		entities[SWORD_I].movement = entities[LINK_I].movement;
		switch(entities[LINK_I].movement) {
		case MOVE_UP:
			entities[SWORD_I].coords = (point_t){entities[LINK_I].coords.x, entities[LINK_I].coords.y - TILESIZE};
			break;
		case MOVE_DOWN:
			entities[SWORD_I].coords = (point_t){entities[LINK_I].coords.x, entities[LINK_I].coords.y + TILESIZE};
			break;
		case MOVE_LEFT:
			entities[SWORD_I].coords = (point_t){entities[LINK_I].coords.x - TILESIZE, entities[LINK_I].coords.y};
			break;
		case MOVE_RIGHT:
			entities[SWORD_I].coords = (point_t){entities[LINK_I].coords.x + TILESIZE, entities[LINK_I].coords.y};
			break;
		default:
			break;
		}
	}
}


void logic_update_movement(entity_t* entity) {
	if(entity->speed_vect.x > 0) {
		entity->movement = MOVE_RIGHT;
	} else if(entity->speed_vect.x < 0) {
		entity->movement = MOVE_LEFT;
	} else if(entity->speed_vect.y > 0) {
		entity->movement = MOVE_DOWN;
	} else if(entity->speed_vect.y < 0) {
		entity->movement = MOVE_UP;
	}
}


int8_t logic_playercol() {

	//Update player sprite
	logic_currsprite(&entities[LINK_I]);

	//Map transition flag stops movement handling
	if(game.changemap_f == TRUE) {
		return 0;
	}

	uint8_t entity_col = FALSE;
	size_t i = 0;

	//Check for sprite collisions if player is in NORMAL state
	if(entities[LINK_I].state == NORMAL) {
		for(i = 1; i < ENTITY_N - 1; i++) {
			if(entities[i].hitpoints != 0) {
				entity_col = logic_aabbcol(entities[i]);
			}

			if(entity_col == TRUE) {
				break;
			}
		}
	}

	//Handle sprite collisions
	if(entity_col == TRUE) {

		if(entities[LINK_I].speed_vect.x == 0 && entities[LINK_I].speed_vect.y == 0) {
			entities[LINK_I].speed_vect.x = 2 * entities[i].speed_vect.x;
			entities[LINK_I].speed_vect.y = 2 * entities[i].speed_vect.y;
		} else {
			entities[LINK_I].speed_vect.x *= -2;
			entities[LINK_I].speed_vect.y *= -2;
		}

		entities[LINK_I].speed_vect.x = clamp_int16(entities[LINK_I].speed_vect.x, -MAX_SPEED, MAX_SPEED);
		entities[LINK_I].speed_vect.y = clamp_int16(entities[LINK_I].speed_vect.y, -MAX_SPEED, MAX_SPEED);

		entities[LINK_I].cooldown.knockback = KNOCK_FRAMES;
		entities[LINK_I].cooldown.iframes = I_FRAMES;

		entities[SWORD_I].state = NORMAL;
		entities[SWORD_I].hitpoints = 0;
		entities[SWORD_I].cooldown.attack = 0;

		logic_update_movement(&entities[LINK_I]);
		entities[LINK_I].state = KNOCKBACK_DMG;

		if(entities[LINK_I].hitpoints != 0) {
			entities[LINK_I].hitpoints--;
			link_hp.number = entities[LINK_I].hitpoints;
			if(entities[LINK_I].hitpoints == 0) {
				latest_event = DIED;
				return 0;
			}
		}
	}

	uint8_t map_col = FALSE;

	//Check for map collisions
	map_col = logic_tilecolcycle(entities[LINK_I]);

	//Map collision triggered map transition
	if(game.changemap_f == TRUE) {
		logic_changemap(game.changemap_dir);
		entities[LINK_I].currsprite = (uint8_t) game.changemap_dir;
		return 0;
	}

	//Handle map collision
	if(map_col == FALSE) {
		entities[LINK_I].coords.x += entities[LINK_I].speed_vect.x;
		entities[LINK_I].coords.y += entities[LINK_I].speed_vect.y;
	} else {
		entities[LINK_I].speed_vect = (vector_t){0, 0};
	}

	return 0;
}


int8_t logic_entitycol() {

	//If scrolling then no movement or collisions happen
	if(game.changemap_f == TRUE) {
		return 0;
	}

	size_t i;
	for(i = 1; i < ENTITY_N - 1; i++) {

		if(entities[i].hitpoints != 0) {
			uint8_t entity_col = FALSE;
			uint8_t map_col = FALSE;

			if(entities[i].cooldown.move == 0 && entities[i].state == NORMAL) {
				logic_enemy_move(&entities[i]);
			}

			//Update entity sprite
			logic_currsprite(&entities[i]);

			if(entities[i].state == NORMAL && entities[SWORD_I].hitpoints != 0) {
				entity_col = logic_swordcol(entities[i]);
			}

			if(entity_col == TRUE) {
				switch(entities[SWORD_I].movement) {
				case MOVE_UP:
					entities[i].speed_vect = (vector_t){0, -MAX_SPEED};
					break;
				case MOVE_DOWN:
					entities[i].speed_vect = (vector_t){0, MAX_SPEED};
					break;
				case MOVE_LEFT:
					entities[i].speed_vect = (vector_t){-MAX_SPEED, 0};
					break;
				case MOVE_RIGHT:
					entities[i].speed_vect = (vector_t){MAX_SPEED, 0};
					break;
				default:
					break;
				}

				entities[i].speed_vect.x = clamp_int16(entities[i].speed_vect.x, -MAX_SPEED, MAX_SPEED);
				entities[i].speed_vect.y = clamp_int16(entities[i].speed_vect.y, -MAX_SPEED, MAX_SPEED);

				entities[i].cooldown.knockback = KNOCK_FRAMES;
				entities[i].cooldown.iframes = I_FRAMES;

				entities[i].state = KNOCKBACK_DMG;

				entities[i].hitpoints--;
				if(entities[i].hitpoints != 0) {
					entities[i].hitpoints--;
				}

				if(entities[i].hitpoints == 0) {
					logic_reset_monster(&entities[i]);
					score.number += 9;
				}
			}

			//Check for map collisions
			map_col = logic_tilecolcycle(entities[i]);

			//Handle map collision
			if(map_col == FALSE) {
				entities[i].coords.x += entities[i].speed_vect.x;
				entities[i].coords.y += entities[i].speed_vect.y;
			} else {
				entities[i].speed_vect = (vector_t){0, 0};
			}
		}
	}

	return 0;
}


void logic_reset_monster(entity_t* entity) {

	stbi_image_free(entity->spritesheet);
	entity->spritesheet = NULL;

	entity->walk_anim_f = FALSE;
	entity->speed_vect = (vector_t) {0, 0};
	entity->cooldown = (cooldown_t) {0, 0, 0, 0, 0};

}


uint8_t logic_swordcol(entity_t entity) {

	switch(entities[SWORD_I].movement) {
	case MOVE_UP:
		if(entities[SWORD_I].coords.x + 8 < entity.coords.x + TILESIZE - 1 &&
				entities[SWORD_I].coords.x + 8 + SWORD_W - 1 > entity.coords.x &&
				entities[SWORD_I].coords.y + 8 < entity.coords.y + TILESIZE - 1 &&
				entities[SWORD_I].coords.y + 8 + SWORD_H - 1 > entity.coords.y) {
			return TRUE;
		}
		break;
	case MOVE_DOWN:
		if(entities[SWORD_I].coords.x + 10 < entity.coords.x + TILESIZE - 1 &&
				entities[SWORD_I].coords.x + 10 + SWORD_W - 1 > entity.coords.x &&
				entities[SWORD_I].coords.y < entity.coords.y + TILESIZE - 1 &&
				entities[SWORD_I].coords.y + SWORD_H - 1 > entity.coords.y) {
			return TRUE;
		}
		break;
	case MOVE_LEFT:
		if(entities[SWORD_I].coords.x + 8 < entity.coords.x + TILESIZE - 1 &&
				entities[SWORD_I].coords.x + 8 + SWORD_H - 1 > entity.coords.x &&
				entities[SWORD_I].coords.y + 10 < entity.coords.y + TILESIZE - 1 &&
				entities[SWORD_I].coords.y + 10 + SWORD_W - 1 > entity.coords.y) {
			return TRUE;
		}
		break;
	case MOVE_RIGHT:
		if(entities[SWORD_I].coords.x < entity.coords.x + TILESIZE - 1 &&
				entities[SWORD_I].coords.x + SWORD_H - 1 > entity.coords.x &&
				entities[SWORD_I].coords.y + 10 < entity.coords.y + TILESIZE - 1 &&
				entities[SWORD_I].coords.y + 10 + SWORD_W - 1 > entity.coords.y) {
			return TRUE;
		}
		break;
	default:
		break;
	}

	return FALSE;
}


uint8_t logic_aabbcol(entity_t entity) {

	if(entities[LINK_I].coords.x < entity.coords.x + TILESIZE - 1 &&
			entities[LINK_I].coords.x + TILESIZE - 1 > entity.coords.x &&
			entities[LINK_I].coords.y < entity.coords.y + TILESIZE - 1 &&
			entities[LINK_I].coords.y + TILESIZE - 1 > entity.coords.y) {
		return TRUE;
	}

	return FALSE;
}


void logic_enemy_move(entity_t* entity) {

	uint32_t move = rand() % 5;

	switch(move) {
	case 0:
		entity->speed_vect.y = -entity->speed;
		entity->speed_vect.x = 0;
		break;
	case 1:
		entity->speed_vect.y = entity->speed;
		entity->speed_vect.x = 0;
		break;
	case 2:
		entity->speed_vect.x = -entity->speed;
		entity->speed_vect.y = 0;
		break;
	case 3:
		entity->speed_vect.x = entity->speed;
		entity->speed_vect.y = 0;
		break;
	default:
		entity->speed_vect.x = 0;
		entity->speed_vect.y = 0;
		break;
	}
}


uint8_t logic_tilecolcycle(entity_t entity) {

	uint8_t result_1, result_2, result_3, result_4, result_5, result_6, result_7, result_8;
	point_t entity_coords; entity_coords.x = entity.coords.x + entity.speed_vect.x; entity_coords.y = entity.coords.y + entity.speed_vect.y;
	point_t test_coords;

	entity_coords.x = clamp_int16(entity_coords.x, -TILESIZE, MWIDTH * TILESIZE + TILESIZE);
	entity_coords.y = clamp_int16(entity_coords.y, -TILESIZE, MHEIGHT * TILESIZE + TILESIZE);

	if(entity_coords.x < 0 || entity_coords.x > (MWIDTH - 1) * TILESIZE) {
		return TRUE;
	}
	if(entity_coords.y < 0 || entity_coords.y > (MHEIGHT - 1) * TILESIZE) {
		return TRUE;
	}

	test_coords.x = entity_coords.x + 2; test_coords.y = entity_coords.y;
	result_1 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + TILESIZE - 3; test_coords.y = entity_coords.y;
	result_2 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + (TILESIZE / 2) - 1; test_coords.y = entity_coords.y;
	result_3 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + 2; test_coords.y = entity_coords.y + (TILESIZE / 2) - 1;
	result_4 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + TILESIZE - 3; test_coords.y = entity_coords.y + (TILESIZE / 2) - 1;
	result_5 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + 2; test_coords.y = entity_coords.y + TILESIZE - 1;
	result_6 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + (TILESIZE / 2) - 1; test_coords.y = entity_coords.y + TILESIZE - 1;
	result_7 = logic_tilecollision(test_coords, entity.isPC);

	test_coords.x = entity_coords.x + TILESIZE - 3; test_coords.y = entity_coords.y + TILESIZE - 1;
	result_8 = logic_tilecollision(test_coords, entity.isPC);

	result_1 += result_2 + result_3 + result_4 + result_5 + result_6 + result_7 + result_8;

	if(result_1 == FALSE) {
		return FALSE;
	}

	return TRUE;
}


uint8_t logic_tilecollision(point_t entity, uint8_t isPC) {

	point_t tile;
	tile = logic_currtile(entity);

	uint8_t col_type = currentmap.collision[tile.x + tile.y * MWIDTH];

	//Full block
	if(col_type == 0) {
		return TRUE;
	//Passable tile
	} else if(col_type == 1) {
		return FALSE;
	}

	point_t topleft_coords; //Top-left corner coordinates of current tile

	if(col_type == 2) {
		topleft_coords.x = tile.x * TILESIZE;
		topleft_coords.y = tile.y * TILESIZE;

		if(entity.y - topleft_coords.y >= (TILESIZE / 2)) {
			if(entity.x - topleft_coords.x <= TILESIZE) {
				return TRUE;
			}
		}
	} else if(col_type == 3) {
		topleft_coords.x = tile.x * TILESIZE;
		topleft_coords.y = tile.y * TILESIZE;

		if(entity.y - topleft_coords.y <= (TILESIZE / 2)) {
			if(entity.x - topleft_coords.x <= TILESIZE) {
				return TRUE;
			}
		}
	} else if(col_type == 4) {
		topleft_coords.x = tile.x * TILESIZE;
		topleft_coords.y = tile.y * TILESIZE;

		if(entity.x - topleft_coords.x <= (TILESIZE / 2)) {
			if(entity.y - topleft_coords.y <= TILESIZE) {
				return TRUE;
			}
		}
	} else if(col_type == 5) {
		topleft_coords.x = tile.x * TILESIZE;
		topleft_coords.y = tile.y * TILESIZE;

		if(entity.x - topleft_coords.x >= (TILESIZE / 2)) {
			if(entity.y - topleft_coords.y <= TILESIZE) {
				return TRUE;
			}
		}

	//Map transition blocks
	} else if(isPC == TRUE) {

		if(col_type == 10) {

			topleft_coords.x = tile.x * TILESIZE;
			topleft_coords.y = tile.y * TILESIZE;

			if(entity.y - topleft_coords.y <= 3) {
				if(entity.x - topleft_coords.x <= TILESIZE) {
					game.changemap_dir = MOVE_UP;
					game.changemap_f = TRUE;
					return TRUE;
				}
			}
		} else if(col_type == 11) {

			topleft_coords.x = tile.x * TILESIZE;
			topleft_coords.y = tile.y * TILESIZE;

			if(entity.y - topleft_coords.y >= TILESIZE - 3) {
				if(entity.x - topleft_coords.x <= TILESIZE) {
					game.changemap_dir = MOVE_DOWN;
					game.changemap_f = TRUE;
					return TRUE;
				}
			}
		} else if(col_type == 12) {

			topleft_coords.x = tile.x * TILESIZE;
			topleft_coords.y = tile.y * TILESIZE;

			if(entity.x - topleft_coords.x <= 3) {
				if(entity.y - topleft_coords.y <= TILESIZE) {
					game.changemap_dir = MOVE_LEFT;
					game.changemap_f = TRUE;
					return TRUE;
				}
			}
		} else if(col_type == 13) {

			topleft_coords.x = tile.x * TILESIZE;
			topleft_coords.y = tile.y * TILESIZE;

			if(entity.x - topleft_coords.x >= TILESIZE - 3) {
				if(entity.y - topleft_coords.y <= TILESIZE) {
					game.changemap_dir = MOVE_RIGHT;
					game.changemap_f = TRUE;
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}


int8_t logic_changemap(event_t direction) {

	char path[64];
	strcpy(path, MAP_PATH);

	switch(direction) {
	case MOVE_UP:
		entities[LINK_I].coords.y = (MHEIGHT - 1) * TILESIZE - 4;
		game.currmap.y--;
		break;
	case MOVE_DOWN:
		entities[LINK_I].coords.y = 0 + 4;
		game.currmap.y++;
		break;
	case MOVE_LEFT:
		entities[LINK_I].coords.x = (MWIDTH - 1) * TILESIZE - 4;
		game.currmap.x--;
		break;
	case MOVE_RIGHT:
		entities[LINK_I].coords.x = 0 + 4;
		game.currmap.x++;
		break;
	default:
		break;
	}

	char concatenate_x[2];
	char concatenate_y[2];

	sprintf(concatenate_x, "%d", game.currmap.x);
	sprintf(concatenate_y, "%d", game.currmap.y);

	strcat(path, concatenate_x);
	strcat(path, "_");
	strcat(path, concatenate_y);
	strcat(path, ".csv");

	logic_lmap(path, &nextmap);
	currentcopy = currentmap; //copy old map for scrolling

	return 0;
}


int8_t logic_lmap(const unsigned char* filename, map_t* map) {
	FILE* mapfile;
	mapfile = fopen(filename, "r");

	if(mapfile == NULL) {
		printf("LoLCOM: logic_lmap: coulnd't open map file\n");
		return -1;
	}

	char* line = NULL;
	size_t len;
	size_t it = 0;
	ssize_t nread;
	uint16_t flags = 0;

	do{
		//Reads stream up to delim char, terminates line with \0
		nread = getdelim(&line, &len, ',', mapfile);


		if(flags == MAPDATA) {
			map->map[it] = parse_ulong(line, 10);
			it++;
			if(it > 175) {
				flags = 0;
			}
		} else if(flags == COLLISION) {
			map->collision[it] = parse_ulong(line, 10);
			it++;
			if(it > 175) {
				flags = 0;
			}
		} else if(flags == TILESPERLINE) {
			map->tilesperline = parse_ulong(line, 10);
			flags = 0;
		} else if(flags == NTILES)  {
			map->ntiles = parse_ulong(line, 10);
			flags = 0;
		} else if(flags == TILESET) {
			int x, y, comp;
			map->tileset = stbi_load(line, &x, &y, &comp, COMPONENTS);

			if(map->tileset == NULL) {
				printf("LoLCOM: logic_lmap: couldn't open tileset PNG\n");
				return -1;
			}
			map->tileset_width = x;
			flags = 0;
		}

		if(strncmp(line, "mapdata", strlen("mapdata")) == 0) {
			it = 0;
			flags = MAPDATA;
		} else if(strncmp(line, "collision", strlen("collision")) == 0) {
			it = 0;
			flags = COLLISION;
		} else if(strncmp(line, "tilesperline", strlen("tilesperline")) == 0) {
			flags = TILESPERLINE;
		} else if(strncmp(line, "ntiles", strlen("ntiles")) == 0) {
			flags = NTILES;
		} else if(strncmp(line, "tileset", strlen("tileset")) == 0) {
			flags = TILESET;
		}

	} while(nread != -1);

	fclose(mapfile);
	return 0;
}


point_t logic_currtile(point_t entity) {
	point_t tilecoords;

	tilecoords.x = entity.x / TILESIZE;
	tilecoords.y = entity.y / TILESIZE;
	return tilecoords;
}


int8_t logic_clear_enemies() {

	rtc_read_register(RTC_STATUS_C); //Make sure nothing is stopping RTC interrupts
	rtc_setalarm_s(SPAWN_RATE);

	size_t i;
	for(i = 1; i < ENTITY_N - 1; i++) {
		entities[i].hitpoints = 0;
	}

	return 0;
}


int8_t logic_rtc_handler() {

	uint32_t regC;
	regC = rtc_read_register(RTC_STATUS_C);

	if(game.state == PLAYER1) {

		if(logic_rtc_spawn("NULL") != 0) {
			return -1;
		}

		rtc_setalarm_s(SPAWN_RATE);
	}

	return 0;
}


int8_t logic_rtc_spawn(unsigned char* enemy_type) {

	point_t enemy_coords, pc_tile, tile;
	uint8_t col_type;

	uint8_t choice = rand() % 3;

	switch(choice) {
	case 0:
		enemy_type = "redmoblin";
		break;
	case 1:
		enemy_type = "bluemoblin";
		break;
	case 2:
		enemy_type = "redoctorok";
		break;
	default:
		enemy_type = "redmoblin";
		break;
	}

	do {
		enemy_coords.x = (rand() % 15) * TILESIZE;
		enemy_coords.y = (rand() % 10) * TILESIZE;

		pc_tile = logic_currtile(entities[LINK_I].coords);
		tile = logic_currtile(enemy_coords);

		col_type = currentmap.collision[tile.x + tile.y * MWIDTH];

	} while((pc_tile.x == tile.x && pc_tile.y == tile.y) || col_type != 1);


	size_t i;
	for(i = 1; i < ENTITY_N - 4; i++) {
		if(entities[i].hitpoints == 0) {
			if(logic_lentity(enemy_type, &entities[i], FALSE) != 0) {
				return -1;
			} else break;
		}
	}

	if(i < ENTITY_N - 4) {
		entities[i].coords.x = enemy_coords.x;
		entities[i].coords.y = enemy_coords.y;
	}

	return 0;
}


int8_t logic_updatedisplay() {

	if(game.state == PLAYER1) {
		vg_clear();

		point_t map_coords, sprite_coords;
		uint16_t topleft_x, topleft_y;

		vg_topleft(&topleft_x, &topleft_y);

		//x,y coords where map area is drawn
		map_coords.x = topleft_x;
		map_coords.y = topleft_y + STATUSBAR_H;

		if(game.changemap_f == TRUE) {
			uint8_t ended = logic_scrollmap(game.changemap_dir);
			if(ended == TRUE) {
				game.changemap_f = FALSE;
				currentmap = nextmap;
				entities[LINK_I].speed_vect = (vector_t) {0, 0};
			}
		}

		vg_draw_map(map_coords);

		size_t i;
		point_t scorecoords = (point_t){topleft_x, topleft_y + FONT_Y_ADJUST};
		for(i = 0; i < score.word_size; i++) {
			if(score.word[i] != 0) {
				vg_font(scorecoords, score.fontdata_width, score.tilesperline, score.fontdata, score.word[i] - FONT_START);
				scorecoords.x += FONT_W + score.coords.x;
			} else break;
		}

		point_t hpcoords = (point_t){topleft_x + 16 * TILESIZE - FONT_X_ADJUST, topleft_y + FONT_Y_ADJUST};
		for(i = 0; i < link_hp.word_size; i++) {
			if(link_hp.word[i] != 0) {
				vg_font(hpcoords, link_hp.fontdata_width, link_hp.tilesperline, link_hp.fontdata, link_hp.word[i] - FONT_START);
				hpcoords.x += FONT_W + link_hp.coords.x;
			} else break;
		}

		if(game.changemap_f == TRUE) {
			sprite_coords.x = map_coords.x + entities[LINK_I].coords.x;
			sprite_coords.y = map_coords.y + entities[LINK_I].coords.y;
			vg_tile(sprite_coords, entities[LINK_I].spritesheet_width, entities[LINK_I].tilesperline, entities[LINK_I].spritesheet, entities[LINK_I].currsprite);
		} else {
			for(i = 0; i < ENTITY_N; i++) {

				if(entities[i].hitpoints != 0) {
					sprite_coords.x = map_coords.x + entities[i].coords.x;
					sprite_coords.y = map_coords.y + entities[i].coords.y;
					vg_tile(sprite_coords, entities[i].spritesheet_width, entities[i].tilesperline, entities[i].spritesheet, entities[i].currsprite);
				}
			}
		}

		vg_refresh();
	} else if(game.state == MENU) {

		vg_clear();

		uint16_t topleft_x, topleft_y;

		vg_topleft(&topleft_x, &topleft_y);

		if(game.menu_frame >= 0 && game.menu_frame < 3) {
			vg_png((point_t){topleft_x, topleft_y}, menu[game.menu_frame].image_width, menu[game.menu_frame].image_height, menu[game.menu_frame].image);
		}

		vg_png((point_t){topleft_x + TRIFORCE_X, topleft_y + TRIFORCE_Y + game.menu_choice * TRIFORCE_ADJUST}, triforce.image_width, triforce.image_height, triforce.image);

		vg_refresh();
	} else if(game.state == GAMEOVER) {
		vg_clear();

		point_t map_coords, sprite_coords;
		uint16_t topleft_x, topleft_y;

		vg_topleft(&topleft_x, &topleft_y);

		if(game_over_stage > 4) {
			vg_png((point_t){topleft_x, topleft_y}, game_over_screen.image_width, game_over_screen.image_height, game_over_screen.image);
		} else {

			//x,y coords where map area is drawn
			map_coords.x = topleft_x;
			map_coords.y = topleft_y + STATUSBAR_H;

			vg_draw_map(map_coords);

			sprite_coords.x = map_coords.x + entities[LINK_I].coords.x;
			sprite_coords.y = map_coords.y + entities[LINK_I].coords.y;
			vg_tile(sprite_coords, entities[LINK_I].spritesheet_width, entities[LINK_I].tilesperline, entities[LINK_I].spritesheet, entities[LINK_I].currsprite);

		}

		vg_refresh();
	}

	return 0;
}


int8_t logic_game_over_fade(uint8_t stage) {

	if(stage < 4) {

		char path[64];

		strcpy(path, OVER_PATH);

		char concat[1];

		sprintf(concat, "%d", stage + 1);
		strcat(path, concat);
		strcat(path, ".png");

		int x, y, comp;
		stbi_image_free(currentmap.tileset);
		currentmap.tileset = stbi_load(path, &x, &y, &comp, COMPONENTS);

		if(currentmap.tileset == NULL) {
			printf("LoLCOM: logic_lmap: couldn't open tileset PNG\n");
			return -1;
		}
		currentmap.tileset_width = x;
	} else {
		if(logic_lpng(&game_over_screen, "GameOver.png") != 0) {
			return -1;
		}
	}

	return 0;
}


uint8_t logic_scrollmap(event_t direction) {

	uint8_t x, y;

	if(direction == MOVE_UP) {

		uint8_t adjust = 0;
		for(y = 0; y < MHEIGHT - scroll_line; y++) {
			for(x = 0; x < MWIDTH; x++) {
				currentmap.map[x + (scroll_line + adjust) * MWIDTH] = currentcopy.map[x + y * MWIDTH];
			}
			adjust++;
		}

		adjust = 0;
		for(y = 0; y < scroll_line; y++) {
			for(x = 0; x < MWIDTH; x++) {
				currentmap.map[x + y * MWIDTH] = nextmap.map[x + (MHEIGHT - scroll_line + adjust) * MWIDTH];
			}
			adjust++;
		}
	} else if(direction == MOVE_DOWN) {

		for(y = 0; y < MHEIGHT - scroll_line; y++) {
			for(x = 0; x < MWIDTH; x++) {
				currentmap.map[x + y * MWIDTH] = currentcopy.map[x + (y + scroll_line) * MWIDTH];
			}
		}

		uint8_t adjust = scroll_line;
		for(y = MHEIGHT - scroll_line; y < MHEIGHT; y++) {
			for(x = 0; x < MWIDTH; x++) {
				currentmap.map[x + y * MWIDTH] = nextmap.map[x + (scroll_line - adjust) * MWIDTH];
			}
			adjust--;
		}
	} else if(direction == MOVE_LEFT) {

		for(x = 0; x < MWIDTH - scroll_line; x++) {
			for(y = 0; y < MHEIGHT; y++) {
				currentmap.map[scroll_line + x + y * MWIDTH] = currentcopy.map[x + y * MWIDTH];
			}
		}

		uint8_t adjust = 0;
		for(x = 0; x < scroll_line; x++) {
			for(y = 0; y < MHEIGHT; y++) {
				currentmap.map[x + y * MWIDTH] = nextmap.map[(MWIDTH - scroll_line + adjust) + y * MWIDTH];
			}
			adjust++;
		}
	} else if(direction == MOVE_RIGHT) {

		for(x = 0; x < MWIDTH - scroll_line; x++) {
			for(y = 0; y < MHEIGHT; y++) {
				currentmap.map[x + y * MWIDTH] = currentcopy.map[scroll_line + x + y * MWIDTH];
			}
		}

		for(x = 0; x < scroll_line; x++) {
			for(y = 0; y < MHEIGHT; y++) {
				currentmap.map[MWIDTH - scroll_line + x + y * MWIDTH] = nextmap.map[x + y * MWIDTH];
			}
		}
	}

	if(scroll_delay == 0) {
		scroll_line++;
		scroll_delay = SCROLL_FRAMES;
	}

	scroll_delay--;

	if(direction == MOVE_UP || direction == MOVE_DOWN) {
		if(scroll_line > MHEIGHT) {
			scroll_line = 0;
			logic_clear_enemies();
			return TRUE;
		}
	} else if(direction == MOVE_LEFT || MOVE_RIGHT) {
		if(scroll_line > MWIDTH) {
			scroll_line = 0;
			logic_clear_enemies();
			return TRUE;
		}
	}

	return FALSE;
}


int8_t logic_kbd_input(uint32_t scancode, uint8_t origin) {

	if(origin == PLAYER2) {

		if(serial_cooldown.number != 0) {
			return 0;
		}

		switch(scancode) {
		case MAKE_1:
			serial_cooldown.number = 3;
			uart_send(scancode);
			break;
		case MAKE_2:
			serial_cooldown.number = 6;
			uart_send(scancode);
			break;
		case MAKE_3:
			serial_cooldown.number = 3;
			uart_send(scancode);
			break;
		case MAKE_4:
			serial_cooldown.number = 9;
			uart_send(scancode);
			break;
		default:
			break;
		}

		return 0;
	}

	if(game.state == PLAYER1 && game.changemap_f != TRUE) {

		if(scancode == ESC_MAKE) {
			latest_event = PLAYER1_QUIT;
			return 0;
		}

		if(scancode == W_MAKE || scancode == S_MAKE || scancode == A_MAKE || scancode == D_MAKE ||
				scancode == BREAK(W_MAKE) || scancode == BREAK(S_MAKE) || scancode == BREAK(A_MAKE) || scancode == BREAK(D_MAKE)) {
			last_keypress = scancode;
		}

		if(entities[LINK_I].state != KNOCKBACK_DMG) {

			switch(scancode) {
			case W_MAKE:
				entities[LINK_I].movement = MOVE_UP;
				entities[LINK_I].speed_vect = (vector_t){0, -entities[LINK_I].speed};
				break;
			case S_MAKE:
				entities[LINK_I].movement = MOVE_DOWN;
				entities[LINK_I].speed_vect = (vector_t){0, entities[LINK_I].speed};
				break;
			case A_MAKE:
				entities[LINK_I].movement = MOVE_LEFT;
				entities[LINK_I].speed_vect = (vector_t){-entities[LINK_I].speed, 0};
				break;
			case D_MAKE:
				entities[LINK_I].movement = MOVE_RIGHT;
				entities[LINK_I].speed_vect = (vector_t){entities[LINK_I].speed, 0};
				break;
			case J_MAKE:
				if(entities[SWORD_I].state != ATTACKING && entities[SWORD_I].cooldown.attack == 0 && entities[LINK_I].state == NORMAL) {
					entities[SWORD_I].hitpoints = 1;
					entities[SWORD_I].cooldown.attack = SWORD_FRAMES;
					entities[SWORD_I].state = ATTACKING;
				}
				break;
			default:
				break;
			}

			if((scancode == BREAK(W_MAKE) && entities[LINK_I].movement == MOVE_UP) || (scancode == BREAK(S_MAKE) && entities[LINK_I].movement == MOVE_DOWN) ||
					(scancode == BREAK(A_MAKE) && entities[LINK_I].movement == MOVE_LEFT) || (scancode == BREAK(D_MAKE) && entities[LINK_I].movement == MOVE_RIGHT)) {
				entities[LINK_I].movement = MOVE_NONE;
				entities[LINK_I].speed_vect = (vector_t){0, 0};
				last_keypress = 0;
			}

			if(scancode == BREAK(J_MAKE) && entities[SWORD_I].state == ATTACKING) {
				entities[SWORD_I].state = NORMAL;
			}
		}
	} else if(game.state == MENU) {
		if(scancode == W_MAKE) {
			if(game.menu_choice == 0) {
				game.menu_choice = MENU_MAX;
			} else game.menu_choice--;
		} else if(scancode == S_MAKE) {
			if(game.menu_choice == MENU_MAX) {
				game.menu_choice = 0;
			} else game.menu_choice++;
		} else if(scancode == ENTER_MAKE) {
			latest_event = MENUOPTION;
		} else if(scancode == ESC_MAKE) {
			latest_event = EXITING;
		}
	} else if(game.state == GAMEOVER) {
		if(game.death_f == FALSE) {
			if(scancode == BREAK(ESC_MAKE) || scancode == BREAK(ENTER_MAKE)) {
				latest_event = EXITING;
			}
		}
	}

	return 0;
}


uint8_t logic_check_end() {
	if(game.state == END) {
		return TRUE;
	} else return FALSE;
}


int8_t logic_mouse_input(uint32_t data, uint8_t* pnumber, uint8_t* sync, uint8_t mode) {

	if(mode != MOUSE_SCROLL_EX) {

		//Reset pnumber if needed
		if(*pnumber == 3) {
			*pnumber = 0;
		}

		packet[*pnumber] = data;

		if((packet[*pnumber] & SYNC_BIT) && *sync == 0) {
			*pnumber = 0;
			*sync = 1;
		}

		(*pnumber)++;

		if((packet[0] & SYNC_BIT) == 0) {
			*sync = 0;
		}

		if(*pnumber == 3 && *sync == 1) {
			if(game.state == PLAYER1) {
				if((packet[0] & MOUSE_XOV) == 0 && (packet[0] & MOUSE_YOV) == 0 && entities[LINK_I].state != KNOCKBACK_DMG) {
					logic_mouse_handler(mode);
				}
			}
		}

		return 0;
	} else {

		//Reset pnumber if needed
		if(*pnumber == 4) {
			*pnumber = 0;
		}

		packet[*pnumber] = data;

		if((packet[*pnumber] & SYNC_BIT) && *sync == 0) {
			*pnumber = 0;
			*sync = 1;
		}

		(*pnumber)++;

		if((packet[0] & SYNC_BIT) == 0) {
			*sync = 0;
		}

		if(*pnumber == 4 && *sync == 1) {
			if(game.state == PLAYER1) {
				if((packet[0] & MOUSE_XOV) == 0 && (packet[0] & MOUSE_YOV) == 0 && entities[LINK_I].state != KNOCKBACK_DMG) {
					logic_mouse_handler(mode);
				}
			}
		}

		return 0;
	}
}


int8_t logic_mouse_handler(uint8_t mode) {

	int16_t mouse_dx, mouse_dy;

	if((packet[0] & MOUSE_XS) == MOUSE_XS) {
		mouse_dx = ((~packet[1] + 1) & 0xFF) * -1;
	} else {
		mouse_dx = packet[1];
	}

	if((packet[0] & MOUSE_YS) == MOUSE_YS) {
		mouse_dy = ((~packet[2] + 1) & 0xFF) * -1;
	} else {
		mouse_dy = packet[2];
	}

	//Check left button
	if((packet[0] & MOUSE_LB) == MOUSE_LB) {
		if(entities[SWORD_I].state != ATTACKING && entities[SWORD_I].cooldown.attack == 0 && entities[LINK_I].state == NORMAL) {
			entities[SWORD_I].hitpoints = 1;
			entities[SWORD_I].cooldown.attack = SWORD_FRAMES;
			entities[SWORD_I].state = ATTACKING;
		}
	} else if((packet[0] & MOUSE_LB) == 0 && entities[SWORD_I].state == ATTACKING) {
			entities[SWORD_I].state = NORMAL;
	}

	uint16_t mouse_dx_t, mouse_dy_t;

	mouse_dx_t = abs(mouse_dx);
	mouse_dy_t = abs(mouse_dy);

	if(mouse_dx_t > mouse_dy_t) {
		if(mouse_dx > MOUSE_TOL) {
			entities[LINK_I].movement = MOVE_RIGHT;
			entities[LINK_I].speed_vect = (vector_t){entities[LINK_I].speed, 0};
			return 0;
		} else if(mouse_dx < -MOUSE_TOL) {
			entities[LINK_I].movement = MOVE_LEFT;
			entities[LINK_I].speed_vect = (vector_t){-entities[LINK_I].speed, 0};
			return 0;
		} else {
			entities[LINK_I].movement = MOVE_NONE;
			entities[LINK_I].speed_vect = (vector_t){0, 0};
			return 0;
		}
	} else if(mouse_dx_t < mouse_dy_t) {
		if(mouse_dy > MOUSE_TOL) {
			entities[LINK_I].movement = MOVE_UP;
			entities[LINK_I].speed_vect = (vector_t){0, -entities[LINK_I].speed};
			return 0;
		} else if(mouse_dy < -MOUSE_TOL) {
			entities[LINK_I].movement = MOVE_DOWN;
			entities[LINK_I].speed_vect = (vector_t){0, entities[LINK_I].speed};
			return 0;
		} else {
			entities[LINK_I].movement = MOVE_NONE;
			entities[LINK_I].speed_vect = (vector_t){0, 0};
			return 0;
		}
	} else {

		entities[LINK_I].movement = MOVE_NONE;
		entities[LINK_I].speed_vect = (vector_t){0, 0};

		if(mode == MOUSE_SCROLL_EX) {
			uint8_t scroll = packet[3] & MOUSE_SCROLL_PACKET;

			if(scroll >= MOUSE_PLUS_MIN && scroll <= MOUSE_PLUS_MAX) {
				entities[LINK_I].movement = MOVE_DOWN;
				entities[LINK_I].speed_vect = (vector_t){0, entities[LINK_I].speed};
				return 0;
			} else if(scroll >= MOUSE_MINUS_MAX && scroll <= MOUSE_MINUS_MIN) {
				entities[LINK_I].movement = MOVE_UP;
				entities[LINK_I].speed_vect = (vector_t){0, -entities[LINK_I].speed};
				return 0;
			} else if((packet[3] & MOUSE_4B) == MOUSE_4B) {
				entities[LINK_I].movement = MOVE_RIGHT;
				entities[LINK_I].speed_vect = (vector_t){entities[LINK_I].speed, 0};
				return 0;
			} else if((packet[3] & MOUSE_5B) == MOUSE_5B) {
				entities[LINK_I].movement = MOVE_LEFT;
				entities[LINK_I].speed_vect = (vector_t){-entities[LINK_I].speed, 0};
				return 0;
			}

				entities[LINK_I].movement = MOVE_NONE;
				entities[LINK_I].speed_vect = (vector_t){0, 0};
		}
	}

	return 0;
}


int8_t logic_serial_spawn(unsigned char* enemy_type) {

	point_t enemy_coords, pc_tile, tile;
	uint8_t col_type;

	do {
		enemy_coords.x = (rand() % 15) * TILESIZE;
		enemy_coords.y = (rand() % 10) * TILESIZE;

		pc_tile = logic_currtile(entities[LINK_I].coords);
		tile = logic_currtile(enemy_coords);

		col_type = currentmap.collision[tile.x + tile.y * MWIDTH];

	} while((pc_tile.x == tile.x && pc_tile.y == tile.y) || col_type != 1);

	size_t i;
	for(i = 4; i < ENTITY_N - 1; i++) {
		if(entities[i].hitpoints == 0) {
			if(logic_lentity(enemy_type, &entities[i], FALSE) != 0) {
				return -1;
			} else break;
		}
	}

	if(i >= 4 && i < ENTITY_N - 1) {
		entities[i].coords.x = enemy_coords.x;
		entities[i].coords.y = enemy_coords.y;
	}

	return 0;
}


int8_t logic_serial_handler(uint32_t serial_rcv) {

	uint8_t data = serial_rcv & 0xFF;

	switch(data) {
	case MAKE_1:
		logic_serial_spawn("redmoblin");
		break;
	case MAKE_2:
		logic_serial_spawn("bluemoblin");
		break;
	case MAKE_3:
		logic_serial_spawn("redoctorok");
		break;
	case MAKE_4:
		logic_serial_spawn("redlynel");
		break;
	default:
		break;
	}

	return 0;
}


int8_t logic_serial_init() {

	if(logic_lpng(&serial_image, "Serial.png") != 0) {
		return -1;
	}

	if(logic_lfont(&serial_cooldown) != 0) {
		return -1;
	}

	strcpy(serial_cooldown.word, "COOLDOWN:");
	serial_cooldown.word_size = strlen("COOLDOWN:");
	serial_cooldown.number = 0;

	return 0;
}


int8_t logic_serial_tick() {

	if(serial_cooldown.number != 0) {
		serial_cooldown.number--;
	}

	logic_font_number(&serial_cooldown);

	return 0;
}


int8_t logic_display_serial() {

	vg_clear();

	vg_png((point_t){0, 0}, serial_image.image_width, serial_image.image_height, serial_image.image);

	size_t i;
	point_t fontcoords = (point_t){205, 450};
	for(i = 0; i < serial_cooldown.word_size; i++) {
		if(serial_cooldown.word[i] != 0) {
			vg_font(fontcoords, serial_cooldown.fontdata_width, serial_cooldown.tilesperline, serial_cooldown.fontdata, serial_cooldown.word[i] - FONT_START);
			fontcoords.x += FONT_W + serial_cooldown.coords.x;
		}
	}

	vg_refresh();
	return 0;

}


int8_t logic_lpng(png_t* png, const unsigned char* filename) {

	char path[64];

	strcpy(path, IMG_PATH);
	strcat(path, filename);

	int x, y, comp;

	//stbi_image_free(png->image);
	png->image = stbi_load(path, &x, &y, &comp, COMPONENTS);

	if(png->image == NULL) {
		printf("LoLCOM: png: couldn't open png image\n");
		return -1;
	}

	png->image_width = x;
	png->image_height = y;

	return 0;
}

int8_t logic_serial_free() {

	stbi_image_free(serial_image.image);
	stbi_image_free(serial_cooldown.fontdata);

	vg_free();

	return 0;
}

//-----------------------------------------------------
//font_t functions
//-----------------------------------------------------

int8_t logic_lfont(font_t* font) {

	int x, y, comp;

	font->fontdata = stbi_load(FONT_PATH, &x, &y, &comp, COMPONENTS);

	if(font->fontdata == NULL) {
		printf("LoLCOM: font: couldn't open font data\n");
		return -1;
	}

	font->fontdata_width = x;
	font->tilesperline = FONT_TILES_LINE;
	font->coords = (point_t){0, 0};

	memset(font->word,0,sizeof(font->word));

	return 0;
}


int8_t logic_font_number(font_t* font) {

	size_t i;
	for(i = 0; i < 64; i++) {
		if(font->word[i] == ':') {
			break;
		}
	}

	if(i < 61) {
		i++;
	} else i = 62;

	if(font->number > 999) {
		font->number = 999;
	}

	if(font->number == 0) {
		font->word_size = i + 1;
		font->word[i] = '0';
		return 0;
	}

	uint16_t number_temp = font->number;
	uint16_t number_size = 0;
	uint16_t numbers[3];

	while (number_temp > 0) {
		uint16_t digit = number_temp % 10;

		numbers[number_size] = digit + '0';

		number_temp /= 10;
		number_size++;
	}

	size_t j;
	for(j = 0; j < number_size; j++) {
		font->word[i + j] = numbers[number_size - j - 1];
	}

	font->word_size = i + number_size;

	return 0;
}

//-----------------------------------------------------
//map_t functions
//-----------------------------------------------------

map_t map_get() {
	return currentmap;
}

//-----------------------------------------------------
//entity_t functions
//-----------------------------------------------------

int8_t logic_lentity(const unsigned char* entity_name, entity_t* entity, uint8_t isPC) {

	char filename[64];

	strcpy(filename, ENTITY_PATH);
	strcat(filename, entity_name);
	strcat(filename, ".csv");

	FILE* entity_data;
	entity_data = fopen(filename, "r");

	if(entity_data == NULL) {
		printf("LoLCOM: entity: coulnd't open entity file\n");
		return -1;
	}

	char* line = NULL;
	size_t len;
	size_t it = 0;
	ssize_t nread;
	uint16_t flags = 0;

	do{
		//Reads stream up to delim char, terminates line with \0
		nread = getdelim(&line, &len, ',', entity_data);


		if(flags == SPRITESHEET) {
			int x, y, comp;

			entity->spritesheet = stbi_load(line, &x, &y, &comp, COMPONENTS);

			if(entity->spritesheet == NULL) {
				printf("LoLCOM: entity: couldn't open entity sprite sheet\n");
				return -1;
			}

			entity->spritesheet_width = x;
			flags = 0;

		} else if(flags == TILESPERLINE) {
			entity->tilesperline = parse_ulong(line, 10);
			flags = 0;
		} else if(flags == NTILES)  {
			entity->ntiles = parse_ulong(line, 10);
			flags = 0;
		} else if(flags == HITPOINTS) {
			entity->hitpoints = parse_ulong(line, 10);
			flags = 0;
		} else if(flags == SPEED) {
			entity->speed = parse_ulong(line, 10);
			flags = 0;
		}

		if(strncmp(line, "spritesheet", strlen("spritesheet")) == 0) {
			flags = SPRITESHEET;
		} else if(strncmp(line, "tilesperline", strlen("tilesperline")) == 0) {
			flags = TILESPERLINE;
		} else if(strncmp(line, "ntiles", strlen("ntiles")) == 0) {
			flags = NTILES;
		} else if(strncmp(line, "hitpoints", strlen("hitpoints")) == 0) {
			flags = HITPOINTS;
		} else if(strncmp(line, "speed", strlen("speed")) == 0) {
			flags = SPEED;
		}

	} while(nread != -1);

	fclose(entity_data);

	if(isPC == TRUE) {
		entity->currsprite = (uint8_t) MOVE_UP;
		entity->movement = MOVE_NONE;

		entity->coords.x = PC_CENTER_X;
		entity->coords.y = PC_CENTER_Y;

		entity->state = NORMAL;
		entity->isPC = TRUE;
	} else {
		entity->currsprite = (uint8_t) MOVE_UP;
		entity->movement = MOVE_NONE;

		entity->state = NORMAL;
		entity->isPC = FALSE;
	}

	return 0;
}
