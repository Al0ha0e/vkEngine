# 实体的生命周期

本文按当前实现说明实体从创建、脚本运行到销毁的过程，并分别列出 C# 与 C++ 侧的生命周期相关接口。实体由 C++ 的运行时 registry 持有，C# 的 `EntityScript` 通过 `UInt32 Entity` 关联实体；释放脚本对象和销毁实体是两个不同的操作。

## 1. 实体创建

### 1.1 实例化预制体

C# 通过 `SceneManager.InstantiatePrefab(prefab, position, rotation, scale, parent)` 提交请求，原生互操作层将其转为 C++ 的 `SceneManager::RequestInstantiate`。

- 请求保存预制体资源句柄及挂接参数，返回 `true` 只表示成功入队，不表示实例化已经完成，也不返回新实体句柄。
- `parent` 默认为 `UInt32.MaxValue`，对应 C++ 的 `entt::null`，表示挂到世界根。位置、旋转和缩放替换预制体根的局部变换；指定父实体时，这些值相对于父实体。
- 入队时检查管理器状态、非空资源句柄、父实体及变换参数；资源加载和实例化在执行队列时进行。父实体必须有效、具有 `Transform`，且未被标记待销毁，执行时也会重新检查。

运行帧在处理销毁请求之后、C# `Update` 之前调用 `ProcessInstantiationRequests()`。它取出当前队列快照，逐项执行：

```text
LoadSceneData(prefab) → Clone() → PrepareSceneData(data) → Instantiate(data, options)
```

缓存预制体通过 `Clone()` 复制后再准备资源，每次实例化创建独立的实体和组件状态。处理失败会记录日志，目前没有向 C# 返回最终结果的回调或任务对象。

处理本批请求时，`Start` 中新提交的实例化请求进入下一批，留到下一运行帧。暂停期间保留队列，退出时清空。初始场景的 `Start` 在主循环开始前执行，其中提交的请求可以在首个运行帧处理。

C++ 也可以直接调用 `Instantiate(data, options)` 同步实例化已经处于 `Ready` 状态的 `SceneData`。省略 `rootTransform` 时保留数据中的根变换；提供它时要求数据只有一个根。`Instantiate` 只返回成功或错误，不返回实体映射或根实体列表。

### 1.2 创建空实体

C++ 的 `SceneManager::CreateEntity(name, pos, scl, rot, isStatic)` 同步执行：

1. 在 registry 中分配实体。
2. 添加 `GameObject`，保存名称和静态标记。
3. 添加 `Transform`，设置初始变换，父实体默认为空。
4. 返回 `entt::entity`；管理器正在退出时返回 `entt::null`。

这里的“空实体”仍然包含 `GameObject` 和 `Transform`。这条路径不加载场景数据、不添加脚本，也不会触发 C# `Load` 或 `Start`。编辑器的创建实体操作使用此接口。

当前 C# 尚未开放创建空实体的接口。

### 1.3 场景初始加载

引擎和编辑器在初始化资源、脚本及场景管理器后，读取配置中的默认场景，主循环开始前同步执行：

```text
LoadSceneFile(defaultScenePath) → PrepareSceneData(data) → Instantiate(data)
```

`LoadSceneFile` 读取并校验场景 JSON；`PrepareSceneData` 展开预制体引用、校验和加载组件资源，使数据进入 `Ready` 状态。初始场景不经过运行时实例化请求队列，可以包含多个根实体。当前入口在上述步骤返回错误时报告致命错误。

场景初始加载与预制体实例化最终共用下面的流程。

### 1.4 Instantiate 的共同创建顺序

1. 校验 `Ready` 数据和挂接参数，准备并编码本批全部脚本数据；这些校验在分配运行时实体前完成。
2. 分配本批全部实体，创建 `GameObject` 和 `Transform`，建立数据实体到运行时实体的内部映射。
3. 建立父子关系及外部挂接，计算世界变换。
4. 构造其余原生组件，构造函数接收对应的 Data 并完成向渲染、物理、音频等系统的注册。
5. 将本批脚本加载数据一次性传给 C# `SceneManager.Load`。生成的 reader 构造脚本、解析导出字段，然后由管理器注册脚本。
6. 全部脚本加载完成后，仅对本批实体调用 `SceneManager.Start`，随后返回。

因此 `Start` 执行时，本批原生组件已注册，本批脚本已完成加载。没有脚本数据时不会调用 C# `Load` 和 `Start`。初始加载时的 `Start` 不受运行帧的暂停判断控制，编辑器初始处于编辑状态时也会执行。

组件注册不返回状态码，C# `Load`、`Start` 不捕获脚本异常；此阶段没有统一的事务回滚。二进制加载协议见 [场景加载互操作](interop.md)，数据准备与预制体规则见 [场景数据](../scene_data.md)。

原生运行时组件禁止复制，Data 类型仍可复制。组件的移动构造和移动赋值转移已有状态及注册句柄，不重新注册，也不改变所属实体；不能用移动把组件移交给另一实体。移动后的源组件只用于销毁或再次赋值，不再执行卸载或其他组件操作；卸载由当前持有资源的组件执行。移动赋值会释放目标原有的注册资源，因此与组件增删一样，只应在主线程安全的结构变更阶段进行。

组件默认允许 EnTT 搬移填补删除空位。`SkeletonAnimator` 的渲染回调仍捕获组件地址，因此显式使用 `in_place_delete`；手动移动时会重新绑定回调。组件析构仍不替代系统管理的卸载流程，Reset 继续直接清理系统状态和组件资源。

## 2. C# 侧生命周期钩子

脚本继承 `EntityScript`，按需覆盖 `IScriptLifecycle` 中的五个方法。基类实现均为空。

| 钩子 | 当前调用时机与语义 |
| --- | --- |
| `Start()` | 本批脚本全部加载并注册后调用。在正常实例化流程中调用一次；分发器本身不维护“已 Start”标记。 |
| `Update()` | 每个运行帧在销毁请求和实例化请求处理后调用。 |
| `FixedUpdate()` | 按固定时间步调用，一帧可能执行零次或多次。C# 先派发物理接触事件，再调用脚本钩子；之后 C++ 才执行该步物理模拟。 |
| `LateUpdate()` | C# 已提供钩子、分发入口和导出函数指针，但当前引擎及编辑器主循环尚未调用。 |
| `Unload()` | 实体实际回收前调用，此时本批原生实体、组件和父子关系仍然存在。随后管理器注销并释放脚本。 |

当前运行帧的主要顺序是：

```text
处理销毁请求 → 处理实例化请求（含新脚本 Start）→ Update
             → 零次或多次（接触事件 → FixedUpdate → 物理模拟）
             → 音频更新 → 渲染
```

引擎暂停时仍处理销毁请求，但不处理实例化队列、`Update` 和固定步更新；编辑器仅在 `Run` 状态执行这些运行步骤。

`LifecycleMask` 根据脚本类型实际覆盖的方法计算并缓存，`Start`、`Update`、`FixedUpdate`、`LateUpdate` 只注册到对应分发列表。`Unload` 由清理路径遍历该实体的全部已注册脚本调用，不依赖这四个列表。实体之间以及同一实体内各脚本的执行顺序没有显式保证，不应依赖父先于子等顺序。

普通生命周期分发使用脚本快照，调用前检查脚本仍已注册、尚未卸载且未 `Dispose`。仅标记实体待销毁不会立即停止回调，例如 `Update` 中请求销毁后，该实体脚本仍可能参与本帧的固定步更新。

`Dispose()` 是托管脚本的资源清理入口，不是实体销毁请求：直接调用它会注销该脚本，但不会自动调用 `Unload()` 或删除原生实体。自定义清理可覆盖 `Dispose(bool disposing)`，并调用基类实现以完成注销和已释放标记；托管对象的内存最终仍由 GC 回收。

## 3. C# 驱动的实体销毁

### 3.1 提交请求并标记子树

C# 可以调用 `SceneManager.DestroyEntity(entity)` 销毁指定实体，或在脚本内调用 `DestroyEntity()` 销毁自身所属实体。两者通过原生接口进入 C++ `SceneManager::DestroyEntity`。

请求提交时遍历当前整棵子树，将有效实体加入 `pendingDestroy` 集合，按完整实体句柄去重。此时不会断开父子关系、更新变换、卸载脚本或删除组件。可通过 `IsPendingDestroy` 查询标记；实体回收后标记会被移除，因此该查询不能代替实体有效性检查。

### 3.2 帧边界清理 C# 脚本

每帧在 C# `Update` 之前调用 `ProcessDestroyRequests()`，暂停时也处理。C++ 将当前待销毁集合复制为快照，通过 `ScriptManager::UnloadEntities` 向 C# 传入实体数组指针及数量。

C# 按实体逐个清理，每个实体的顺序为：

1. 取得该实体的脚本快照，逐个标记进入卸载并调用 `Unload()`。
2. 按实体从脚本集合及各生命周期分发列表中注销。
3. 逐个调用脚本的 `Dispose()`。
4. 注销该实体的 `RigidBody`、`Sensor` 物理回调。

单个脚本的 `Unload` 或 `Dispose` 抛出异常时记录错误并继续清理。回调中新提交的销毁请求不属于当前快照，正常运行时留到下一帧；重入保护避免嵌套执行销毁处理。

### 3.3 回收 C++ 实体与组件

整批 C# 清理返回之前，本批全部原生实体、组件和父子关系都保留。随后 C++ 对快照中的每个实体执行：

1. 卸载原生组件在引擎子系统中的注册，包括调用相应组件的 `UnloadFromEngine` 和移除灯光。
2. 删除 C++ 保存的该实体脚本状态。
3. 从仍有效的父实体的 `children` 中移除引用。
4. 调用 `registry.destroy(entity)` 删除组件和实体。
5. 从 `pendingDestroy` 中移除标记。

待销毁集合无序，不保证父子实体的清理先后顺序。这里保证的是整批托管清理先于整批原生回收。

### 3.4 退出时的收尾

`SceneManager::Dispose()` 先设置退出标记，使 `CreateEntity`、`Instantiate` 和 `RequestInstantiate` 不再接受创建操作，并清空实例化队列。之后将场景实体全部标记销毁，循环处理剩余销毁请求，直到集合为空。

最后调用 C# `SceneManager.Unload()` 清理剩余脚本集合及物理回调，移除物理更新监听并清空 registry 等场景状态。退出时没有下一帧，因此不会把剩余销毁请求留给后续帧。

## 4. C# 侧开放接口

整场景批量重置另有 `SceneManager::Reset()` / `Engine::Reset()`，直接清空各系统注册，不执行本节前述的逐实体卸载流程，也不调用脚本 `Unload` / `Dispose`。调用约束、资源边界和清理顺序见 [系统 Reset](../reset.md)。

### 4.1 游戏脚本可调用的接口

以下接口位于 `vkEngine.EngineCore`。

| 接口 | 用途 |
| --- | --- |
| `SceneManager.InstantiatePrefab(UInt64 prefab, NVec3 position, NQuat rotation, NVec3 scale, UInt32 parent = UInt32.MaxValue) -> bool` | 提交预制体实例化请求，返回是否成功入队。 |
| `SceneManager.DestroyEntity(UInt32 entity) -> void` | 请求销毁指定实体及其当前子树。 |
| `SceneManager.IsPendingDestroy(UInt32 entity) -> bool` | 查询指定实体是否已标记待销毁。 |
| `EntityScript.Entity : UInt32` | 当前脚本所属实体的只读句柄。 |
| `EntityScript.DestroyEntity() -> void` | 请求销毁当前脚本所属实体及其当前子树。 |
| `EntityScript.IsPendingDestroy : bool` | 查询当前脚本所属实体的待销毁状态。 |
| `EntityScript.LifecycleMask : ScriptLifecycleMask` | 当前脚本覆盖的生命周期钩子掩码。 |
| `EntityScript.Start/Update/FixedUpdate/LateUpdate/Unload() -> void` | 可覆盖的生命周期钩子，调用时机见第 2 节。 |
| `EntityScript.Dispose() -> void`、`protected virtual Dispose(bool disposing)` | 释放脚本自身资源；不负责销毁原生实体。 |

当前没有 C# 创建空实体、同步实例化、取得异步实例化结果或切换初始场景的对应接口。原生函数表未注册时，实例化和待销毁查询返回 `false`，销毁调用不执行操作。

### 4.2 导出给 C++ 的生命周期入口

这些入口通过 `SceneManager.GetFunctions()` 返回的 `SceneManagerFunctions` 函数表开放，带有 `[UnmanagedCallersOnly]`，用于 C++ 驱动托管生命周期，不供普通 C# 代码直接调用。

| 入口 | 用途 |
| --- | --- |
| `SceneManager.Load(ScriptLoadData* data, UInt32 cnt)` | 批量构造、解析并注册脚本。 |
| `SceneManager.Start(UInt32* entities, UInt32 cnt)` | 仅分发指定批次实体的 `Start`。 |
| `SceneManager.Update()`、`FixedUpdate()`、`LateUpdate()` | 分发对应运行钩子；`LateUpdate` 尚未接入主循环。 |
| `SceneManager.UnloadEntities(UInt32* entities, UInt32 cnt)` | 清理指定实体批次的脚本和物理回调。 |
| `SceneManager.Unload()` | 清理剩余全部脚本、物理回调和分发集合。 |
| `SceneManager.Reset()` | 直接清空脚本、物理回调和分发集合，不执行用户卸载或释放钩子。 |

## 5. C++ 侧开放接口

以下管理器位于 `vke_common`；`SceneManager` 的实例方法通过 `GetInstance()` 调用。

| 接口 | 返回值及用途 |
| --- | --- |
| `SceneManager::CreateEntity(std::string &name, glm::vec3 pos, glm::vec3 scl, glm::quat rot, bool isStatic)` | 实例方法；返回 `entt::entity`，同步创建空实体。注意参数顺序为位置、缩放、旋转。 |
| `SceneManager::Instantiate(const SceneData &data, const InstantiateOptions &options = {})` | 静态方法；返回 `SceneResult<void>`，同步实例化 `Ready` 数据并完成脚本 `Load`、`Start`。 |
| `SceneManager::RequestInstantiate(AssetHandle prefab, const InstantiateOptions &options = {})` | 静态方法；返回 `SceneResult<void>`，只提交预制体实例化请求。 |
| `SceneManager::ProcessInstantiationRequests()` | 实例方法，返回 `void`；在运行帧处理当前实例化队列快照。 |
| `SceneManager::DestroyEntity(entt::entity entity)` | 实例方法，返回 `void`；标记指定实体及其当前子树。 |
| `SceneManager::IsPendingDestroy(entt::entity entity) const` | 实例方法，返回 `bool`；查询待销毁标记。 |
| `SceneManager::ProcessDestroyRequests()` | 实例方法，返回 `void`；批量清理脚本并回收原生实体。 |
| `SceneManager::Init()`、`SceneManager::Dispose()` | 静态方法；分别返回 `SceneManager*` 和 `void`，初始化场景管理器及执行退出收尾。 |
| `AssetManager::LoadSceneFile(const std::filesystem::path &path)` | 返回 `SceneResult<SceneData>`，读取初始场景文件。 |
| `AssetManager::LoadSceneData(AssetHandle handle)` | 返回 `SceneResult<std::shared_ptr<const SceneData>>`，取得展开后的只读预制体缓存。 |
| `SceneData::Clone() const` | 返回可修改的 `SceneData` 副本。 |
| `AssetManager::PrepareSceneData(SceneData &data)` | 返回 `SceneResult<void>`，展开引用并准备资源，使数据进入 `Ready` 状态。 |

`InstantiateOptions` 包含 `parent`（默认 `entt::null`）和可选的 `rootTransform`（`std::optional<TransformData>`）。

C++ 的 `ScriptManager` 还提供以下静态生命周期桥接方法，返回值均为 `void`：

- `Load(const CSharpScriptLoadData *data, uint32_t cnt)`。
- `Start(const std::vector<entt::entity> &entities)`。
- `Update()`、`FixedUpdate()`。
- `UnloadEntities(const std::vector<entt::entity> &entities)`、`Unload()`。

`CSharpSceneManagerFunctions` 中保留 `lateUpdate` 函数指针，但当前没有 `ScriptManager::LateUpdate()` 包装方法。反方向的原生函数表向 C# 提供 `InstantiatePrefab`、`DestroyEntity`、`IsEntityPendingDestroy` 三个实体生命周期入口，分别对应 C# 的实例化、销毁和待销毁查询接口。

## 6. 原生组件增删

C++ `SceneManager` 统一提供以下同步接口，返回 `SceneResult<void>`：

- `AddComponent(entity, ComponentType)`：创建默认组件并立即注册到子系统。
- `AddComponent(entity, const XxxData&)`：为十二种原生组件提供数据重载，复制数据并在创建组件前验证、加载资源引用。调用方负责提供符合组件字段约束的完整 Data；从 JSON 构造 Data 前仍需调用相应的 `ValidateJSON`。
- `RemoveComponent(entity, ComponentType)`：先注销子系统状态，再从 registry 删除组件。

接口拒绝无效实体、待销毁实体及管理器退出期间的修改。重复添加、删除不存在的组件均返回错误，不修改现有状态。Transform 由实体创建流程建立，不能通过这些接口移除；Script 的按类型动态增删需要单独的托管生命周期接口，目前返回明确错误。

SkeletonAnimator 必须通过数据重载提供材质、网格和骨骼；按枚举默认添加时返回错误。其他原生组件提供默认参数。编辑器的 Add/Remove Component 菜单直接使用 SceneManager；尚无动画资源选择流程，因此默认添加菜单禁用 SkeletonAnimator。

默认参数由各组件的 Data 定义，SceneManager 的枚举入口只负责分派。Camera、物理组件和灯光的 Data 默认构造即可得到对应的默认参数，其中 CharacterController 默认使用胶囊体。RenderableObjectData 的普通默认构造保留空资源引用，`RenderableObjectData::Default()` 则提供内置球体和默认材质的句柄，实际资源验证、加载仍由添加流程负责。

场景实例化与单组件添加共用原生组件构造、注册逻辑；整实体销毁与单组件移除共用原生卸载、删除逻辑。本批原生组件全部注册完成后再加载和启动脚本。

Camera 和 Transform 使用 EnTT 默认紧凑存储，删除其他组件可能搬移它们。Camera 的 resize 和选中回调保存 registry 与 entity，调用时重新查询组件；卸载时仍须注销回调，registry 的生命周期覆盖整个注册期间。渲染单元持有模型矩阵副本，不能缓存 Transform 或其 model 的地址。运行时变换应通过 `SceneTransformSystem` 修改，以同步渲染、阴影、UI 和子节点；直接调用 Transform 的局部方法只修改组件本身。SkeletonAnimator 的根运动也通过该系统更新，在帧 fence 完成后、相机和灯光等数据上传前执行。

移除 RigidBody 或 Sensor 时，通过新增的 `UnregisterComponentCallbacks(entity, componentType)` 托管导出，在原生 BodyID 仍可查询时清理对应类型的回调，不卸载实体脚本或另一种物理组件的回调。旧的 C# 物理包装对象仍持有旧 BodyID，组件重加后需要重新获取包装对象并订阅回调。原生与托管函数表必须一同重新编译、部署。

组件添加不执行全局 WaitIdle：注册修改 CPU 状态或创建新资源，相机、灯光、文字和动画的帧缓冲更新沿用 FrameGraph::Sync 后的同步路径。相机、灯光、文字移除同样不执行全局等待。单独移除 RenderableObject 或 SkeletonAnimator 暂时保留 WaitIdle，因为它们可能分别释放最后一个网格引用或组件自有的骨骼 GPU 缓冲；后续应由资源延迟回收机制替代这类销毁等待。资源首次加载中的上传 fence 等待仍由原有资源加载器负责。这些接口只允许主线程同步调用，不能在遍历受影响的组件池或渲染回调中修改结构；当前尚未向 C# 游戏脚本开放组件增删函数。

## 7. 访问规则与待完善事项

- 标记待销毁不会限制普通组件访问和回调；`Unload` 中仍可访问尚未释放的原生组件。脚本开始卸载或已经 `Dispose` 后，不再参与普通生命周期分发。
- 已失效实体的组件写入跳过、读取返回默认值，body ID 查询返回无效 ID。接受 body ID 的互操作接口直接调用 Jolt，由 Jolt 处理句柄有效性，不额外检查和加锁。
- C# 尚无修改父子关系的接口。后续增加时，应检查被移动实体和目标父实体的待销毁状态，禁止修改待销毁子树；C++ 的 `SceneTransformSystem::SetParent` 目前也没有此检查。
- C# 创建空实体及获取异步实例化结果的接口、主循环中的 `LateUpdate` 调用尚待补充。
- 生命周期分发仍复制回调列表，因为 `Dispose` 会直接注销脚本；若要去掉快照，需要一起调整注册和注销时机。
- 物理回调在托管卸载时按实体清理，并在原生刚体、传感器回收前按组件再次清理，避免本批后续 `Unload` 重新订阅前面实体的回调而造成残留。
- 一个脚本在 `Unload` 中 `Dispose` 同实体的另一个脚本时，清理快照仍可能调用后者的 `Unload` 和 `Dispose`。`TryBeginUnload` 只检查是否已开始卸载，尚未检查已释放状态。

实现入口：C++ 的 [scene.hpp](../../include/scene.hpp)、[scene.cpp](../../src/scene.cpp)、[script.hpp](../../include/script.hpp)；C# 的 [SceneManager.cs](../../csharp/EngineCore/SceneManager.cs)、[Script.cs](../../csharp/EngineCore/Script.cs)。
