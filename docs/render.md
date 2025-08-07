三部分：

- 资源管理：frame graph 实现，声明每个 pass 输入输出的资源，创建并维护资源
- 材质管理：SPIRV reflect，解析材质，根据材质所需创建 pipeline 和 descriptor set
- 二者之间整合，把资源绑定到对应的 ds 上

### 使用 Vulkan 特性

- bindless
- dynamic rendering
- buffer device address
- mesh shader?

### 参考资料

https://developer.nvidia.com/blog/vulkan-dos-donts/
https://docs.vulkan.org/guide/latest/synchronization_examples.html
https://docs.vulkan.org/spec/latest/chapters/synchronization.html
https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html

### 坑

如何区分 VARIABLE DESCRIPTOR COUNT
https://www.reddit.com/r/vulkan/comments/1jelm0z/spirvreflect_and_bindlessruntime_arrays/
https://github.com/KhronosGroup/SPIRV-Cross/issues/2171#issuecomment-1607198720

### 资源管理及绑定

- pipeline 资源：包括 pipeline、pipeline layout 和若干 descriptor set，对应一个 json 配置文件，其中指定了对应的 shader，管线属性和管线对应的 descriptor set，push constant 的值、大小和偏移量
- material：包括若干 descriptor set，对应一个 json 配置文件，其中制定了对应的 shader，texture 及其对应的 descriptor set（可以是 material 特有的或管线对应的），push constant 的值、大小和偏移量
- mesh: 包括顶点和索引数组，push constant 的值、大小和偏移量

### Frame Graph 备忘

两条执行线：

- graphics
- compute：执行 compute shader
- transfer 传输队列

#### 编译过程：

- 标记 valid：

  - 找到所有输出为 targetResources 的 Task 节点标记为 valid
  - 迭代找到所有 valid 节点的输入资源的输出节点标记为 vaild

- 排序：
  - 收集第一批入度为 0 的 Task
  - 迭代把入度为 0 的 Task 的所有出边去掉

#### 同步方法

对于使用同一资源的前后两个 Task：

- Queue 不同：Semaphore 同步:
  - 后面一个读：需要 Barrier 进行所有权转移
  - 后面一个写：不需要 Barrier
- Queue 相同，Layout 不同：Layout Transfer
- Queue 相同，Layout 相同：
  - 两个都是读操作：无需特别处理
  - 其他情况：插入一个 Barrier
    所有 Task 共用一个 timelineSemaphore，每个 Task 以其自身在顺序列表中的下标为编号触发信号量

实际过程中：

- 外部资源的第一次使用（prevUsedRef==nullptr）：如果 Layout 不同，Image 需要做 Layout Transition
- prevUsedRef!=nullptr，收集属于同一个 prevUsedTask 的所有 prevUsedRef：
  - prevUsedTask 的 Type 和当前 Task 不同且不使用同一个队列：等待 prevUsedTask 对应的 timelineSemaphore，并对每个资源进行所有权转换
  - 其他情况：对每个资源除了 R-R 的情况插入一个 Barrier，Layout 不同则进行 Layout Transition

Task 运行结束后：

- 更新所有用到的资源的 lastUsed
- 如果存在 crossQueue 的资源，进行所有权转移并 signal Timeline Semaphore

#### 注意事项

- Timeline Semaphore 不能重置
- 每个 Pass 对一个资源的操作都需要了解要对其进行的下一个操作以保证所有权转移可以正常进行，因此某一帧的最后一个操作也需要了解下一帧的第一个操作

#### 坑

SYNC-HAZARD-WRITE-AFTER-READ error with swapchain acquisition and command buffer submission
https://stackoverflow.com/questions/77871832/sync-hazard-write-after-read-error-with-swapchain-acquisition-and-command-buffer
https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/7193#issuecomment-1875960974

### TODO

用 vkCmdPipelineBarrier2 打包多个 barrier（vkCmdPipelineBarrier 不支持每个 barrier 一个 stageMask）
实现 descriptor set 回收
