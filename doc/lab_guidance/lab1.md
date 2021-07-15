# Lab1

本实验所需源码可从 [lzu_oslab库](https://github.com/LZU-OSLab/lzu_oslab) 下载。

## 实验环境配置

### 一键部署

脚本适用于 Ubuntu 及其衍生发行版，目前一键部署脚本通过 `Ubuntu 18.04.5 x86`与 `Ubuntu 20.04 AMD64`测试。

1. 下载源码

   - 首先通过包管理器安装 `git`

   ```shell
   sudo apt update
   sudo apt install git
   ```

   - 任意选择目录，下载源代码

   ```shell
   git clone https://github.com/LZU-OSLab/lzu_oslab.git
   ```
2. 进入目录，部署QEMU和GDB

   - QEMU用于模拟RISC-V的运行环境。该实验的环境使用脚本 `setup.sh`一键部署。

   ```shell
   cd lzu_oslab    # 进入目录
   ./setup.sh    # 一键部署
   ```

### 手动配置

以下内容即setup.sh的实现逻辑。

1. 安装内核编译与环境部署所必要的软件包并从 github 下载源代码。（初次使用sudo会需要输入密码）

   ```shell
   sudo apt update
   sudo apt install -y build-essential gettext pkg-config libglib2.0-dev python3-dev libpixman-1-dev binutils libgtk-3-dev texinfo make gcc-riscv64-linux-gnu libncurses5-dev ninja-build tmux axel git

   git clone https://github.com/LZU-OSLab/lzu_oslab.git
   ```
2. 创建并进入资源目录，下载qemu与gdb的源码包，并解压。

   ```shell
   mkdir resource
   cd resource

   #下载qemu安装包。axel 是一个多线程下载器，可以使用其他下载器，若出现错误或长时间无响应可以减少线程数（15）
   axel -n 15 https://download.qemu.org/qemu-6.0.0.tar.xz
   axel -n 15 https://mirror.lzu.edu.cn/gnu/gdb/gdb-10.2.tar.xz

   # 解压
   tar xJf qemu-6.0.0.tar.xz
   tar xJf gdb-10.2.tar.xz
   ```
3. 编译并安装qemu虚拟机

   ```shell
   cd qemu-6.0.0

   # 使用 ./configure 配置安装平台的目标特征：指定系统版本，构建对应RISC-V架构的内容。
   # GTK是一套跨平台的图形用户界面工具包，配置后qemu支持使用GUI。
   ./configure --target-list=riscv32-softmmu,riscv64-softmmu --enable-gtk

   # 编译并安装
   # $(nproc)是CPU核心数，-j表示多线程编辑并行编译，-j$(nproc)表示最多允许线程数和CPU核心数相同。
   make -j$(nproc)
   sudo make install
   ```

4. 编译并安装gdb

   ```shell
   cd ../gdb-10.2/
   
   # 指定编译配置，开启tui（字符界面）模式，添加 Python3 调试脚本支持
   ./configure --target=riscv64-unknown-elf --enable-tui=yes -with-python=python3
   make -j$(nproc)
   sudo make install
   ```
5. 打印版本信息，检查环境安装是否成功并删除安装包

   ```shell
   riscv64-unknown-elf-gdb -v    # 打印gdb版本信息
   qemu-system-riscv64 --version    # 打印qemu版本信息

   cd ../..
   rm -rf resource
   ```

## 编译链接过程与 Makefile 解释

这部分请参考 `os_src/lab1/Makefile` 的注释

## 启动过程

RISC-V 处理器通常有 3 个特权级，由高到底分别为 *M 模式*（*machine mode*）、*S 模式*（*supervisor mode*）和  *U 模式*（*user mode*）。M 模式下运行最受信任的代码，S 模式下运行操作系统，U 模式下运行用户程序。M 模式下的代码对整个系统拥有绝对控制权，可以控制一切硬件资源。

M 模式下通常运行一种叫做 *SBI*（*Supervisor Binary Interface*）的程序。SBI 是一个规范，定义了 M 模式和 S 模式之间的接口(加载内核、设置定时器、控制硬件线程等硬件相关的功能)，它有多种不同实现（如本实验使用的 OpenSBI、Rust语言编写的 RustSBI）。SBI 为 S 模式下的操作系统提供了统一的底层环境，提高了 RISC-V 操作系统的可移植性。SBI 的设计遵守 RISC-V 的通用设计原则：设计与实现分离，有一个小的核心，并提供可选的模块化拓展。所有 RISC-V 处理器的 SBI 都必须实现基本拓展，基本拓展主要负责检测 SBI 版本和实现的拓展。除了基本拓展，还有定时器拓展、硬件线程管理拓展等。因此在运行操作系统内核时，要先使用基本拓展检测系统依赖的拓展是否实现，如果没有实现，内核就无法正常运行。

SBI 被加载到物理地址 0x80000000 处，通常占据 200 K 空间。SBI 会常驻在内存中，当操作系统需要执行 M 态操作时（如设置定时器时），只需向 M 态的 SBI 发起 *ecall*(*environment call*) 调用请求，M 态的 SBI 会完成相应的任务。本实验将使用到的 ecall 调用封装成了 C 函数。

启动时 SBI 会加载并跳转到物理地址 0x80200000执行。启动后的地址空间布局如下：

```
| ... |    SBI    | kernel|
      ^           ^
      |           |
 0x80000000   0x80200000
```

因此需要将内核的第一条指令（函数）放置到地址 0x80200000 上，本实验通过链接脚本 linker.ld 完成了这项工作，具体细节参考其中的注释。

## 实验题

1. **熟悉实验环境，使用 gdb 连接到 qemu 模拟器，反汇编并单步跟踪**

在命令行中输入 `make debug`即可进入调试界面。注意不要提前启动 tmux。<CTRL + A>X可以退出qemu。

2. **了解 RISC-V 汇编，设置堆栈并跳转到 main 函数**

**详细步骤：**

1. 声明全局符号 `boot_stack`和 `_start`（入口函数）。伪指令 `.globl`声明全局符号。
2. 将 `_start`放置在节 `.text.entry`中。伪指令 `.section`声明后续代码所在的节。
3. 实现 `_start`，将临时堆栈堆栈栈顶 `boot_stack_top`地址加载到堆栈指针 `sp`中。伪指令 `la`将符号地址加载到寄存器中。
4. 跳转到 `main`函数。伪指令 `call`用于调用函数。
5. 分配堆栈 `boot_stack`，栈顶为 `boot_stack_top`，大小为 16 页（4096 *16 字节），该堆栈放置在 `.bss`节中。伪指令 `.section`声明后续数据所在的节，`.space`分配内存。

**提示：**

- 如果 `main()`函数不打印任何字符，不论是否成功跳转到 `main()`都不会有现象，只能通过 gdb 观察，可以考虑先在 `main()`中打印 `Hello LZU OS`，再写 entry.s。
- 请不要有心理负担，本题只要求你有 RISC-V 的最基础知识，程序只有几行。

3. **了解 RISC-V SBI，使用封装好的 ecall 检测环境，并打印 `Hello LZU OS`。**

ecall 调用已经封装成了 C 函数，不用深究原理，直接调用即可。

调用封装好的 ecall 检测环境，成功时应打印成功信息，失败时应打印失败信息。输出格式没有要求，检测的内容、数量也没有要求，可以检测当前实现版本、ID、SBI 版本，也可以检测系统是否支持某个拓展，还可以检测系统是否支持某个特定函数。

**提示：**

- sbi.h 中定义了一些 SBI ID、拓展 ID 相关的宏，可以直接使用。
- `struct sbiret`中的 `error`成员为 `SBI_SUCCESS`表示 ecall 调用成功。
- `sbi_console_putchar()`提供了在调试控制台打印字符的功能。该函数打印的字符只能在终端下的 qemu 中看到。该函数可以打印换行符 `\n`。
- 实验到这里还没有实现格式化打印函数 `printf()`，如果需要格式化打印（如打印数字），请手动完成字符串到数字的转换，或者自行实现 `printf()`。

Makefile 中已经完成了项目的构建，以上两个题目都不需要修改 Makefile。

题目 2 在 entry.s 中完成，题目 3 在 main.c 中完成。

题目所需的所有背景知识均可在[参考资料](#参考资料)中找到。

## 参考资料

理解编译链接：

- 《深入理解计算机系统（原书第 3 版）》: 第 7 章介绍了链接过程。
- [Understanding Symbols Relocation](https://stac47.github.io/c/relocation/elf/tutorial/2018/03/01/understanding-relocation-elf.html#:~:text=R_X86_64_COPY%20tells%20the%20dynamic%20linker%20to%20copy%20the,code%20will%20access%20the%20variable%20via%20the%20GOT.): 详细介绍了 x86 平台上动态链接的位置无关代码进行符号重定位的过程。
- GCC 的 man page: 介绍了 GCC 的全部编译选项

理解 SBI：

- [riscv-sbi-doc](https://github.com/riscv/riscv-sbi-doc/blob/master/riscv-sbi.adoc): SBI 规范。本实验封装了其中一部分 ecall 调用。

理解 RISC-V 指令集和汇编语言：

- [RISC-V Assembly Language](https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md)：RISC-V 官方的汇编语言教程。篇幅很短，都是例子，需要有 RISC-V 指令的基础。
- [RISC-V Reader（中译版）](http://crva.ict.ac.cn/documents/RISC-V-Reader-Chinese-v2p1.pdf)：完整的 RISC-V 教程，包括主要的指令集拓展和汇编语言。该书假设读者有计算机组成原理、体系结构的知识，了解至少一种指令集，有使用汇编语言编程的经验。
- 《计算机组成与设计-硬件/软件接口（原书第 5 版）》：RISC-V 的两位主要设计者撰写的教材。第二章介绍 RISC-V 指令，并使用 RISC-V 指令编程。该书假设读者没有任何关于计算机组成原理和汇编语言的知识。

理解内联汇编和调用规范：

- [How to Use Inline Assembly Language in C Code](https://gcc.gnu.org/onlinedocs/gcc-10.2.0/gcc/Using-Assembly-Language-with-C.html#Using-Assembly-Language-with-C):GCC 内联汇编文档，主要以 x86 为例。
- [GNU C 拓展：寄存器变量](https://gcc.gnu.org/onlinedocs/gcc-10.2.0/gcc/Local-Register-Variables.html#Local-Register-Variables)：介绍了 GCC 对于寄存器变量的语言拓展。GCC 内联汇编对 RISC-V 的支持比较有限，不允许在内联汇编中指定变量被分配到的寄存器，需要使用寄存器变量来绕过 GCC 的限制。
- [RISC-V Calling Convention](https://riscv.org/wp-content/uploads/2015/01/riscv-calling.pdf)：RISC-V C 调用规范。介绍了 C 数据类型在 RISC-V 平台上的大小了对齐要求，RVG 调用规范和软件模拟的浮点数调用规范。
- 《深入理解计算机系统（原书第 3 版》：第 3 章比较详细地介绍了 C 程序在汇编层面的表示，使用 x86-64 汇编，但是原理类似。

工具的使用：

- [RMS&#39;s gdb Debugger Tutorial](http://www.unknownroad.com/rtfm/gdbtut/gdbtoc.html)：一个简单易读的 GDB 基础教程。这里的 RMS 似乎不是 Richard M Stallman（传奇黑客，自由软件之父，GNU 项目发起者，GCC/GDB/Emacs 等自由软件作者）。
- [Debugging with GDB](https://sourceware.org/gdb/current/onlinedocs/gdb/): GDB 手册。介绍使用方法的篇幅只有100 多页，可以按需阅读。
- [GNU Make Manual](https://www.gnu.org/software/make/manual/make.html): GNU make 使用手册。本实验编写的 Makefile 比较简单，读完前四章大部分内容和隐式规则即可。
- [Tmux 配置：打造最适合自己的终端复用工具](https://www.cnblogs.com/zuoruining/p/11074367.html)：比较详细的 tmux 教程。

**建议**：

RISC-V 汇编远比 x86 简单，学习起来很快。RISC-V 是精简指令集，设计者认为大道至简，指令集只需要实现了最基础、最常用的功能，复杂指令应该由编译器设计者通过组合简单指令实现。为了方便汇编程序员编程，汇编器添加了很多伪指令来降低程序员的工作量。比如，RISC-V 指令集中没有将 32 位立即数加载到寄存器中的指令，汇编器提供了伪指令 `li reg, imm`来实现这个功能，在汇编后这条伪指令会被翻译为多条机器指令。因此，在学习 RISC-V 汇编语言之前需要先学习 RV32I 指令集。建议以以下顺序阅读 RISC-V 汇编的参考资料：

1. 阅读《计算机组成与设计-硬件/软件接口（原书第 5 版）》第 2 章，了解 RISC-V 设计原则和 RV32I 指令集
2. 阅读[RISC-V Reader（中译版）](http://crva.ict.ac.cn/documents/RISC-V-Reader-Chinese-v2p1.pdf)第二、三、九章学习 RISC-V 汇编
3. 具体用法可以参考[RISC-V Assembly Language](https://github.com/riscv/riscv-asm-manual/blob/master/riscv-asm.md)

内联汇编、调用规范、链接过程等属于高级话题，不理解不影响实验，可以按兴趣阅读参考资料。

Tmux、gdb、make 的使用不是实验重点。一定要掌握 gdb 基本用法，否则将无法调试程序。不会 tmux 和 Makefile 不影响实验。掌握 tmux 的常用技巧（如临时最大化窗口，复制粘贴）可能能改善调试体验，了解 Makefile 的编写可以自由地修改项目结构并了解实际项目的构建。本实验默认使用命令行 gdb,你可以使用 TUI 版本（如 cgdb 或 gdb 内键入命令 layout src/asm），可以使用别人做好的 gdb 配置（如 托管在 github 上的 peda,gdb-dashboard），可以使用 GUI 前端（如 ddd,gdb-gui 等），还可以使用 IDE 进行调试，请自行配置。
