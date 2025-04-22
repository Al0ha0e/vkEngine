三部分：
- 资源管理：frame graph实现，声明每个pass输入输出的资源，创建并维护资源
- 材质管理：SPIRV reflect，解析材质，根据材质所需创建pipeline和descriptor set
- 二者之间整合，把资源绑定到对应的ds上

### 使用Vulkan特性

- bindless
- dynamic rendering
- buffer device address

### 坑

如何区分VARIABLE DESCRIPTOR COUNT
https://www.reddit.com/r/vulkan/comments/1jelm0z/spirvreflect_and_bindlessruntime_arrays/
https://github.com/KhronosGroup/SPIRV-Cross/issues/2171#issuecomment-1607198720


### TODO

实现descriptor set回收