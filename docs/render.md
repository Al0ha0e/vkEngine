三部分：
- 资源管理：frame graph实现，声明每个pass输入输出的资源，创建并维护资源
- 材质管理：SPIRV reflect，解析材质，根据材质所需创建pipeline和descriptor set
- 二者之间整合，把资源绑定到对应的ds上

### 使用Vulkan特性

- bindless
- dynamic rendering
- buffer device address
- mesh shader?

### 坑

如何区分VARIABLE DESCRIPTOR COUNT
https://www.reddit.com/r/vulkan/comments/1jelm0z/spirvreflect_and_bindlessruntime_arrays/
https://github.com/KhronosGroup/SPIRV-Cross/issues/2171#issuecomment-1607198720


### Frame Graph备忘

两条执行线：

- graphics
- compute：执行compute shader
- transfer 传输队列

#### 编译过程：

- 标记valid：
  - 找到所有输出为targetResources的Task节点标记为valid
  - 迭代找到所有valid节点的输入资源的输出节点标记为vaild

- 排序：
  - 收集第一批入度为0的Task
  - 迭代把入度为0的Task的所有出边去掉

#### 同步方法

对于使用同一资源的前后两个Task：
- Queue不同：Semaphore同步:
  - 后面一个读：需要Barrier进行所有权转移
  - 后面一个写：不需要Barrier
- Queue相同，Layout不同：Layout Transfer
- Queue相同，Layout相同：
  - 两个都是读操作：无需特别处理
  - 其他情况：插入一个Barrier
所有Task共用一个timelineSemaphore，每个Task以其自身在顺序列表中的下标为编号触发信号量


实际过程中：
- 外部资源的第一次使用（lastUsedRef==nullptr）：如果Layout不同，Image需要做Layout Transition
- lastUsedRef!=nullptr，收集属于同一个lastUsedTask的所有lastUsedRef：
  - lastUsedTask的Type和当前Task不同且不使用同一个队列：等待lastUsedTask对应的timelineSemaphore，并对每个资源进行所有权转换
  - 其他情况：对每个资源除了R-R的情况插入一个Barrier，Layout不同则进行Layout Transition

Task运行结束后：
- 更新所有用到的资源的lastUsed
- 如果存在crossQueue的资源，进行所有权转移并signal Timeline Semaphore

#### 注意事项

- Timeline Semaphore 不能重置
- 每个Pass对一个资源的操作都需要了解要对其进行的下一个操作以保证所有权转移可以正常进行，因此某一帧的最后一个操作也需要了解下一帧的第一个操作


#### 坑

SYNC-HAZARD-WRITE-AFTER-READ error with swapchain acquisition and command buffer submission
https://stackoverflow.com/questions/77871832/sync-hazard-write-after-read-error-with-swapchain-acquisition-and-command-buffer
https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/7193#issuecomment-1875960974

### TODO

Transfer支持
用vkCmdPipelineBarrier2 打包多个barrier（vkCmdPipelineBarrier 不支持每个barrier一个stageMask）
实现descriptor set回收