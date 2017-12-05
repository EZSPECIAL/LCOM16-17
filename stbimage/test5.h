#ifndef __TEST5_H
#define __TEST5_H

/*
 * Sets a 24-bit video mode, loads and displays a PNG image
 * 
 * @param mode 24-bit direct color mode to set
 * @param image_path path to PNG file to display
 * @return 0 on success, non zero otherwise
 */
int test_png(uint16_t mode, char* image_path);

#endif /* __TEST5_H */
