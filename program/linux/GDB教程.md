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

### 2.1 aarch64编译步骤

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

