#ifndef __VIDEO_GR_H
#define __VIDEO_GR_H

/** @defgroup video_gr video_gr
 * @{
 *
 * Functions for outputing data to screen in graphics mode
 */

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

 /**
 * @brief Returns to default Minix 3 text mode (0x03: 25 x 80, 16 colors)
 * 
 * @return 0 upon success, non-zero upon failure
 */
int vg_exit(void);

//Draws pixel at x, y with "color"
//Can draw in 8bpp indexed, 16bpp, 24bpp, 32bpp as long as its linear framebuffer
int draw_pixel(unsigned short x, unsigned short y, unsigned long color);

//Draws line with an adaptation of Bresenham's Line Algorithm
//Source: https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
//Algorithm adapted: http://www.roguebasin.com/index.php?title=Bresenham%27s_Line_Algorithm
int draw_line(unsigned short xi, unsigned short yi, unsigned short xf, unsigned short yf, unsigned long color);

//Draw square at x, y coords with size and color passed as arguments
int draw_square(unsigned short x, unsigned short y, unsigned short size, unsigned long color);

//Initialize static variables with the info from VBE Function 01h
int vg_init_values(unsigned short mode);

//Draw sprite at xi, yi coords with width, height and sprite[] being the return from read_xpm
int draw_sprite(unsigned short xi, unsigned short yi, unsigned short width, unsigned short heigth, char *sprite);

//Erase sprite at xi, yi coords with width, height and sprite[] being the return from read_xpm
int erase_sprite(unsigned short xi, unsigned short yi, unsigned short width, unsigned short heigth, char* sprite);

//Returns a 20bit physical address when given a 32bit linear address base:offset
int vg_aux_offset(phys_bytes phys_address);

//Calls VBE function 00h to fill vbe_controller_info_t struct
//isVBE2 decides whether VBE Signature should be preset to VBE2
//videoPtrMode has 3 modes
//		- VIDEOMODEPTR - uses the VideoModePtr to list VBE modes
//		- VBE_RESERVED - uses the Reserved area of the struct to list VBE modes
//		- BOTH_MODES   - uses both modes
int vg_controller(int isVBE2, int videoPtrMode);

//Move sprite starting from xi, yi coords. hor determines if movement is horizontal or vertical
//delta determines how many pixels the sprite should move
//time is the time the movement should take
//width, height and sprite are returns from read_xpm
int vg_move(float xi, float yi, char* sprite, unsigned short hor, short delta,
									unsigned short time, unsigned short width, unsigned short height);

//Free double buffer from memory
int vg_free();

 /** @} end of video_gr */
 
#endif /* __VIDEO_GR_H */
