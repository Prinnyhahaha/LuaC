make clean
make all BUILDMODE=static CC="gcc -m64 -O3" XCFLAGS=-DLUAJIT_ENABLE_GC64
mv ./src/libluajit.a ./lib/libluajit.a
make clean