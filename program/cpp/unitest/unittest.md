# 单元测试

## 0 静态测试

使用cppcheck进行代码缺陷静态测试

## 1 覆盖率测试

### 1.1 环境搭建

#### 1.1.1 gtest下载编译

```bash
cmake -Dgtest_build_samples=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../ ..
make install
```

#### 1.1.2 命令安装

```bash
sudo apt install lcov
```

### 1.2 工程设置

在链接时需要加上gcov链接参数。这样生成的目标文件目录中会生成对应的 gcno 文件

```bash
-fprofile-arcs -ftest-coverage
```

如果cmake工程加上如下这段

```cmake
OPTION(ENABLE_GCOV "Enable gcov (debug, Linux builds only)" ON)
IF (ENABLE_GCOV AND NOT WIN32 AND NOT APPLE)
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
  SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage -lgcov")
ENDIF()
```



### 1.3 使用LCOV统计覆盖率

LCOV 是GCOV的可视化工具，GCOV是linux代码覆盖率统计工具。

运行生成的可执行程序，gtest_demo，可以生成gcda文件

执行lcov命令，生成代码覆盖率文件gtest_demo.info，使用--no-external参数可以忽略项目外部代码的统计

```bash
lcov --capture --directory . --output-file gtest_demo.info --test-name gtest_demo
```

执行genhtml命令，将覆盖率结果文件转换为html文件，并输出到指定目录

```bash
genhtml gtest_demo.info --output-directory output --title "JCI GoogleTest/LCOV Demo" --show-details --legend
```

查看index.html文件

## 2 性能测试

### 2.1 gprof

#### 2.1.1 环境搭建

gprof是gcc自带的工具，只要安装了gcc自动会同步安装

```bash
sudo apt install python3 graphviz
```

也可以下载windows版本graphviz 2.38版本

#### 2.1.2 gprof工具介绍

gprof是GNU组织下的一个比较有用的性能测试功能：

　　主要功能：  找出应用程序中消耗CPU时间最多的函数；

　　　　　　　　产生程序运行时的函数调用关系、调用次数

　　基本原理：  首先用户要使用gprof工具，必须在GCC编译该文件的选项中添加-pg选项，然后GCC会在用户应用程序的每一个函数中加入一个名为mcount（或者是_mcount、__mcount，这依赖于编译器或操作系统）的函数，即应用程序中每一个函数都要调用mcount函数，而mcount函数使用后会在内存中保存函数调用图，并通过函数调用堆栈的形式查找子函数和父函数的地址，这张调用图也保存了所有与函数调用相关的调用时间、调用次数等信息。当应用程序执行完毕，会在当前目录下产生gmon.out文件，gprof工具正是通过分析gmon.out文件才得出统计资料的。

　　使用gprof工具的主要格式：

　　　　gprof [选项] 用户应用程序 gmon.out

　　gprof命令选项：

　　　　-b　　不再输出统计表格中的详细信息，仅显示简要信息

　　　　-p　　只输出函数的调用图

　　　　-i　　 输出该统计总结信息

　　　　-T　　以传统的BSD格式打印输出信息

　　　　-q　　仅输出函数的时间消耗列表

　　　　-w width　　设置输出的宽度

　　　　-e Name　　不输出函数Name以及其子函数的调用图

　　　　-f Name　　输出函数Name及其子函数的调用图

　　　　-z　　显示从未使用过的函数信息

　　　　-D　　忽略函数中未知的变量

　　gprof输出信息列表项解释：

　　　　name　　函数名称

　　　　%time　　函数使用占全部时间的百分比

　　　　cumulative seconds　　函数累计执行的时间

　　　　self seconds　　函数本身执行的时间

　　　　calls　　函数被调用的次数

　　　　self call　　每次调用，花费在函数上的时间

　　　　total call　　每一次调用，花费在函数及其子函数的平均时间

#### 2.1.3 工程设置

必须在GCC编译该文件的选项中添加-pg选项

#### 2.1.4 统计性能

运行程序，在当前目录生成gmon.out

```bash
./sysmgrsdktest*
```

生成dot文件

```bash
gprof  -b sysmgrsdktest  gmon.out >>report.txt
python /home/rimon/rimon/study/unittest/gprof2dot-master/gprof2dot.py report.txt >>report.dot
dot report.dot -T png -o report.png # （命令行显示内存不足）
```

在windows上，用gvedit.exe直接转为pdf文件（目前没出错）

### 2.2 valgrind

#### 2.2.1 环境搭建 

```bash
sudo apt install valgrind 
sudo apt install kcachegrind
```

另外valgrind也可以通过源码安装，脚本如下

```bash
tar -xvf valgrind-3.16.1.tar.bz2
cd valgrind-3.16.1
./configure --prefix=/usr/local/valgrind
make
make install
```



#### 2.2.2 valgrind介绍

valgrind是运行在Linux上的一套基于仿真技术的程序调试和分析工具，用于构建动态分析工具的装备性框架。它包括一个工具集，每个工具执行某种类型的调试、分析或类似的任务，以帮助完善你的程序。主要包含以下工具：

```assembly
1、memcheck：检查程序中的内存问题，如泄漏、越界、非法指针等。

2、callgrind：检测程序代码的运行时间和调用过程，以及分析程序性能。

3、cachegrind：分析CPU的cache命中率、丢失率，用于进行代码优化。

4、helgrind：用于检查多线程程序的竞态条件。

5、massif：堆栈分析器，指示程序中使用了多少堆内存等信息。
```

#### 2.2.3 valgrind工具的使用

所有代码编译最好加上Debug选项

```bash
usage: valgrind [options] prog-and-args
```

选项可以通过`valgrind --help`来进行查看。

```bash
--tool: 是最常用的选项，用于选择使用valgrind工具集中的哪一个工具。默认值为memcheck。

--version: 用于打印valgrind的版本号

-q/--quiet: 安静的运行，只打印错误消息；

-v/--verbose: 打印更详细的信息；

--trace-children: 是否跟踪子进程，默认值为no;

--track-fds: 是否追踪打开的文件描述符，默认为no

--time-stamp=no|yes: 是否在打印出的每条消息之前加上时间戳信息。默认值为no

--log-file=<file>: 指定将消息打印到某个文件

--default-suppressions: 加载默认的抑制参数。

--alignment: 指定malloc分配内存时的最小对齐字节数；

```

如下的一些选项用于Memcheck工具：

```bash
--leak-check=no|summary|full: 在退出时是否查找内存泄露。默认值为summary

--show-leak-kinds=kind1,kind2,..: 显示哪一种类型的内存泄露。默认显示definite和possible这两种；
```

------

- (1) memcheck

用来检测程序中出现的内存问题，所有对内存的读写都会被检测到，一切对`malloc`、`free`、`new`、`delete`的调用都会被捕获，能检测以下的问题：

```assembly
1、使用未初始化的内存。如果在定义一个变量时没有赋初始值，后边即使赋值了，使用这个变量的时候Memcheck也会报"uninitialised value"错误。使用中会发现，valgrind提示很多这个错误，由于关注的是内存泄漏问题，所以可以用--undef-value-errors=选项把这个错误提示屏蔽掉，具体可以看后面的选项解释。

2、读/写释放后的内存块；

3、内存读写越界（数组访问越界／访问已经释放的内存),读/写超出malloc分配的内存块；

4、读/写不适当的栈中内存块；

5、内存泄漏，指向一块内存的指针永远丢失；

6、不正确的malloc/free或new/delete匹配（重复释放／使用不匹配的分配和释放函数）；

7、内存覆盖，memcpy()相关函数中的dst和src指针重叠。
```

示例程序如下:

```c
void func() {
    char *ptr = new char[10];
    ptr[10] = 'a';   // 内存越界

    memcpy(ptr + 1, ptr, 5);   // 踩内存

    delete []ptr;
    delete []ptr; // 重复释放

    char *p;
    *p = 1;   // 非法指针
}

int main() {
    func();

    return 0;
}
```



编译好程序后执行以下命令, 也可以让每行报告打印加上时间，`--collect-systime=`

```bash
g++ -g -o test leak.cpp
valgrind --tool=memcheck --leak-check=full ./test
```

检测结果如下：

```bash
==22786== Memcheck, a memory error detector
==22786== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==22786== Using Valgrind-3.16.1 and LibVEX; rerun with -h for copyright info
==22786== Command: ./test
==22786== 
==22786== Invalid write of size 1      // 内存越界
==22786==    at 0x4007FB: func() (mixed.cpp:6)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786==  Address 0x5a2404a is 0 bytes after a block of size 10 alloc'd
==22786==    at 0x4C2AC58: operator new[](unsigned long) (vg_replace_malloc.c:431)
==22786==    by 0x4007EE: func() (mixed.cpp:5)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786== 
==22786== Source and destination overlap in memcpy(0x5a24041, 0x5a24040, 5)  // 踩内存
==22786==    at 0x4C2E83D: memcpy@@GLIBC_2.14 (vg_replace_strmem.c:1033)
==22786==    by 0x400819: func() (mixed.cpp:8)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786== 
==22786== Invalid free() / delete / delete[] / realloc()    // 重复释放
==22786==    at 0x4C2BBAF: operator delete[](void*) (vg_replace_malloc.c:649)
==22786==    by 0x40083F: func() (mixed.cpp:11)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786==  Address 0x5a24040 is 0 bytes inside a block of size 10 free'd
==22786==    at 0x4C2BBAF: operator delete[](void*) (vg_replace_malloc.c:649)
==22786==    by 0x40082C: func() (mixed.cpp:10)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786==  Block was alloc'd at
==22786==    at 0x4C2AC58: operator new[](unsigned long) (vg_replace_malloc.c:431)
==22786==    by 0x4007EE: func() (mixed.cpp:5)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786== 
==22786== Use of uninitialised value of size 8    // 非法指针
==22786==    at 0x400844: func() (mixed.cpp:14)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786== 
==22786== 
==22786== Process terminating with default action of signal 11 (SIGSEGV): dumping core
==22786==  Bad permissions for mapped region at address 0x4008B0
==22786==    at 0x400844: func() (mixed.cpp:14)
==22786==    by 0x400851: main (mixed.cpp:18)
==22786== 
==22786== HEAP SUMMARY:
==22786==     in use at exit: 0 bytes in 0 blocks
==22786==   total heap usage: 1 allocs, 2 frees, 10 bytes allocated
==22786== 
==22786== All heap blocks were freed -- no leaks are possible
==22786== 
==22786== Use --track-origins=yes to see where uninitialised values come from  
==22786== For lists of detected and suppressed errors, rerun with: -s
==22786== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
Segmentation fault (core dumped)
```

**结果说明：**

先看看输出信息中的HEAP SUMMARY，它表示程序在堆上分配内存的情况，其中的`1 allocs`表示程序分配了 1次内存，`0 frees`表示程序释放了 0 次内存，`10 bytes allocated`表示分配了 10 个字节的内存。另外，valgrind也会报告程序是在哪个位置发生内存泄漏。

上面`LEAK SUMMARY`会打印5种不同的类型，这里我们简单介绍一下：

```assembly
definitely lost: 明确丢失的内存。程序中存在内存泄露，应尽快修复。当程序结束时如果一块动态分配的内存没有被释放并且通过程序内的指针变量均无法访问这块内存则会报这个错误；

indirectly lost: 间接丢失。当使用了含有指针成员的类或结构体时可能会报这个错误。这类错误无需直接修复，它们总是与definitely lost一起出现，只要修复definitely lost即可。

possibly lost: 可能丢失。大多数情况下应视为与definitely lost一样需要尽快修复，除非你的程序让一个指针指向一块动态分配的内存（但不是这块内存的起始地址），然后通过运算得到这块内存的起始地址，再释放它。当程序结束时如果一块动态分配的内存没有被释放并且通过程序内的指针变量均无法访问这块内存的起始地址，但可以访问其中的某一部分数据，则会报这个错误。

stil reachable: 可以访问，未丢失但也未释放。如果程序是正常结束的，那么它可能不会造成程序崩溃，但长时间运行有可能耗尽系统资源。
```

- (2) callgrind

和gprof类似的分析工具，但它对程序的运行观察更为入微，能给我们提供更多的信息。callgrind收集程序运行时的一些数据，建立函数调用关系图，还可以有选择地进行cache模拟。在运行结束时，它会把分析数据写入一个文件。callgrind.out.xxx就是我们需要的文件(其中xxx是进程号)，此文件可以通过kcachegrind工具可视化查看

- (3) cachegrind

cache分析器，它模拟CPU中的`一级缓`存和`二级缓存`，能够精确地指出程序中cache的丢失和命中。如果需要，它还能够为我们提供cache丢失次数，内存引用次数，以及每行代码，每个函数，每个模块，整个程序产生的指令数。这对优化程序有很大的帮助。

它的使用方法也是：`valgrind –tool=cachegrind ./程序名`

- 4） helgrind

它主要用来检查多线程程序中出现的竞争问题。helgrind寻找内存中被多个线程访问，而又没有一贯加锁的区域，这些区域往往是线程之间失去同步的地方，而且会导致难以发觉的错误。helgrind实现了名为eraser的竞争检测算法，并做了进一步改进，减少了报告错误的次数。不过，helgrind仍然处于实验状态。

- 5） massif

堆栈分析器，它能测量程序在堆栈中使用了多少内存，告诉我们堆块，堆管理块和栈的大小。

## 参考文档

[C/C++单测覆盖率分析](https://segmentfault.com/a/1190000020293568)

[GCC高级测试功能扩展——程序性能测试工具gprof、程序覆盖测试工具gcov](https://blog.csdn.net/doubleface999/article/details/78430491)

[valgrind的callgrind工具进行多线程性能分析](https://www.cnblogs.com/zengkefu/p/5642991.html)





