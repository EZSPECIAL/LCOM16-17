#ifndef __VIDEO_GR_H
#define __VIDEO_GR_H

void *vg_init(unsigned short mode);

int vg_exit(void);

//Draws pixel at (x, y) coords with specified color (24-bit direct color modes only)
void draw_pixel(uint16_t x, uint16_t y, uint32_t color);

//Draws previously loaded png image at (start_x, start_y) coords
void vg_png(unsigned char* image, int width, int height, uint16_t start_x, uint16_t start_y);

//Sets buffer to all black for clearing screen
void vg_clear();

//Copies RAM buffer to VRAM for double buffering
void vg_copy();

//Free double buffer
void vg_free();

#endif /* __VIDEO_GR_H */
