
#ifndef ANIMATION_H__
#define ANIMATION_H__

#include <stdint.h>

#define ANIM_W 320
#define ANIM_H 200

#define ANIM_MAX_FRAMES 250

struct anim_t {
	const uint8_t *ptr;
	const uint8_t *pal;
	uint8_t frames[ANIM_MAX_FRAMES];
	int current_frame;
};

extern int	animation_init(const char *filename, const uint8_t *palette);
extern void	animation_fini();
extern int	animation_next_frame();
extern struct anim_t	*animation_get();

#endif /* ANIMATION_H__ */
