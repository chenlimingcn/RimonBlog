# Linux下PCI设备驱动开发详解

# 第一章

PCI总线是目前应用最广泛的计算机总线标准，而且是一种兼容性最强，功能最全的计算机总线。 而linux作为一种开源的操作系统，同时也为PCI总线与各种新型设备互联成为可能。尤其被现在的异构计算GPU/FPGA、软硬结合新的方向广泛运用。

## 一、PCI设备和驱动概述

应用程序位于用户空间，驱动程序位于内核空间。linux系统规定，用户空间不可以直接调用内核函数，所以必须经过系统调用，应用程序才可以调用驱动程序的函数。另外应用程序通过系统调用去调用驱动程序的函数，还有一个前提就是驱动程序必须留有接口，这里的接口就是ops函数的操作集合。

![](Linux下PCI设备驱动开发详解.res/pcie_framework.awebp)

驱动最终通过与文件系统相关的系统调用或C库函数（本质也是基于系统调用）被访问，而设备驱动的结构也是为了迎合应用程序，提供给应用程序的SDK API。sys：linux设备驱动模型中的总线、驱动和设备都可以在sysfs文件系统中找到相应的节点。当内核检测到系统中出现新的设备后，内核会在sysfs文件系统中为设备生成一项新的记录。

sysfs是一个虚拟的文件系统，它可以产生一个包括所有系统硬件的层级视图，与提供进程和状态的proc文件系统十分类似。可以让用户空间存取，向用户空间导出内核数据结构以及它的属性。

在linux内核中，设备和驱动是分开注册的，注册1个设备的时候，并不需要驱动已经存在，而1个驱动被注册的时候，也不需要对应的设备被注册。设备和驱动各自涌入内核，而每个设备和驱动涌入内核的时候，都会寻找另外一半^_^。而正是bus_type的match（）成员函数将两者绑定在一起。

简单来说，设备和驱动就是红尘的男女，而bus_type的match()则是牵引红线的月老，它可以识别什么设备与什么驱动，是配对的。一旦成功，xxx_driver的probe就被执行。

## 二、PCI总线描述

![](Linux下PCI设备驱动开发详解.res/pcie_bus.awebp)

PCI是CPU和外围设备通信的高速传输总线。普通PCI总线带宽一般为132MB/s或者264MB/s。

PCI总线体系结构是一种层次式的体系结构，PCI桥设备占据重要的地位，它将父总线与子总线连接在一起，从而使整个系统看起来像一颗倒置的树形结构。

## 三、PCI配置空间

PCI有3种地址空间：PCI配置空间、PCI/IO空间、PCI内存地址空间。

### 1. PCI配置空间

![](Linux下PCI设备驱动开发详解.res/pci_cfg_space.awebp)

deviceID和vendorID寄存器：由pcisig分配，只读，vendorID代表pci设备的厂商，deviceID代表厂商的具体设备； status：设备状态字； command：设备状态字； base address register:决定pci/pcie设备空间映射到系统具体位置的寄存器，IO和memory映射两种。

### 2. PCI/IO空间（PIO）

pio端口的编址是独立于系统的地址空间，其实是一段地址区域，所有外设的地址都映射到这段区域中。不同外设的IO端口不同，访问IO端口需要特殊的IO指令，OUT/IN，out用于write操作，in用于read操作； IO地址空间有限；还有比如ioread32用于读操作，iowrite32写操作。

### 3. PCI内存地址空间（MMIO）

io内存就是把寄存器的地址空间直接映射到系统的地址空间，系统地址空间保留一段内存用于MMIO的映射。

上述的方案只适用于外设和内存进行小数据量的传输时，假如进行大数据量的传输，PIO以字节为单位的传输不用说了，MMIO虽然进行了内存映射，但是范围相对于大量的数据，不值得一提，所以即使采用了MMIO仍然满足不了需要，会让吧CPU大部分时间处理繁琐的映射，极大浪费CPU资源。在这种情况下，引入了DMA，由DMA控制器控制，完成后中断通知CPU，极大解放了CPU。后面的文章会接收DMA。

## 四、PCI设备驱动组成

PCI本质上就是一种总线，具体的PCI设备可以是字符设备、网络设备、USB等，所以PCI设备驱动应该包括两个部分：

1. PCI通用驱动
2. 根据实际需要的设备驱动 根据需求的设备驱动是最终目的，PCI驱动只是手段帮助设备驱动达到最终目的而已。换句话，PCI设备驱动不仅实现PCI驱动还要包括具体需求的设备驱动。

![](Linux下PCI设备驱动开发详解.res/driver_framework.awebp)

PCI驱动注册与注销：

```c
int pci_register_driver(struct pci_driver *driver);
int pci_unregister_driver(struct pci_driver *driver);
```

PCI_driver结构体：

```c
struct pci_driver {
	struct list_head	node;
	const char		*name; //驱动名称
	const struct pci_device_id *id_table;	// 指向设备驱动程序感兴趣的设备ID的一个列表
	int  (*probe)(struct pci_dev *dev, const struct pci_device_id *id);	/* 新设备插入并符合上述列表的调用函数 */
	void (*remove)(struct pci_dev *dev);	/* 新设备拔除调用的函数 */
	int  (*suspend)(struct pci_dev *dev, pm_message_t state);	/* 挂起设备使之处于节能状态的调用函数 */
	int  (*resume)(struct pci_dev *dev);	/* 唤醒挂起的设备的调用函数 */
	void (*shutdown)(struct pci_dev *dev); // 关闭设备的调用函数
	int  (*sriov_configure)(struct pci_dev *dev, int num_vfs); /* On PF，用于多设备模拟调用 */
	int  (*sriov_set_msix_vec_count)(struct pci_dev *vf, int msix_vec_count); /* On PF，用于多设备模拟调用 */
	u32  (*sriov_get_vf_total_msix)(struct pci_dev *pf); /* On PF，用于多设备模拟调用 */
	const struct pci_error_handlers *err_handler;
	const struct attribute_group **groups;
	const struct attribute_group **dev_groups;
	struct device_driver	driver;
	struct pci_dynids	dynids;
	bool driver_managed_dma;
};
```



# 第二章

根据上一章的概念，PCI驱动包括PCI通用的驱动，以及根据实际需要设备本身的驱动。

所谓的编写设备驱动，其实就是编写设备本身驱动，因为linux内核的PCI驱动是内核自带的。

为了更好的学习PCI设备驱动，我们需要明白内核具体做了什么，下面我们研究一下，linux PCI通用的驱动到底做了什么？

注：代码对应的 kernel-3.10.1

## 一、PCI 拓扑架构

### 1.1 PCI的系统拓扑

在分析PCIe初始化枚举流程之前，先描述下PCIe的拓扑结构。 如下图所示：

![](Linux下PCI设备驱动开发详解.res/pcie_topology.awebp)

整个PCIe是一个树形的拓扑：

```asciiarmor
(1) root complex是树的根，它一般实现了一个主桥设备（host bridge），一条内部PCIe总线bus0，以及通过若干PCI bridge扩展出一些root port。host bridge可以完成CPU地址总线到PCI域地址的转换，pci bridge用于系统扩展，没有地址转换功能；

(2) switch是转换设备，目的是扩展PCIe总线。switch中有一个upstream port和若干个downstream port，每个端口相当于一个pci bridge；

(3) PCIe EP device是叶子节点设备，比如PCIe网卡，显卡。NVMe卡等；
```

### 1.2 PCIe的软件框架

PCIe模块涉及到的代码文件很多，在分析PCIe的代码前，先对PCIe涉及的代码梳理。

如下： 这里以arm架构为例，PCIe代码主要分散在3个目录：

```bash
drivers/pci/*
drivers/acpi/pci/*
arch/arm/match-xxx/pci.c
```

将PCIe代码按照如下层次划分：

![](Linux下PCI设备驱动开发详解.res/pcie_source_framework.awebp)

arch PCIe driver：放一些和架构强相关的PCIe的函数实现，对应arch/arm/xxx/pci.c

acpi PCIe driver: acpi扫描时所涉及的PCIe代码，包括host bridge的解析初始化，PCIe bus的创建，ecam的映射等，对应drivers/acpi/pci*.c

PCIe core driver：PCIe的子系统代码，包括PCIe的枚举流程，资源分配流程，中断流程等，主要对应drivers/pci/*.c

PCIe port bus driver：PCIe port的四个service代码的整合，四个service主要是指PCIe dpc/pme/aer/hp，对应drivers/pci/pcie/*

PCIe ep driver：叶子节点的设备驱动，比如显卡、网卡、NVMe；对应drivers/pci/endpoint

## 二、Linux内核实现

PCIe的代码文件这么多，初始化涉及的调用也很多，从哪里开始看呢？

### 1. PCIe初始化流程

```bash
cat System.map |grep pci|grep initcall
```

```bash
ffffffff82ada230 t __initcall_pci_realloc_setup_params0
ffffffff82ada32c t __initcall_pcibus_class_init2
ffffffff82ada330 t __initcall_pci_driver_init2
ffffffff82ada3d4 t __initcall_acpi_pci_init3
ffffffff82ada3e4 t __initcall_register_xen_pci_notifier3
ffffffff82ada3f8 t __initcall_pci_arch_init3
ffffffff82ada544 t __initcall_pci_slot_init4
ffffffff82ada67c t __initcall_pci_subsys_init4
ffffffff82ada6d0 t __initcall_pci_eisa_init_early4s
ffffffff82ada7c0 t __initcall_pcibios_assign_resources5
ffffffff82ada7e4 t __initcall_pci_apply_final_quirks5s
ffffffff82ada7f0 t __initcall_pci_iommu_initrootfs
ffffffff82ada9e8 t __initcall_pwm_lpss_driver_pci_init6
ffffffff82ada9f0 t __initcall_pci_proc_init6
ffffffff82ada9f4 t __initcall_pcie_portdrv_init6
ffffffff82ada9f8 t __initcall_pci_hotplug_init6
ffffffff82adaa00 t __initcall_pci_ep_cfs_init6
ffffffff82adaa04 t __initcall_pci_epc_init6
ffffffff82adaa08 t __initcall_pci_epf_init6
ffffffff82adaa0c t __initcall_dw_plat_pcie_driver_init6
ffffffff82adaa78 t __initcall_virtio_pci_driver_init6
ffffffff82adaab4 t __initcall_serial_pci_driver_init6
ffffffff82adab54 t __initcall_sis_pci_driver_init6
ffffffff82adab58 t __initcall_ata_generic_pci_driver_init6
ffffffff82adab80 t __initcall_vfio_pci_init6
ffffffff82adab90 t __initcall_ehci_pci_init6
ffffffff82adab9c t __initcall_ohci_pci_init6
ffffffff82adabac t __initcall_xhci_pci_init6
ffffffff82adad2c t __initcall_pci_resource_alignment_sysfs_init7
ffffffff82adad30 t __initcall_pci_sysfs_init7
ffffffff82adad90 t __initcall_pci_mmcfg_late_insert_resources7
```

pcibus_class_init：注册pci_bus_class，完成后创建了/sys/class/pci_bus目录；

pci_driver_init：注册pci_bus_type，完成后创建了/sys/bus/pci目录；

acpi_pci_init：注册acpi_pci_bus，并设置电源管理相应的操作；

acpi_init()：acpi启动所涉及到的初始化流程，PCIe基于acpi的启动流程从该接口进入；

### 2. PCIe枚举流程

我们先看内核代码：

```c
struct pci_bus *pci_acpi_scan_root(struct acpi_pci_root *root) // 内核代码可能因版本不同而不同
{
    struct acpi_device *device = root->device;
    struct pci_root_info *info = NULL;
    int domain = root->segment;
    int busnum = root->secondary.start;
    ...
    if (!setup_mcfg_map(info, domain, (u8)root->secondary.start, 
        (u8)root->secondary.end, root->mcfg_addr)) 
        bus = pci_create_root_bus(NULL，busnum, &pci_root_ops, sd, &resources);
  
    ...
}
```

这个函数主要是建立ecam映射，将ecam的空间进行映射，这样cpu就可以通过内存访问到相应设备的配置空间；

pci_create_root_bus()：用来创建该{segment: busnr}下的根总线。传递的参数：

NULL：host bridge设备的parent节点；

busnum：总线号；

pci_root_ops：配置空间的操作接口；

resource：私有数据，用来保存总线号，IO空间，mem空间等信息；

以下依次函数调用是：

```c
pci_scan_child_bus()
    +-> pci_scan_child_bus_extend()
        +-> for dev range(0, 256)
            pci_scan_slot()
                +-> pci_scan_single_device()
                    +-> pci_scan_device()
                        +-> pci_bus_read_dev_vendor_id()
                        +-> pci_alloc_dev()
                        +-> pci_setip_device()
                    +-> pci_add_device()
            
                +-> for each pci bridge
                    +-> pci_scan_bridge_extend()
```

总的来说，枚举流程分为3步：

```markdown
1.  发现主桥设备和根总线
2.  发现主桥设备下的所有PCI设备
3.  如果主桥下面的是PCI bridge，那么再次遍历这个PCI bridge桥下的所有PCI设备，依次递归，直到将当前PCI总线树遍历完毕，返回host bridge的subordinate总线号。

```

### 3. PCIe的资源分配

PCIe设备枚举完成后，PCI总线号已经分配，PCIe ecam的映射、PCIe设备信息、bar的个数以及大小等已经ready，但是此时并没有给PCI device的bar、IO、mem分配资源。

这时就需要走到PCIe的资源分配流程，整个资源分配的过程就是从系统的总资源里给每个PCI device的bar分配资源。给每个PCI桥的base、limit的寄存器分配资源。

PCIe的资源分配流程整体比较复杂，主要介绍下总体的流程，对关键的函数再做展开。

PCIe资源分配的入口在pci_acpi_scan_root()->pci_bus_assign_resources()，详细代码如下：

```c
void __ref __pci_bus_assign_resources(const struct pci_bus *bus,
				      struct list_head *realloc_head,
				      struct list_head *fail_head)
{
	struct pci_bus *b;
	struct pci_dev *dev;

	pbus_assign_resources_sorted(bus, realloc_head, fail_head);

	list_for_each_entry(dev, &bus->devices, bus_list) {
		b = dev->subordinate;
		if (!b)
			continue;

		__pci_bus_assign_resources(b, realloc_head, fail_head);

		switch (dev->class >> 8) {
		case PCI_CLASS_BRIDGE_PCI:
			if (!pci_is_enabled(dev))
				pci_setup_bridge(b);
			break;

		case PCI_CLASS_BRIDGE_CARDBUS:
			pci_setup_cardbus(b);
			break;

		default:
			dev_info(&dev->dev, "not setting up bridge for bus "
				 "%04x:%02x\n", pci_domain_nr(b), b->number);
			break;
		}
	}
}

```

其中pbus_assign_resources_sorted，这个函数先对当前总线下设备请求的资源进行排序。

总而言之，PCIe的资源枚举过程可以概括为如下：

```markdown
1. 获取上游PCI桥设备所管理的系统资源范围；
2. 使用DFS对所有的pci ep device进行bar资源的分配；
3. 使用DFS对当前PCI桥设备的base limit的值，并对这些寄存器更新；
```

# 第三章

在进行PCIe实际软硬件开发之前，我们要先非常清晰几个概念，这些概念可以让我们高屋建瓴，了解整个PCIe软硬异构系统如何运行的，以及PCIe驱动和PCIe device处在整个系统的什么位置，非常关键。

## 一、PCIe软硬异构系统的概念

### 1. 应用程序、库、内核以及驱动程序

- 应用程序：应用程序调用一系列函数库，通过对文件的操作完成一系列的功能；

  应用程序以文件的形式访问各种硬件设备（linux特定的抽象方式，把所有的硬件访问抽象成对文件的读写、设置）。

- 库：部分库函数不需要内核的支持，由库函数内部通过代码实现，直接完成功能；

  部分库函数，涉及到硬件操作或者内核的支持，由内核完成对应的功能，称之为系统调用。

- 内核：内核负处理系统调用，根据设备文件的类型，主设备号、从设备号、调用设备驱动程序。

- 驱动程序：驱动负责直接与硬件通信；

### 2. 设备类型

当前linux把所有的硬件设备分为3大类：字符设备、块设备、网络设备。

- 字符设备：char型设备是个能够像字节流一样被访问的设备；对字符设备驱动程序至少实现open、close、read和write系统调用；
- 块设备：传输固定大小的数据（512或者1k）来访问设备；
- 网络设备：任何网络事务都经过一个网咯接口形成，即一个能够和其他主机交换数据的设备。访问网络接口仍然是给他们分配一个唯一的名字（如eth0）。内核和网络设备驱动程序的通信，完全不同于内核与字符、块设备的通信，内核调用一套与数据传输相关的函数（socket），而不是write、read。

### 3. 设备文件、主设备号、从设备号

有了设备类型的划分，那么应用程序应该怎么访问具体的硬件设备？

答案：姓名，在linux中就是设备的文件名；

那么重名怎么办呢？

答案：身份证号，在linux中就是设备号（主、从）；

设备文件：

在linux中，有一个约定成俗的说法，“一切皆文件”，应用程序使用设备文件节点对应的设备，linux下的各种硬件以文件的形式放在/dev目录下，linux把对硬件的操作全部抽象成对文件的操作（open、read、write、close）等；

在设备管理中，除了设备类型外，内核还需要一对主从设备号，才能唯一标识一个设备，类似于ID；

主设备号：用于标识驱动程序，相同的主设备号使用相同的驱动程序；

次设备号：用于标识同一驱动程序的不同硬件；

### 4. 驱动程序、应用程序

应用程序以main()开始，驱动程序没有main()，它以一个模块初始化函数作为入口；

应用程序从头到尾执行一个任务，驱动程序完成初始化之后不再运行，除非等待系统调用；

应用程序可以使用glibc等标准的c函数库，驱动程序不能使用标准c库；

### 5. 用户态、内核态

驱动程序是内核的一部分，工作在内核态；应用程序工作在用户态；

数据访问问题：

```
无法通过指针直接将二者的数据地址进行传递；系统提供了一系列函数完成数据空间的转换：

get_user、put_user、copy_from_user、copy_to_user
```

### 6. linux驱动程序功能

- 对设备初始化和释放资源；
- 把数据从内核传送到硬件和从硬件读取数据；
- 读取应用程序传递给设备文件的数据和回送给应用程序请求的数据；
- 检测和处理设备出现的错误；
- 用于区分具体设备的实例；

## 二、总线、设备和驱动

设备驱动离不开3部曲：总线、设备和驱动。

linux设备驱动模型，由总线（bus）、设备（device）、驱动（driver）三部分组成；总线是处理器和设备之间的通道，在设备模型中，所有的设备都是通过总线相连；总线作为linux设备驱动模型的核心架构，系统中设备都和驱动挂接在相应的总线上了，来完成各自的工作；

### 1. 总线、设备、驱动概述

- 总线

  分为物理层面总线和软件层面总线：

  物理层面的总线，需要共同遵循一样的时序，不同总线硬件的通信也是不同的，如iic总线、usb总线、PCIe总线； 软件层面的总线，负责管理设备和驱动。设备和驱动要让系统感知自己，需要向总线注册自己，并明确自己所属的总线；总线上的设备和驱动匹配，设备提自己对驱动的条件（名字），驱动告知总线自己支持的设备条件（ids），设备注册的时候，总线就会遍历它上面的驱动，找到最适合这个设备的驱动；同样，驱动注册的时候，总线也会遍历在上面的设备，找到其支持的设备，即这是总线match做的事情； 总线匹配后，不知道设备是否正常，这个时候驱动就需要探测一下，即probe干的事情。

- 设备

  代表真实存在的物理器件，每个器件都有不同的通信时序，iic、usb、PCIe这些代表了不同的时序，这样就和总线挂钩了。

- 驱动

  驱动代表操作设备的方式和流程，以应用来说，在程序open设备时，接着read这个这个设备，驱动就是实现应用访问的具体过程。驱动就是一个通信官和翻译官，一是通过对soc的控制寄存器编程，按总线要求输出相应时序的命令，与设备交互，一是对得到数据进行处理，给上层提供特定格式数据。

### 2. 结合代码和实际分析三者关系

![](Linux下PCI设备驱动开发详解.res/pcie_model.awebp)

系统启动后，会调用buses_init()函数创建/sys/bus文件目录，这部分系统在开机是已经帮我们准备好了，接下去就是通过总线注册函数bus_register()进行总线注册，注册完成后，在/sys/bus目录下生成device文件夹和driver文件夹，最后分别通过device_register()以及driver_register()函数注册对应的设备和驱动。



#### 2.1 总线初始化

系统启动后，会调用buses_init()函数创建/sys/bus这个文件目录，这部分操作在系统开机的时候帮我们准备好了。

#### 2.2 总线注册

系统中不一定有你需要的总线，linux提供了一些函数来添加或注销总线，大部分情况下编写linux驱动模块时，内核已经为我们准备了大部分总线驱动，正常情况下我们一般不会去注册一个新的总线。

总线注册和注销的函数原型如下：

```c
int bus_register(struct bus_type *bus)
```

当配对成功（match）后，内核就会调用指定驱动中的probe函数进行初始化。

以注册xbus总线为例：

```rust
创建/sys/bus/xbus目录，目录名xbus为我们新注册的总线名；
创建/sys/bus/xbus/devices目录，并创建属性文件；
创建/sys/bus/xbus/drivers目录，并创建属性文件；
初始化priv->klist_device链表头；
初始化priv->klist_driver链表头
```

下图是系统中注册的设备总线

![](Linux下PCI设备驱动开发详解.res/pcie_bus_dir.awebp)

#### 2.3 设备注册

添加设备，关联硬件相关代码：

```c
int device_register(struct device *dev)
```

```rust
创建/sys/bus/xbus/devices/yyy目录；
加入bus->priv->devices_kset链表；
加入bus->priv->klist_devices链表；
加入bus->priv->klist_drivers，执行bus->match()寻找合适的drv；
dev关联driv，执行drv->probe()
```

下面就是实际的PCI设备：

![](Linux下PCI设备驱动开发详解.res/pcie_pci_dir.awebp)

![](Linux下PCI设备驱动开发详解.res/pcie_pci_tree.awebp)

#### 2.4 驱动注册

添加驱动，关联软件相关代码：

```c
int driver_register(struct device_driver *drv)
```

```rust
创建/sys/bus/xbus/driver/zzz目录；
加入bus->priv->driver_kset目录；
加入bus->priv->klist_driver链表；
遍历bus->priv->klist_device链表，执行bus->match()寻找合适的dev；
driv关联dev，执行drv->probe();
```

下面就是实际的PCI驱动：

![](Linux下PCI设备驱动开发详解.res/pcie_driver.awebp)

## 三、总结

linux设备驱动模型，由总线（bus）、设备（device）、驱动（driver）三部分组成；总线是处理器与设备之间的通道，在设备模型中，所有的设备都是通过总线相连；总线作为linux设备驱动模型的核心架构，系统中的设备都和驱动挂接在相应的总线上，来完成各自的工作。



作者：北京不北
链接：https://juejin.cn/post/7312353213347741708
来源：稀土掘金
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。

# 第四章

一般来说，用模块方式编写PCI设备驱动，通常至少要实现以下几个部分：初始化设备模块、设备打开模块、数据读写模块、中断处理模块、设备释放模块、设备卸载模块。

下面我们直接给出一个demo实例：

```c
...

/* 指明该驱动程序适用于哪一些PCI设备 */
static struct pci_device_id demo = {
    PCI_VENDOR_ID_DEMO,
    PCI_DEVICE_ID_DEMO,
    PCI_ANY_ID,
    0,
    0,
    DEMO
};

/* 对特定PCI设备进行描述的数据结构 */
struct demo_card {
    unsigned int magic;
    /* 使用链表保存所有同类的PCI设备 */
    struct demo_card *next;
    ...
};

/* 中断处理模块 */
static void demo_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    /* ... */
};

/* 设备文件操作接口 */
static struct file_operations demo_fops = {
    owner: THIS_MODULE, /* demo_fops 所属的设备模块 */
    read: demo_read, /* 读设备操作 */
    write: demo_write, /* 写设备操作 */
    ioctl: demo_ioctl, /* 控制设备操作 */
    mmap：demo_mmap, /* 内存重映射操作 */
    open：demo_open, /* 打开设备操作 */
    release: demo_release /* 释放设备操作 */
    /* ... */
};

/* 设备模块信息 */
static struct pci_driver demo_pci_driver = {
    name: demo_MODULE_NAME, /* 设备模块名称 */
    id_table：demo_pci_tbl, /* 能够驱动的设备列表 */
    probe：demo_probe; /* 查找并初始化设备 */
    remove：demo_remove /* 卸载设备模块 */
    /* ... */
};

static int __init demo_init_module (void)
{
    /* ... */
};

static void __exit demo_cleanup_module(void)
{
    pci_unregister_driver(&demo_pci_driver);
}

/* 加载驱动程序模块入口 */
module_init(demo_init_nodule);

/* 卸载驱动程序模块入口 */
module_exit(demo_cleanup_module);
```

上面代码给了一个典型的PCI设备驱动程序的框架，是一种相对固定的模式。需要注意的是，加载和卸载模块相关的函数或数据结构都要在前面加上__init、__exit等标志符，以使同普通函数区分开来。构造出一个这样的框架后，接下来就是如何完成框架的功能单元了。

## 一、初始化设备模块

在linux系统下，想要完成对一个PCI设备的初始化，需要完成以下的工作：

- 检查PCI总线是否被linux内核支持；
- 检查设备是否插在总线插槽上，如果在的话则保存它所占用的插槽的位置等信息；
- 读出配置头中的信息给驱动程序使用；

当linux内核启动并完成对所有PCI设备扫描、登录和分配资源等初始化操作的同时，会建立起系统中所有PCI设备的拓扑结构，此后当PCI驱动程序需要对设备进行初始化时，一般都会调用如下代码：

```c
static int __init demo_init_module(void)
{
    /* 检查系统是否支持PCI总线 */
    if (!pci_present())
        return -ENODEV;
    
    /* 注册硬件驱动程序 */
    if (!pci_register_driver(&demo_pci_driver)) {
        pci_unregister_driver(&demo_pci_driver);
            return -ENODEV;
    }

    /* ... */
    return 0;
}
```

驱动程序首先调用pci_present()检查PCI总线是否被linux内核支持，如果系统支持PCI总线结构，这个函数的返回值为0，如果驱动程序在调用这个函数时得到一个非0的返回值，那么驱动程序就必须得中止自己的任务。调用pci_register_driver()函数来注册PCI设备的驱动程序，此时需要提供一个“demo_pci_driver”结构，在该结构中给出的probe探测例程负责完成对硬件的检测工作。

## 二、probe探测硬件设备

具体的探测设备流程包括，启动PCI设备、设备DMA标识、内核空间动态申请内存、获取PCI信息、设置成总线DMA模式、申请IO资源（具体的流程可以根据需要修改）。

```c
static int __init demo_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id)
{
    struct demo_card *card;
    /* 启动PCI设备 */
    if (pci_enable_device(pci_dev))
        return -EIO;

    /* 设备DMA标识 */
    if (pci_set_dma_mask(pci_dev, DEMO_DMA_MASK))
        return -ENODEV;

    /* 在内核空间中申请内存，存放device的信息 */
    if ((card = kmalloc(sizeof(struct demo_card), GFP_KERNEL) == NULL)) {
        printk(KERN_ERR "pci_demo: out of memory!\n");
        return -ENOMEM;
    }

    memset(card, 0, sizeof(*card));

    /* 读取PCI配置信息 */
    card->iobase = pci_resource_start(pci_dev, 1);
    card->pci_dev = pci_dev;
    card->pci_id = pci_id->device;
    card->irq = pci_dev->irq;
    card->next = devs;
    card->magic = DEMO_CARD_MAGIC;

    /* 设置成总线主DMA模式 */
    pci_set_master(pci_dev);

    /* 申请IO资源 */
    request_region(card->iobase, 64, card_names[pci_id->driver_data]);

    return 0;

}

```

## 三、打开设备模块

在这个模块里面主要实现申请中断、检查读写模式以及申请对设备的控制权等。在申请控制权的时候，非阻塞模式遇忙返回，否则进程主动接受调度，进入睡眠状态，等待其他进程释放对设备的控制权。

```c
static int demo_open(struct inode *inode, struct file *file)
{
    /* 申请中断，注册中断处理程序 */
    request_irq(card->irq, &demo_interrupt, SA_SHIRQ,
        card_names[pci_id->driver_data], card) {
            /* 检查读写模式 */
            if (file->f_mode & FMODE_READ) {
                /*...*/
            }
            if (file->f_mode & FMODE_WRITE) {
                /*...*/
            }
            /* 申请对设备的控制权 */
            down(&card->open_sem);
            while(card->open_mode & file->f_mode) {
                if (file->f_flags & O_NONBLOCK) {
                    /* nonblock 模式， 返回—EBUSY */
                    up(&card->opensem);
                    return -EBUSY;
                } else {
                    /*等待调度，获得控制权 */
                    card->open_mode |= f_mode & (FMODE_READ | FMODE_WRIET);

                    /* 设备打开计数增1 */
                    MOD_INC_USE_COUNT;
                    /* ... */
                }
            }
        }
}
```

## 四、数据读写和控制信息模块

PCI设备驱动程序可以通过demo_fops结构中的函数demo_ioctl()，向应用程序提供对硬件进行控制的接口。

例如，通过它可以从IO寄存器里读取一个数据，并传送到用户空间里；

```c
static int demo_ioctl(struct inode *inode, struct file *file, 
            unsigned int cmd, unsigned long arg)
{
    /* ... */
    switch (cmd) {
        case DEMO_RDATA;
        /* 从IO端口读取4字节的数据 */
        val = inl(card->iobase + 0x10);
    
    /* 将读取的数据传输到用户空间 */
    return 0；

    /* ... */
    }
}
```

实际上，在demo_fops里还可以实现诸如demo_read()、demo_mmap()等操作，linux内核源代码中的driver目录里提供了许多设备驱动程序的源代码，那里有很多类似的例子。在对资源的访问方式上，除了IO指令外，还有对外设IO内存的访问。对这个内存的操作，一方面通过把IO内存重新映射后作为普通内存（mmap）进行操作，另外一方面通过总线主DMA（bus master DMA）的方式让设备把数据通过DMA传送到系统内存中。

## 五、中断处理模块

pc的中断资源一般比较有限（irq，不过现在很多采用msi/msix），只有中断号0-15，所以大部分都是以共享的形式申请中断号的。当中断发生时，中断处理程序首先对中断进行识别，然后再做进一步处理。

```c
static void demo_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    struct demo_card *card = (struct demo_card *)dev_id;
    u32 status;
 
    spin_lock(&card->lock);
 
    /* 识别中断 */
    status = inl(card->iobase + GLOB_STA);
    if(!(status & INT_MASK))
    {
        spin_unlock(&card->lock);
        return; /* not for us */
    }
 
    /* 告诉设备已经收到中断 */
    outl(status & INT_MASK, card->iobase + GLOB_STA);
    spin_unlock(&card->lock);
   
    /* 其它进一步的处理，如更新 DMA 缓冲区指针等 */
}
```

## 六、释放设备模块

释放设备模块主要负责释放对设备的控制权，释放占用的内存和中断等，所做的事情正好和打开设备模块相反；

```c
static int demo_release(struct inode *inode, struct file *file)
{
    /* ... */

    /*释放对设备的控制权*/
    card->open_mode &= (FMODE_READ|FMODE_WRITE);
    
    /* 唤醒其他等待获取控制权的进程 */
    wake_up(&card->open_wait);
    up(&card->open_sem);

    /*释放中断*/
    free_irq(card->irq, card);
    /* 设备打开计数增1 */
    MOD_DEC_USE_COUNT;

    /*...*/
}
```

## 七、卸载设备模块

卸载设备模块与初始化设备模块时对应的，实现起来相对简单，主要调用函数pci_unregister_driver()从linux内核中注销设备驱动程序；

```c
static void __exit demo_cleanup_module(void)
{
    pci_unregister_driver(&demo_pci_driver);
}
```

## 八、总结

本文从一个demo例子详细介绍了，一个驱动的框架主要包括：初始化设备模块、设备打开模块、数据读写模块、中断处理模块、设备释放模块、设备卸载模块。

# 第五章

本章及其以后的几章，我们将从用户态软件、内核态驱动、FPGA逻辑介绍一个通过PCI Express总线实现CPU和FPGA数据通信的简单框架。

这个框架就是开源界非常有名的RIFFA（reuseable integration framework for FPGA accelerators）,它是一个FPGA加速器的一种可重用性集成框架，是一个第三方开源PCIe框架。

该框架要求具备一个支持PCIe的工作站和一个带有PCIe连接器的FPGA板卡。RIFFA支持windows、linux，altera和xilinx，可以通过c/c++、python、matlab、java驱动来实现数据的发送和接收。驱动程序可以在linux或windows上运行，每一个系统最多支持5个FPGA device。

在用户端有独立的发送和接收端口，用户只需要编写几行简单代码即可实现和FPGA IP内核通信。

riffa使用直接存储器访问（DMA）传输和中断信号传输数据。这实现了PCIe链路上的高带宽，运行速率可以达到PCIe链路饱和点。

开源地址：[KastnerRG/riffa](https://github.com/KastnerRG/riffa)

## 一、riffa体系结构。

![](Linux下PCI设备驱动开发详解.res/riffa_struct.awebp)

在硬件方面，简化了接口，以便通过FIFO简便的将数据取出或存入。数据的传输由riffa的rx和tx DMA engine模块用分散聚合方法来实现。rx engine模块收集上位机来的有效数据，收集完成发给channel模块，tx engine收集channel模块传送来的数据，打包发给PCI express端点。

在软件方面，PC接收FPGA板卡数据是用户应用程序调用库函数fpga_recv，然后由FPGA端启动。用户应用程序线程进入内核驱动程序，然后开始接收上游FPGA的读请求，将数据分包发送，如果没收到请求，将会等待它达到。

启动发送函数后，服务器将建立一个散列收集元素的列表，将数据存储地址和长度等信息放入其中，将其写入共享缓冲区。用户应用程序将缓冲区地址和数据长度等信息发送给FPGA。FPGA读取散射收集数据，然后发出相应地址的数据写入请求,如果散列收集元素列表的地址有多个，FPGA将通过中断发出多次请求。

TX搬移的数据全部写入缓存区后，驱动程序读取FPGA写入的字节数，确认是否与发送数据长度一致。这样就完成了传输。其过程如下图所示：

![](Linux下PCI设备驱动开发详解.res/riffa_timeline5.awebp)

PC机发送数据到FPGA板卡过程与PC机接收FPGA板卡数据过程相似，下图说明了该流程。 刚开始也是用户应用程序调用库函数fpga_send，传输线程进入内核驱动程序，然后FPGA启动传输。

启动fpga_send，服务器将申请一些空间，将要发送的数据写入其中，然后建立一个分散收集列表，将存储数据的地址和长度放入其中，并将分散收集列表的地址和要发生的数据长度等信息发给FPGA。FPGA 收到列表地址后，读取该列表的信息，然后发出相应地址和长度的读请求，然后将数据存储，最后一起发给FPGA板卡。

![](Linux下PCI设备驱动开发详解.res/riffa_timeline6.awebp)

驱动程序首先调用pci_present()检查PCI总线是否被linux内核支持，如果系统支持PCI总线结构，这个函数的返回值为0，如果驱动程序在调用这个函数时得到一个非0的返回值，那么驱动程序就必须得中止自己的任务。调用pci_register_driver()函数来注册PCI设备的驱动程序，此时需要提供一个“demo_pci_driver”结构，在该结构中给出的probe探测例程负责完成对硬件的检测工作。

下面将从user logic -> PCIe IP -> 驱动层 -> library-> 用户态C++/C按照实例一一进行分析，包括理论基础、实际操作、源代码分析。

首先我们先从FPGA xilinx integrated block for PCI express分析，因为这个有承上启下的作用。

## 二、PCIe hard IP分析

### 1. PCIE IP 核配置

AXI总线时钟选择62.5M，AXI总线接口位宽设置为64bit。

![](Linux下PCI设备驱动开发详解.res/verilog1.awebp)

在IDs界面是PCIe设备的相关信息，主机在上电时BIOS系统中识别到的PCIe设备，就是通过这些ID号来进行识别的。

在本实验中，关于ID的设定全部保持为默认值即可，若用户对ID进行了更改，可能导致计算机在启动时不能正确识别设备从而导致蓝屏死机。

Vendor ID是厂商ID，本实验中的厂商ID代值的就是Xilinx；

Device ID代表了PCIe设备，其中7指的是Xilinx 7 系列FPGA，02指的是使用的PCIe 2.0的协议，1指的是含有一个PCIe的传输lane；

Base class Menu指的是PCIe设备的种类，常见的有声卡、显卡、网卡等，各种不同种类的设备都有其对应的驱动，若驱动与其PCIe的种类不对应，就会导致系统的内存访问错误，从而导致蓝屏。

![](Linux下PCI设备驱动开发详解.res/verilog2.awebp)

在PCIe的bars配置界面对PCIe的bar进行设置，BAR空间对应的就是在内存中开辟一段空间用于存放PCIe设备的信息。只使用到一个bar0一个bar且将其内存空间的大小设置为1k。

![](Linux下PCI设备驱动开发详解.res/verilog3.awebp)

IP核的负载选择最大负载长度为512字节，勾选对数据进行缓冲。

![](Linux下PCI设备驱动开发详解.res/verilog4.awebp)

在中断配置界面，取消勾选传统类型的中断，只选择消息类型的中断。

![](Linux下PCI设备驱动开发详解.res/verilog5.awebp)

在share logic界面取消勾选包含其他逻辑，这样在PCIe的IP核中就包含了全部功能。

![](Linux下PCI设备驱动开发详解.res/verilog6.awebp)

在IP核接口参数配置界面，只选择其中用于配置核控制的参数，这是由于riffa框架的特性所提供的。

![](Linux下PCI设备驱动开发详解.res/verilog7.awebp)

下面代码是实际的top level:

```verilog
PCIeGen1x4If64 PCIeGen1x4If64_i
        (//---------------------------------------------------------------------
         // PCI Express (pci_exp) Interface                                     
         //---------------------------------------------------------------------
         // Tx
         .pci_exp_txn                               ( PCI_EXP_TXN ),
         .pci_exp_txp                               ( PCI_EXP_TXP ),        
         // Rx
         .pci_exp_rxn                               ( PCI_EXP_RXN ),
         .pci_exp_rxp                               ( PCI_EXP_RXP ),        
         //---------------------------------------------------------------------
         // AXI-S Interface                                                     
         //---------------------------------------------------------------------
         // Common
         .user_clk_out                              ( user_clk ),
         .user_reset_out                            ( user_reset ),
         .user_lnk_up                               ( user_lnk_up ),
         .user_app_rdy                              ( user_app_rdy ),
         // TX
         .s_axis_tx_tready                          ( s_axis_tx_tready ),
         .s_axis_tx_tdata                           ( s_axis_tx_tdata ),
         .s_axis_tx_tkeep                           ( s_axis_tx_tkeep ),
         .s_axis_tx_tuser                           ( s_axis_tx_tuser ),
         .s_axis_tx_tlast                           ( s_axis_tx_tlast ),
         .s_axis_tx_tvalid                          ( s_axis_tx_tvalid ),
         // Rx
         .m_axis_rx_tdata                           ( m_axis_rx_tdata ),
         .m_axis_rx_tkeep                           ( m_axis_rx_tkeep ),
         .m_axis_rx_tlast                           ( m_axis_rx_tlast ),
         .m_axis_rx_tvalid                          ( m_axis_rx_tvalid ),
         .m_axis_rx_tready                          ( m_axis_rx_tready ),
         .m_axis_rx_tuser                           ( m_axis_rx_tuser ),
         .tx_cfg_gnt                                ( tx_cfg_gnt ),
         .rx_np_ok                                  ( rx_np_ok ),
         .rx_np_req                                 ( rx_np_req ),
         .cfg_trn_pending                           ( cfg_trn_pending ),
         .cfg_pm_halt_aspm_l0s                      ( cfg_pm_halt_aspm_l0s ),
         .cfg_pm_halt_aspm_l1                       ( cfg_pm_halt_aspm_l1 ),
         .cfg_pm_force_state_en                     ( cfg_pm_force_state_en ),
         .cfg_pm_force_state                        ( cfg_pm_force_state ),
         .cfg_dsn                                   ( cfg_dsn ),
         .cfg_turnoff_ok                            ( cfg_turnoff_ok ),
         .cfg_pm_wake                               ( cfg_pm_wake ),
         .cfg_pm_send_pme_to                        ( 1'b0 ),
         .cfg_ds_bus_number                         ( 8'b0 ),
         .cfg_ds_device_number                      ( 5'b0 ),
         .cfg_ds_function_number                    ( 3'b0 ),
         //---------------------------------------------------------------------
         // Flow Control Interface                                              
         //---------------------------------------------------------------------
         .fc_cpld                                   ( fc_cpld ),
         .fc_cplh                                   ( fc_cplh ),
         .fc_npd                                    ( fc_npd ),
         .fc_nph                                    ( fc_nph ),
         .fc_pd                                     ( fc_pd ),
         .fc_ph                                     ( fc_ph ),
         .fc_sel                                    ( fc_sel ),
         //---------------------------------------------------------------------
         // Configuration (CFG) Interface                                       
         //---------------------------------------------------------------------
         .cfg_device_number                         ( cfg_device_number ),
         .cfg_dcommand2                             ( cfg_dcommand2 ),
         .cfg_pmcsr_pme_status                      ( cfg_pmcsr_pme_status ),
         .cfg_status                                ( cfg_status ),
         .cfg_to_turnoff                            ( cfg_to_turnoff ),
         .cfg_received_func_lvl_rst                 ( cfg_received_func_lvl_rst ),
         .cfg_dcommand                              ( cfg_dcommand ),
         .cfg_bus_number                            ( cfg_bus_number ),
         .cfg_function_number                       ( cfg_function_number ),
         .cfg_command                               ( cfg_command ),
         .cfg_dstatus                               ( cfg_dstatus ),
         .cfg_lstatus                               ( cfg_lstatus ),
         .cfg_pcie_link_state                       ( cfg_pcie_link_state ),
         .cfg_lcommand                              ( cfg_lcommand ),
         .cfg_pmcsr_pme_en                          ( cfg_pmcsr_pme_en ),
         .cfg_pmcsr_powerstate                      ( cfg_pmcsr_powerstate ),
         //------------------------------------------------//
         // EP Only                                        //
         //------------------------------------------------//
         .cfg_interrupt                             ( cfg_interrupt ),
         .cfg_interrupt_rdy                         ( cfg_interrupt_rdy ),
         .cfg_interrupt_assert                      ( cfg_interrupt_assert ),
         .cfg_interrupt_di                          ( cfg_interrupt_di ),
         .cfg_interrupt_do                          ( cfg_interrupt_do ),
         .cfg_interrupt_mmenable                    ( cfg_interrupt_mmenable ),
         .cfg_interrupt_msienable                   ( cfg_interrupt_msienable ),
         .cfg_interrupt_msixenable                  ( cfg_interrupt_msixenable ),
         .cfg_interrupt_msixfm                      ( cfg_interrupt_msixfm ),
         .cfg_interrupt_stat                        ( cfg_interrupt_stat ),
         .cfg_pciecap_interrupt_msgnum              ( cfg_pciecap_interrupt_msgnum ),
         //---------------------------------------------------------------------
         // System  (SYS) Interface                                             
         //---------------------------------------------------------------------
         .sys_clk                                    ( pcie_refclk ),
         .sys_rst_n                                  ( pcie_reset_n )
         );
```

从顶层代码接口可以看出来，TX/RX差分信号、AXIS数据common接口信号、tx/rx数据面信号、FC流控信号、configuration（CFG）interface、EP中断信号、系统时钟/复位信号。

详细使用参考xilinx PCIe spec官方文档。

PCIe IP位于整个设计架构的这个位置：

![](Linux下PCI设备驱动开发详解.res/pcie_xilinx.awebp)

### 2. tx_engine和rx_engine

下面我们分析一下tx_engine和rx_engine这两个核心模块，这两个模块负责转换axis data和tlp data。

我们先看一下top level的源代码：

```verilog
riffa_wrapper_ac701
        #(/*AUTOINSTPARAM*/
          // Parameters
          .C_LOG_NUM_TAGS               (C_LOG_NUM_TAGS),
          .C_NUM_CHNL                   (C_NUM_CHNL),
          .C_PCI_DATA_WIDTH             (C_PCI_DATA_WIDTH),
          .C_MAX_PAYLOAD_BYTES          (C_MAX_PAYLOAD_BYTES))
    riffa
        (
         // Outputs
         .CFG_INTERRUPT                 (cfg_interrupt),
         .M_AXIS_RX_TREADY              (m_axis_rx_tready),
         .S_AXIS_TX_TDATA               (s_axis_tx_tdata[C_PCI_DATA_WIDTH-1:0]),
         .S_AXIS_TX_TKEEP               (s_axis_tx_tkeep[(C_PCI_DATA_WIDTH/8)-1:0]),
         .S_AXIS_TX_TLAST               (s_axis_tx_tlast),
         .S_AXIS_TX_TVALID              (s_axis_tx_tvalid),
         .S_AXIS_TX_TUSER               (s_axis_tx_tuser[`SIG_XIL_TX_TUSER_W-1:0]),
         .FC_SEL                        (fc_sel[`SIG_FC_SEL_W-1:0]),
         .RST_OUT                       (rst_out),
         .CHNL_RX                       (chnl_rx[C_NUM_CHNL-1:0]),
         .CHNL_RX_LAST                  (chnl_rx_last[C_NUM_CHNL-1:0]),
         .CHNL_RX_LEN                   (chnl_rx_len[(C_NUM_CHNL*`SIG_CHNL_LENGTH_W)-1:0]),
         .CHNL_RX_OFF                   (chnl_rx_off[(C_NUM_CHNL*`SIG_CHNL_OFFSET_W)-1:0]),
         .CHNL_RX_DATA                  (chnl_rx_data[(C_NUM_CHNL*C_PCI_DATA_WIDTH)-1:0]),
         .CHNL_RX_DATA_VALID            (chnl_rx_data_valid[C_NUM_CHNL-1:0]),
         .CHNL_TX_ACK                   (chnl_tx_ack[C_NUM_CHNL-1:0]),
         .CHNL_TX_DATA_REN              (chnl_tx_data_ren[C_NUM_CHNL-1:0]),
         // Inputs
         .M_AXIS_RX_TDATA               (m_axis_rx_tdata[C_PCI_DATA_WIDTH-1:0]),
         .M_AXIS_RX_TKEEP               (m_axis_rx_tkeep[(C_PCI_DATA_WIDTH/8)-1:0]),
         .M_AXIS_RX_TLAST               (m_axis_rx_tlast),
         .M_AXIS_RX_TVALID              (m_axis_rx_tvalid),
         .M_AXIS_RX_TUSER               (m_axis_rx_tuser[`SIG_XIL_RX_TUSER_W-1:0]),
         .S_AXIS_TX_TREADY              (s_axis_tx_tready),
         .CFG_BUS_NUMBER                (cfg_bus_number[`SIG_BUSID_W-1:0]),
         .CFG_DEVICE_NUMBER             (cfg_device_number[`SIG_DEVID_W-1:0]),
         .CFG_FUNCTION_NUMBER           (cfg_function_number[`SIG_FNID_W-1:0]),
         .CFG_COMMAND                   (cfg_command[`SIG_CFGREG_W-1:0]),
         .CFG_DCOMMAND                  (cfg_dcommand[`SIG_CFGREG_W-1:0]),
         .CFG_LSTATUS                   (cfg_lstatus[`SIG_CFGREG_W-1:0]),
         .CFG_LCOMMAND                  (cfg_lcommand[`SIG_CFGREG_W-1:0]),
         .FC_CPLD                       (fc_cpld[`SIG_FC_CPLD_W-1:0]),
         .FC_CPLH                       (fc_cplh[`SIG_FC_CPLH_W-1:0]),
         .CFG_INTERRUPT_MSIEN           (cfg_interrupt_msienable),// TODO: Rename
         .CFG_INTERRUPT_RDY             (cfg_interrupt_rdy),
         .USER_CLK                      (user_clk),
         .USER_RESET                    (user_reset),
         .CHNL_RX_CLK                   (chnl_rx_clk[C_NUM_CHNL-1:0]),
         .CHNL_RX_ACK                   (chnl_rx_ack[C_NUM_CHNL-1:0]),
         .CHNL_RX_DATA_REN              (chnl_rx_data_ren[C_NUM_CHNL-1:0]),
         .CHNL_TX_CLK                   (chnl_tx_clk[C_NUM_CHNL-1:0]),
         .CHNL_TX                       (chnl_tx[C_NUM_CHNL-1:0]),
         .CHNL_TX_LAST                  (chnl_tx_last[C_NUM_CHNL-1:0]),
         .CHNL_TX_LEN                   (chnl_tx_len[(C_NUM_CHNL*`SIG_CHNL_LENGTH_W)-1:0]),
         .CHNL_TX_OFF                   (chnl_tx_off[(C_NUM_CHNL*`SIG_CHNL_OFFSET_W)-1:0]),
         .CHNL_TX_DATA                  (chnl_tx_data[(C_NUM_CHNL*C_PCI_DATA_WIDTH)-1:0]),
         .CHNL_TX_DATA_VALID            (chnl_tx_data_valid[C_NUM_CHNL-1:0]),
         .RX_NP_OK                      (rx_np_ok),
         .TX_CFG_GNT                    (tx_cfg_gnt),
         .RX_NP_REQ                     (rx_np_req)
         /*AUTOINST*/);
```

这个模块可以通过C_NUM_CHNL配置通道个数，C_PCI_DATA_WIDTH配置PCIe data的位宽，C_LOG_NUM_TAGS配置PCIe tag的个数。

module riffa_wrapper_ac701负责将xilinx PCIe IP的TX、RX、Configuration、flow control、中断信号转换为riffa的输入输出信号。

阅读riffa_wrapper_ac701.v源代码发现这个模块分为两个模块：

·       translation_xilinx ·       engine_layer ·       riffa

transtion_xilinx：负责提供统一的PCIe接口信息，比如altera、xilinx；

engine_layer：负责封装了tx_engine和rx_engine。其中tx engine负责上传DMA通道（写通道），即Interface: TX Classic；rx engine负责下发DMA通道（读通道），即Interface: RX Classic；

riffa：负责将tx/rx engine的信号转换为user logic的通道信号，方便我们使用，接口代码如下：

```verilog
input [C_NUM_CHNL-1:0]                     CHNL_RX_CLK, 
     output [C_NUM_CHNL-1:0]                    CHNL_RX, 
     input [C_NUM_CHNL-1:0]                     CHNL_RX_ACK, 
     output [C_NUM_CHNL-1:0]                    CHNL_RX_LAST, 
     output [(C_NUM_CHNL*32)-1:0]               CHNL_RX_LEN, 
     output [(C_NUM_CHNL*31)-1:0]               CHNL_RX_OFF, 
     output [(C_NUM_CHNL*C_PCI_DATA_WIDTH)-1:0] CHNL_RX_DATA, 
     output [C_NUM_CHNL-1:0]                    CHNL_RX_DATA_VALID, 
     input [C_NUM_CHNL-1:0]                     CHNL_RX_DATA_REN,
    
     input [C_NUM_CHNL-1:0]                     CHNL_TX_CLK, 
     input [C_NUM_CHNL-1:0]                     CHNL_TX, 
     output [C_NUM_CHNL-1:0]                    CHNL_TX_ACK,
     input [C_NUM_CHNL-1:0]                     CHNL_TX_LAST, 
     input [(C_NUM_CHNL*32)-1:0]                CHNL_TX_LEN, 
     input [(C_NUM_CHNL*31)-1:0]                CHNL_TX_OFF, 
     input [(C_NUM_CHNL*C_PCI_DATA_WIDTH)-1:0]  CHNL_TX_DATA, 
     input [C_NUM_CHNL-1:0]                     CHNL_TX_DATA_VALID, 
     output [C_NUM_CHNL-1:0]                    CHNL_TX_DATA_REN
```

对应整个实际框架的这一部分，如下图所示：

![](Linux下PCI设备驱动开发详解.res/pcie_xilinx2.awebp)

### 3. user logic

经过tx engine和rx engine模块，输出CHNL_RX_*和CHNL_TX_*信号，下面我们看一下user如何使用的，源代码如下：

```verilog
generate
    for (chnl = 0; chnl < C_NUM_CHNL; chnl = chnl + 1) begin : test_channels
        chnl_tester 
                #(
                  .C_PCI_DATA_WIDTH(C_PCI_DATA_WIDTH)
                  ) 
        module1 
                (.CLK(user_clk),
                 .RST(rst_out),    // riffa_reset includes riffa_endpoint resets
                 // Rx interface
                 .CHNL_RX_CLK(chnl_rx_clk[chnl]), 
                 .CHNL_RX(chnl_rx[chnl]), 
                 .CHNL_RX_ACK(chnl_rx_ack[chnl]), 
                 .CHNL_RX_LAST(chnl_rx_last[chnl]), 
                 .CHNL_RX_LEN(chnl_rx_len[32*chnl +:32]), 
                 .CHNL_RX_OFF(chnl_rx_off[31*chnl +:31]), 
                 .CHNL_RX_DATA(chnl_rx_data[C_PCI_DATA_WIDTH*chnl +:C_PCI_DATA_WIDTH]), 
                 .CHNL_RX_DATA_VALID(chnl_rx_data_valid[chnl]), 
                 .CHNL_RX_DATA_REN(chnl_rx_data_ren[chnl]),

                 // Tx interface
                 .CHNL_TX_CLK(chnl_tx_clk[chnl]), 
                 .CHNL_TX(chnl_tx[chnl]), 
                 .CHNL_TX_ACK(chnl_tx_ack[chnl]), 
                 .CHNL_TX_LAST(chnl_tx_last[chnl]), 
                 .CHNL_TX_LEN(chnl_tx_len[32*chnl +:32]), 
                 .CHNL_TX_OFF(chnl_tx_off[31*chnl +:31]), 
                 .CHNL_TX_DATA(chnl_tx_data[C_PCI_DATA_WIDTH*chnl +:C_PCI_DATA_WIDTH]), 
                 .CHNL_TX_DATA_VALID(chnl_tx_data_valid[chnl]), 
                 .CHNL_TX_DATA_REN(chnl_tx_data_ren[chnl])
                 );    
    end
endgenerate
```

这个模块的用于执行RIFFA TX 和 RX 接口。在RX接口上接收数据并保存最后接收的值。在TX接口上发送回相同数量的数据。返回的数据从接收到的最后一个值开始，重置并递增，直到等于TX接口上发回的（4字节）字数的值结束。

![](Linux下PCI设备驱动开发详解.res/pcie_xilinx3.awebp)

## 三、总结

这篇文章通过经典开源项目RIIFA的FPGA部分，结合源代码详细分析了框架部分的Xilinx PCIe hard IP、TX/RX engine和RIFFA模块，最后结合了user logic部分的chnl_tester，介绍了如何使用CHNL_TX_*和CHNL_RX_*接口。

# 第六章

本章及其以后的几章，我们将通过PCI Express总线实现CPU和FPGA数据通信的简单框架，介绍linux PCI内核态设备驱动的实战开发（KMD）。

这个框架就是开源界非常有名的RIFFA（reuseable integration framework for FPGA accelerators）,它是一个FPGA加速器的一种可重用性集成框架，是一个第三方开源PCIe框架。

该框架要求具备一个支持PCIe的工作站和一个带有PCIe连接器的FPGA板卡。RIFFA支持windows、linux，altera和xilinx，可以通过c/c++、python、matlab、java驱动来实现数据的发送和接收。驱动程序可以在linux或windows上运行，每一个系统最多支持5个FPGA device。

在用户端有独立的发送和接收端口，用户只需要编写几行简单代码即可实现和FPGA IP内核通信。

riffa使用直接存储器访问（DMA）传输和中断信号传输数据。这实现了PCIe链路上的高带宽，运行速率可以达到PCIe链路饱和点。

开源地址：[KastnerRG/riffa](https://github.com/KastnerRG/riffa)

## 一、linux下PCI驱动结构

在《Linux下PCI设备驱动开发详解（四）》文章中，我们了解到，一般来说，用模块方式编写PCI设备驱动，通常至少要实现以下几个部分：初始化设备模块、设备打开模块、数据读写模块、中断处理模块、设备释放模块、设备卸载模块。 一般如下方式：

```c
/* 指明该驱动程序适用于哪一些PCI设备 */
static struct pci_device_id demo = {
    PCI_VENDOR_ID_DEMO,
    PCI_DEVICE_ID_DEMO,
    PCI_ANY_ID,
    0,
    0,
    DEMO
};

/* 对特定PCI设备进行描述的数据结构 */
struct demo_card {
    unsigned int magic;
    /* 使用链表保存所有同类的PCI设备 */
    struct demo_card *next;
    ...
};

/* 中断处理模块 */
static void demo_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    /* ... */
};

/* 设备文件操作接口 */
static struct file_operations demo_fops = {
    owner: THIS_MODULE, /* demo_fops 所属的设备模块 */
    read: demo_read, /* 读设备操作 */
    write: demo_write, /* 写设备操作 */
    ioctl: demo_ioctl, /* 控制设备操作 */
    mmap：demo_mmap, /* 内存重映射操作 */
    open：demo_open, /* 打开设备操作 */
    release: demo_release /* 释放设备操作 */
    /* ... */
};

/* 设备模块信息 */
static struct pci_driver demo_pci_driver = {
    name: demo_MODULE_NAME, /* 设备模块名称 */
    id_table：demo_pci_tbl, /* 能够驱动的设备列表 */
    probe：demo_probe; /* 查找并初始化设备 */
    remove：demo_remove /* 卸载设备模块 */
    /* ... */
};

static int __init demo_init_module (void)
{
    /* ... */
};

static void __exit demo_cleanup_module(void)
{
    pci_unregister_driver(&demo_pci_driver);
}

/* 加载驱动程序模块入口 */
module_init(demo_init_nodule);

/* 卸载驱动程序模块入口 */
module_exit(demo_cleanup_module);
```

好的，带着这个框架我们进入到下面RIFFA框架的driver源代码分析。

## 二、初始化设备模块

我们直接给出源代码：

```c
/**
 * Called to initialize the PCI device. 
 */
static int __init fpga_init(void) 
{
	int i;
	int error;

    /* 初始化host最大支持FPGA设备的个数 */
	for (i = 0; i < NUM_FPGAS; i++)
		atomic_set(&used_fpgas[i], 0);

    /* 注册硬件驱动程序 */
	error = pci_register_driver(&fpga_driver);
	if (error != 0) {
		printk(KERN_ERR "riffa: pci_module_register returned %d\n", error);
		return (error);
	}
    /* 向内核注册一个字符设备（可选） */
	error = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fpga_fops);
	if (error < 0) {
		printk(KERN_ERR "riffa: register_chrdev returned %d\n", error);
		return (error);
	}

    /* 向内核注册class */
	#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
		mymodule_class = class_create(THIS_MODULE, DEVICE_NAME);
	#else
		mymodule_class = class_create(DEVICE_NAME);
	#endif

	if (IS_ERR(mymodule_class)) {
		error = PTR_ERR(mymodule_class);
		printk(KERN_ERR "riffa: class_create() returned %d\n", error);
		return (error);
	}

    /* 创建设备文件节点 */
	devt = MKDEV(MAJOR_NUM, 0);
	device_create(mymodule_class, NULL, devt, "%s", DEVICE_NAME);

	return 0;
}
```

OK，我们有看到了几个关键词，驱动程序、字符设备、class、文件节点。在《Linux下PCI设备驱动开发详解（三）》中，我们知道总线、设备、驱动模型：

```scss
系统启动后，会调用buses_init()函数创建/sys/bus文件目录，这部分系统在开机时已经帮我们准备好了。

接下去就是通过总线注册函数bus_register()进行总线注册（可选，一般不会注册新的总线），注册完成后，

在/sys/bus目录下生成device文件夹和driver文件夹，最后分别通过device_register()以及driver_register()函数注册对应的设备和驱动。
```

![](Linux下PCI设备驱动开发详解.res/pcie_topology.awebp)

硬件拓扑描述linux设备模型中四个重要概念：

```arduino
bus（总线）：linux认为，总线是CPU和一个或多个设备之间信息交互的通道。而为了方便设备模型的抽象，所有设备都要连接到总线上。

class（分类）：linux设备模型中，class的概念非常类似面向对象程序设计的class（类），它主要是集合具有类似功能或属性的设备，这样就可以抽象出一套可以在多个设备之间公用的数据结构和接口函数。

device（设备）：抽象系统中所有的硬件设备，描述它的名字、属性、从属bus，从属class等信息。

driver（驱动）：包括设备初始化、管理、read、write、销毁等接口实现。
```

## 三、probe探测硬件设备

```c
/**
 * probe设备
 */
 static int __devinit fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
 {
    ...

    // Setup the PCIe device.
	error = pci_enable_device(dev);
	if (error < 0) {
		printk(KERN_ERR "riffa: pci_enable_device returned %d\n", error);
		return (-ENODEV);
	}

	// Enable bus master
	pci_set_master(dev);

	// Set the mask size
	error = pci_set_dma_mask(dev, DMA_BIT_MASK(64));
	if (!error)
		error = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64));
	if (error) {
		printk(KERN_ERR "riffa: cannot set 64 bit DMA mode\n");
		pci_disable_device(dev);
		return error;
	}

    // Allocate device structure.
    sc = kzalloc(sizeof(*sc), GFP_KERNEL);
    ...

    // Setup the BAR memory regions
    error = pci_request_regions(dev, sc->name);
    ...

    // PCI BAR 0
    ...
    sc->bar0 = ioremap(sc->bar0_addr, sc->bar0_len);
    ...

    // setup msi interrupts
    error = pci_enable_msi(dev);
    ...

    // Request an interrupt
    error = request_irq(dev->irq, intrpt_handler, IRQF_SHARED, sc->name, sc);

    // Set extended tag bit
    error = pcie_capability_read_dword(dev,PCI_EXP_DEVCTL,&devctl_result);
    ...
    error = pcie_capability_write_dword(dev,PCI_EXP_DEVCTL,(devctl_result|PCI_EXP_DEVCTL_EXT_TAG));

    // Set IDO bits
    ...
    error = pcie_capability_read_dword(dev,PCI_EXP_DEVCTL2,&devctl2_result);
    error = pcie_capability_write_dword(dev,PCI_EXP_DEVCTL2,(devctl2_result | PCI_EXP_DEVCTL2_IDO_REQ_EN | PCI_EXP_DEVCTL2_IDO_CMP_EN));

    // Set RCB to 128
    error = pcie_capability_read_dword(dev,PCI_EXP_LNKCTL,&lnkctl_result);
    ...
    error = pcie_capability_write_dword(dev,PCI_EXP_LNKCTL,(lnkctl_result|PCI_EXP_LNKCTL_RCB));

    // Read device configuration
    ...

    // Create chnl_dir structs.
    sc->recv = (struct chnl_dir **) kzalloc(sc->num_chnls*sizeof(struct chnl_dir*), GFP_KERNEL);  
	sc->send = (struct chnl_dir **) kzalloc(sc->num_chnls*sizeof(struct chnl_dir*), GFP_KERNEL);  
    ...
    j = allocate_chnls(dev, sc);

    // Create spill buffer (for overflow on receive).
    sc->spill_buf_addr = pci_alloc_consistent(dev, SPILL_BUF_SIZE, &hw_addr);
	sc->spill_buf_hw_addr = hw_addr;

    // Save pointer to structure 
    ...
 }
```

这个fpga_probe函数非常重要和关键了：

```scss
1.  pci_enable_device()：把PCI配置空间的command域的bit0和bit1设置成1，从而达到开启设备的目的，即把config控制寄存器映射成IO/MEM空间。

2.  pci_set_master()：设置主总线为DMA模式。

3.  pci_set_dma_mask()：辅助函数用于检查总线是否可以接收给定大小的总线地址（mask），如果可以，则通知总线层给定外围设备将使用该大小的总线地址。

4.  kzalloc()：内核态的内存分配，struct fpga_state 保存device_id、vendor_id、bar空间、通道数、接收/发送通道的地址信息等。

5.  pci_request_regions()：该函数用于请求PCIe设备的IO资源。在probe函数中，驱动程序会调用pci_request_regions函数来请求设备的IO资源。

6.  ioremap()：此函数用于映射PCIe设备的IO空间到内核地址空间。在probe函数中，驱动程序会调用pci_iomap函数来映射设备的IO空间。

7.  pci_enable_msi()：它允许PCI设备使用MSI中断机制，当函数被调用时，它将被指定的PCI设备启用MSI中断，并返回中断号。

8.  request_irq()：注册中断服务函数，当中断发生时，系统调用这个函数。

9.  pcie_capability_write_dword(..., ..., PCI_EXP_DEVCTL_EXT_TAG)：PCI_EXP_DEVCTL_EXT_TAG，标识设备的DevCtl设置了ExtTag+，设置了这个标识位，读请求tlp中requester ID字段会扩展一个8位的tag，表示能暂存数据包的数量，但是需要FPGA PCIe设备支持，否则会出现数据溢出而丢包。

10. pcie_capability_write_dword(..., ..., PCI_EXP_DEVCTL2_IDO_REQ_EN)：IDO标识位。

11. pcie_capability_write_dword(..., ..., PCI_EXP_LNKCTL_RCB)：读完成边界，是 Completer 响应读请求的一种地址边界对齐策略，应用于 CplD，RC 的 RCB 可以为 64B 或 128B，默认 64 B；EP、Bridge、Switch 等其他设备的 RCB 只能为 128 B。

12. allocate_chnls()：这个主要是通过pci_alloc_consistent申请dma的读/写multi-page内存，并形成sglist环形链表，并保存在fpga_state中，完成用户态多通道dma的读写请求。
```

## 四、写操作

基本的读写操作通过ioctl来调用对应的driver驱动的实现。我们补充一下，ioctl是设备驱动程序中设备控制接口函数，一个字符设备驱动通常会实现设备打开、关闭、读、写等功能，在一些需要细分的情境下，如果需要扩展新的功能，通常以增设 ioctl() 命令的方式实现。

直接给出代码：

```c
static long fpga_ioctl(struct file *filp, unsigned int ioctlnum, 
		unsigned long ioctlparam)
{
	int rc;
	fpga_chnl_io io;
	fpga_info_list list;

	switch (ioctlnum) {
	case IOCTL_SEND:
		if ((rc = copy_from_user(&io, (void *)ioctlparam, sizeof(fpga_chnl_io)))) {
			printk(KERN_ERR "riffa: cannot read ioctl user parameter.\n");
			return rc;
		}
		if (io.id < 0 || io.id >= NUM_FPGAS || !atomic_read(&used_fpgas[io.id]))
			return 0;
		return chnl_send_wrapcheck(fpgas[io.id], io.chnl, io.data, io.len, io.offset,
				io.last, io.timeout);
	case IOCTL_RECV:
		if ((rc = copy_from_user(&io, (void *)ioctlparam, sizeof(fpga_chnl_io)))) {
			printk(KERN_ERR "riffa: cannot read ioctl user parameter.\n");
			return rc;
		}
		if (io.id < 0 || io.id >= NUM_FPGAS || !atomic_read(&used_fpgas[io.id]))
			return 0;
		return chnl_recv_wrapcheck(fpgas[io.id], io.chnl, io.data, io.len, io.timeout);
	case IOCTL_LIST:
		list_fpgas(&list);
		if ((rc = copy_to_user((void *)ioctlparam, &list, sizeof(fpga_info_list))))
			printk(KERN_ERR "riffa: cannot write ioctl user parameter.\n");
		return rc;
	case IOCTL_RESET:
		reset((int)ioctlparam);
		break;
	default:
		return -ENOTTY;
		break;
	}
	return 0;
}
```

在处理ioctl_send的时候，我们发现实现用户数据拷贝到内核态之后，调用了chnl_send_wrapcheck，将api层打包过来的参数一一传递过去。

直接给出chnl_send_wrapcheck()：

```c
static inline unsigned int chnl_send_wrapcheck(struct fpga_state * sc, int chnl,
				const char  __user * bufp, unsigned int len, unsigned int offset,
				unsigned int last, unsigned long long timeout)
{
	// Validate the parameters.
    ...
	// Ensure no simultaneous operations from several threads
	...
	ret = chnl_send(sc, chnl, bufp, len, offset, last, timeout);

	// Clear the busy flag
    ...

	return ret;
}
```

这段代码主要做了一些避免错误的判断，值得一提的就是通过自旋锁避免了多线程错误的判断，其实我们可以知道riffa架构支持多线程，之后调用了chnl_send.

```c
static inline unsigned int chnl_send(struct fpga_state * sc, int chnl,
				const char  __user * bufp, unsigned int len, unsigned int offset,
				unsigned int last, unsigned long long timeout)
{
    ...
	// Convert timeout to jiffies.
	...

	// Clear the message queue.
	while (!pop_circ_queue(sc->send[chnl]->msgs, &msg_type, &msg));

	// Initialize the sg_maps
	sc->send[chnl]->sg_map_0 = NULL;
	sc->send[chnl]->sg_map_1 = NULL;

	// Let FPGA know about transfer.
	DEBUG_MSG(KERN_INFO "riffa: fpga:%d chnl:%d, send (len:%d off:%d last:%d)\n", sc->id, chnl, len, offset, last);
	write_reg(sc, CHNL_REG(chnl, RX_OFFLAST_REG_OFF), ((offset<<1) | last));
	write_reg(sc, CHNL_REG(chnl, RX_LEN_REG_OFF), len);
	if (len == 0)
		return 0;

	// Use the send common buffer to share the scatter gather data
	sg_map = fill_sg_buf(sc, chnl, sc->send[chnl]->buf_addr, udata, length, 0, DMA_TO_DEVICE);
	if (sg_map == NULL || sg_map->num_sg == 0)
		return (unsigned int)(sent>>2);

	// Update based on the sg_mapping
	udata += sg_map->length;
	length -= sg_map->length;
	sc->send[chnl]->sg_map_1 = sg_map;

	// Let FPGA know about the scatter gather buffer.
	write_reg(sc, CHNL_REG(chnl, RX_SG_ADDR_LO_REG_OFF), (sc->send[chnl]->buf_hw_addr & 0xFFFFFFFF));
	write_reg(sc, CHNL_REG(chnl, RX_SG_ADDR_HI_REG_OFF), ((sc->send[chnl]->buf_hw_addr>>32) & 0xFFFFFFFF));
	write_reg(sc, CHNL_REG(chnl, RX_SG_LEN_REG_OFF), 4 * sg_map->num_sg);
	DEBUG_MSG(KERN_INFO "riffa: fpga:%d chnl:%d, send sg buf populated, %d sent\n", sc->id, chnl, sg_map->num_sg);

	// Continue until we get a message or timeout.
	while (1) {
		while ((nomsg = pop_circ_queue(sc->send[chnl]->msgs, &msg_type, &msg))) {
			prepare_to_wait(&sc->send[chnl]->waitq, &wait, TASK_INTERRUPTIBLE);
			// Another check before we schedule.
			if ((nomsg = pop_circ_queue(sc->send[chnl]->msgs, &msg_type, &msg)))
				tymeout = schedule_timeout(tymeout);
			finish_wait(&sc->send[chnl]->waitq, &wait);
			if (signal_pending(current)) {
				free_sg_buf(sc, sc->send[chnl]->sg_map_0);
				free_sg_buf(sc, sc->send[chnl]->sg_map_1);
				return -ERESTARTSYS;
			}
			if (!nomsg)
				break;
			if (tymeout == 0) {
				printk(KERN_ERR "riffa: fpga:%d chnl:%d, send timed out\n", sc->id, chnl);
				free_sg_buf(sc, sc->send[chnl]->sg_map_0);
				free_sg_buf(sc, sc->send[chnl]->sg_map_1);
				return (unsigned int)(sent>>2);
			}
		}
		tymeout = tymeouto;

		// Process the message.
		switch (msg_type) {
		case EVENT_SG_BUF_READ:
			// Release the previous scatter gather data?
			if (sc->send[chnl]->sg_map_0 != NULL)
				sent += sc->send[chnl]->sg_map_0->length;
			free_sg_buf(sc, sc->send[chnl]->sg_map_0);
			sc->send[chnl]->sg_map_0 = NULL;
			// Populate the common buffer with more scatter gather data?
			if (length > 0) {
				sg_map = fill_sg_buf(sc, chnl, sc->send[chnl]->buf_addr, udata, length, 0, DMA_TO_DEVICE);
				if (sg_map == NULL || sg_map->num_sg == 0) {
					free_sg_buf(sc, sc->send[chnl]->sg_map_0);
					free_sg_buf(sc, sc->send[chnl]->sg_map_1);
					return (unsigned int)(sent>>2);
				}
				// Update based on the sg_mapping
				udata += sg_map->length;
				length -= sg_map->length;
				sc->send[chnl]->sg_map_0 = sc->send[chnl]->sg_map_1;
				sc->send[chnl]->sg_map_1 = sg_map;
				write_reg(sc, CHNL_REG(chnl, RX_SG_ADDR_LO_REG_OFF), (sc->send[chnl]->buf_hw_addr & 0xFFFFFFFF));
				write_reg(sc, CHNL_REG(chnl, RX_SG_ADDR_HI_REG_OFF), ((sc->send[chnl]->buf_hw_addr>>32) & 0xFFFFFFFF));
				write_reg(sc, CHNL_REG(chnl, RX_SG_LEN_REG_OFF), 4 * sg_map->num_sg);
				DEBUG_MSG(KERN_INFO "riffa: fpga:%d chnl:%d, send sg buf populated, %d sent\n", sc->id, chnl, sg_map->num_sg);
			}
			break;

		case EVENT_TXN_DONE:
			// Update with the true value of words transferred.
			sent = (((unsigned long long)msg)<<2);
			// Return as this is the end of the transaction.
			free_sg_buf(sc, sc->send[chnl]->sg_map_0);
			free_sg_buf(sc, sc->send[chnl]->sg_map_1);
			DEBUG_MSG(KERN_INFO "riffa: fpga:%d chnl:%d, sent %d words\n", sc->id, chnl, (unsigned int)(sent>>2));
			return (unsigned int)(sent>>2);
			break;

		default: 
			printk(KERN_ERR "riffa: fpga:%d chnl:%d, received unknown msg: %08x\n", sc->id, chnl, msg);
			break;
		}
	}

	return 0;
}
```

将数据写入指定的FPGA通道。除非配置了非零超时，否则将阻塞，直到所有数据都发送到 FPGA。如果超时不为零，则该函数将阻塞，直到发送所有数据或超时毫秒过去。来自 bufp 指针的用户数据将被发送，最多 len 字（每个字 == 32 位）。通道将被告知预期数据量和偏移量。如果 last == 1，则 FPGA 通道将在发送后将此事务识别为完成。如果 last == 0，则 FPGA 通道将需要额外的事务。

成功后，返回发送的字数。出错时，返回负值。

核心思想就是，初始化sg_maps，通过bar空间告知FPGA通道号、长度、大小等信息、使用通用buffer发送数据、更新sg_mapping，最后进入到while（1）的循环函数中。

while(1)大循环，只有当处理完Tx数据完成中断或出错时函数才会返回。在每一轮执行中，首先执行内嵌的小while，在小while中首先读取对应通道上的send消息队列，若返回值为0说明成功出队，小while运行一遍后就会执行下面的代码；若返回值为1说明队列可能是空的，也就是还没有中断到来，此时调用prepare_to_wait函数将本进程添加到等待队列里，然后执行schedule_timeout休眠该进程（有阻塞时间限制），此时在用户看来表现为ioctl函数阻塞等待，但中断还能在后台运行（中断也是一个进程）。

若此时驱动接收到一个该通道的Tx中断，那么在中断回调函数里将中断信息推入消息队列后就会唤醒chnl_send所在的进程。进程唤醒后调用finish_wait函数将本进程pop出等待队列并用signal_pending查看是否因信号而被唤醒，如果是 需要返回给用户并让其再次重试。如果不是被信号唤醒，则再去读一下消息队列，此时会将消息类型存入msg_type，消息存入msg中，然后退出小while。

接下来进入一个switch语句，这个switch是根据msg_type消息类型选择处理动作的，即中断处理的下半部。

若执行Tx SG读完成中断，则消息类型发送EVENT_SG_BUF_READ，数据填0，其实是没用的数据。在这里如果剩余长度大于0或者剩余溢出值大于0时就会重新执行上一段讲述的过程，即从上一次分配的结尾处再分配SG缓冲区，并发送SG链表给FPGA等等，不过一般不会发送这种情况，除非分配页时的get_user_pages函数锁定物理页出现了问题，少分了页才会出现这样的现象。

然后FPGA就会按SG链表一个一个SG缓存块的进行流式DMA传输，传输完毕后FPGA发送一个Tx数据读完成中断，即EVENT_TXN_DONE消息类型。这里比较好处理，调用dma_unmap_sg取消内存空间的SGDMA映射，然后释放掉页。

## 五、读操作

读操作和写操作类似，不再详细描述。

函数chnl_recv用于将FPGA发送的数据读到缓冲区内。

首先调用宏DEFINE_WAIT初始化等待队列项；然后把传入的参数timeout换算成毫秒，这个时间是最长阻塞时间。

剩下的就是中断处理过程，等待读完成。

## 六、销毁/卸载设备

释放设备模块主要是负责释放对设备的控制权，释放占用的内存和中断等，所做的事情正好和打开设备模块相反。

```c
static void __exit fpga_exit(void)
{
	device_destroy(mymodule_class, devt); 
	class_destroy(mymodule_class);
	pci_unregister_driver(&fpga_driver);
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}
```

本文从详细介绍了RIFFA框架的驱动模块，涉及的内容非常多，内核页面、中断处理等。

一个驱动的框架主要包括：初始化设备模块、设备打开模块、数据读写模块、中断处理模块、设备释放模块、设备卸载模块。

# 第七章

本章及其以后的几章，我们将通过PCI Express总线实现CPU和FPGA数据通信的简单框架。

这个框架就是开源界非常有名的RIFFA（reuseable integration framework for FPGA accelerators）,它是一个FPGA加速器的一种可重用性集成框架，是一个第三方开源PCIe框架。

该框架要求具备一个支持PCIe的工作站和一个带有PCIe连接器的FPGA板卡。RIFFA支持windows、linux，altera和xilinx，可以通过c/c++、python、matlab、java驱动来实现数据的发送和接收。驱动程序可以在linux或windows上运行，每一个系统最多支持5个FPGA device。

在用户端有独立的发送和接收端口，用户只需要编写几行简单代码即可实现和FPGA IP内核通信。

riffa使用直接存储器访问（DMA）传输和中断信号传输数据。这实现了PCIe链路上的高带宽，运行速率可以达到PCIe链路饱和点。

本文主要介绍消息队列，即riffa.c和riffa.h文件。

riffa是为了在内核中使用而编写的消息队列，用于同步中断和进程。

开源地址：[KastnerRG/riffa](https://github.com/KastnerRG/riffa)

## 一、消息队列是什么？

在学习消息队列之前，我们先说一下什么是队列（queue）。队列queue可以说是一个数据结构，可以存储数据，如下图，我们从左侧插入元素（入队），从队头获取元素（出队）。

![](Linux下PCI设备驱动开发详解.res/mq1.awebp)

对于这样一个数据结构大家并不陌生，java/c++也是实现好多队列。例如，创建线程池我们需要一个阻塞队列，JDK的lock也需要队列。

了解到队列后，我们看一下什么是消息队列，消息队列就是我们常说的MQ，message queue，是作为一个单独的中间件产品存在的，独立部署。

![](Linux下PCI设备驱动开发详解.res/mq2.awebp)

## 二、为什么要用消息队列呢？

消息队列在某些场景下发挥奇效，例如：**解耦、异步、削峰**。

### 1. 解耦

解耦都不陌生吧，就是降低耦合度，我们都知道Spring的主要目的是降低耦合，那MQ又是如何解耦的呢？

如下图所示，系统A是一个关键性的系统，产生数据后需要通知到系统B和系统C做响应的反应，三个系统都写好了，稳定运行；某一天，系统D也需要在系统A产生数据后作出反应，那就得系统A改代码，去调系统D的接口，好，改完了，上线了。假设过了某段时间，系统C因为某些原因，不需要作出反应了，不要系统A调它接口了，就让系统A把调接口的代码删了，系统A的负责人可定会很烦，改来改去，不停的改，不同的测，还得看会不会影响系统B，系统D。没办法，这种架构下，就是这样麻烦。

![](Linux下PCI设备驱动开发详解.res/mq3.awebp)

而且这样还没考虑异常情况，假如系统A产生了数据，本来需要实时调系统B的，结果系统B宕机了或重启了，没调成功咋办，或者调用返回失败怎么办，系统A是不是要考虑要不要重试？还要开发一套重试机制，系统A要考虑的东西也太多了吧。

那如果使用MQ会是什么样的效果呢？如下图所示，系统A产生数据之后，将该数据写到MQ中，系统A就不管了，不用关心谁消费，谁不消费，即使是再来一个系统E，或者是系统D不需要数据了，系统A也不需要做任何改变，而系统B、C、D是否消费成功，也不用系统A去关心，通过这样一种机制，系统A和其他各系统之间的强耦合是不是一下子就解除了，系统A是不是一下子清爽了很多？

![](Linux下PCI设备驱动开发详解.res/mq4.awebp)

### 2. 异步

同步/异步大家都知道吧，举个例子，你早上起床后边吃早饭边看电视（异步），而不是吃完饭再看电视（同步）。在上述例子中，没有使用MQ时，系统A要调系统B、C、D的接口，我们看一下下面的伪代码想一下是不是这样：

```cpp
Data newData = produceData(); // 系统A经过一系列逻辑处理产生了数据，耗时200ms
    Response respB = callsysB(newData); //系统A调用系统B接口发送数据，耗时300ms
    Response respC = callsysyC(newData); //系统A调用系统C发送接口，耗时300ms
    Response respD = callsysyD(newData); //系统A调用系统D发送接口，耗时300ms
```

这样系统A的用户做完这个操作需要等待：

```ini
200ms+300ms+300ms+300ms=1100ms=1.1s
```

假设使用了MQ呢？系统A就只需要把产生的数据放到MQ里就行了，就可以立马返回用户响应。伪代码如下：

```cpp
//系统A的代码
    Data newData = produceData(); //系统A经过一些逻辑后产生了数据，耗时200ms
    writeDataToMQ(newData); //往MQ写消息，耗时50ms
```

这样系统A的用户做完这个操作就只需要等待200ms+50ms=250ms，给用户的感觉就是一瞬间的事儿，点一下就好了，用户体验提升了很多。

系统A把数据写到MQ里，系统B、C、D就自己去拿，慢慢消费就行了。一般就是一些时效性要求不高的操作，比如下单成功系统A调系统B发下单成功的短信，短信晚几秒发都是OK的。

## 三、RIFFA驱动中的消息队列

消息队列是一种线程间同步的方法，在Linux应用编程中又称为无名管道，其主体就是一个FIFO，读写跨接在两个线程间。

消息队列主要是用来同步并且传递数据，所以都有阻塞功能，但因为RIFFA中用的是自己编写的消息队列不自带阻塞，故需要配合阻塞I/O实现完整的消息队列功能。

消息队列的操作函数和数据结构在circ_queue.c与circ_queue.h中，下面看一下这两个文件。

circ_queue.h中定义了消息队列的数据结构circ_queue，里面的成员中，writeIndex和readIndex是写指针和读指针，类型是原子变量，为了解决多线程操作时的竞争问题；**vals是指针数组，指向消息队列的存储体；len是消息队列长度。

```c
/* struct for the circular queue */
struct circ_queue {
    atomic_t writeIndex;
    atomic_t readIndex;
    unsigned int ** vals;
    unsigned int len;
};
typedef struct circ_queue circ_queue;
```

circ_queue.c中定义了消息队列的初始化函数init_circ_queue，如下所示：

```c
/**
 * initializes a circ_queue with depth/length len. returns non-NULL or success,
 * NULL if there was a problem creating the queue.
 */
 circ_queue * init_circ_queue(int len)
 {
    int i;
    circ_queue *q;

    q = kzalloc(sizeof(circ_queue), GFP_KERNEL);
    if (q == NULL) {
        printk(KERN_ERR "not enough memory to allocate circ_queue.");
        return NULL;
    }

    atomic_set(&q->writeIndex, 0);
    atomic_set(&q->readIndex, 0);
    q->len = len;

    q->vals = (unsigned int **) kzalloc(len*sizeof(unsigned int *), GFP_KERNEL);
    if (q->vals == NULL) {
        printk(KERN_ERR "not enough memory to allocate circ_queue array.");
        return NULL;
    }
    for (i = 0; i < len; i++) {
        q->vals[i] = (unsigned int *)kzalloc(2*sizeof(unsigned int), GFP_KERNEL);
        if (q->vals[i] == NULL) {
            printk(KERN_ERR "not enough memory to allocate circ_queue position.");
            return NULL;
        }
    }

    return q;
 }
```

首先调用kzalloc创建了circ_queue结构体指针q，这个q就是描述消息队列的对象，然后把读写指针复位，接着又调用kzalloc分配了消息队列存储体的数组整体，每个元素都是一个空指针（此时消息队列还没有生成），最后为每个空指针都分配两个数据空间，此时消息队列建立完毕，q中所描述的是一个二维数组，行数2，列数len，也就是一个存储单元能存2个数。

再看一个函数queue_count_to_index，这个函数用来将读写指针变成数组的索引号：

```c
/**
 * internal function to help count. returns the queue size normalized position.
 */
unsigned int queue_count_to_index(unsigned int count, unsigned int len)
{
    return (count % len);
}
```

可以看到这个函数对count参数做了模运算，说明是一个环形队列，消息队列的全貌如下：

![](Linux下PCI设备驱动开发详解.res/mq5.awebp)

读写消息队列函数就没什么好说的了，就是对这个环形缓冲区进行一次读写，每个单元存两个值，成功返回0，失败返回1。值得注意的是，和FIFO一样，读数据不会让数据消失，只是读写指针自增了而已。

写消息队列函数为push_circ_queue;读消息队列函数为pop_circ_queue;

这里直接给出源代码：

```c
int push_circ_queue(circ_queue * q, unsigned int val1, unsigned int val2)
{
    unsigned int curReadIndex;
    unsigned int curWriteIndex;

    curWriteIndex = atomic_read(&q->writeIndex);
    currReadIndex  = atomic_read(&q->readIndex);

    // The queue is full
    if (queue_count_to_index(currWriteIndex+1, q->len) == queue_count_to_index(currReadIndex, q->len)) {
		return 1;
	}

    // Save the data into the queue
	q->vals[queue_count_to_index(currWriteIndex, q->len)][0] = val1;
	q->vals[queue_count_to_index(currWriteIndex, q->len)][1] = val2;
	// Increment atomically write index. Now a consumer thread can read
	// the piece of data that was just stored.
	atomic_inc(&q->writeIndex);

    return 0;
}

int pop_circ_queue(circ_queue * q, unsigned int * val1, unsigned int * val2)
{
	unsigned int currReadIndex;
	unsigned int currMaxReadIndex;

	do
	{
		currReadIndex = atomic_read(&q->readIndex);
		currMaxReadIndex = atomic_read(&q->writeIndex);
		// The queue is empty or a producer thread has allocate space in the queue
		// but is waiting to commit the data into it
		if (queue_count_to_index(currReadIndex, q->len) == queue_count_to_index(currMaxReadIndex, q->len)) {
			return 1;
		}

		// Retrieve the data from the queue
		*val1 = q->vals[queue_count_to_index(currReadIndex, q->len)][0];
		*val2 = q->vals[queue_count_to_index(currReadIndex, q->len)][1];

		// Try to perfrom now the CAS operation on the read index. If we succeed
		// label & val already contain what q->readIndex pointed to before we 
		// increased it.
		if (atomic_cmpxchg(&q->readIndex, currReadIndex, currReadIndex+1) == currReadIndex) {
			// The lable & val were retrieved from the queue. Note that the
			// data inside the label or value arrays are not deleted.
			return 0;
		}

		// Failed to retrieve the elements off the queue. Someone else must
		// have read the element stored at countToIndex(currReadIndex)
		// before we could perform the CAS operation.       
	} while(1); // keep looping to try again!

	return 1;
}
```

# 第八章

RIFFA的Linux驱动文件夹下有6个C源码文件，riffa_driver.c、riffa_driver.h、circ_queue.c、circ_queue.h、riffa.c、riffa.h。

其中riffa.c和riffa.h不属于驱动源码，它们是系统函数调用驱动封装的一层接口，属于用户态应用程序的一部分。

在讲解riffa之前，我们先看一下什么是系统调用。

开源地址：[KastnerRG/riffa](https://github.com/KastnerRG/riffa)

## 一、系统调用

### 1. 理论基础

![](Linux下PCI设备驱动开发详解.res/riffa_source1.awebp)

探究系统调用，以ioctl为例子，通俗来讲，其实就是探究操作系统实现应用层的ioctl对应上特定驱动程序的ioctl的过程。

由于应用程序的ioctl处于用户空间，驱动程序的ioctl处于内核空间，所以这两者之间不属于简单的调用关系；另外，考虑到内核空间操作的安全性，系统调用过程大量的安全性处理，进而使得系统调用看起来十分复杂。

**ioctl作用**：应用层的ioctl函数传入的cmd和arg参数会直接传入驱动层的ioctl接口，在对应驱动文件会对相应的命令进行操作。对于传递的ioctl命令有一定的规范，具体可以参考：/include/asm/ioctl.h,/Documentation/ioctl-number.txt 这两个文件。

应用层和驱动程序联系如下：

应用层和驱动程序联系如下：

```perl
最终ioctl是通过系统调用sys_ioctl软中断陷入到内核，通过系统中断号最终调用到内核态的ioctl函数。
```

### 2. 代码实例

构造ioctl命令linux已经给我们提供了宏命令：

```c
_IO    an ioctl with no parameters
_IOW   an ioctl with write parameters (copy_from_user)
_IOR   an ioctl with read parameters  (copy_to_user)
_IOWR  an ioctl with both write and read parameters
相关参数：
/*
    type:    幻数（设备相关的一个字母，避免和内核冲突）
    nr:      命令编号
    datatype:数据类型
*/
_IO（type,nr）           //无参数的命令
_IOR(type,nr,datatype)  //应用从驱动中读数据
_IOW(type,nr,datatype)  //应用从驱动中写数据
```

下面给出简单的实例用户态应用层代码：

```c
//应用程序
 
#define MOTOR_CMD_BASE'M'  
#define SET_MOTOR_STEP _IOW(MOTOR_CMD_BASE, 1u, int)
#define GET_MOTOR_STEP _IOW(MOTOR_CMD_BASE, 2u, int)
 ...
    int step= 0;
    int value = 0;
    int fd = -1;
 
    fd = open("/dev/motor",O_RDWR);   
    if (fd== -1) {   
        printf("open device error!/n");   
        return -1;   
    }  
   ...  
    printf("Please input the motor step:/n"  
    scanf("%d",&step);    
    if(ioctl(fd, SET_MOTOR_STEP, &step)<0){  
          perror("ioctl error");  
        exit(1);  
    }  
 
```

驱动程序的代码：

```c
//驱动程序
//假设该驱动程序建立了名为motor的字符设备
 
#define MOTOR_CMD_BASE'M'  
#define SET_MOTOR_STEP _IOW(MOTOR_CMD_BASE, 1u, int)
#define GET_MOTOR_STEP _IOW(MOTOR_CMD_BASE, 2u, int)
 
 
int motor_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)  
{
    int step=0;   
    int value = 0;
    switch (cmd) {  
        case SET_MOTOR_STEP :  
            if(copy_from_user(&step, (int*)arg, sizeof(int)))
                  return fail;
          //处理程序
            break;
        case GET_MOTOR_STEP :
            value = 100;
            if(copy_to_user((int*)arg, &value, sizeof(int)))
                return fail;
            break;
    ...
    }
}  
```

## 二、RIFFA代码分析

这里我们直接给出源代码：

```c
fpga_t * fpga_open(int id) 
{
	fpga_t * fpga;

	// Allocate space for the fpga_dev
	fpga = (fpga_t *)malloc(sizeof(fpga_t));
	if (fpga == NULL)
		return NULL;
	fpga->id = id;	

	// Open the device file.
	fpga->fd = open("/dev/" DEVICE_NAME, O_RDWR | O_SYNC);
	if (fpga->fd < 0) {
		free(fpga); 
		return NULL;
	}
	
	return fpga;
}

void fpga_close(fpga_t * fpga) 
{
	// Close the device file.
	close(fpga->fd);
	free(fpga);
}

int fpga_send(fpga_t * fpga, int chnl, void * data, int len, int destoff, 
	int last, long long timeout)
{
	fpga_chnl_io io;

	io.id = fpga->id;
	io.chnl = chnl;
	io.len = len;
	io.offset = destoff;
	io.last = last;
	io.timeout = timeout;
	io.data = (char *)data;

	return ioctl(fpga->fd, IOCTL_SEND, &io);
}

int fpga_recv(fpga_t * fpga, int chnl, void * data, int len, long long timeout)
{
	fpga_chnl_io io;

	io.id = fpga->id;
	io.chnl = chnl;
	io.len = len;
	io.timeout = timeout;
	io.data = (char *)data;

	return ioctl(fpga->fd, IOCTL_RECV, &io);
}

void fpga_reset(fpga_t * fpga)
{
	ioctl(fpga->fd, IOCTL_RESET, fpga->id);
}

int fpga_list(fpga_info_list * list) {
	int fd;
	int rc;

	fd = open("/dev/" DEVICE_NAME, O_RDWR | O_SYNC);
	if (fd < 0)
		return fd;
	rc = ioctl(fd, IOCTL_LIST, list);
	close(fd);
	return rc;
}
```

对应的ioctls

```c
// IOCTLs
#define IOCTL_SEND _IOW(MAJOR_NUM, 1, fpga_chnl_io *)
#define IOCTL_RECV _IOR(MAJOR_NUM, 2, fpga_chnl_io *)
#define IOCTL_LIST _IOR(MAJOR_NUM, 3, fpga_info_list *)
#define IOCTL_RESET _IOW(MAJOR_NUM, 4, int)
```

riffa.c代码定义了几个用户操作：打开、关闭、读、写、以及复位。其实都是通过ioctl实现的。如果我们后期扩展，想加上自己的函数，调用一个指定编号的ioctl，同时在驱动里面自己写好这个驱动，就可以实现我们想要的功能了。 eg，

```c
#define IOCTL_XXX _IOR(MAJOR_NUM, 5, XXX)
```

分析代码我们看到其实这些功能实现都是想组装一个IOCTRL结构，之后通过IOCTRL把这些参数传递给下层驱动进行控制。

这些参数应该是在每次读写开始的时候都要写到FPGA里面进行设置的。

另外，riffa.c是编译成静态库文件(.a)，供大家调用的，其实使用的时候直接包含进来这个riffa.c的源文件也可以。

当然我们也可以使用动态调用.so文件。

## 三、写在最后的话

```css
至此，Linux下PCI设备驱动开发详解（一）-（八），详细介绍了linux下驱动、内核、FPGA、用户态的理论知识，以及结合著名的开源RIFFA框架，分析了源代码的细节、框架、层次，做到了理论结合实践的最佳应用。

欢迎大家随时留言、沟通交流，详情+V:beijing_bubei(备注来意)
```

## 四、参考资料

https://juejin.cn/post/7321912587729518631
