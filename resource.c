
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fileio.h"
#include "util.h"
#include "unpack.h"
#include "resource.h"

#define RESOURCE_FLAG_PACKED (1 << 0)

#define MAX_RESOURCE_SIZE 48000 // used by dump_file

#define PALETTE_SIZE (16 * 3)

typedef struct {
	char extension[4];
	uint32_t offset; // LE24
	uint32_t size; // LE24
	uint16_t flags;
	uint8_t bundle;
} resource_child_entry_t;

typedef struct {
	char filename[9];
	uint16_t offset;
	uint16_t entries_count;
	resource_child_entry_t *entries_table;
} resource_parent_entry_t;

static const char *_dir_fname;
static int _dir_entries_count;
static resource_parent_entry_t *_dir_entries_table;

static resource_parent_entry_t *resource_open_dir_file(const char *filename, int *count) {
	int i, j;
	resource_parent_entry_t *entries_table = 0;
	const int f = fio_open(filename, 0);
	if (!(f < 0) && fio_read16le(f) == 1) {
		int files_count = fio_read16le(f);
		print_debug(DBG_RESOURCE, "dir '%s' count %d", filename, files_count);
		entries_table = (resource_parent_entry_t *)malloc(sizeof(resource_parent_entry_t) * files_count);
		if (entries_table) {
			for (i = 0; i < files_count; ++i) {
				resource_parent_entry_t *re = &entries_table[i];
				fio_read(f, re->filename, 8);
				re->filename[8] = '\0';
				re->offset = fio_read16le(f);
				re->entries_count = fio_read16le(f);
				print_debug(DBG_RESOURCE, "entry #%d filename '%s' offs %d count %d", i, re->filename, re->offset, re->entries_count);
				re->entries_table = (resource_child_entry_t *)malloc(sizeof(resource_child_entry_t) * re->entries_count);
				if (re->entries_table) {
					const int cur_pos = fio_tell(f);
					fio_seek(f, 4 + files_count * 12 + re->offset, SEEK_SET);
					for (j = 0; j < re->entries_count; ++j) {
						resource_child_entry_t *e = &re->entries_table[j];
						fio_read(f, e->extension, 3);
						e->extension[3] = '\0';
						e->offset = fio_read24le(f);
						e->size = fio_read24le(f);
						e->flags = fio_read16le(f);
						fio_read(f, &e->bundle, 1);
						print_debug(DBG_RESOURCE, "\t #%d '%s' 0x%X %d 0x%X %d", j, e->extension, e->offset, e->size, e->flags, e->bundle);
					}
					fio_seek(f, cur_pos, SEEK_SET);
				}
			}
			*count = files_count;			
		}
		fio_close(f);
	}
	return entries_table;
}

static void resource_close_dir_file(resource_parent_entry_t *e) {
	if (e) {
		free(e->entries_table);
		free(e);
	}
}

static resource_child_entry_t *resource_find_file(const resource_parent_entry_t *entries_table, int entries_count, const char *filename) {
	int i;
	char base_filename[13];
	strcpy(base_filename, filename);
	char *ext = strchr(base_filename, '.');
	if (ext) {
		*ext = '\0';
		while (entries_count--) {
			if (strncmp(entries_table->filename, base_filename, strlen(base_filename)) == 0) {
				for (i = 0; i < entries_table->entries_count; ++i) {
					resource_child_entry_t *r = &entries_table->entries_table[i];
					if (strcmp(r->extension, ext + 1) == 0) {
						return r;
					}
				}
			}
			++entries_table;
		}
	}
	return 0;
}

void load_dir(const char *fname) {
	if (_dir_entries_table) {
		resource_close_dir_file(_dir_entries_table);
	}
	_dir_fname = fname;
	_dir_entries_table = resource_open_dir_file(fname, &_dir_entries_count);
}

void load_tune(const char *fname) {
}

// from https://en.wikipedia.org/wiki/Enhanced_Graphics_Adapter
static const uint8_t _paletteEGA[] = {
	0x00, 0x00, 0x00, // black #0
	0x00, 0x00, 0xAA, // blue #1
	0x00, 0xAA, 0x00, // green #2
	0x00, 0xAA, 0xAA, // cyan #3
	0xAA, 0x00, 0x00, // red #4
	0xAA, 0x00, 0xAA, // magenta #5
	0xAA, 0x55, 0x00, // yellow, brown #20
	0xAA, 0xAA, 0xAA, // white, light gray #7
	0x55, 0x55, 0x55, // dark gray, bright black #56
	0x55, 0x55, 0xFF, // bright blue #57
	0x55, 0xFF, 0x55, // bright green #58
	0x55, 0xFF, 0xFF, // bright cyan #59
	0xFF, 0x55, 0x55, // bright red #60
	0xFF, 0x55, 0xFF, // bright magenta #61
	0xFF, 0xFF, 0x55, // bright yellow #62
	0xFF, 0xFF, 0xFF, // bright white #63
};

static int load_file_palette(const char *fname, const char *ext, uint8_t *pal) {
	int size = 0;
	char name[16];
	strncpy(name, fname, sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;
	char *p = name + strlen(name) - 3;
	assert(p > name);
	strcpy(p, ext);
	for (p = name; *p && *p != '.'; ++p) {
		if (*p == '_') {
			*p = ' ';
		}
	}
	resource_child_entry_t *se = resource_find_file(_dir_entries_table, _dir_entries_count, name);
	if (se) {
		strncpy(name, _dir_fname, sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;
		p = strrchr(name, '.');
		if (p) {
			sprintf(p + 1, "%03d", se->bundle);
			int f = fio_open(name, 0);
			if (!(f < 0)) {
				fio_seek(f, se->offset, SEEK_SET);
				fio_read(f, pal, se->size);
				size = se->size;
				if (se->flags & RESOURCE_FLAG_PACKED) {
					size = unpack(pal, size);
				}
				if (strcmp(ext, "EGA") == 0) {
					assert(size == 16);
					uint8_t buf[16];
					memcpy(buf, pal, 16);
					for (int i = 0; i < 16; ++i) {
						assert(buf[i] < 16);
						memcpy(pal + i * 3, _paletteEGA + buf[i] * 3, 3);
					}
				} else {
					assert(size == 16 * 3);
				}
				fio_close(f);
			}
		}
	}
	return size;
}

int load_palette(const char *fname, uint8_t *pal) {
	int len;

	len  = load_file_palette(fname, "VGA", pal);
	len += load_file_palette(fname, "EGA", pal + len);

	return len;
}

int load_file(const char *fname, uint8_t *buf) {
	char *p;
	char name[16];
	int size = 0;
	strncpy(name, fname, sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;
	for (p = name; *p && *p != '.'; ++p) {
		if (*p == '_') {
			*p = ' ';
		}
	}
	resource_child_entry_t *se = resource_find_file(_dir_entries_table, _dir_entries_count, name);
	if (se) {
		print_debug(DBG_RESOURCE, "File '%s' size %d", name, se->size);
		strncpy(name, _dir_fname, sizeof(name) - 1);
		name[sizeof(name) - 1] = 0;
		p = strrchr(name, '.');
		if (p) {
			sprintf(p + 1, "%03d", se->bundle);
			int f = fio_open(name, 0);
			if (!(f < 0)) {
				fio_seek(f, se->offset, SEEK_SET);
				size = fio_read(f, buf, se->size);
				if (size != se->size) {
					print_warning("Read error, count %d, ret %d", se->size, size);
					return 0;
				}
				if (se->flags & RESOURCE_FLAG_PACKED) {
					size = unpack(buf, size);
				}
				if (strncmp(fname, "STOPL", 5) == 0) {
					// STOPL___.004 to STOPL___.009 are missing the two bytes header
					if (READ_LE_UINT16(buf) == 0x70) {
						fprintf(stdout, "Fixup %s header (1)\n", fname);
						memmove(buf + 2, buf, size);
						buf[0] = 1;
						buf[1] = 0;
						size += 2;
					}
					// STOPL___.003 is missing another two bytes
					if (strcmp(fname, "STOPL___.003") == 0 && READ_LE_UINT16(buf + 2) == 0x70) {
						fprintf(stdout, "Fixup %s header (2)\n", fname);
						memmove(buf + 6, buf + 4, size - 4);
						buf[4] = 0x50;
						buf[5] = 0;
						size += 2;
					}
				}
				fio_close(f);
			}
		}
	} else {
		print_warning("File '%s' not found", name);
	}
	return size;
}

void load_font(const char *fname, uint32_t offset) {
}

static void dump_file(const char *fname, const resource_child_entry_t *e, const char *name, uint8_t *buffer) {
	int f;
	char *ext;
	char filepath[MAXPATHLEN];
	int size = 0;

	strcpy(filepath, fname);
	ext = strrchr(filepath, '.');
	if (ext) {
		sprintf(ext + 1, "%03d", e->bundle);
		f = fio_open(filepath, 1);
		fio_seek(f, e->offset, SEEK_SET);
		size = fio_read(f, buffer, e->size);
		if (size != e->size) {
			print_warning("Read error, count %d ret %d", e->size, size);
			return;
		}
		if (e->flags & RESOURCE_FLAG_PACKED) {
			size = unpack(buffer, size);
		}
		assert(size <= MAX_RESOURCE_SIZE);
		fio_close(f);
		snprintf(filepath, sizeof(filepath), "dump/%s", name);
		{
			for (char *p = filepath; *p; ++p) {
				if (*p == ' ') {
					*p = '_';
				}
			}
		}
		FILE *fp = fopen(filepath, "wb");
		if (fp) {
			fwrite(buffer, 1, size, fp);
			fclose(fp);
		}
	}
}

void resource_dump_files(const char *fname) {
	int i, j, count;
	resource_parent_entry_t *entries;
	char filepath[MAXPATHLEN];
	uint8_t *buffer;

	print_debug(DBG_RESOURCE, "dump dir '%s'", fname);

	buffer = (uint8_t *)malloc(MAX_RESOURCE_SIZE);
	if (!buffer) {
		return;
	}

	entries = resource_open_dir_file(fname, &count);
	if (entries) {
		for (i = 0; i < count; ++i) {
			for (j = 0; j < entries[i].entries_count; ++j) {
				resource_child_entry_t *e = &entries[i].entries_table[j];
				snprintf(filepath, sizeof(filepath), "%s.%s", entries[i].filename, e->extension);
				print_debug(DBG_RESOURCE, "file '%s' size %d offset 0x%x flags 0x%x", filepath, e->size, e->offset, e->flags);
				dump_file(fname, e, filepath, buffer);
			}
		}
		resource_close_dir_file(entries);
	}
	free(buffer);
}
