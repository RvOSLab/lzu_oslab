sudo dnf install g++ ncurses-devel python3-devel ninja-build texinfo git
sudo dnf install autoconf automake python3 libmpc-devel mpfr-devel gmp-devel gawk  bison flex texinfo patchutils gcc gcc-c++ zlib-devel expat-devel

wget https://download.qemu.org/qemu-6.0.0.tar.xz
wget https://mirror.lzu.edu.cn/gnu/gdb/gdb-10.2.tar.xz

tar xJf qemu-6.0.0.tar.xz
tar xJf gdb-10.2.tar.xz

git clone https://github.com/riscv/riscv-gnu-toolchain
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv
sudo make -j$(nproc)
echo 'PATH=$PATH:/opt/riscv/bin' >> ~/.bashrc

cd qemu-6.0.0
echo -e "\033[41;37m 开始编译安装 QEMU \033[0m"
./configure --target-list=riscv32-softmmu,riscv64-softmmu --enable-gtk
make -j$(nproc)
sudo make install

echo -e "\033[41;37m 开始编译安装 GDB \033[0m"
cd ../gdb-10.2/
./configure --target=riscv64-unknown-elf --enable-tui=yes -with-python=python3
make -j$(nproc)
sudo make install

echo -e "\033[41;37m GDB安装完成，版本信息： \033[0m"
riscv64-unknown-elf-gdb -v
echo -e "\033[41;37m QEMU安装完成，版本信息： \033[0m"
qemu-system-riscv64 --version
