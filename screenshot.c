
#include <assert.h>
#include <stdio.h>
#include <png.h>
#include "screenshot.h"

static void fwriteUint16LE(FILE *fp, uint16_t n) {
	fputc(n & 0xFF, fp);
	fputc(n >> 8, fp);
}

static void fwriteUint32LE(FILE *fp, uint32_t n) {
	fwriteUint16LE(fp, n & 0xFFFF);
	fwriteUint16LE(fp, n >> 16);
}

static const uint16_t TAG_BM = 0x4D42;

void saveBMP(const char *filename, const uint8_t *bits, const uint8_t *pal16, int w, int h) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		int alignWidth = (w + 3) & ~3;
		int imageSize = alignWidth * h;

		// Write file header
		fwriteUint16LE(fp, TAG_BM);
		fwriteUint32LE(fp, 14 + 40 + 4 * 256 + imageSize);
		fwriteUint16LE(fp, 0); // reserved1
		fwriteUint16LE(fp, 0); // reserved2
		fwriteUint32LE(fp, 14 + 40 + 4 * 256);

		// Write info header
		fwriteUint32LE(fp, 40);
		fwriteUint32LE(fp, w);
		fwriteUint32LE(fp, h);
		fwriteUint16LE(fp, 1); // planes
		fwriteUint16LE(fp, 8); // bit_count
		fwriteUint32LE(fp, 0); // compression
		fwriteUint32LE(fp, imageSize); // size_image
		fwriteUint32LE(fp, 0); // x_pels_per_meter
		fwriteUint32LE(fp, 0); // y_pels_per_meter
		fwriteUint32LE(fp, 0); // num_colors_used
		fwriteUint32LE(fp, 0); // num_colors_important

		// Write palette data
		for (int i = 0; i < 16; ++i) {
			fputc(pal16[2], fp);
			fputc(pal16[1], fp);
			fputc(pal16[0], fp);
			fputc(0, fp);
			pal16 += 3;
		}
		for (int i = 16; i < 256; ++i) {
			fwriteUint32LE(fp, 0);
		}

		// Write bitmap data
		const int pitch = w;
		bits += h * pitch;
		for (int i = 0; i < h; ++i) {
			bits -= pitch;
			fwrite(bits, w, 1, fp);
			int pad = alignWidth - w;
			while (pad--) {
				fputc(0, fp);
			}
		}

		fclose(fp);
	}
}

void savePNG(const char *filename, const uint8_t *bits, const uint8_t *pal16, const uint8_t *trns, int w, int h) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		png_struct *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		assert(png);
		png_init_io(png, fp);

		png_info *info = png_create_info_struct(png);
		assert(info);
		png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		png_color png_pal[PNG_MAX_PALETTE_LENGTH];
		for (int i = 0; i < 16; ++i) {
			png_pal[i].red = pal16[0];
			png_pal[i].green = pal16[1];
			png_pal[i].blue = pal16[2];
			pal16 += 3;
		}
		for (int i = 16; i < PNG_MAX_PALETTE_LENGTH; ++i) {
			png_pal[i].red = 0;
			png_pal[i].green = 0;
			png_pal[i].blue = 0;
		}
		png_set_PLTE(png, info, png_pal, PNG_MAX_PALETTE_LENGTH);

		png_set_tRNS(png, info, trns, PNG_MAX_PALETTE_LENGTH, 0);
		png_write_info(png, info);

		for (int i = 0; i < h; ++i) {
			png_write_row(png, bits + i * w);
		}
		png_write_end(png, info);

		png_destroy_write_struct(&png, &info);

		fclose(fp);
	}
}
