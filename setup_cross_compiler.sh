#!/bin/bash
BASEDIR=$(cd `dirname $0` && pwd)
export PREFIX="${BASEDIR}/out/path/"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

echo "[ ] Installing dependencies"
sudo apt update
sudo apt -y install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev nasm qemu-system-i386 xorriso

echo "[ ] Creating directories"
rm -rf ${BASEDIR}/out/
mkdir -p ${PREFIX}
mkdir -p ${BASEDIR}/out/src/
cd ${BASEDIR}/out/src/

echo "[ ] Downloading Binutils and GCC"
wget --no-clobber https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.gz -O binutils-2.37.tar.gz
wget --no-clobber https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.gz -O gcc-11.2.0.tar.gz

echo "[ ] Extracting Binutils"
tar -xzf binutils-2.37.tar.gz

echo "[ ] Building Binutils"
mkdir build-binutils
cd build-binutils
../binutils-2.37/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

echo "[ ] Extracting GCC"
tar -xzf gcc-11.2.0.tar.gz

echo "[ ] Building GCC"
mkdir build-gcc
cd build-gcc
../gcc-11.2.0/configure \
    --target=$TARGET \
    --prefix="$PREFIX" \
    --disable-nls \
    --enable-languages=c,c++ \
    --without-headers

make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc

echo "[ ] Cross-compiler installation completed successfully!"
echo "[ ] You can now use ${PREFIX}/bin/${TARGET}-gcc for your OS development."