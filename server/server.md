# server(服务器)

## 1 linux

### 1.1 性能调优

#### 1.1.1 bios性能相关的参数

#### 1.1.2 cpu参数调整

安装cpupower:

```bash
sudo apt install linux-tools-5.13.0-51-generic
```

设置cpu性能模式:

```bash
cpupower -c all frequency-set -g performance
```

设置处理器节能策略

```bash
sudo cpupower -c all set --perf-bias 0 # 分配的值范围从 0 到 15，其中 0 是最优性能，15 是最佳节能
```

查看当前可用策略:

```bash
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
```

查看当前生效策略:

```bash
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

查看当前频率:

```bash
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
```

#### 1.1.3 cpu亲和力

##### (1) 基础命令

查看某个进程在哪个逻辑cpu上运行:

```bash
ps -eo pid,args,psr | grep flowexec
```

查看某个进程的每个线程在哪个逻辑cpu上运行:

```bash
ps -To 'pid,lwp,psr,cmd' -p `ps -ef | grep flowexec | awk 'NR==1{print $2}'`
```

(2) 进程级cpu亲和力

```bash
taskset -a -pc 0,3,7-11 `ps -ef | grep flowexec | awk 'NR==1{print $2}'`
```



(3) 线程级cpu亲和力



# 参考

# [linux系统之cpupower命令](https://www.cnblogs.com/HByang/p/17957747)

# [Linux中CPU亲和性(affinity)](https://www.cnblogs.com/LubinLew/p/cpu_affinity.html)