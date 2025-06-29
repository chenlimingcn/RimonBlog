# GCC教程

​                                                                                                                                                                                                                                Edited by Rimon Chen

## 1 源码下载

地址：http://ftp.gnu.org/gnu/gcc/

buildroot下载地址: 

## 2 编译GDB工具

对于标准的x86_64发行版本环境一般提供了软件仓库(apt \yum)安装GDB，但对于arm/aarch64等特殊的目标板，基本只有gcc/g++，而不提供GDB工具链接，因此有必要自己编译。

### 2.1 x86_64编译步骤

(1) 下载依赖包

```bash
./contrib/download_prerequisites
```

(2) 编译依赖包(可选)

```bash
cd /usr/local/src/gcc-10.3.0/gmp-6.1.0
./configure --prefix=/usr/local/gmp-6.1.0
make && make install

cd /usr/local/src/gcc-10.3.0/mpfr-3.1.4
./configure --prefix=/usr/local/mpfr-3.1.4 --with-gmp=/usr/local/gmp-6.1.0
make && make install

cd /usr/local/src/gcc-10.3.0/mpc-1.0.3
./configure --prefix=/usr/local/mpc-1.0.3 --with-gmp=/usr/local/gmp-6.1.0 --with-mpfr=/usr/local/mpfr-3.1.4
make && make install

cd /usr/local/src/gcc-10.3.0/isl-0.18
./configure --prefix=/usr/local/isl-0.18  --with-gmp=system --with-gmp-prefix=/usr/local/gmp-6.1.0
make && make install

```

(3) 配置并编译gcc

```bash
./configure --prefix=/usr/local/gcc-10.3.0 --enable-threads=posix --enable--long-long --enable-languages=c,c++ --disable-checking --disable-multilib
make -j 4
make install
```

### 2.2 aarch64编译步骤

(1) 下载buildroot软件包

(2) 解压

```bash
xz -d buildroot-2023.08.tar.xz
tar -xf buildroot-2023.08.tar
```

(3) 配置

```bash
make menuconfig
```

```bash
因为只需要交叉编译链，所以只需要配置几个地方即可，按Enter选择，按ESC两次退出当前菜单。

Target options -> Target Architecture -> 选择cpu架构（ARC、ARM、MIPS、PowerPC等等）
-> Target Architecture -> 选择类型（如ARM分为armv4、armv5、armv6等等）

Toolchain -> C library -> 选择c库类型（uclibc、glibc、musl）
->Kernel Headers -> 选择内核版本（也可以手动指定下载）
```



(4) 编译

make toolchain -j4 V=0