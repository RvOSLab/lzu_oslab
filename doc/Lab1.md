# Lab1

本实验所需源码可从 [lzu_oslab库](https://github.com/LZU-OSLab/lzu_oslab.git) 下载。

## 实验环境配置

### 一键部署

目前一键部署脚本只通过了Ubuntu 18.04.5 x86测试。

1. 下载源码

   - 任意选择目录，下载源代码

   ```shell
   wget https://github.com/LZU-OSLab/lzu_oslab.git
   ```

2. 进入目录，部署QEMU和GDB

   - QEMU用于模拟RISC-V的运行环境。该实验的环境使用脚本`setup.sh`一键部署。

   ```shell
   cd lzu_oslab	# 进入目录
   ./setup.sh		# 一键部署
   ```


### 手动配置

以下内容即setup.sh的实现逻辑。

1. 安装环境部署和内核编译所必要的软件包。

   ```shell
   sudo apt install -y build-essential pkg-config libglib2.0-dev libpixman-1-dev binutils texinfo axel git make gcc-riscv64-linux-gnu libncurses5-dev tmux
   ```

 2. 下载必要源码包，并解压。

    ```shell
    mkdir resource	#创建资源目录
    cd resource		#进入资源目录
    
    axel -n 15 https://gitee.com/Hanabichan/lzu-oslab-resource/attach_files/521696/download/qemu-5.1.0.tar.xz		#下载qemu安装包
    axel -n 15 https://gitee.com/Hanabichan/lzu-oslab-resource/attach_files/521695/download/gdb-10.1.tar.xz			#下载gdb安装包
    ```

    ```shell
    tar xJf qemu-5.1.0.tar.xz		#解压qemu包
    tar -xvJ -f gdb-10.1.tar.xz		#解压gdb包
    ```

3. 编译并安装qemu虚拟机

   - 进入目录

   ```shell
   cd qemu-5.1.0
   ```

   - 指定系统版本，构建对应RISC-V架构的内容。

   - `./configure` 命令配置安装平台的目标特征。

   ```shell
   ./configure --target-list=riscv32-softmmu,riscv64-softmmu
   ```

   - 编译并安装qemu。

   - \$(nproc)是CPU核心数，-j表示多线程编辑并行编译，-j​\$(nproc)表示最多允许线程数和CPU核心数相同。

   ```shell
   make -j$(nproc)		#编译
   sudo make install	#安装
   ```

4. 编译并安装gdb

   - 进入gdb的目录

   ```shell
   cd ../gdb-10.1/
   ```

   - 指定编译内容，开启tui模式

   ```shell
   ./configure --target=riscv64-unknown-elf --enable-tui=yes
   make -j$(nproc)
   sudo make install	#安装
   ```

5. 打印版本信息，检查环境安装是否成功并删除安装包

   ```shell
   riscv64-unknown-elf-gdb -v		# 打印gdb版本信息
   qemu-system-riscv64 --version	# 打印qemu版本信息
      
   cd ../..
   rm -rf resource
   ```
