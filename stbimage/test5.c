#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <minix/syslib.h>
#include "test5.h"
#include "video_gr.h"
#include "video.h"
#include "stbi_wrapper.h"

/*
 * Sets a 24-bit video mode, loads and displays a PNG image
 *
 * @param mode 24-bit direct color mode to set
 * @param image_path path to PNG file to display
 * @return 0 on success, non zero otherwise
 */
int test_png(uint16_t mode, char* image_path) {

	if(mode != 0x10F && mode != 0x112 && mode != 0x115 && mode != 0x118 && mode != 0x11B) {
		printf("Only 24-bit direct color modes are supported for this demo\n (0x10F | 0x112 | 0x115 | 0x118 | 0x11B)\n");
		return -1;
	}

	//Get mode info with VBE F01h and set it with VBE F02h
	if(vg_init(mode) == NULL) {
		return -1;
	}

	int width, height;
	unsigned char* image = stbi_png_load(&width, &height, image_path);

	//Image failed loading
	if(image == NULL) {
		vg_exit();
		return -1;
	}

	//Usual draw loop inside gameloop (clear, draw entities, copy to VRAM)
	vg_clear(); //Clear screen
	vg_png(image, width, height, 0, 0); //Write image to buffer
	vg_copy(); //Copy buffer to VRAM

	sleep(2);

	//Return to text mode 0x03
	vg_exit();

	vg_free(); //Free buffer used for double buffering
	stbi_free(image); //Free image, lib has its own free

	return 0;
}
