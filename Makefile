.PHONY: clean

INCL_DEPS = include/recorder.h include/recorder-dynamic.h

CFLAGS_SHARED = -fPIC -I./include -shared -DRECORDER_PRELOAD

lib/librecorder.so: lib/recorder-mpi-io.po lib/recorder-mpi-init-finalize.po lib/recorder-posix.po
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) $(LDFLAGS) -ldl -o $@ $^ -lpthread -lrt -lz

%.po: %.c $(INCL_DEPS) | lib
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) -c $< -o $@

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so
