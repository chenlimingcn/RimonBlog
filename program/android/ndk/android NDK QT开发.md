# android NDK QT开发

## 1 android ndk对应表

此为行车记录仪AIBOX（TBOX+AI）TBOX开发学习笔记

(aibox使用的是android 7.1,但ndk已无此api级别，就低不就高，选24)

| 代号        | 版本  | API 级别/NDK 版本 | 备注                                                         |
| ----------- | ----- | ----------------- | ------------------------------------------------------------ |
| Android11   | 11    | 30                |                                                              |
| Android10   | 10    | 29                |                                                              |
| Pie         | 9     | 28                |                                                              |
| Oreo        | 8.1.0 | 27                |                                                              |
| Oreo        | 8.0.0 | 26                | 这一版本开始增加了sharememory\semaphore\message queue等内容<br />后续如果需要全面使用可以考虑升级android版本 |
| Nougat      | 7.1   | 25                |                                                              |
| Nougat      | 7.0   | 24                |                                                              |
| Marshmallow | 6.0   | 23                |                                                              |



## 2 工具链制作

下载android ndk：https://developer.android.com/ndk/downloads

这里我下载的是android-ndk-r19c-linux-x86_64.zip解压,并运行以下脚本，（记得先装python)

```bash
./build/tools/make-standalone-toolchain.sh --toolchain=arm-linux-androidabi-4.9 --arch=arm --install-dir=/opt/arm-linux-android-4.9_ndklevel24 --platform=android-24 --stl=gnustl

```

定义供source的profile如下：

```bash
export NDK_HOME=/opt/arm-linux-android-4.9_ndklevel24
export PATH=$NDK_HOME/bin:$PATH
export QTDIR=/usr/local/Trolltech/QtEmbedded-4.8.6-arm
export PATH=$QTDIR/bin:$PATH
export LD_LIBRARY_PATH=$NDK_HOME/sysroot/usr/lib/arm-linux-androideabi/24:$QTDIR/lib
```

注意最新版的ndk已经放弃make-standalone-toolchain.sh制作工具链的方式，虽然此脚本还存在，但已不维护且残缺不全，此时可以直接用ndk-build,相应的

供source的profile如下：

```bash
export NDK=/opt/android-ndk-r19c-linux-x86_64
alias arm-linux-androideabi-gcc $NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang \
    -target aarch64-linux-android24
alias arm-linux-androideabi-g++ $NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
    -target aarch64-linux-android24
```

## 3 Qt4.8.6交叉编译

下载并解压qt-everywhere-opensource-src-4.8.6.tar.gz

在qt-everywhere-opensource-src-4.8.6\src\corelib\corelib.pro增加如下行

```properties
LIBS += -llog -landroid
```

增加mkspecs: qt-everywhere-opensource-src-4.8.6\mkspecs\qws\linux-arm-g++

qmake.conf

```properties
#
# qmake configuration for building with arm-linux-androideabi-g++
#

include(../../common/linux.conf)
include(../../common/gcc-base-unix.conf)
include(../../common/g++-unix.conf)
include(../../common/qws.conf)

# modifications to g++.conf
QMAKE_CC                = arm-linux-androideabi-gcc
QMAKE_CXX               = arm-linux-androideabi-g++
QMAKE_LINK              = arm-linux-androideabi-g++
QMAKE_LINK_SHLIB        = arm-linux-androideabi-g++

# modifications to linux.conf
QMAKE_AR                = arm-linux-androideabi-ar cqs
QMAKE_OBJCOPY           = arm-linux-androideabi-objcopy
QMAKE_STRIP             = arm-linux-androideabi-strip

QMAKE_CXXFLAGS          = -D__ARM_ARCH_7A__

CONFIG += plugin

load(qt_config)

```

qplatformdefs.h

```c++
#include "../../linux-g++/qplatformdefs.h"
```

configure脚本把如下行用#注释起来(因为后续已经加了空函数)

```bash
QCONFIG_FLAGS="$QCONFIG_FLAGS QT_NO_SYSTEMSEMAPHORE QT_NO_SHAREDMEMORY"
```

运行configure脚本如下

```bash
#!/bin/bash
make confclean
./configure  -prefix /usr/local/Trolltech/QtEmbedded-4.8.6-arm -opensource -confirm-license -embedded arm -xplatform qws/linux-arm-g++ -little-endian -host-little-endian -armfpa -stl -no-gui -no-qt3support -no-libmng -no-opengl -no-mmx -no-sse -no-sse2 -no-3dnow -no-webkit -no-qvfb -no-phonon -qt-freetype -no-svg -no-javascript-jit -no-script -no-scripttools -no-gif -no-rpath -no-pch -no-avx -no-neon -no-nis -no-cups -no-dbus -platform linux-g++ -no-glib -nomake examples -nomake docs -nomake demos -nomake tools -no-declarative -no-pch -v
```

Qt5才原生支持android,因此Qt4编译过程中会遇到很多错误，依据我们的使用情况，做折衷修改，不能算是十全十美的修改。下面详细列出以备出bug时参考考虑:

(1) nl_langinfo  函数无定义

在文件qt-everywhere-opensource-src-4.8.6\src\corelib\codecs\qtextcodec.cpp定义

因为我们只用utf-8编码，直接定义函数如下

```cpp
char *nl_langinfo(nl_item item)
{
    return "UTF-8";
}
```



(2) 汇编指令

在文件qt-everywhere-opensource-src-4.8.6\src\corelib\arch\qatomic_armv7.h把

```cpp
#define Q_DATA_MEMORY_BARRIER asm volatile("dmb\n":::"memory") 
#include "QtCore/qatomic_armv6.h"  
```

替换成

```c++
# define Q_DATA_MEMORY_BARRIER asm volatile("":::"memory")
#include "QtCore/qatomic_armv5.h"  
```

q_atomic_swp函数改成如下定义

```cpp
inline char q_atomic_swp(volatile char *ptr, char newval)
{
    register char ret;
    // Modified by Rimon 
    // asm volatile("swpb %0,%2,[%3]"
    //: "=&r"(ret), "=m" (*ptr)
    //              : "r"(newval), "r"(ptr)
    //              : "cc", "memory");
    ret = *ptr;
   *ptr = newval;
    return ret;
}
```

(3) pthread库相关

因为在android ndk中pthread函数族已经定义在标准库中，同时ndk没定义pthread_setcancelstate等函数，

为了不改configure相关文件，直接定义一个假的 libpthread.a ，代码如下:

```c
void test()
{
}
```

 编译放到qt-everywhere-opensource-src-4.8.6\lib目录中

在文件qt-everywhere-opensource-src-4.8.6\src\corelib\thread\qthread_unix.cpp中加上如下代码:

```c++
#include <pthread.h>
#define PTHREAD_CANCEL_DISABLE 0
#define PTHREAD_CANCEL_ENABLE 1
#define SIG_CANCEL_SIGNAL SIGUSR1
int pthread_setcancelstate(int state, int *oldstate)
{
    sigset_t   newState, old;
    int ret;
    sigemptyset (&newState);
    sigaddset (&newState, SIG_CANCEL_SIGNAL);

    ret = pthread_sigmask(state == PTHREAD_CANCEL_ENABLE ? SIG_BLOCK : SIG_UNBLOCK, &newState , &old);
    if(oldstate != NULL)
    {
        *oldstate =sigismember(&old,SIG_CANCEL_SIGNAL) == 0 ? PTHREAD_CANCEL_DISABLE : PTHREAD_CANCEL_ENABLE;
    }
    return ret;
}

#define PTHREAD_EXPLICIT_SCHED 0
#define PTHREAD_INHERIT_SCHED 1
int pthread_attr_setinheritsched(pthread_attr_t *attr,int inheritsched)
{
    return 0;
}
int pthread_cancel(pthread_t id)
{
    return pthread_kill(id, SIG_CANCEL_SIGNAL);
}

int pthread_testcancel()
{
    return pthread_kill(pthread_self(), NULL);
}
```



(4) network socket相关	socklen_t  struct __res_state

在qt-everywhere-opensource-src-4.8.6\src\network\kernel\qhostinfo_unix.cpp文件中加入以下代码

```cpp
/* res_state: the global state used by the resolver stub.  */
#define MAXNS                   3       /* max # name servers we'll track */
#define MAXDFLSRCH              3       /* # default domain levels to try */
#define MAXDNSRCH               6       /* max # domains in search path */
#define MAXRESOLVSORT           10      /* number of net to sort on */

struct __res_state {
  int   retrans;                /* retransmition time interval */
  int   retry;                  /* number of times to retransmit */
  unsigned long options;                /* option flags - see below. */
  int   nscount;                /* number of name servers */
  struct sockaddr_in
    nsaddr_list[MAXNS]; /* address of name server */
  unsigned short id;            /* current message id */
  /* 2 byte hole here.  */
  char  *dnsrch[MAXDNSRCH+1];   /* components of domain to search */
  char  defdname[256];          /* default domain (deprecated) */
  unsigned long pfcode;         /* RES_PRF_ flags - see below. */
  unsigned ndots:4;             /* threshold for initial abs. query */
  unsigned nsort:4;             /* number of elements in sort_list[] */
  unsigned ipv6_unavail:1;      /* connecting to IPv6 server failed */
  unsigned unused:23;
  struct {
    struct in_addr      addr;
    uint32_t    mask;
  } sort_list[MAXRESOLVSORT];
  /* 4 byte hole here on 64-bit architectures.  */
  void * __glibc_unused_qhook;
  void * __glibc_unused_rhook;
  int   res_h_errno;            /* last one set for this context */
  int   _vcsock;                /* PRIVATE: for res_send VC i/o */
  unsigned int _flags;          /* PRIVATE: see below */
  /* 4 byte hole here on 64-bit architectures.  */
  union {
    char        pad[52];        /* On an i386 this means 512b total. */
    struct {
      uint16_t          nscount;
      uint16_t          nsmap[MAXNS];
      int                       nssocks[MAXNS];
      uint16_t          nscount6;
      uint16_t          nsinit;
      struct sockaddr_in6       *nsaddrs[MAXNS];
#ifdef _LIBC
      unsigned long long int __glibc_extension_index
        __attribute__((packed));
#else
      unsigned int              __glibc_reserved[2];
#endif
    } _ext;
  } _u;
};

typedef struct __res_state *res_state_ptr;

typedef int (*res_init_proto)(void);
static res_init_proto local_res_init = 0;
typedef int (*res_ninit_proto)(res_state_ptr);
static res_ninit_proto local_res_ninit = 0;
typedef void (*res_nclose_proto)(res_state_ptr);
static res_nclose_proto local_res_nclose = 0;
static res_state_ptr local_res = 0;
```

(5) shm QT_NO_SHAREDMEMORY 增加空函数，不要使用相关功能

在qt-everywhere-opensource-src-4.8.6\src\corelib\kernel\qsharedmemory_unix.cpp文件第54行后中增加以下代码:

```c++
void* shmat(int __shm_id, const void* __addr, int __flags) __INTRODUCED_IN(26)
{
    return NULL;
}
int shmctl(int __shm_id, int __cmd, struct shmid_ds* __buf) __INTRODUCED_IN(26)
{
    return NULL;
}
int shmdt(const void* __addr) __INTRODUCED_IN(26)
{
    return NULL;

}
int shmget(key_t __key, size_t __size, int __flags) __INTRODUCED_IN(26)
{
    return NULL;
}

```

(6) ipc msgsnd msgrcv msgget...

qt-everywhere-opensource-src-4.8.6\src\corelib\kernel\qsystemsemaphore_unix.cpp文件增加如下代码

```c++
int semctl(int __sem_id, int __sem_num, int __cmd, ...)
{
    return 0;
}
int semget(key_t __key, int __sem_count, int __flags)
{
    return 0;
}
int semop(int __sem_id, struct sembuf* __ops, size_t __op_count)
{
    return 0;
}
```

(7) 关于qt插件

在qt-everywhere-opensource-src-4.8.6\src\corelib\plugin\qfactoryloader.cpp文件中加入如下include

```cpp
#ifdef __ANDROID__
#include <android/log.h>
#endif//__ANDROID__
```

并把QFactoryLoader::updateDir函数重新写成如下：

```cpp
void QFactoryLoader::updateDir(const QString &pluginDir, QSettings& settings)
{
    Q_D(QFactoryLoader);
    QString path = pluginDir;// + d->suffix;
    if (!QDir(path).exists(QLatin1String(".")))
        return;

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_DEBUG, "QT486", "plugins path: %s", path.toStdString().c_str()); 
#endif//__ANDROID__
 QStringList plugins = QDir(path).entryList(QDir::Files);
    QLibraryPrivate *library = 0;
    for (int j = 0; j < plugins.count(); ++j) {
        QString fileName = QDir::cleanPath(path + QLatin1Char('/') + plugins.at(j));

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_DEBUG, "QT486", "plugins filename: %s", fileName.toStdString().c_str()); 
#endif//__ANDROID__
        if (qt_debug_component()) {
            qDebug() << "QFactoryLoader::QFactoryLoader() looking at" << fileName;
        }
        library = QLibraryPrivate::findOrCreate(QFileInfo(fileName).canonicalFilePath());
        if (!library->isPlugin(&settings)) {
            if (qt_debug_component()) {
                qDebug() << library->errorString;
                qDebug() << "         not a plugin";
            }
            library->release();
            continue;
        }
        QString regkey = QString::fromLatin1("Qt Factory Cache %1.%2/%3:/%4")
                         .arg((QT_VERSION & 0xff0000) >> 16)
                         .arg((QT_VERSION & 0xff00) >> 8)
                         .arg(QLatin1String(d->iid))
                         .arg(fileName);
        QStringList reg, keys;
        reg = settings.value(regkey).toStringList();
        if (reg.count() && library->lastModified == reg[0]) {
            keys = reg;
            keys.removeFirst();
        } else {
            if (!library->loadPlugin()) {
                if (qt_debug_component()) {
                    qDebug() << library->errorString;
                    qDebug() << "           could not load";
                }
                library->release();
                continue;
            }
            QObject *instance = library->instance();
            if (!instance) {
                library->release();
                // ignore plugins that have a valid signature but cannot be loaded.
                continue;
            }
            QFactoryInterface *factory = qobject_cast<QFactoryInterface*>(instance);
            if (instance && factory && instance->qt_metacast(d->iid))
                keys = factory->keys();
            if (keys.isEmpty())
                library->unload();
            reg.clear();
            reg << library->lastModified;
            reg += keys;
            settings.setValue(regkey, reg);
        }
        if (qt_debug_component()) {
            qDebug() << "keys" << keys;
        }

        if (keys.isEmpty()) {
            library->release();
            continue;
        }

        int keysUsed = 0;
        for (int k = 0; k < keys.count(); ++k) {
            // first come first serve, unless the first
            // library was built with a future Qt version,
            // whereas the new one has a Qt version that fits
            // better
            QString key = keys.at(k);
            if (!d->cs)
                key = key.toLower();
            QLibraryPrivate *previous = d->keyMap.value(key);
            if (!previous || (previous->qt_version > QT_VERSION && library->qt_version <= QT_VERSION)) {
                d->keyMap[key] = library;
                d->keyList += keys.at(k);
                keysUsed++;
            }
        }
        if (keysUsed)
            d->libraryList += library;
        else
            library->release();
    }
}

```

(8) 关于翻译

把qt-everywhere-opensource-src-4.8.6\tools\linguist\shared\translator.cpp文件中的方法bool Translator::load替换成如下定义

```cpp
bool Translator::load(const QString &filename, ConversionData &cd, const QString &format)
{
    QTextStream stream(stderr);
    stream << "Translator::load begin" << "\n";
    cd.m_sourceDir = QFileInfo(filename).absoluteDir();
    cd.m_sourceFileName = filename;

    QFile file;
    if (filename.isEmpty() || filename == QLatin1String("-")) {
#ifdef Q_OS_WIN
        // QFile is broken for text files
# ifdef Q_OS_WINCE
        ::_setmode(stdin, _O_BINARY);
# else
        ::_setmode(0, _O_BINARY);
# endif
#endif
        if (!file.open(stdin, QIODevice::ReadOnly)) {
            cd.appendError(QString::fromLatin1("Cannot open stdin!? (%1)")
                .arg(file.errorString()));
            return false;
        }
    } else {
        file.setFileName(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            cd.appendError(QString::fromLatin1("Cannot open %1: %2")
                .arg(filename, file.errorString()));
            return false;
        }
    }

    QString fmt = guessFormat(filename, format);
    QList<Translator::FileFormat> fmts = registeredFileFormats();
    //foreach (const FileFormat &format1, fmts) {
    for (int i = 0; i < fmts.size(); ++i) {
        Translator::FileFormat format1 = fmts[i];
       stream << "Translator::load Try format:" << fmt << " " << format1.extension << "\n";
        if (fmt == format1.extension) {
            stream << fmt << "load\n";
            if (format1.loader)
                return (*format1.loader)(*this, file, cd);
            cd.appendError(QString(QLatin1String("No loader for format %1 found"))
                .arg(fmt));
            return false;
        }
        else
        {
            stream << "guess ?= try: " << fmt << " != " << format1.extension << "\n";
        }
    }

    cd.appendError(QString(QLatin1String("--Unknown format %1 for file %2"))
        .arg(format, filename));
    return false;
}

```

(9) 其它见招拆招即可

## 4 项目编译

(1) 直接编译即可

(2) 生成java native函数的命令

```bash
javah com.benan.intelligenceBTComm.BJavaCppSwap
javac com\benan\intelligenceBTComm\BJavaCppSwap.java
javap -s -p com.benan.intelligenceBTComm.BJavaCppSwap

```

