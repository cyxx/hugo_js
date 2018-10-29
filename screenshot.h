
#ifndef SCREENSHOT_H__
#define SCREENSHOT_H__

#include <stdint.h>

void saveBMP(const char *filename, const uint8_t *bits, const uint8_t *pal16, int w, int h);
void savePNG(const char *filename, const uint8_t *bits, const uint8_t *pal16, const uint8_t *trns, int w, int h);

#endif
