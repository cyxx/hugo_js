
DEFINES = 

CPPFLAGS += -MMD -g -O -Wall -Wpedantic $(DEFINES)

all: convert_ani extract_dir

convert_ani: animation.o convert_ani.o fileio.o resource.o screenshot.o unpack.o util.o
	$(CC) -o $@ $^ -lpng -lz

extract_dir: extract_dir.o fileio.o resource.o unpack.o util.o
	$(CC) -o $@ $^

clean:
	rm -f *.o *.d
