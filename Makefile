prefix = /usr/local/
libdir = ${prefix}/lib

CC ?= mpicc
LD = @LD@

RECORDER_LOG_FORMAT = recorder-log-format.h
INCL_DEPS = include/recorder.h include/recorder-dynamic.h $(recorder_LOG_FORMAT)

CFLAGS_SHARED = -fPIC -I./include -shared -DRECORDER_PRELOAD

LIBS += -lz @LIBBZ2@
CFLAGS += $(CFLAGS_SHARED)

all: lib/librecorder.so

lib:
	@mkdir -p $@

%.po: %.c $(INCL_DEPS) | lib
	$(CC) $(CFLAGS) -c $< -o $@

lib/librecorder.so: lib/recorder-mpi-io.po lib/recorder-mpi-init-finalize.po lib/recorder-posix.po
	$(CC) $(CFLAGS) $(LDFLAGS) -ldl -o $@ $^ -lpthread -lrt -lz

install:: all
	install -d $(libdir)
	install -m 755 lib/librecorder.so $(libdir)

clean::
	rm -f *.o *.a lib/*.o lib/*.po lib/*.a lib/*.so

distclean:: clean
