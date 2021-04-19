# Lab2

本实验所需源码可从 [lzu_oslab库](https://github.com/LZU-OSLab/lzu_oslab.git) 下载。

## 目录结构

这一节我们介绍中断的开启、控制以及相关的内容。从这一节开始，我们将对实验添加一定的目录结构，在这一节中所添加的目录结构如下：

- include: 包含了所有的头文件。头文件中主要是函数的定义、部分宏函数以及数据结构与常量的定义
- init: 包含了与启动相关的源代码，主要为汇编引导程序与“主”函数
- lib: 包含了编写系统时常用的函数等
- tools: 包含了代码生成、环境启动等需要的文件
- trap: 中断相关的代码主要就存放在这个目录内

同时，由于目录改变，`Makefile`文件的内容也须随之改变，`Makefile`内添加了对子文件夹的递归引用，以保证能完整编译所需要的所有文件。

## RISC-V 中断简单介绍

在实验二中，我们依然处于 S 态中，并不涉及 U 态。

在 RISC-V 中，异常分为两类。一类是同步异常(Exception)，这类异常在指令执行期间产生错误而导致的，如访问了无效的地址或执行了无效的指令。另一类是中断(Interrupt)，它是与指令流异步的外部事件，比如鼠标的单击，时钟的中断。其中，同步异常(Exception)是不可被屏蔽的（出现了错误CPU不能视而不见），而中断是可以屏蔽的（CPU可以不理睬外设的请求）。

### CSR 寄存器

在 RISC-V 中，有八个重要的寄存器，叫控制状态寄存器（CSR），在M态分别是：

- mtvec（Machine Trap Vector）它保存发生异常时处理器需要跳转到的地址（它有两种模式，其中一种相当于x86的中断向量表的地址，我们只使用这种模式）。
- mepc（Machine Exception PC）它保存异常发生时PC的值。
- mcause（Machine Exception Cause）它指示发生异常的种类。
- mie（Machine Interrupt Enable）它指示可处理和被忽略的中断。
- mip（Machine Interrupt Pending）它列出目前正准备处理的中断。
- mtval（Machine Trap Value）它保存了陷入（trap）的附加信息：地址例外中出错的地址、发生非法指令例外的指令本身，对于其他异常，它的值为 0。
- mscratch（Machine Scratch）它可以用于暂存一个字大小的数据。
- mstatus（Machine Status）它保存全局中断使能，以及许多其他的状态

在不同的CPU特权级，寄存器的第一个字母更换成对应的不同特权级首字母即可。

对于 `mstatus` 寄存器的官方解释如下：

![](images/mstatus.png)

我们暂时只要关注其中的MPIE、SPIE、UPIE、MIE、SIE、UIE这几位（分别位于 `mstatus` 的第7、5、4、3、1、0位）：xIE代表某特权级的异常处理是否启用（1为启用），xPIE会在中断发生时临时记录中断前xIE的值。

下面给出一张异常/中断的编号表，这张表与`mie`, `mip`, `mcause` 寄存器均有关联。

若 `mie` 与 `mip` 寄存器的**第N位**为1，则分别代表第N类中断不屏蔽、第N类中断发生。若这两个寄存器的第N位同时为1，表明第N类中断可被处理，且已经发生，此时CPU就会去处理中断（若不考虑中断的优先级）。

在 `mcause` 寄存器中，最高位指示中断(Interrupt, 1)与同步异常(Exception, 0)，见下表最左列；其余位的**数字为N**（注意与`mie`, `mip` 寄存器的位表示法不同）表示发生的异常/中断是第N类。

| Interrupt | Exception Code | Description                      |
| :-------: | :------------: | :------------------------------- |
|     1     |       0        | User software interrupt          |
|     1     |       1        | Supervisor software interrupt    |
|     1     |       2        | Reserved for future standard use |
|     1     |       3        | Machine software interrupt       |
|     1     |       4        | User timer interrupt             |
|     1     |       5        | Supervisor timer interrupt       |
|     1     |       6        | Reserved for future standard use |
|     1     |       7        | Machine timer interrupt          |
|     1     |       8        | User external interrupt          |
|     1     |       9        | Supervisor external interrupt    |
|     1     |       10       | Reserved for future standard use |
|     1     |       11       | Machine external interrupt       |
|     1     |     12–15      | Reserved for future standard use |
|     1     |      ≥16       | Reserved for platform use        |
|     0     |       0        | Instruction address misaligned   |
|     0     |       1        | Instruction access fault         |
|     0     |       2        | Illegal instruction              |
|     0     |       3        | Breakpoint                       |
|     0     |       4        | Load address misaligned          |
|     0     |       5        | Load access fault                |
|     0     |       6        | Store/AMO address misaligned     |
|     0     |       7        | Store/AMO access fault           |
|     0     |       8        | Environment call from U-mode     |
|     0     |       9        | Environment call from S-mode     |
|     0     |       10       | Reserved                         |
|     0     |       11       | Environment call from M-mode     |
|     0     |       12       | Instruction page fault           |
|     0     |       13       | Load page fault                  |
|     0     |       14       | Reserved for future standard use |
|     0     |       15       | Store/AMO page fault             |
|     0     |     16–23      | Reserved for future standard use |
|     0     |     24–31      | Reserved for custom use          |
|     0     |     32–47      | Reserved for future standard use |
|     0     |     48–63      | Reserved for custom use          |
|     0     |      ≥64       | Reserved for future standard use |

### 在 S 态中断/异常发生时CPU会做什么？

上面提到的都是M态的CSR，而S态与U态的CSR只是M态CSR的子集，结构相同，不再赘述，以下直接使用S态的CSR来说明。

中断发生有一个前提，`sstatus` 寄存器中 `SIE` 位为1，`sie` 寄存器中对应中断类的位也为1，此时若 `sip` 寄存器中对应中断类的位转为1，则表明中断发生，CPU要开始处理中断。若出现同步异常，CPU会直接开始处理（同步异常不可屏蔽）。

对于同步异常，大多是在指令执行过程中发生的，此时pc指向正在执行的指令；对于中断，只有在一条指令执行完到下一条指令执行前CPU才会检查是否有中断需要响应，此时pc指向下一条指令。

处理例外时，发生例外的指令的 PC 被存入 `sepc`，且 PC 被设置为 `stvec` 寄存器中存入的值。scause 根据异常类型被设置，stval 被设置成出错的地址或者其它特定异常的信息字。随后 sstatus 中的 SIE 置零，屏蔽中断，且 SIE 之前的值被保存在 SPIE 中。发生例外时的权限模式被保存在 sstatus 的 SPP 位，然后设置当前模式为 S 模式。

因为此时PC已经被设成了 `stvec` 寄存器中存入的值，对应的是中断处理程序的地址，因此下一条指令就是中断处理程序的第一条指令，中断处理开始。

### 拓展：中断委托

常规情况下，每次发生中断，CPU需要切换至更高一级特权级来处理，然而在很多时候我们在S态（实验1、2均在S态下执行）发生的中断并没有切换到M态（SBI所在的特权级）处理，这是为什么呢？

原因是在SBI初始化过程中，默认设置了两个寄存器：`MIDELEG`（M态中断委托）与 `MEDELEG`（M态异常委托），这两个寄存器的每一位与上面提到的表所对应（表中Interrupt分别取1和0），若该位为1，则表明这一中断/异常被直接委托给S态处理，无需进入M态处理，这样就避免了特权级切换带来的额外开销。

```
Boot HART MIDELEG         : 0x0000000000000222
Boot HART MEDELEG         : 0x000000000000b109
```

## 代码讲解

### 开启中断

`main` 函数中调用的 `trap_init()` 函数的作用是初始化中断。

我们的中断处理程序入口由汇编语言写成，代码位于 `trapentry.s` 文件中，入口的标记是 `__alltraps`， 因此在 `trap_init` 函数中我们直接引入外部函数 `__alltraps`，用其地址设置 `stvec` 寄存器的值，随后将 `sstatus` 寄存器中 SIE 位置1，中断就开启了。

### 处理中断

中断处理函数是 `trapentry.s` 文件中的 `__alltraps`，主要作用就是保存当前所有寄存器的值到栈内。保存时存储结构与 `trap.h` 中定义的结构体 `trapframe` 相对应，便于后续我们在C语言中直接使用结构体来操作栈。

随后使用 `jal trap` 指令跳转到 `trap.c` 文件中的 `trap` 函数处理。我们用汇编指令 `mv a0, sp` 为 `trap` 函数传入了一个参数（C语言函数的第一个传入参数保存在寄存器a0）中，即当前的栈指针。

进入 `trap` 函数后就正式开始处理中断。此处中断处理函数可以自行编写，实验2中的代码主要任务就是根据不同的中断/异常类型分配不同的处理方法。

在 `trap` 函数结束后PC会回到 `jal trap` 后的下一条指令（这个操作由编译器在编译时自动添加相关指令），随后恢复上下文（即保存在栈内的所有寄存器），随后退出中断处理程序，返回中断前的PC位置。

### 断点异常

接下来着重讲两类中断的处理，一类是断点异常，另一类是时钟中断。

断点异常触发方式非常简单，汇编指令是 `ebreak`，C语言内嵌汇编只需要写 `__asm__ __volatile__("ebreak \n\t");` 即可，便于我们测试中断处理程序的总体流程。

注意：断点异常属于同步异常，不可被屏蔽，因此无论中断是否开启均会被 CPU 响应，但要处理仍需设定 `stvec` 寄存器。

随后在中断处理程序 `trap` 及 `exception_handler` 中根据栈中保存的 `scause` 寄存器中的值来判断是否为断点异常，并对其做出处理：

```c
kputs("breakpoint");
print_trapframe(tf);
tf->epc += 2;
break;
```

即打印一条信息，随后打印当前的上下文信息，最后让中断处理结束后的pc+2（RISC-V中大多数指令是4字节长度，但GCC默认开启压缩指令，会导致一部分指令的长度为2字节，这里 `ebreak` 指令就是2字节长的指令）。

### 时钟中断

时钟中断属于可屏蔽的中断，默认是被屏蔽的，因此除了要在中断处理函数中做相应的处理，还需要额外开启时钟中断的处理。

因此在函数 `clock_init()` 中，我们将 `SIE` 寄存器的对应位置1，开启了时钟中断。

麻烦的是，RISC-V中的时钟中断不是设置了一次启动就一劳永逸的，每次时钟中断发生时，都需要重新设置下一次时钟中断的发生时间。因此我们额外编写了 `clock_set_next_event` 函数，设置下一次时钟发生的时间为当前时间加上一定的间隔(timebase)。这一函数需要在初始化时执行，也需要在每次时钟中断发生时再次执行。

注：QEMU 的时钟频率为 10MHz，设置timebase = 100000表示时钟中断频率为100Hz，即每秒发生100次时钟中断。

中断处理函数并不复杂，除了与断点异常相同的部分外，区别在于对时钟中断的特殊处理：首先设定下一次中断发生时间，随后每发生100次（PLANED_TICK_NUM）中断，将输出一次中断计数，10次计数后关机。

```c
clock_set_next_event();
if (++ticks % PLANED_TICK_NUM == 0) {
    kprintf("%u ticks\n", ticks);
    if (++ticks / PLANED_TICK_NUM == 10){
        sbi_shutdown();
    }
}
break;
```

## 问题

1. 为什么我们在中断处理时要保存和恢复所有的寄存器？
2. 为什么断点异常处理需要在结束时让pc加2而时钟中断不需要？
3. 说说哪些中断/异常被委托到S态处理了？
