# Recorder

[![Build Status](https://travis-ci.com/glennklockwood/librecorder.svg?branch=master)](https://travis-ci.com/glennklockwood/librecorder)

**This is a version of Recorder extended and maintained by Glenn K. Lockwood.**

**Recorder took a lot of code and techniques from Darshan version 2 which has
been superceded by Darshan 3 which includes its own I/O tracing which is much
better supported than what this library offers.  If you need a modern, supported
I/O tracing tool only for reads and writes, you should use [Darshan 3][]; only
use Recorder if you need to trace metadata operations which Darshan DXT does
not trace as of Darshan 3.2.1.**

Recorder is a multi-level I/O tracing framework that can capture I/O function
calls at multiple levels of the I/O stack including MPI-IO and POSIX I/O.
Recorder requires no modification or recompilation of the application and users
can control what levels are traced.

Recorder uses function interpositioning to prioritize itself over standard
functions.  Once Recorder is specified using `LD_PRELOAD`, it intercepts
function calls issued by the application and reroutes them to the tracing
implementation where the timestamp, function name, and function parameters are
recorded. 

## Installation

    make CC=mpicc

## Usage

    srun -N 1 -n 1 bash -c "LD_PRELOAD=$PWD/lib/librecorder.so ./mdtest -e 1 -w 1 -n 29"

## Publication

For a full description of the Recorder library see:

    H. Luu, B. Behzad, R. Aydt and M. Winslett, "A multi-level approach for
    understanding I/O activity in HPC applications," 2013 IEEE International
    Conference on Cluster Computing (CLUSTER), 2013, pp. 1-5, doi: [10.1109/CLUSTER.2013.6702690][].

[10.1109/CLUSTER.2013.6702690]: https://dx.doi.org/10.1109/CLUSTER.2013.6702690
[Darshan 3]: https://xgitlab.cels.anl.gov/darshan/darshan
