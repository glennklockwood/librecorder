# Build image:
#
#   docker build -t librecorder .
#
# Run image to play with librecorder:
#
#   docker run -it --rm librecorder /bin/bash
#
# or docker-compose if you want to bindmount . instead of copy it:
#
#   docker-compose run --rm dev-shell
#
# Run IOR within container:
#
#   mpirun -n 1 /usr/local/bin/ior
#
# This will generate recorder logs in /work/root_ior
#

FROM centos:7

RUN yum install -y gcc vim \
                   mpich mpich-devel \
                   git make \
                   pkg-config autoconf automake && \
                   yum clean all && \
                   rm -rf /var/cache/yum

ENV PATH=/usr/lib64/mpich/bin:$PATH

WORKDIR /work

# Check version of librecorder and rebuild if necessary
COPY . ./librecorder
RUN cd librecorder && \
    make CC=mpicc && \
    ln -s /work/librecorder/lib/librecorder.so /usr/local/lib64

ENV LD_LIBRARY_PATH=/usr/local/lib64

# Build IOR against librecorder
RUN git clone --depth 1 https://github.com/hpc/ior.git && \
    mkdir -p ior/build && \
    cd ior && \
    autoreconf -ivf && \
    cd build && \
    ../configure CC=mpicc --prefix=/usr/local && make LDFLAGS=-lrecorder && make install


