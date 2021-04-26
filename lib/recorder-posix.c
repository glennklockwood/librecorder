/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 by The HDF Group
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 * 4. All publications or advertising materials mentioning features or use of
 *    this software are asked, but not required, to acknowledge that it was
 *    developed by The HDF Group and by the National Center for Supercomputing
 *    Applications at the University of Illinois at Urbana-Champaign and
 *    credit the contributors.
 *
 * 5. Neither the name of The HDF Group, the name of the University, nor the
 *    name of any Contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission from
 *    The HDF Group, the University, or the Contributor, respectively.
 *
 * DISCLAIMER:
 * THIS SOFTWARE IS PROVIDED BY THE HDF GROUP AND THE CONTRIBUTORS
 * "AS IS" WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED. In no
 * event shall The HDF Group or the Contributors be liable for any damages
 * suffered by the users arising out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * Portions of Recorder were developed with support from the Lawrence Berkeley
 * National Laboratory (LBNL) and the United States Department of Energy under
 * Prime Contract No. DE-AC02-05CH11231.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#ifdef ENABLE_DUMP_CALL_STACK
#include <execinfo.h>
#endif

#include "recorder.h"

#ifndef HAVE_OFF64_T
typedef int64_t off64_t;
#endif

#ifdef RECORDER_PRELOAD
#include <dlfcn.h>
#include <stdlib.h>

#define RECORDER_FORWARD_DECL(name, ret, args) ret(*__real_##name) args = NULL;

#define RECORDER_DECL(__name) __name

#define RECORDER_MPI_CALL(func) __real_##func

#define MAP_OR_FAIL(func)                                                  \
    if (!(__real_##func)) {                                                \
        __real_##func = dlsym(RTLD_NEXT, #func);                           \
        if (!(__real_##func)) {                                            \
            fprintf(stderr, "Recorder failed to map symbol: %s\n", #func); \
            exit(1);                                                       \
        }                                                                  \
    }

extern double (*__real_PMPI_Wtime)(void);

#else

#define RECORDER_FORWARD_DECL(name, ret, args) extern ret __real_##name args;

#define RECORDER_DECL(__name) __wrap_##__name

#define MAP_OR_FAIL(func)

#define RECORDER_MPI_CALL(func) func

#endif

/*
 * POSIX
 */
RECORDER_FORWARD_DECL(open, int, (const char* path, int flags, ...));
RECORDER_FORWARD_DECL(open64, int, (const char* path, int flags, ...));
RECORDER_FORWARD_DECL(__open_2, int, (const char* path, int oflag));
RECORDER_FORWARD_DECL(openat, int, (int dirfd, const char* path, int flags, ...));
RECORDER_FORWARD_DECL(openat64, int, (int dirfd, const char* path, int flags, ...));
RECORDER_FORWARD_DECL(creat, int, (const char* path, mode_t mode));
RECORDER_FORWARD_DECL(creat64, int, (const char* path, mode_t mode));
// DARSHAN_FORWARD_DECL(dup, int, (int oldfd));
// DARSHAN_FORWARD_DECL(dup2, int, (int oldfd, int newfd));
// DARSHAN_FORWARD_DECL(dup3, int, (int oldfd, int newfd, int flags));
// DARSHAN_FORWARD_DECL(fileno, int, (FILE *stream));
// DARSHAN_FORWARD_DECL(mkstemp, int, (char *template));
// DARSHAN_FORWARD_DECL(mkostemp, int, (char *template, int flags));
// DARSHAN_FORWARD_DECL(mkstemps, int, (char *template, int suffixlen));
// DARSHAN_FORWARD_DECL(mkostemps, int, (char *template, int suffixlen, int flags));
RECORDER_FORWARD_DECL(read, ssize_t, (int fd, void* buf, size_t count));
RECORDER_FORWARD_DECL(write, ssize_t, (int fd, const void* buf, size_t count));
RECORDER_FORWARD_DECL(pread, ssize_t,
    (int fd, void* buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pwrite, ssize_t,
    (int fd, const void* buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pread64, ssize_t,
    (int fd, void* buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(pwrite64, ssize_t,
    (int fd, const void* buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(readv, ssize_t,
    (int fd, const struct iovec* iov, int iovcnt));
// #ifdef HAVE_PREADV
//     DARSHAN_FORWARD_DECL(preadv, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
//     DARSHAN_FORWARD_DECL(preadv64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
// #endif
// #ifdef HAVE_PREADV2
//     DARSHAN_FORWARD_DECL(preadv2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
//     DARSHAN_FORWARD_DECL(preadv64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
// #endif
RECORDER_FORWARD_DECL(writev, ssize_t,
    (int fd, const struct iovec* iov, int iovcnt));
// #ifdef HAVE_PWRITEV
//     DARSHAN_FORWARD_DECL(pwritev, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset));
//     DARSHAN_FORWARD_DECL(pwritev64, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset));
// #endif
// #ifdef HAVE_PWRITEV2
//     DARSHAN_FORWARD_DECL(pwritev2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off_t offset, int flags));
//     DARSHAN_FORWARD_DECL(pwritev64v2, ssize_t, (int fd, const struct iovec *iov, int iovcnt, off64_t offset, int flags));
// #endif
RECORDER_FORWARD_DECL(lseek, off_t, (int fd, off_t offset, int whence));
RECORDER_FORWARD_DECL(lseek64, off64_t, (int fd, off64_t offset, int whence));
RECORDER_FORWARD_DECL(__xstat, int,
    (int vers, const char* path, struct stat* buf));
RECORDER_FORWARD_DECL(__xstat64, int,
    (int vers, const char* path, struct stat64* buf));
RECORDER_FORWARD_DECL(__lxstat, int,
    (int vers, const char* path, struct stat* buf));
RECORDER_FORWARD_DECL(__lxstat64, int,
    (int vers, const char* path, struct stat64* buf));
RECORDER_FORWARD_DECL(__fxstat, int, (int vers, int fd, struct stat* buf));
RECORDER_FORWARD_DECL(__fxstat64, int, (int vers, int fd, struct stat64* buf));
RECORDER_FORWARD_DECL(mmap, void*, (void* addr, size_t length, int prot, int flags, int fd, off_t offset));
RECORDER_FORWARD_DECL(mmap64, void*, (void* addr, size_t length, int prot, int flags, int fd, off64_t offset));
RECORDER_FORWARD_DECL(fsync, int, (int fd));
RECORDER_FORWARD_DECL(fdatasync, int, (int fd));
RECORDER_FORWARD_DECL(close, int, (int fd));
// DARSHAN_FORWARD_DECL(aio_read, int, (struct aiocb *aiocbp));
// DARSHAN_FORWARD_DECL(aio_write, int, (struct aiocb *aiocbp));
// DARSHAN_FORWARD_DECL(aio_read64, int, (struct aiocb64 *aiocbp));
// DARSHAN_FORWARD_DECL(aio_write64, int, (struct aiocb64 *aiocbp));
// DARSHAN_FORWARD_DECL(aio_return, ssize_t, (struct aiocb *aiocbp));
// DARSHAN_FORWARD_DECL(aio_return64, ssize_t, (struct aiocb64 *aiocbp));
// DARSHAN_FORWARD_DECL(lio_listio, int, (int mode, struct aiocb *const aiocb_list[], int nitems, struct sigevent *sevp));
// DARSHAN_FORWARD_DECL(lio_listio64, int, (int mode, struct aiocb64 *const aiocb_list[], int nitems, struct sigevent *sevp));
RECORDER_FORWARD_DECL(rename, int, (const char* old, const char* new));
RECORDER_FORWARD_DECL(unlink, int, (const char* path));
RECORDER_FORWARD_DECL(unlinkat, int, (int dirfd, const char* pathname, int flags));
RECORDER_FORWARD_DECL(rmdir, int, (const char* path));
RECORDER_FORWARD_DECL(mkdir, int, (const char* path, mode_t mode));
RECORDER_FORWARD_DECL(mkdirat, int, (int dirfd, const char* pathname, mode_t mode));
RECORDER_FORWARD_DECL(mknod, int, (const char* path, mode_t mode, dev_t dev));

/*
 *  STDIO
 */
RECORDER_FORWARD_DECL(fopen, FILE*, (const char* path, const char* mode));
RECORDER_FORWARD_DECL(fopen64, FILE*, (const char* path, const char* mode));
// DARSHAN_FORWARD_DECL(fdopen, FILE*, (int fd, const char *mode));
// DARSHAN_FORWARD_DECL(freopen, FILE*, (const char *path, const char *mode, FILE *stream));
// DARSHAN_FORWARD_DECL(freopen64, FILE*, (const char *path, const char *mode, FILE *stream));
RECORDER_FORWARD_DECL(fclose, int, (FILE * fp));
// DARSHAN_FORWARD_DECL(fflush, int, (FILE *fp));
RECORDER_FORWARD_DECL(fwrite, size_t, (const void* ptr, size_t size, size_t nmemb, FILE* stream));
// DARSHAN_FORWARD_DECL(fputc, int, (int c, FILE *stream));
// DARSHAN_FORWARD_DECL(putw, int, (int w, FILE *stream));
// DARSHAN_FORWARD_DECL(fputs, int, (const char *s, FILE *stream));
// DARSHAN_FORWARD_DECL(fprintf, int, (FILE *stream, const char *format, ...));
// DARSHAN_FORWARD_DECL(printf, int, (const char *format, ...));
// DARSHAN_FORWARD_DECL(vfprintf, int, (FILE *stream, const char *format, va_list));
// DARSHAN_FORWARD_DECL(vprintf, int, (const char *format, va_list));
RECORDER_FORWARD_DECL(fread, size_t,
    (void* ptr, size_t size, size_t nmemb, FILE* stream));
// DARSHAN_FORWARD_DECL(fgetc, int, (FILE *stream));
// DARSHAN_FORWARD_DECL(getw, int, (FILE *stream));
// DARSHAN_FORWARD_DECL(_IO_getc, int, (FILE *stream));
// DARSHAN_FORWARD_DECL(_IO_putc, int, (int, FILE *stream));
// DARSHAN_FORWARD_DECL(fscanf, int, (FILE *stream, const char *format, ...));
// #ifndef HAVE_FSCANF_REDIRECT
// DARSHAN_FORWARD_DECL(__isoc99_fscanf, int, (FILE *stream, const char *format, ...));
// #endif
// DARSHAN_FORWARD_DECL(vfscanf, int, (FILE *stream, const char *format, va_list ap));
// DARSHAN_FORWARD_DECL(fgets, char*, (char *s, int size, FILE *stream));
RECORDER_FORWARD_DECL(fseek, int, (FILE * stream, long offset, int whence));
// DARSHAN_FORWARD_DECL(fseeko, int, (FILE *stream, off_t offset, int whence));
// DARSHAN_FORWARD_DECL(fseeko64, int, (FILE *stream, off64_t offset, int whence));
// DARSHAN_FORWARD_DECL(fsetpos, int, (FILE *stream, const fpos_t *pos));
// DARSHAN_FORWARD_DECL(fsetpos64, int, (FILE *stream, const fpos64_t *pos));
// DARSHAN_FORWARD_DECL(rewind, void, (FILE *stream));

#ifdef ENABLE_DUMP_CALL_STACK
void dump_call_stack(void)
{
    /* compile with -rdynamic to get symbols */
    char buf[4096];
    void* array[10];
    char** strings;

    if (__recorderfh != NULL) {
        int size = backtrace(array, 10);
        if ((strings = backtrace_symbols(array, size))) {
            fprintf(__recorderfh, "  Trace:\n");
            for (int i = 0; i < size; i++)
                fprintf(__recorderfh, "    %s\n", strings[i]);
        }
        free(strings);
    }
    return;
}
#endif

char* fd2name(int fd)
{
    size_t len = 256;
    struct stat sb;
    char fdname[len];
    sprintf(fdname, "/proc/self/fd/%d", fd);
    if (lstat(fdname, &sb) == -1) {
        return "null";
    }

    char* linkname = malloc(sb.st_size + 1);
    int r = readlink(fdname, linkname, sb.st_size + 1);
    linkname[r] = '\x00';
    return linkname;
}

int RECORDER_DECL(close)(int fd)
{
    double tm1;
    int ret;

    MAP_OR_FAIL(close);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    char* actual_filename = fd2name(fd);
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f close (%s)", tm1, actual_filename);
#endif

    ret = __real_close(fd);

#ifndef DISABLE_POSIX_TRACE

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, recorder_wtime() - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(fclose)(FILE* fp)
{
    double tm1;
    int ret;

    MAP_OR_FAIL(fclose);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fclose (fp=%p ) ", tm1, fp);
#endif

    ret = __real_fclose(fp);

#ifndef DISABLE_POSIX_TRACE
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, recorder_wtime() - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(fsync)(int fd)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(fsync);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    char* actual_filename = fd2name(fd);
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fsync (%s)", tm1, actual_filename);
#endif

    ret = __real_fsync(fd);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    if (ret < 0)
        return (ret);

    return (ret);
}

int RECORDER_DECL(fdatasync)(int fd)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(fdatasync);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    char* actual_filename = fd2name(fd);
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fdatasync (%s)", tm1, actual_filename);
#endif

    ret = __real_fdatasync(fd);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

void* RECORDER_DECL(mmap64)(void* addr, size_t length, int prot, int flags,
    int fd, off64_t offset)
{
    void* ret;

    MAP_OR_FAIL(mmap64);

    ret = __real_mmap64(addr, length, prot, flags, fd, offset);
    if (ret == MAP_FAILED)
        return (ret);

    return (ret);
}

void* RECORDER_DECL(mmap)(void* addr, size_t length, int prot, int flags,
    int fd, off_t offset)
{
    void* ret;

    MAP_OR_FAIL(mmap);

    ret = __real_mmap(addr, length, prot, flags, fd, offset);
    if (ret == MAP_FAILED)
        return (ret);

    return (ret);
}

int RECORDER_DECL(creat)(const char* path, mode_t mode)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(creat);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f creat (%s, %o)", tm1, path, mode);
#endif

    ret = __real_creat(path, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(creat64)(const char* path, mode_t mode)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(creat64);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f creat64 (%s, %o)", tm1, path, mode);
#endif

    ret = __real_creat64(path, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(open64)(const char* path, int flags, ...)
{
    int mode = 0;
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(open64);
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);

#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL)
            fprintf(__recorderfh, "%.5f open64 (%s, %d, %o)", tm1, path, flags, mode);
#endif

        ret = __real_open64(path, flags, mode);

#ifndef DISABLE_POSIX_TRACE
        tm2 = recorder_wtime();

        if (__recorderfh != NULL)
            fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif
    } else {
#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL)
            fprintf(__recorderfh, "%.5f open64 (%s, %d)", tm1, path, flags);
#endif

        ret = __real_open64(path, flags);

#ifndef DISABLE_POSIX_TRACE
        tm2 = recorder_wtime();

        if (__recorderfh != NULL)
            fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif
    }

    return (ret);
}

int RECORDER_DECL(__open_2)(const char* path, int oflag)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__open_2);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f __open_2 (%s, %d)", tm1, path, oflag);
#endif

    ret = __real___open_2(path, oflag);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif
    return (ret);
}

int RECORDER_DECL(open)(const char* path, int flags, ...)
{
    int mode = 0;
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(open);

    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);

#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL)
            fprintf(__recorderfh, "%.5f open (%s, %d, %o)", tm1, path, flags, mode);
#endif
        ret = __real_open(path, flags, mode);
    } else {
#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL)
            fprintf(__recorderfh, "%.5f open (%s, %d)", tm1, path, flags);
#endif
        ret = __real_open(path, flags);
    }

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(openat)(int dirfd, const char* pathname, int flags, ...)
{
    mode_t mode = 0;
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(openat);
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);

#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL) {
            if (dirfd == AT_FDCWD) {
                fprintf(__recorderfh, "%.5f openat (AT_FDCWD, %s, %d, %o)", tm1, pathname, flags, mode);
            } else {
                char* actual_filename = fd2name(dirfd);
                fprintf(__recorderfh, "%.5f openat (%s, %s, %d, %o)", tm1, actual_filename, pathname, flags, mode);
                free(actual_filename);
            }
#endif
            ret = __real_openat(dirfd, pathname, flags, mode);
        }
    } else {
#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL) {
            if (dirfd == AT_FDCWD) {
                fprintf(__recorderfh, "%.5f openat (AT_FDCWD, %s, %d)", tm1, pathname, flags);
            } else {
                char* actual_filename = fd2name(dirfd);
                fprintf(__recorderfh, "%.5f openat (%s, %s, %d)", tm1, actual_filename, pathname, flags);
                free(actual_filename);
            }
#endif
            ret = __real_openat(dirfd, pathname, flags);
        }
    }

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}
int RECORDER_DECL(openat64)(int dirfd, const char* pathname, int flags, ...)
{
    mode_t mode = 0;
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(openat64);
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        mode = va_arg(arg, int);
        va_end(arg);

#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL) {
            if (dirfd == AT_FDCWD) {
                fprintf(__recorderfh, "%.5f openat64 (AT_FDCWD, %s, %d, %o)", tm1, pathname, flags, mode);
            } else {
                char* actual_filename = fd2name(dirfd);
                fprintf(__recorderfh, "%.5f openat64 (%s, %s, %d, %o)", tm1, actual_filename, pathname, flags, mode);
                free(actual_filename);
            }
#endif
            ret = __real_openat64(dirfd, pathname, flags, mode);
        }
    } else {
#ifndef DISABLE_POSIX_TRACE
        tm1 = recorder_wtime();
        if (__recorderfh != NULL) {
            if (dirfd == AT_FDCWD) {
                fprintf(__recorderfh, "%.5f openat64 (AT_FDCWD, %s, %d)", tm1, pathname, flags);
            } else {
                char* actual_filename = fd2name(dirfd);
                fprintf(__recorderfh, "%.5f openat64 (%s, %s, %d)", tm1, actual_filename, pathname, flags);
                free(actual_filename);
            }
#endif
            ret = __real_openat64(dirfd, pathname, flags);
        }
    }

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

FILE* RECORDER_DECL(fopen64)(const char* path, const char* mode)
{
    FILE* ret;
    double tm1, tm2;

    MAP_OR_FAIL(fopen64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fopen64 (%s, %s)", tm1, path, mode);
#endif

    ret = __real_fopen64(path, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %p %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

FILE* RECORDER_DECL(fopen)(const char* path, const char* mode)
{
    FILE* ret;
    double tm1, tm2;

    MAP_OR_FAIL(fopen);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fopen (%s, %s)", tm1, path, mode);

#endif

    ret = __real_fopen(path, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %p %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__xstat64)(int vers, const char* path, struct stat64* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__xstat64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f __xstat64 (%d, %s, %p)", tm1,
            vers, path, buf);
#endif

    ret = __real___xstat64(vers, path, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__lxstat64)(int vers, const char* path, struct stat64* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__lxstat64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f __lxstat64 (%d, %s, %p)", tm1,
            vers, path, buf);
#endif

    ret = __real___lxstat64(vers, path, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__fxstat64)(int vers, int fd, struct stat64* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__fxstat64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* path = fd2name(fd);
        fprintf(__recorderfh, "%.5f __fxstat64 (%d, %s, %p)", tm1,
            vers, path, buf);
        free(path);
    }
#endif

    ret = __real___fxstat64(vers, fd, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__xstat)(int vers, const char* path, struct stat* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__xstat);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f __xstat (%d, %s, %p)", tm1,
            vers, path, buf);
#endif

    ret = __real___xstat(vers, path, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__lxstat)(int vers, const char* path, struct stat* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__lxstat);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f __lxstat (%d, %s, %p)", tm1,
            vers, path, buf);
#endif

    ret = __real___lxstat(vers, path, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(__fxstat)(int vers, int fd, struct stat* buf)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(__fxstat);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* path = fd2name(fd);
        fprintf(__recorderfh, "%.5f __fxstat64 (%d, %s, %p)", tm1,
            vers, path, buf);
        free(path);
    }
#endif

    ret = __real___fxstat(vers, fd, buf);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(pread64)(int fd, void* buf, size_t count,
    off64_t offset)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(pread64);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL) {
        char* actual_file = fd2name(fd);
        fprintf(__recorderfh, "%.5f pread64 (%s, %p, %zu, %td)", tm1,
            actual_file, buf, count, offset);
        free(actual_file);
    }
#endif

    ret = __real_pread64(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(pread)(int fd, void* buf, size_t count, off_t offset)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(pread);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* actual_file = fd2name(fd);
        fprintf(__recorderfh, "%.5f pread(%s, %p, %zu, %td)", tm1,
            actual_file, buf, count, offset);
        free(actual_file);
    }
#endif

    ret = __real_pread(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(pwrite)(int fd, const void* buf, size_t count,
    off_t offset)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(pwrite);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f pwrite (%s, %p, %zu, %td)", tm1,
            actual_filename, buf, count, offset);
        free(actual_filename);
    }
#endif

    ret = __real_pwrite(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(pwrite64)(int fd, const void* buf, size_t count,
    off64_t offset)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(pwrite64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f pwrite64 (%s, %p, %zu, %td)", tm1,
            actual_filename, buf, count, offset);
        free(actual_filename);
    }
#endif

    ret = __real_pwrite64(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(readv)(int fd, const struct iovec* iov, int iovcnt)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(readv);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f readv (%d, iov, %d)", tm1, fd, iovcnt);
#endif

    ret = __real_readv(fd, iov, iovcnt);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(writev)(int fd, const struct iovec* iov, int iovcnt)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(writev);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f writev (%s, const iov, %d)", tm1,
            actual_filename, iovcnt);
        free(actual_filename);
    }
#endif

    ret = __real_writev(fd, iov, iovcnt);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

size_t RECORDER_DECL(fread)(void* ptr, size_t size, size_t nmemb,
    FILE* stream)
{
    size_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(fread);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* path = fd2name(fileno(stream));
        fprintf(__recorderfh, "%.5f fread (ptr, %zu, %zu, %s)", tm1, size, nmemb,
            path);
        free(path);
    }

#endif

    ret = __real_fread(ptr, size, nmemb, stream);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %zu %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(read)(int fd, void* buf, size_t count)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(read);

    tm1 = recorder_wtime();

#ifndef DISABLE_POSIX_TRACE
    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f read (%s, %p, %zu)", tm1,
            actual_filename, buf, count);
        free(actual_filename);
    }
#endif

    ret = __real_read(fd, buf, count);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %td %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

ssize_t RECORDER_DECL(write)(int fd, const void* buf, size_t count)
{
    ssize_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(write);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f write (%s, %p, %zu)", tm1,
            actual_filename, buf, count);
        free(actual_filename);
    }
#endif

    ret = __real_write(fd, buf, count);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %zu %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

size_t RECORDER_DECL(fwrite)(const void* ptr, size_t size, size_t nmemb,
    FILE* stream)
{
    size_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(fwrite);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL) {
        char* path = fd2name(fileno(stream));
        fprintf(__recorderfh, "%.5f fwrite (%p, %zu, %zu, %s)", tm1,
            ptr, size, nmemb, path);
        free(path);
    }
#endif

    ret = __real_fwrite(ptr, size, nmemb, stream);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %ld %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

off64_t RECORDER_DECL(lseek64)(int fd, off64_t offset, int whence)
{
    off_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(lseek64);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f lseek64 (fd, offset, whence)", tm1);
#endif

    ret = __real_lseek64(fd, offset, whence);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %ld %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

off_t RECORDER_DECL(lseek)(int fd, off_t offset, int whence)
{
    off_t ret;
    double tm1, tm2;

    MAP_OR_FAIL(lseek);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        char* actual_filename = fd2name(fd);
        fprintf(__recorderfh, "%.5f fseek (%s, %ld, %d)", tm1,
            actual_filename, offset, whence);
        free(actual_filename);
    }

#endif

    ret = __real_lseek(fd, offset, whence);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %ld %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(fseek)(FILE* stream, long offset, int whence)
{
    int ret;
    double tm1, tm2;

    MAP_OR_FAIL(fseek);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f fseek (%p, %ld, %d)", tm1,
            stream, offset, whence);
#endif

    ret = __real_fseek(stream, offset, whence);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

double recorder_wtime(void)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec + ((double)time.tv_usec / 1000000));
}

int RECORDER_DECL(unlink)(const char* pathname)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(unlink);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f unlink (%s)", tm1, pathname);
#endif

    ret = __real_unlink(pathname);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(unlinkat)(int dirfd, const char* pathname, int flags)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(unlinkat);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        if (dirfd == AT_FDCWD) {
            fprintf(__recorderfh, "%.5f unlinkat (AT_FDCWD, %s, %o)", tm1, pathname, flags);
        } else {
            char* actual_filename = fd2name(dirfd);
            fprintf(__recorderfh, "%.5f unlinkat (%s, %s, %o)", tm1, actual_filename, pathname, flags);
            free(actual_filename);
        }
    }
#endif

    ret = __real_unlinkat(dirfd, pathname, flags);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(rmdir)(const char* pathname)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(rmdir);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f rmdir (%s)", tm1, pathname);
#endif

    ret = __real_rmdir(pathname);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(mkdir)(const char* path, mode_t mode)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(mkdir);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f mkdir (%s, %o)", tm1, path, mode);
#endif

    ret = __real_mkdir(path, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(mkdirat)(int dirfd, const char* pathname, mode_t mode)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(mkdirat);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL) {
        if (dirfd == AT_FDCWD) {
            fprintf(__recorderfh, "%.5f mkdirat (AT_FDCWD, %s, %o)", tm1, pathname, mode);
        } else {
            char* actual_filename = fd2name(dirfd);
            fprintf(__recorderfh, "%.5f mkdirat (%s, %s, %o)", tm1, actual_filename, pathname, mode);
            free(actual_filename);
        }
    }
#endif

    ret = __real_mkdirat(dirfd, pathname, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(rename)(const char* old, const char* new)
{
    double tm1, tm2;
    int ret;

    MAP_OR_FAIL(rename);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f rename (%s, %s)", tm1, old, new);
#endif

    ret = __real_rename(old, new);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

    return (ret);
}

int RECORDER_DECL(mknod)(const char* path, mode_t mode, dev_t dev)
{
    double tm1;
    int ret;

    MAP_OR_FAIL(mknod);
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    if (__recorderfh != NULL)
        fprintf(__recorderfh, "%.5f mknod (%s, %d, %ju)", tm1, path, mode, (uintmax_t)dev);
#endif

    ret = __real_mknod(path, mode, dev);

#ifndef DISABLE_POSIX_TRACE
    if (__recorderfh != NULL)
        fprintf(__recorderfh, " %d %.5f\n", ret, recorder_wtime() - tm1);
#endif

    return (ret);
}
