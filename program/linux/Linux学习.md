# Linux学习指引

​                                                                                                                                                                                   Edited by Rimon Chen

## 1 Linux发行版本选择

选择合适自己的，个人建议Ubuntu，原因如下：

(1) 有比较全的软件仓库(APT)，同时输入命令时如果未安装会提示安装的软件包(yum就没那么智能)

​    ![image-20220829224929388](apt智能提示.png)

(2) 相对易用的软件界面（作为入门还是可以的，后面慢慢需要摆脱出来）

​    推荐方法虚拟机+samba+ssh

(3) 基于标准内核之上的相对独立的修改（而不是魔改版本，如雨林木风OS)

![image-20220829224929388](linux内核源码目录.png)

(4) 完善的论坛、社区（不要永远只会百度)      https://askubuntu.com/

## 2 Linux环境设置

openssh(证书登录)

samba

vim(配合nerdtree插件)

## 3 Linux Shell

鸟哥 Linux 私房菜：基础版.pdf

man: 不懂就问男人，不要永远只会百度，你出现的问题别人不一定遇到，别人遇到解决后也不一定发到网上，哪怕发到网上的也不一定解释全面，man帮助手册是最全最权威性的资料

![image-20220829231501711](man.png)

## 4 Linux编程

### 4.1 Linux编程环境准备

make

gcc

cgdb

cmake/qt

### 4.1 Linux工程管理

makefile

CMakeLists.txt

project.pro

### 4.2 Linux系统API编程

Linux系统编程.pdf

errno: 函数调用的常见错误码

守护进程编写（促使你去了解文件描述符、标准输入输出、错误输出、fork、进程、进程树、会话、工作目录等概念）

### 4.3 Linux网络编程

Linux网络编程.pdf

### 4.4 Linux程序调试

gdb命令

cgdb

远程gdb+vscode

ulimit -c unlimited

善于使用proc/sys虚拟文件系统获取信息

查看/var/log下面相关的log

demsg

### 4.5 Linux驱动程序了解

虽然不一定要写驱动，稍微了解一下框架有助于理解Linux的一些概念，便于定位问题

### 4.6 Linux源码阅读

## 5 Linux交叉编译

uboot(x86_64下为grub)

Linux开机引导过程

没有开发板用wine，简单的选项处理，不需要精通，有个概念即可

