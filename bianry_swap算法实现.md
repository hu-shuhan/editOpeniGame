the binary swap method utilizes a binary tree which pairs
up processors in order of compositing. It requires log N
stages of communication for N compositing processors. Un-
like a typical binary tree compositing algorithm where half
of the participating processors become idle after each stage
of the composition, the binary swap algorithm exploits more
parallelism in compositing so that every processor partici-
pates in all stages of the compositing process. The key idea is
that at each stage, instead of having only one processor from
each pair composite for the whole image space, the image
space is divided into two partitions and each processor takes
the responsibility for one of the two partitions. A swapping
of the partitions between the two processors is needed in
the algorithm, thus the name binary swap [7, 8]. Note that
the image partition needed for compositing becomes smaller
as we move up to the root of the binary tree. After log N
stages, the compositing completes and each processor holds
exactly 1/N of the final image. Binary swap can also take
advantage of faster nearest neighbor communications in a
tree or hypercube network: in early stages of the algorithm,
processors exchange messages with their nearest neighbors,
which is exactly when the amount of pixels transferred is the
largest. This nice property makes binary swap much more
scalable than other methods.

我想明确三件事：

1. 在合成最开始，是不是只有叶子节点在做交换合成，而所有父节点都在idle？
2. 为什么越向根节点，交换的部分就越少？而且这里似乎没有说子节点(paired)怎么向父节点传递信息？
3. 为什么最后所有节点都会拿到1/N的最终图像，那岂不是最后仍然要向根节点一起发消息（N-1条）？

这三个问题都源于把 binary swap 理解成了“传统二叉树归约合成”：叶子渲染，父节点逐层向上合成，最后根节点拿到完整图像。**但 binary swap 不是这种树形归约。** 它是一系列**对等处理器之间的两两交换与局部合成**，二叉树只是描述配对顺序的抽象。下面逐条澄清。

---

## 1. 最开始是不是只有叶子节点在工作，父节点 idle？

**不是。**  
在 binary swap 中，**所有 N 个处理器从第一阶段开始就全部活跃**，没有“父节点 idle”这回事。二叉树里的“父节点”并不是独立的处理器，而是配对关系的抽象：第 1 阶段两两配对，第 2 阶段配对的对再两两配对，以此类推。所有处理器始终都是“叶子”，也始终都在参与交换和合成。

以 N=4 为例：

- 初始：P0、P1、P2、P3 各自持有自己渲染的完整图像缓冲（带深度）。
- 阶段 1：配对 (P0,P1) 和 (P2,P3)。  
  每对中，一个负责图像左半，一个负责右半。  
  P0 把右半发给 P1，P1 把左半发给 P0；然后各自把收到的半张与本地保留的半张做深度合成。  
  结果：P0 持有左半的合成结果，P1 持有右半的合成结果；P2、P3 同理。
- 阶段 2：重新配对，比如 (P0,P2) 和 (P1,P3)。  
  此时 P0 持有左半，P2 也持有左半（但来自不同数据）。它们再把左半分成上下两半，交换其中一半，合成。  
  最终 P0 持有左上 1/4，P2 持有左下 1/4，P1 持有右上 1/4，P3 持有右下 1/4。

**每个阶段所有处理器都在发送、接收、合成。** 没有谁闲着。

---

## 2. 为什么越向根节点，交换的部分越少？子节点怎么向父节点传递？

### 为什么越向根节点交换越少？

因为每个处理器持有的图像区域在每一阶段**减半**：

- 初始：每个处理器持有整个图像空间（或它渲染的完整子图像）。
- 阶段 1 后：每个处理器只负责整个图像的 1/2。
- 阶段 2 后：每个处理器只负责 1/4。
- …
- 阶段 log N 后：每个处理器只负责 1/N。

所以每一阶段需要交换和合成的像素数量也按比例减少。早期阶段交换的是大块（整半张图），后期阶段只交换很小的分区。这就是“越向根节点，交换的部分越少”的含义。

### 子节点怎么向父节点传递？

**没有“子节点向父节点传递”这一步。**  
Binary swap 不是树形归约。每一阶段，处理器是**横向配对**，直接与另一个处理器交换数据并合成，然后更新自己负责的那一小块分区。配对关系画成二叉树，但信息不是沿着树向上汇聚，而是在每一阶段通过交换，把不同处理器的数据逐渐合并到每个处理器最终负责的分区里。

可以这样理解：  
二叉树只表示“谁和谁在第几阶段配对”。第 1 阶段是兄弟配对，第 2 阶段是表亲配对，第 3 阶段是更远的亲戚配对……最终每个处理器都独立地完成了自己那 1/N 分区的完整合成。**不存在一个根节点收集所有数据。**

---

## 3. 最后所有节点都拿到 1/N 最终图像，岂不是还要向根节点发 N-1 条消息？

**不需要。**  
Binary swap 结束时，最终图像被分成 N 个空间分区，每个处理器持有其中一个分区的**完整合成结果**。也就是说：

- P0 持有最终图像的 1/N（比如左上角）；
- P1 持有最终图像的另一个 1/N（比如右上角）；
- …
- P_{N-1} 持有最后一块。

每个处理器手里的那一块已经是正确的、经过所有相关数据合成的最终像素。**它们合起来就是完整图像，但不需要再集中到一个根节点。**  
如果最终需要把完整图像显示或保存，那可以并行地让每个处理器把自己的分区写入文件，或者通过集合通信（如 MPI_Gather）发到显示节点。但那是**输出步骤**，不是 binary swap 合成算法的一部分。Binary swap 的设计目标之一就是避免最后集中到根节点，从而保持可扩展性。

所以“最后所有节点都拿到 1/N”恰恰是优点：**合成结果分布式地存在，没有单点瓶颈。**

---

## 总结

| 你的疑问 | 实际情况 |
|---|---|
| 只有叶子在工作，父节点 idle | 所有处理器从始至终都活跃，二叉树只是配对顺序的抽象 |
| 子节点向父节点传递 | 没有树形归约，每一阶段横向配对交换并合成 |
| 越向根交换越少 | 因为每个处理器持有的图像区域每阶段减半，交换量自然减少 |
| 最后还要向根节点发 N-1 条 | 不需要，最终图像已分成 N 份，每处理器持有一份完整合成结果 |

一句话：**Binary swap 是“所有处理器一起逐层交换并缩小自己负责的分区”，不是“叶子渲染、父节点逐层向上合成、根节点收集”。**


## Binary Swap 要实现半透明体渲染所需的有序合成，需要应用程序与 IceT 算法（模仿IceT,使用iGameVis源码实现）协同完成以下步骤：

第一步：应用程序提供几何深度顺序

应用程序负责确定一个全局的、线性的进程深度排序，保证：若进程 A 的几何体在任意相机射线上位于进程 B 的前方，则该前后关系对所有相机射线均成立。这个顺序以进程 rank 数组的形式，通过 icetCompositeOrder() 传递给 IceT。

第二步：启用有序合成并确认策略支持

调用 icetEnable(ICET_ORDERED_COMPOSITE) 开启有序合成，并确保当前策略支持有序性（通过 ICET_STRATEGY_SUPPORTS_ORDERING 状态变量验证）。若使用 SEQUENTIAL 或 REDUCE 策略，该条件满足。

第三步：IceT 对进程进行 swizzle 重排

IceT 在合成开始前，根据 icetCompositeOrder 提供的深度顺序，对参与合成的进程进行逻辑 rank 的 swizzle 重排，使进程的逻辑顺序与几何体的可见性顺序一致。

第四步：Binary Swap 基于 swizzle 后的逻辑 rank 执行配对与混合

Binary Swap 的每一阶段中，进程按照 swizzle 后的逻辑 rank 两两配对，各发送一半图像给对方，并接收对方的另一半。在混合图像时，严格依据 swizzle 后的 rank 维持 over/under 关系，确保深度顺序在递归合成过程中始终被保持。

第五步：每帧更新顺序

由于几何体的可见性顺序会随相机视角变化而改变，应用程序需要在每一帧渲染前重新计算深度顺序并再次调用 icetCompositeOrder()。

核心结论：Binary Swap 算法本身不具备感知几何深度的能力，其有序性完全依赖于 IceT 框架的 swizzle 机制——应用程序提供深度排序，IceT 据此重排进程逻辑 rank，Binary Swap 在重排后的逻辑 rank 上执行配对与混合，从而保证半透明体渲染的混合顺序正确。

## 细节注意
每个rank相互传递合成时，仍然可以沿用之前的ROI方法，减少传输量。
最后各rank向rank0发送1/N数据时，也是用ROI方法，rank0不再需要排序，直接拼合所有图像就可以。
