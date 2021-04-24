.PHONY: clean

CC ?= mpicc

INCL_DEPS = include/recorder.h include/recorder-dynamic.h

CFLAGS_SHARED = -fPIC -I./include -shared -DRECORDER_PRELOAD

CFLAGS += $(CFLAGS_SHARED)

all: lib/librecorder.so

%.po: %.c $(INCL_DEPS) | lib
	$(CC) $(CFLAGS) -c $< -o $@

lib/librecorder.so: lib/recorder-mpi-io.po lib/recorder-mpi-init-finalize.po lib/recorder-posix.po
	$(CC) $(CFLAGS) $(LDFLAGS) -ldl -o $@ $^ -lpthread -lrt -lz

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so
