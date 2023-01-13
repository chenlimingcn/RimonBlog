# cppcheck

## 1 简述

Cppcheck 是一种 C/C++ 代码缺陷静态检查工具。

## 2 安装

```bash
sudo apt install cppcheck
```



## 3 使用选项

### 3.1 平台

--platform=<type>

    * unix32 : 32 bit unix variant
    * unix64 : 64 bit unix variant
    * win32A : 32 bit Windows ASCII character encoding
    * win32W : 32 bit Windows UNICODE character encoding
    * win64 : 64 bit Windows
    * avr8 : 8 bit AVR microcontrollers
    * native : Type sizes of host system are assumed, but no further assumptions.
    * unspecified : Unknown type sizes

### 3.2 消息等级

| 等级         | 意义                                                         | --enable option |
| ------------ | ------------------------------------------------------------ | --------------- |
| 所有         | 所有                                                         | all             |
| 错误         | 当发现 bug 时使用                                            | error           |
| 警告         | 关于防御性编程，以防止 bug 的建议                            | warning         |
| 风格         | 风格有关问题的代码清理（未使用的函数、冗余代码、常量性等等） | style           |
| 性能         | 建议使代码更快。这些建议只是基于常识，即使修复这些消息，也不确定会得到任何可测量的性能提升。 | performance     |
| 可移植性     | 可移植性警告。64 位的可移植性，代码可能在不同的编译器中运行结果不同。 | portability     |
| 消息         | 配置问题                                                     | information     |
| 未包含头文件 | 未包含头文件                                                 | missingInclude  |

### 3.3 C++标准

--std=<id>

    * c89 : C code is C89 compatible
    * c99 : C code is C99 compatible
    * c11 : C code is C11 compatible (default)
    * c++03 : C++ code is C++03 compatible
    * c++11 : C++ code is C++11 compatible
    * c++14 : C++ code is C++14 compatible
    * c++17 : C++ code is C++17 compatible
    * c++20 : C++ code is C++20 compatible (default)

### 3.4 规则文件

--rule-file=<file>

单个规则

```xml
<?xml version="1.0"?>
<rule>
  <tokenlist>normal</tokenlist>
  <pattern><![CDATA[\}\s*catch\s*\(.*\)\s*\{\s*\}]]></pattern>
  <message>
    <severity>style</severity>
    <summary>Empty catch block found.</summary>
  </message>
</rule>
```

多个规则

```xml
<?xml version="1.0"?>
<rules>
  <rule version="1">
    <pattern>Token :: (?:findm|(?:simple|)M)atch \([^,]+,\s+"(?:\s+|[^"]+?\s+")</pattern>
    <message>
      <id>TokenMatchSpacing</id>
      <severity>style</severity>
      <summary>Useless extra spacing for Token::*Match.</summary>
    </message>
  </rule>
  <rule version="1">
    <pattern>(?U)Token :: Match \([^,]+,\s+"[^%|!\[\]]+" \)</pattern>
    <message>
      <id>UseTokensimpleMatch</id>
      <severity>error</severity>
      <summary>Token::simpleMatch should be used to match tokens without special pattern requirements.</summary>
    </message>
  </rule>
</rules>
```



### 3.5 工程文件

--project=compile_commands.json

cmake加上如下选项

```cmake
set (CMAKE_EXPORT_COMPILE_COMMANDS on)
```

## 4 使用方法

```bash
cd TH
cppcheck --platform=unix64 --rule-file=cpp.rules --std=c++17 sysmgr/src
```

# 5 其它

也可以使用公司的SonarQube进行代码上传，在线看报告

## 5.1 环境准备

解压sonar-scanner-cli-4.7.0.2747-linux.zip到/opt目录

解压cppcheck-2.9.zip到/opt目录

## 5.2 代码上传

先在当前命令行窗口执行如下脚本（或者放到一个文件source)

```Bash
export CPPCHECK_HOME=/opt/cppcheck-2.9
export SONAR_SCANNER_VERSION=4.7.0.2747
export SONAR_SCANNER_HOME=/opt/sonar-scanner-$SONAR_SCANNER_VERSION-linux
export SONAR_SCANNER_OPTS="-server"
export PATH=$SONAR_SCANNER_HOME/bin:$PATH:$CPPCHECK_HOME
```

代码上传执行如下指令（host和login属性需要根据公司提供的改变，目前是不需要变）

```Bash
sonar-scanner -D"sonar.projectKey=$1" -D"sonar.sources=$ROOT_FOLDER/$1/src" -D"sonar.cxx.file.suffixes=.h,.cpp" -D"sonar.cxx.cppcheck.reportPaths=report.xml" -D"sonar.host.url=http://10.100.102.157:9000" -D"sonar.login=sqa_6c30635f239154cadd4606d5ba7e4cd3bdff1658"
```

 

## 参考文档

https://blog.csdn.net/liang19890820/article/details/52778149

https://github.com/embeddedartistry/cppcheck-rules

