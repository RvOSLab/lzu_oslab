echo -e "\033[41;37m 正在安装必要软件包 \033[0m"
sudo apt install -y build-essential pkg-config libglib2.0-dev libpixman-1-dev binutils texinfo axel git make gcc-riscv64-linux-gnu libncurses5-dev
echo -e "\033[41;37m 正在下载必要源码包 \033[0m"
mkdir resource
cd resource
wget https://gitee.com/Hanabichan/lzu-oslab-resource/raw/master/gdb-10.1.tar.xz
wget https://gitee.com/Hanabichan/lzu-oslab-resource/raw/master/qemu-5.1.0.tar.xz
echo -e "\033[41;37m 正在解压 \033[0m"
tar xJf qemu-5.1.0.tar.xz
tar -xvJ -f gdb-10.1.tar.xz
echo -e "\033[41;37m 解压完成 \033[0m"
cd qemu-5.1.0
echo -e "\033[41;37m 开始编译安装 QEMU \033[0m"
./configure --target-list=riscv32-softmmu,riscv64-softmmu
make -j$(nproc)
sudo make install
echo -e "\033[41;37m 开始编译安装 GDB \033[0m"
cd ../gdb-10.1/
./configure --target=riscv64-unknown-elf --enable-tui=yes
make -j$(nproc)
sudo make install
echo -e "\033[41;37m GDB安装完成，版本信息： \033[0m"
riscv64-unknown-elf-gdb -v
echo -e "\033[41;37m QEMU安装完成，版本信息： \033[0m"
qemu-system-riscv64 --version
echo -e "\033[41;37m 清理安装文件 \033[0m"
cd ../..
rm -rf resource