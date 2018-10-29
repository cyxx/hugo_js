
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "animation.h"
#include "fileio.h"
#include "resource.h"

#define ANIM_HEADER_SIZE 62

struct anim_t _anim;

static const char *animation_signature = "PC-Animation Version 1.1";
static uint8_t offscreen_layer[ANIM_W * ANIM_H];
static int file_ptr;
static uint8_t header_buffer[ANIM_MAX_FRAMES * 2 + ANIM_HEADER_SIZE];
static uint8_t *read_buffer_ptr;
static int read_buffer_size;
static int frames_count;
static uint8_t anim_palette[32 * 3]; // .vga + .ega

static uint8_t *read_next_frame(int size) {
	if (size > read_buffer_size) {
		free(read_buffer_ptr);
		read_buffer_size = size;
		read_buffer_ptr = (uint8_t *)malloc(read_buffer_size);
	}
	fio_read(file_ptr, read_buffer_ptr, size);
	return read_buffer_ptr;
}

static void build_greyscale_palette(uint8_t *pal_data) {
	int i;
	for (i = 16; i > 0; --i) {
		pal_data[0] = pal_data[1] = pal_data[2] = (255 * i) >> 4;
		pal_data += 3;
	}
}

static void plane_bit_copy(const uint8_t *src) {
	int y, x, b, p;
	uint8_t *dst = offscreen_layer;
	for (y = 0; y < 200; ++y) {
		for (x = 0; x < 40; ++x) {
			for (b = 0; b < 8; ++b) {
				const uint8_t mask = (1 << (7 - b));
				uint8_t color = 0;
				for (p = 0; p < 4; ++p) {
					if (src[p * 8000] & mask) {
						color |= (1 << p);
					}
				}
				*dst++ = color;
			}
			++src;
		}
	}
}

static void line_bit_copy(const uint8_t *src, int p, int y, int x, int len) {
	int b;
	uint8_t *dst = offscreen_layer + y * 320 + x * 8;
	assert(p < 4);
	while (len--) {
		for (b = 0; b < 8; ++b) {
			const uint8_t mask = (1 << (7 - b));
			if (*src & mask) {
				*dst |= (1 << p);
			} else {
				*dst &= ~(1 << p);
			}
			++dst;
		}
		++src;
	}
}

static void decode_frame(const uint8_t *frame_data, int frame_size) {
	const uint8_t *frame_data_end = frame_data + frame_size;
	uint8_t page = 0;
	int y = 0;
	while (y < ANIM_H) {
		const uint8_t code = *frame_data++;
		if (code == 0x81) {
			assert(frame_data_end - frame_data == 0);
			break;
		}
		if (code & 0x80) {
			page -= (int8_t)code;
			y += (page >> 2);
			page &= 3;
		} else {
			const uint8_t len = *frame_data++;
			line_bit_copy(frame_data, page, y, code, len);
			frame_data += len;
		}
	}
}

static const char *dir_files[] = {
	"MAIN.DIR",
	"MOUNTAIN.DIR",
	"START.DIR",
	"THEEND.DIR",
	"TRAIN.DIR",
	0
};

static int load_animation_palette(const char *filename, uint8_t *pal) {
	int i, found = 0;

	for (i = 0; dir_files[i]; ++i) {
		load_dir(dir_files[i]);
		if (load_palette(filename, pal)) {
			found = 1;
			break;
		}
	}
	return found;
}

int animation_init(const char *filename, const uint8_t *palette) {
	int count;

	read_buffer_ptr = 0;
	read_buffer_size = 0;
	file_ptr = fio_open(filename, 0);
	if (file_ptr < 0) {
		print_warning("Can't open '%s'", filename);
		return 0;
	}
	count = fio_read(file_ptr, header_buffer, sizeof(header_buffer));
	if (count != sizeof(header_buffer)) {
		print_warning("Fail reading animation header, ret %d", count);
		return 0;
	} else if (memcmp(header_buffer, animation_signature, strlen(animation_signature)) != 0) {
		print_warning("Bad signature found");
		return 0;
	}
	if (palette) {
		memcpy(anim_palette, palette, 16 * 3);
	} else if (!load_animation_palette(filename, anim_palette)) {
		print_warning("Fail to load palette for '%s'", filename);
		build_greyscale_palette(anim_palette);
	}
	_anim.current_frame = 0;
	frames_count = READ_LE_UINT16(header_buffer + ANIM_HEADER_SIZE - 2);
	print_debug(DBG_ANIMATION, "animation '%s' frames_count %d", filename, frames_count);
	_anim.ptr = offscreen_layer;
	_anim.pal = anim_palette;
	return 1;
}

void animation_fini() {
	fio_close(file_ptr);
}

int animation_next_frame() {
	if (_anim.current_frame >= frames_count) {
		return 0;
	}
	const int frame_size = READ_LE_UINT16(header_buffer + ANIM_HEADER_SIZE + _anim.current_frame * 2);
	print_debug(DBG_ANIMATION, "frame #%d size %d", _anim.current_frame, frame_size);
	uint8_t *frame_data = read_next_frame(frame_size);
	if (_anim.current_frame == 0) {
		plane_bit_copy(frame_data);
	} else {
		decode_frame(frame_data, frame_size);
	}
	++_anim.current_frame;
	return 1;
}

struct anim_t *animation_get() {
	return &_anim;
}
