#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stbi_wrapper.h"

#define STBI_ONLY_PNG //Use only PNG loading feature
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Loads png image and returns it, width and height are changed to image values
unsigned char* stbi_png_load(int* width, int* height, const char* image_path) {

	int comp;

	unsigned char* image = stbi_load(image_path, width, height, &comp, 3);

	if(image == NULL) {
		printf("Couldn't open PNG image, path might be invalid\n");
		return NULL;
	}

	return image;
}

//Free image using API from stb_image
void stbi_free(unsigned char* image) {
	stbi_image_free(image);
}
