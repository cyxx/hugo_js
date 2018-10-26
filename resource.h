
#ifndef RESOURCE_H__
#define RESOURCE_H__

#include <stdint.h>

extern void	load_dir(const char *fname);
extern void	load_tune(const char *fname);
extern int	load_palette(const char *fname, uint8_t *pal);
extern int	load_file(const char *fname, uint8_t *buf);
extern void	load_font(const char *fname, uint32_t offset);

extern void	resource_dump_files(const char *filename);

#endif /* RESOURCE_H__ */
