#ifndef HELPER_H
#define HELPER_H

ssize_t getdelim(char** lineptr, size_t* n, int delimiter, FILE* stream);

unsigned long parse_ulong(char* str, int base);

int16_t clamp_int16(int16_t value, int16_t min, int16_t max);

#endif //HELPER_H
