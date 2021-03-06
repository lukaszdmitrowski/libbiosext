#!/bin/bash
gcc -c -g -fpack-struct -Wall -O0 -o ami.o ami.c
gcc -c -g -fpack-struct -Wall -O0 -o award.o award.c
gcc -c -g -fpack-struct -Wall -O0 -o phoenix.o phoenix.c
gcc -c -g -fpack-struct -Wall -O0 -o libbiosext.o libbiosext.c
gcc -c -g -fpack-struct -Wall -O0 -o compat.o compat.c
gcc -c -g -fpack-struct -Wall -O0 -o lh5_extract.o lh5_extract.c
ar rcs libbiosext.a ami.o award.o phoenix.o libbiosext.o compat.o lh5_extract.o

cp libbiosext.a libbiosext.h ../coreboot_flashtool/libbiosext

rm *.o
rm libbiosext.a

