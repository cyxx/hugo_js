
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "fileio.h"
#include "util.h"

#define MAX_FILEIO_SLOTS 4

struct fileio_slot_t {
	int used;
	FILE *fp;
	int size;
} fileio_slot_t;

static const char *_data_path = "";
static struct fileio_slot_t _fileio_slots_table[MAX_FILEIO_SLOTS];

static int find_free_slot() {
	int i, slot = -1;
	for (i = 0; i < MAX_FILEIO_SLOTS; ++i) {
		if (!_fileio_slots_table[i].used) {
			slot = i;
			break;
		}
	}
	return slot;
}

void fio_init(const char *data_path) {
	_data_path = data_path;
	memset(_fileio_slots_table, 0, sizeof(_fileio_slots_table));
}

void fio_fini() {
}

static int open_file(struct fileio_slot_t *file_slot, const char *filename) {
	char buf[MAXPATHLEN];
	snprintf(buf, sizeof(buf), "%s/%s", _data_path, filename);
	file_slot->fp = fopen(buf, "rb");
	if (file_slot->fp) {
		fseek(file_slot->fp, 0, SEEK_END);
		file_slot->size = ftell(file_slot->fp);
		fseek(file_slot->fp, 0, SEEK_SET);
		return 1;
	}
	return 0;
}

int fio_open(const char *filename, int error_flag) {
	int slot = find_free_slot();
	if (slot < 0) {
		print_error("Unable to find free slot for '%s'", filename);
	} else {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		memset(file_slot, 0, sizeof(fileio_slot_t));
		if (!open_file(file_slot, filename)) {
			if (error_flag) {
				print_error("Unable to open file '%s'", filename);
			}
			print_warning("Unable to open file '%s'", filename);
			slot = -1;
		} else {
			file_slot->used = 1;
		}
	}
	return slot;
}

void fio_close(int slot) {
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->used);
		fclose(file_slot->fp);
		memset(file_slot, 0, sizeof(fileio_slot_t));
	}
}

int fio_size(int slot) {
	int size = 0;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->used);
		size = file_slot->size;
	}
	return size;
}

int fio_seek(int slot, int offset, int whence) {
	int pos = -1;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->used);
		pos = fseek(file_slot->fp, offset, whence);
	}
	return pos;
}

int fio_tell(int slot) {
	int pos = -1;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->used);
		pos = ftell(file_slot->fp);
	}
	return pos;
}

int fio_read(int slot, void *data, int len) {
	int sz = 0;
	if (slot >= 0) {
		struct fileio_slot_t *file_slot = &_fileio_slots_table[slot];
		assert(file_slot->used);
		sz = fread(data, 1, len, file_slot->fp);
	}
	return sz;
}

int fio_read16le(int slot) {
	uint8_t buf[2];
	if (fio_read(slot, buf, sizeof(buf)) == sizeof(buf)) {
		return buf[0] | (buf[1] << 8);
	}
	return 0;
}

int fio_read24le(int slot) {
	uint8_t buf[3];
	if (fio_read(slot, buf, sizeof(buf)) == sizeof(buf)) {
		return buf[0] | (buf[1] << 8) | (buf[2] << 16);
	}
	return 0;
}

int fio_read32le(int slot) {
	uint8_t buf[4];
	if (fio_read(slot, buf, sizeof(buf)) == sizeof(buf)) {
		return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	}
	return 0;
}
