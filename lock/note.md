## Lock指令前缀

Lock是一个指令前缀，Lock会使紧跟其后的指令变成原子指令。

LOCK指令前缀只能加在以下这些指令前面
- ADD，ADC，AND，BTC，BTR，BTS，CMPXCHG，CMPXCH8B，CMPXCHG16B，DEC，INC，NEG，NOT，OR，SBB，SUB，XOR，XADD，XCHG


### 总线锁
在多处理器环境中，CPU提供了在指令执行期间对总线加锁的手段。CPU芯片上有一条引线LOCK，如果汇编语言的程序中在一条指令前面加上前缀`LOCK`，经过汇编以后的机器代码就是CPU在执行这条指令的时候把引线LOCK的电位拉低，持续到这条指令结束时放开，从而把总线锁住，这样同一总线上别的CPU就暂时不能通过总线访问内存了，保证了这条指令在多处理器环境中的原子性。

总线锁这种做法锁定的范围太大了，导致CPU利用率急剧下降，因为使用LOCK#是把CPU和内存之间的通信锁住了，这使得锁定期间其他处理器不能操作其内存地址的数据，所以总线锁的开销比较大

### 缓存锁
如果访问的内存区域已经缓存在处理器的缓存行中，LOCK#信号不会被发送。它会对CPU缓存中的缓存行进行锁定，在锁定期间，其他CPU不能同时缓存此数据。在修改之后通过缓存一致性协议（MESI）来保证修改的原子性。这个操作被称为“缓存锁”


## CAS自旋锁
CAS是Compare and Swap的缩写，表示比较并交换，是自旋锁或乐观锁的核心操作。

xchg实现
```c
// 一次原子的compare and swap
int xchg(volatile int* addr, int newval) {
    int result;
    // 将newval值放入result，并交换result和addr的值。
    asm volatile("lock xchg %0, %1" : "+m"(*addr), "=a"(result) : "1"(newval));
    return result;
}
```

> xchg指令：两个寄存器或者寄存器和内存变量之间交换内容的指令。
> gcc内联汇编参考文档：http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html

自旋锁实现：
自旋锁包含三个操作数：当前内存值M，期望内存值E，要修改的新值U
以下面代码为例：YES的时候表示可以上锁，NOPE的时候表示锁已经被别人持有
- 调用lock时，期望内存值为YES，要修改的新值为NOPE。如果当前内存值为YES，则xchg的结果就为YES，表示上锁成功。若果当前内存值为NOPE，NOPE和NOPE交换之后结果还是NOPE，所以上锁失败，重新回到retry处尝试上锁。
- 调用unlock的时候，就只需要将锁归还即可，即将YES换进内存
```c
#define YES 1
#define NOPE 0
int table = YES;

void lock() {
retry:
  int got = xchg(&table, NOPE);
  if (got == NOPE)
    goto retry;
  assert(got == YES);
}

void unlock() {
    xchg(&table, YES);
}
```

```c
// 简化版本：
int locked = 0;
void lock() { while (xchg(&locked, 1)); }
void unlock() { xchg(&locked, 0); }
```

### 自旋锁的缺陷
- 自旋（共享变量）会触发处理器间的缓存同步，延迟增加
- 除了进入临界区的线程，其他处理器上的线程都在空转
> 因为这条，所以使用自旋锁的程序需要保证每个线程上锁之后操作的时间不会太长
> 这里应该也是被称为**乐观锁**的原因，程序乐观任务线程上锁后不会占用锁很长时间
- 争抢锁的处理器越多，利用率越低
- 获得自旋锁的线程可能**会被操作系统切换出去**
> 在抢占式调度中，线程可能运行着运行着就会被切换出去，换其他线程执行
> 一旦获得自旋锁的线程被切换出去，则会造成资源的100%浪费

### 自旋锁适用场景
- 临界区几乎不拥堵
- 持有自旋锁时禁止执行流切换（可以通过关中断和抢占实现）

适用场景：操作系统内核的并发数据结构（短临界区）

## 互斥锁（Mutex）
自旋锁的缺陷在于获取锁失败的线程仍会占用CPU资源空转，不会挂起，导致资源的浪费。

互斥锁的出现解决这一问题，可以使用下面两个系统调用进行加锁、解锁
- syscall(SYSCALL_lock, &lk);
  - 试图获取lk，如果失败，则**会将自己挂起，切换其他线程执行**
- syscall(SYSCALL_unlock, &lk);
  - 释放lk，如果有等待锁的线程就唤醒
> 操作系统通过自旋锁来确保处理锁的过程是原子的

## Futex
分析自旋锁和睡眠锁：

自旋锁：
- 更快的fast path
  - xchg成功->立即进入临界区，开销很小
- 更慢的slow path
  - xchg失败->浪费CPU资源自旋空转

睡眠锁（系统调用方式）
- 更快的slow path
  - 上锁失败，直接挂起，不再占用CPU资源
- 更慢的fast path
  - 上锁时，需要进出内核（syscall），操作较慢

综合上面两种锁的优缺点，出现了Futex：
- fast path：一条原子指令，上锁成功则立即返回
- slow path：上锁失败，执行系统调用睡眠

> - 如果获得锁，直接进入
> - 加锁失败，直接调用系统调用休眠（FUTEX_WAIT,线程休眠）
> - 解锁以后也需要调用系统调用（FUTEX_WAKE,唤醒线程）

pthread.h中的mutex使用的就是futex


> 参考链接：https://www.jianshu.com/p/2171e180bdbd

