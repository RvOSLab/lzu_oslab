echo -e "\033[41;37m 正在安装必要软件包 \033[0m"
sudo apt install -y build-essential pkg-config libglib2.0-dev libpixman-1-dev
cd resource
echo -e "\033[41;37m 正在解压 \033[0m"
tar xJf qemu-5.1.0.tar.xz
echo -e "\033[41;37m 解压完成 \033[0m"
cd qemu-5.1.0
echo -e "\033[41;37m 开始编译安装 \033[0m"
./configure --target-list=riscv32-softmmu,riscv64-softmmu
make -j$(nproc)
sudo make install
echo -e "\033[41;37m QEMU安装完成，版本信息： \033[0m"
qemu-system-riscv64 --version
