
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unpack.h"
#include "util.h"

#define UNPACK_DICTW_SIZE 8
#define UNPACK_DICTB_SIZE 12
#define UNPACK_CODES_SIZE 16

struct unpack_t {
	const uint8_t *base;
	const uint8_t *src;
	int size;
	int cflag;
	uint16_t dictw[UNPACK_DICTW_SIZE];
	uint8_t dictb[UNPACK_DICTB_SIZE];
	int len;
	int idx;
	uint8_t bits;
	uint8_t first_bytes[12];
};

static const uint8_t _codes[UNPACK_CODES_SIZE] = {
	6, 10, 10, 18, 1, 1, 1, 1, 2, 3, 3, 4, 4, 5, 7, 14
};

static struct unpack_t _unpack;

static int bytes_left() {
	return _unpack.src - _unpack.base;
}

static uint8_t next_byte() {
	--_unpack.src;
	const int offset = _unpack.src - _unpack.base;
	assert(offset >= 0);
	return offset < 12 ? _unpack.first_bytes[offset] : *_unpack.src;
}

static uint8_t next_bit() {
	const uint8_t value = next_byte();
	const int carry = _unpack.cflag;
	_unpack.cflag = (value & 0x80) != 0 ? 1 : 0;
	return (value << 1) | carry;
}

static int read_bit() {
	_unpack.cflag = (_unpack.bits & 0x80) != 0 ? 1 : 0;
	_unpack.bits <<= 1;
	if (_unpack.bits == 0) {
		_unpack.bits = next_bit();
	}
	return _unpack.cflag;
}

static uint16_t rcl(uint16_t value) {
	assert((value & 0x8000) == 0);
	const int carry = _unpack.cflag;
	_unpack.cflag = 0;
	return (value << 1) | carry;
}

static void next_code() {
	if (!read_bit()) {
		_unpack.len = 2;
		_unpack.idx = 0;
	} else if (!read_bit()) {
		_unpack.len = 3;
		_unpack.idx = 1;
	} else if (!read_bit()) {
		_unpack.len = 4;
		_unpack.idx = 2;
	} else if (!read_bit()) {
		_unpack.len = 5;
		_unpack.idx = 3;
	} else if (read_bit()) {
		_unpack.len = next_byte();
		_unpack.idx = 3;
	} else {
		read_bit();
		_unpack.len = rcl(_unpack.len);
		read_bit();
		_unpack.len = rcl(_unpack.len);
		read_bit();
		_unpack.len = rcl(_unpack.len);
		_unpack.len += 6;
		_unpack.idx = 3;
	}
}

int unpack(uint8_t *src, uint32_t size) {
	int i, count, offset;
	const uint8_t *p;
	uint16_t code1, code2;

	memset(&_unpack, 0, sizeof(_unpack));

	if (memcmp(src, "MG!2", 4) != 0) {
		print_warning("Unexpected signature for packed data %x%x%x%x", src[0], src[1], src[2], src[3]);
	} else {
		_unpack.size = READ_LE_UINT32(src + 4);
		const int input_size = READ_LE_UINT32(src + 8);
		print_debug(DBG_UNPACK, "unpackMG2 unpacked_size %d packed_size %d", _unpack.size, input_size);

		assert(input_size + 46 == size);

		p = src + input_size;
		for (i = 8; i >= 0; i -= 4) {
			memcpy(_unpack.first_bytes + i, p, 4);
			p += 4;
		}
		code2 = (int16_t)READ_LE_UINT16(p); p += 4;
		count = (int16_t)READ_LE_UINT16(p); p += 2;
		print_debug(DBG_UNPACK, "code2 0x%x count 0x%x", code2, count);

		for (i = 0; i < UNPACK_DICTW_SIZE; ++i) {
			_unpack.dictw[i] = READ_LE_UINT16(p);
			p += 2;
		}
		for (i = 0; i < UNPACK_DICTB_SIZE; ++i) {
			_unpack.dictb[i] = *p++;
		}
		assert(p == src + input_size + 46);

		_unpack.base = src;
		_unpack.src = src + input_size;
		uint8_t *q = src + _unpack.size - 1;

		_unpack.bits = count & 255;

		if (count >= 0) {
			--_unpack.src;
		}

		while (1) {

			// literal bytes
			for (int i = 0; i < code2 && q >= src && bytes_left() > 0; ++i) {
				const uint8_t value = next_byte();
				*q-- = value;
			}
			if (q <= src || bytes_left() <= 0) {
				break;
			}

			next_code();
			print_debug(DBG_UNPACK, "next code idx %d len %d offset (dst) %p (src) %p", _unpack.idx, _unpack.len, q - src, _unpack.src - _unpack.base);

			// next literal bytes length
			if (read_bit()) {
				if (read_bit()) {
					assert(_unpack.idx < UNPACK_CODES_SIZE);
					code1 = _codes[_unpack.idx];
					assert(_unpack.idx + 12 < UNPACK_CODES_SIZE);
					count = _codes[12 + _unpack.idx];
				} else {
					code1 = 2;
					assert(_unpack.idx + 8 < UNPACK_CODES_SIZE);
					count = _codes[8 + _unpack.idx];
				}
			} else {
				code1 = 0;
				assert(_unpack.idx + 4 < UNPACK_CODES_SIZE);
				count = _codes[4 + _unpack.idx];
			}
			code2 = 0;
			for (int i = 0; i < count; ++i) {
				read_bit();
				code2 = rcl(code2);
			}
			code2 += code1;

			// reference bytes offset
			if (read_bit()) {
				if (read_bit()) {
					assert(_unpack.idx + 4 < UNPACK_DICTW_SIZE);
					offset = _unpack.dictw[_unpack.idx + 4];
					assert(_unpack.idx + 8 < UNPACK_DICTB_SIZE);
					count = _unpack.dictb[_unpack.idx + 8];
				} else {
					assert(_unpack.idx < UNPACK_DICTW_SIZE);
					offset = _unpack.dictw[_unpack.idx];
					assert(_unpack.idx + 4 < UNPACK_DICTB_SIZE);
					count = _unpack.dictb[_unpack.idx + 4];
				}
			} else {
				offset = 0;
				assert(_unpack.idx < UNPACK_DICTB_SIZE);
				count = _unpack.dictb[_unpack.idx];
			}
			code1 = 0;
			for (i = 0; i < count; ++i) {
				read_bit();
				code1 = rcl(code1);
			}

			// reference bytes
			_unpack.idx = offset + 1 + code1;
			for (i = 0; i < _unpack.len && q >= src; ++i) {
				*q = q[_unpack.idx];
				--q;
			}
			_unpack.len = 0;
		}
	}
	return _unpack.size;
}
