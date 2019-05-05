# 操作系统lab 3实验报告



## 一、思考题

**Thinking 3.1**

**为什么我们在构造空闲进程链表时必须使用特定的插入的顺序？(顺序或者逆序)**

逆序插入，调用env_alloc函数的时候，就会返回envs[0]



**Thinking 3.2 思考env.c/mkenvid 函数和envid2env 函数**:

**• 请你谈谈对mkenvid 函数中生成id 的运算的理解，为什么这么做？**

**• 为什么envid2env 中需要判断e->env_id != envid 的情况？如果没有这步判断会发生什么情况？**

```c
//mkenvid中的id生成运算 
(++next_env_id<<(1+LOG2NENV))|idx 
//mkenvid函数当前被调用的次数，它确保了每一次调用mkenvid函数都能生成不同的id。
++next_env_id 
//代表e在envs数组中的位置，确保可以通过id找到对应的env结构
idx=e-envs
```

  envid所代表的进程可能已经被回收，或者是一个全新的进程，没有这一步会将错误进程调出



**Thinking 3.3**

**结合include/mmu.h 中的地址空间布局，思考env_setup_vm 函数：**

**• 我们在初始化新进程的地址空间时为什么不把整个地址空间的pgdir 都清零，而是复制内核的boot_pgdir作为一部分模板？(提示:mips 虚拟空间布局)**

**• UTOP 和ULIM 的含义分别是什么，在UTOP 到ULIM 的区域与其他用户区相比有什么最大的区别？**

**• 在step4 中我们为什么要让pgdir[PDX(UVPT)]=env_cr3?(提示: 结合系统自映射机制)**

**• 谈谈自己对进程中物理地址和虚拟地址的理解**

​	因为UTOP之上的所有映射对于任意一个地址空间都是一样的，只有为每个进程都拷贝一份内核才能使用的虚页表，进程才会有成为临时内核的资格，才可以发出申请变成内核态下运行的进程。

​	ULIM是0x80000000，是操作系统分配给用户地址空间的最大值，UTOP是0x7f400000，是用户能够操控的地址空间的最大值。UTOP到ULIM这段空间用户不能写只能读，也应属于“内核态”，是在映射过程中留出来给用户进程查看其他进程信息的，用户在此处读取不会陷入异常。

​	UVPT需要自映射到它在进程的pgdir中对应的页目录地址。这样当我们需要将UVPT这块区域的虚拟地址转换为物理地址时，就能方便地找到对应的页目录



**Thinking 3.4 思考user_data 这个参数的作用。没有这个参数可不可以？为什么？（如果你能说明哪些应用场景中可能会应用这种设计就更好了。可以举一个实际的库中的例子）**

​	不可以。

​	user_data这个参数指向需要被加载的env结构，告诉了我们这个进程的页目录地址。



**Thinking 3.5 结合load_icode_mapper 的参数以及二进制镜像的大小，考虑该函数可能会面临哪几种复制的情况？你是否都考虑到了？** 

1. 将binary内容复制到[va，va+BY2PG-offset]
2. 将binary内容复制到v+BY2PG-offset以后每一个整个的页面
3. 当binary内容不满一个页面时，将它剩下的内容复制进页面，并不这个页面剩下的空间置0



**Thinking 3.6 思考上面这一段话，并根据自己在lab2 中的理解，回答：**

**• 我们这里出现的” 指令位置” 的概念，你认为该概念是针对虚拟空间，还是物理内存所定义的呢？**

**• 你觉得entry_point其值对于每个进程是否一样？该如何理解这种统一或不同？**

虚拟空间。

一样，因为它对应的是进程的PC储存在逻辑地址空间中的位置，对每个程序来讲是固定的。



**Thinking 3.7 思考一下，要保存的进程上下文中的env_tf.pc的值应该设置为多少？为什么要这样设置？**

env_tf.pc值应该被设置为上一个进程的cp0_epc值。

因为这样就记录下了上一个进程发生中断的位置，当上一个进程再次开始时，可以从发生中断之后的指令开始执行。



**Thinking 3.8 思考TIMESTACK 的含义，并找出相关语句与证明来回答以下关于TIMESTACK 的问题：**

**• 请给出一个你认为合适的TIMESTACK 的定义**

**• 请为你的定义在实验中找出合适的代码段作为证据(请对代码段进行分析)**

**• 思考TIMESTACK 和第18 行的KERNEL_SP 的含义有何不同**

​	TIMESTACK是发生中断时操作系统用来保存现场的栈顶指针。

![1555907217465](C:\Users\yusong\AppData\Roaming\Typora\typora-user-images\1555907217465.png)

​	当excpetion code==0时sp是0x82000000，当excpetion code！=0时，sp中存储的是KERNEL_SP的值。

​	KERNEL_SP是产生非中断异常时所用到的栈指针，TIMESTACK则是发生中断异常时用到的栈指针，在本实验中，只有当exception code不是0，即不是中断导致的异常时，保存的上下文才是在KERNEL_SP中。



**Thinking 3.9 阅读 kclock_asm.S  文件并说出每行汇编代码的作用**

```
#include <asm/regdef.h>
#include <asm/cp0regdef.h>
#include <asm/asm.h>
#include <kclock.h>

.macro  setup_c0_status set clr
        .set    push
        mfc0    t0, CP0_STATUS
        or      t0, \set|\clr
        xor     t0, \clr
        mtc0    t0, CP0_STATUS
        .set    pop
.endm

        .text
LEAF(set_timer)

        li t0, 0x01
        sb t0, 0xb5000100 //向0xb5000100的位置写入1
        sw      sp, KERNEL_SP
setup_c0_status STATUS_CU0|0x1001 0 // 触发4号中断
        jr ra // 返回

        nop
END(set_timer)
```



**Thinking 3.10 阅读相关代码，思考操作系统是怎么根据时钟周期切换进程的。**

​	在时钟中断产生时，检查当前运行的进程的时间片是否用完。

​	如果没用完就继续运行，如果用完了就切换到下个就绪的进程。



## 二、实验难点

1. 在进行调度函数的填写时，没有将就绪的进程放入env_sched_list中，没有考虑清楚双队列的调度情况
2. 在填写envid2env函数的时候，没有考虑清楚特殊情况的处理 ，de了很久的bug才发现是一个小问题
3. 填写load_icode_mapper这个函数的时候，遇到的困难较大



## 三、体会与感想

个人感觉这次的实验还是很难的，尤其是在加载二进制文件那边，完全摸不清楚头脑，感觉指导书在这一块的说明上略显模糊。

在进行调度的时候，碰到了好几种too low的情况，核心问题其实就是没有将就绪进程插入到env_sched_list中，或者说没有弄明白如何使用双队列进行调度。