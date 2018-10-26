
#ifndef UTIL_H__
#define UTIL_H__

#include <stdint.h>
#include <sys/param.h>

static inline uint16_t READ_LE_UINT16(const uint8_t *p) {
	return p[0] | (p[1] << 8);
}

static inline uint32_t READ_LE_UINT32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

#define DBG_GAME      (1 << 0)
#define DBG_FILEIO    (1 << 1)
#define DBG_RESOURCE  (1 << 2)
#define DBG_MIXER     (1 << 3)
#define DBG_SYSTEM    (1 << 4)
#define DBG_UNPACK    (1 << 5)
#define DBG_DECODE    (1 << 6)
#define DBG_ANIMATION (1 << 7)

extern int g_util_debug_mask;

extern void	string_lower(char *p);
extern void	string_upper(char *p);
extern void	print_debug(int debug_channel, const char *msg, ...);
extern void	print_warning(const char *msg, ...);
extern void	print_error(const char *msg, ...);
extern void	print_info(const char *msg, ...);

#endif /* UTIL_H__ */
