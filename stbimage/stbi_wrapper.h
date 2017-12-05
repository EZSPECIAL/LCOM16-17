#ifndef __STBI_WRAPPER_H
#define __STBI_WRAPPER_H

//Loads png image and returns it, width and height are changed to image values
unsigned char* stbi_png_load(int* width, int* height, const char* image_path);

//Free image using API from stb_image
void stbi_free(unsigned char* image);

#endif /* __STBI_WRAPPER_H */
