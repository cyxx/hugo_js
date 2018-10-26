
#ifndef FILEIO_H__
#define FILEIO_H__

#include "util.h"

extern void	fio_init(const char *datapath);
extern void	fio_fini();
extern int	fio_open(const char *filename, int error_flag);
extern void	fio_close(int slot);
extern int	fio_size(int slot);
extern int	fio_seek(int slot, int offset, int whence);
extern int	fio_tell(int slot);
extern int	fio_read(int slot, void *data, int len);
extern int	fio_read16le(int slot);
extern int	fio_read24le(int slot);
extern int 	fio_read32le(int slot);

#endif /* FILEIO_H__ */
