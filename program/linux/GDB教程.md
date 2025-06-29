# GDB教程

​                                                                                                                                                                                                                                Edited by Rimon Chen

## 1 源码下载

地址：http://ftp.gnu.org/gnu/gdb/

## 2 编译GDB工具

对于标准的x86_64发行版本环境一般提供了软件仓库(apt \yum)安装GDB，但对于arm/aarch64等特殊的目标板，基本只有gcc/g++，而不提供GDB工具链接，因此有必要自己编译。

### 2.1 x86_64编译步骤

(0) 依赖包下载或者安装

```bash
sudo apt install libboost-regex-dev
sudo apt install libsource-highlight-dev
```

(1) 解压源码gdb-9.2.tar.gz

(2) 配置configure

```bash
mkdir build
cd build
../configure --target=aarch64-linux-gnu --prefix=/usr/local/arm-gdbserver
```

或者不带prefix默认路径安装也可以

(3) 编译安装

```bash
make
sudo make install
```

### 2.2 aarch64编译步骤

(1) 依赖包下载编译

gdb的源码依赖source-highlight，而source-hightlight又依赖于boost，因此我们这里需要下载boost_1_74_0_rc2.tar.gz库和source-highlight-3.1.8.tar.gz

(2) 配置boost并编译

```bash
./bootstrap.sh --with-libraries=regex --prefix=/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver
```

然后修改project-config.jam

把

```json
using gcc ::  ;
```

修改为

```json
using gcc : arm : /usr/bin/aarch64-linux-gnu-gcc ;
```

然后执行以下命令进行编译安装

```bash
./b2
./b2 install
```

(3) 配置source-highlight并编译

这里要指定上一步boost编译安装所在目录，否则会导致出现boost部分类未定义的错误，这是因为它首先找到了x86下的boost库头文件和库文件了。

```bash
mkdir build
cd build
../configure --build=x86_64-linux-gnu --host=aarch64-linux-gnu --prefix=/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver --with-boost=/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver/ --with-boost-regex=boost_regex --with-boost-libdir=/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver/lib/
```

然后编译安装

```bash
make
make install
```

(3) gdb源码编译安装

```bash
mkdir build
cd build
../configure --host=aarch64-linux-gnu --prefix=/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver CPPFLAGS=-I/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver/include LDFLAGS=-L/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver/lib
```

然后编译安装

```bash
make
make install
```

(4) 最后把/home/rimon/rimon/work/ATE6000/Tools/toolchain/gdb-9.2-arm/install/arm-gdbserver目录打包复制到板子上即可使用

## 3 GDB的使用



## 4 Segfault分析

### 4.1 代码

libread.so代码：

```c
int get_num_by_ptr(int *ptr)
{
	return *ptr;
}

void set_num_by_ptr(int *ptr, int num)
{
	num = *ptr;
}

```

主函数代码：

```c
extern int get_num_by_ptr(int *ptr);
int main(void)
{
	int num = get_num_by_ptr(0); //空指针解引用
	return 0;
}

```

makefile:

```makefile
.PHONY : all
all : read main

read : read.c
	gcc read.c -g -fPIC -shared -o libread.so

main : main.c
	gcc main.c -g -L. -lread -o main


```

程序跑飞了，由于设置问题，没产生coredump文件，但在dmesg或者/var/log/kern.log会留下如下记录：

```bash
[294290.850107] main[219522]: segfault at 0 ip 00007f30e79a0109 sp 00007ffcdf8ec5e0 error 4 in libread.so[7f30e79a0000+1000]
[294290.850117] Code: 2f 00 00 01 5d c3 0f 1f 00 c3 0f 1f 80 00 00 00 00 f3 0f 1e fa e9 77 ff ff ff f3 0f 1e fa 55 48 89 e5 48 89 7d f8 48 8b 45 f8 <8b> 00 5d c3 f3 0f 1e fa 55 48 89 e5 48 89 7d f8 89 75 f4 48 8b 45
[294436.081215] main[219667]: segfault at 0 ip 00007f947411c109 sp 00007ffd59872330 error 4 in libread.so[7f947411c000+1000]
[294436.081225] Code: 2f 00 00 01 5d c3 0f 1f 00 c3 0f 1f 80 00 00 00 00 f3 0f 1e fa e9 77 ff ff ff f3 0f 1e fa 55 48 89 e5 48 89 7d f8 48 8b 45 f8 <8b> 00 5d c3 f3 0f 1e fa 55 48 89 e5 48 89 7d f8 89 75 f4 48 8b 45
[294654.536439] main[219831]: segfault at 0 ip 00007fb7dafc4109 sp 00007ffc3d2f2330 error 4 in libread.so[7fb7dafc4000+1000]
[294654.536447] Code: 2f 00 00 01 5d c3 0f 1f 00 c3 0f 1f 80 00 00 00 00 f3 0f 1e fa e9 77 ff ff ff f3 0f 1e fa 55 48 89 e5 48 89 7d f8 48 8b 45 f8 <8b> 00 5d c3 f3 0f 1e fa 55 48 89 e5 48 89 7d f8 89 75 f4 48 8b 45
[306702.447743] INFO: NMI handler (perf_event_nmi_handler) took too long to run: 7.277 msecs
[308773.340338] audit: type=1400 audit(1718640005.489:53): apparmor="DENIED" operation="capable" profile="/usr/sbin/cups-browsed" pid=229018 comm="cups-browsed" capability=23  capname="sys_nice"
[340955.270761] main[249992]: segfault at 0 ip 00007f2245cab109 sp 00007ffd9f5bc2d0 error 4 in libread.so[7f2245cab000+1000]
[340955.270778] Code: 2f 00 00 01 5d c3 0f 1f 00 c3 0f 1f 80 00 00 00 00 f3 0f 1e fa e9 77 ff ff ff f3 0f 1e fa 55 48 89 e5 48 89 7d f8 48 8b 45 f8 <8b> 00 5d c3 f3 0f 1e fa 55 48 89 e5 48 89 7d f8 89 75 f4 48 8b 45
```

### 4.2 内核sefault消息

通过该信息，可以定位到崩溃位置的函数（如果是debug版本还可以定位到具体代码行号）

sefault主要信息如下：

- at 0 *变量内容*
- ip 00007f30e79a0109*指令地址*
- sp 00007ffcdf8ec5e0*栈顶地址*
- error 4 *错误类型*
- libread.so *崩溃的库*
- [7f30e79a0000+1000] *7f30e79a0000是库在内存映射中的 基地址，+1000是偏移地址*

错误类型对应的号位信息如下定义：

```bash
bit 0 == 0: no page found 1: protection fault
bit 1 == 0: read access 1: write access
bit 2 == 0: kernel-mode access 1: user-mode access
bit 3 == 1: use of reserved bit detected
bit 4 == 1: fault was an instruction fetch
bit 5 == 1: protection keys block access
```

### 4.3 分析

由崩溃信息得到，崩溃时的指令地址为0x7f30e79a0109，libread.so在内存映射中的基地址为0x7f30e79a0000，那么崩溃地址在libread.so中的相对偏移地址为0x109= 0x7f30e79a0109 - 0x7f30e79a0000。

绝对地址为 0x1109 = 0x109 + 0x1000

输入命令：

```bash
addr2line -f -C -e libread.so 1109
```

输出:

get_num_by_ptr

说明这个函数存在内存非法访问，如代码所示，即空指针访问。

要想知道每个符号函数的地址，输入如下指令：

```bash
objdump -S libread.so
objdump -tT libread.so
```

### 4.4 结论

当应用程序崩溃sefault时，通过有限的信息仍能有效找到应用程序崩溃的大致位置，从而为难以复现，同时又没有生成coredump文件的情况提供很好的分析手段
