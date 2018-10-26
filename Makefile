
DEFINES = 

CPPFLAGS += -MMD -g -O -Wall -Wpedantic $(DEFINES)

extract_dir: extract_dir.o fileio.o resource.o unpack.o util.o
	$(CC) -o $@ $^

clean:
	rm -f *.o *.d
