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

### 2.1 环境搭建

```bash
sudo apt install python3 graphviz
```

2.2 gprof工具介绍

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

### 2.2 工程设置

必须在GCC编译该文件的选项中添加-pg选项

### 2.3 统计性能

运行程序，在当前目录生成gmon.out

```bash
./sysmgrsdktest*
```

生成dot文件

```bash
gprof  -b sysmgrsdktest  gmon.out >>report.txt
python /home/rimon/rimon/study/unittest/gprof2dot-master/gprof2dot.py report.txt >>report.dot
dot report.dot -T png -o report.png
```



## 参考文档

[C/C++单测覆盖率分析](https://segmentfault.com/a/1190000020293568)

[GCC高级测试功能扩展——程序性能测试工具gprof、程序覆盖测试工具gcov](https://blog.csdn.net/doubleface999/article/details/78430491)





