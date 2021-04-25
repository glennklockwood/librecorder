# Build image:
#
#   docker build -t librecorder .
#
# Run image to play with librecorder:
#
#   docker run -it --rm librecorder /bin/bash
#
# Run IOR within container:
#
#   mpirun -n 1 /usr/local/bin/ior
#
# This will generate recorder logs in /data/root_ior
#

FROM centos:7

RUN yum install -y gcc vim \
                   mpich mpich-devel \
                   git make \
                   pkg-config autoconf automake

ENV PATH=/usr/lib64/mpich/bin:$PATH

WORKDIR /data

RUN git clone --depth 1 https://github.com/glennklockwood/librecorder.git && \
    cd librecorder && \
    make CC=mpicc && \
    cp lib/librecorder.so /usr/local/lib64

ENV LD_LIBRARY_PATH=/usr/local/lib64

RUN git clone --depth 1 https://github.com/hpc/ior.git && \
    mkdir -p ior/build && \
    cd ior && \
    autoreconf -ivf && \
    cd build && \
    ../configure CC=mpicc --prefix=/usr/local && make LDFLAGS=-lrecorder && make install
