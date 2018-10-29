
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "animation.h"
#include "fileio.h"
#include "screenshot.h"

int main(int argc, char *argv[]) {
	if (argc == 2) {
		char *p = strrchr(argv[1], '/');
		if (p) {
			*p++ = 0;
			fio_init(argv[1]);
		} else {
			fio_init(".");
			p = argv[1];
		}
		fio_init(argv[1]);
		if (animation_init(p, 0)) {
			char name[16];
			int anim_frame = 0;

			while (animation_next_frame()) {
				snprintf(name, sizeof(name), "ANIM-%03d.BMP", anim_frame);
				++anim_frame;
				saveBMP(name, animation_get()->ptr, animation_get()->pal, ANIM_W, ANIM_H);
			}
			animation_fini();
		}
		fio_fini();
		return 0;
	}
	return 0;
}
