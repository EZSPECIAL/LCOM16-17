#ifndef VIDEO_GR_H
#define VIDEO_GR_H

#include "LoLCOM.h"

/**
 * @brief Initializes the video module in graphics mode
 * 
 * Uses the VBE INT 0x10 interface to set the desired
 *  graphics mode, maps VRAM to the process' address space
 *  Doesn't initialize values since vg_init_values() does that
 * 
 * @param mode 16-bit VBE mode to set
 * @return Virtual address VRAM was mapped to. NULL, upon failure.
 */
void *vg_init(unsigned short mode);

//Returns to default Minix 3 text mode (0x03: 25 x 80, 16 colors)
//return 0 upon success, non-zero upon failure
int vg_exit(void);

//Draws pixel at x, y with "color"
//Can draw in 8bpp indexed, 16bpp, 24bpp, 32bpp as long as it is linear framebuffer
int draw_pixel(unsigned short x, unsigned short y, unsigned long color);

//Initialize static variables with the info from VBE Function 01h
int vg_init_values(unsigned short mode);

int8_t vg_tile(point_t coords, uint16_t tilesheet_width, uint8_t tilesperline, unsigned char* tileset, uint8_t tilenumber);

int8_t vg_draw_map(point_t coords);

int vg_clear();

//Free double buffer from memory
int vg_free();

int8_t vg_refresh();

void vg_topleft(uint16_t* topleft_x, uint16_t* topleft_y);

int8_t vg_pageflip();

int8_t vg_font(point_t coords, uint16_t fontdata_width, uint8_t tilesperline, unsigned char* fontdata, uint8_t tilenumber);

int8_t vg_png(point_t coords, uint16_t image_width, uint16_t image_height, unsigned char* image);

void vg_change_buffering(uint8_t mode);

#endif //VIDEO_GR_H
