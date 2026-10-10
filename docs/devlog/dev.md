# dev 分支的修改日志

不是正式的设计文档，语言组织比较随意，版本和日志按时间顺序排列，标题下方的日志**不是**标题所示版本进行的修改

## 75d85ceff78a29433b8bdcce10cf000cfef7d255 editor log panel

引擎组件序列化要重构，分以下几步：
- 把light修好，scene不持有light数据，lightmanager持有全部
- 把glyph修好，同上
- 把每个组件的纯数据提出来，单独做序列化和反序列化
- 提一个scenedata结构出来，按列存所有组件的序列化数据，统一反序列化，scenedata包括以下结构：
  - 组件数据，列存所有组件数据
  - entity对组件引用数据
  - 每个entity是否引用其他scenedata，若引用，它对应的组件数据会覆盖被引用的scenedata默认数据
  - entity之间的父子关系
- scenedata资源化，由一个单独的类似assetmanager的东西管理，分三个加载步骤
  - 反序列化，解析JSON/二进制数据，构造纯数据
  - 实例化，把scenedata里引用的其他prefab展开添加进组件列表里
  - 资源加载，用资源管理器加载所有组件依赖的资源

这一轮修改完成light的重构，留了一些暂时性问题：
- light用于序列化的数据以组件形式存在entt里，和lightmanager的形成了两个版本
- 序列化用的entt，但是编辑器和C#脚本直接改的lightmanager的版本

后面会把light组件给变成纯数据放scenedata里，引擎端只要lightmanager的，只有序列化/反序列化流程才用到scenedata

## 087696926f97120f34c89c1d0b31117e94a13d7e light data refactor

glyph修好了，现在只有loadtoengine的时候才会实际计算glyph数据，不然就只是存了字符串和颜色，这个也比较容易拆出纯数据类

## f0297deb85cf742697d1f5675510db6c070707e1 glyph data refactor
## f7ac11272799fddf532776a93b1066e7076f7cbe frame graph profiler（从render分支合并而来）

这一轮更改把每个组件的纯数据部分都单独抽出来了，scene的纯数据也抽出来了。
还留下一个问题，transform 中父子级的解析以及相应的transform更新到底是在data加载时进行，还是在scene加载scenedata的时候进行，暂时没想好，目前倾向于在加载scenedata时进行。下一步就是scenedata的资源化。另外，下一个版本里，loadtoengine就是把scenedata加载到scenemanager中的唯一结构中，最后只会剩两种，一种是数据，另一种是全局唯一的加载到引擎的组件状态，现在中间还剩了个完成加载scenedata没有loadtoengine的scene是多余的，这也是为啥光源相关组件目前仍保存有光源结构体的副本，把中间状态去掉会清爽很多。

## 1ced4e83d10c337ea789d3f07fb04e4573ce29fd ecs refactor1

这一轮修改去掉了scene，现在scenemanager维护加载到引擎的实体和组件，scenedata表示待加载到引擎的实体和组件状态，还去除了多余的layer标记。另外独立的实体id只在序列化和反序列化的阶段发挥作用，引擎内只有一个entt分配的entity，这个每次运行都可能不同，然后在序列化的时候重新构建序列化的id来标记父子关系（实际之前的id也只有标记父子关系的作用，但是引入了很多运行时的负担）

有一些遗留问题：
- 现在scenedata加载到引擎里还是分了两步，第一步是构造对应的组件，第二步是调用组件的loadtoscene，这个和scenemanager预期表示已经loaded to engine的状态还是有点不一致
- entity对应脚本状态的生命周期和组件的生命周期对不上，这个需要后面专门设计一下
- scenemanager应该单独开一个 AddComponent 函数给编辑器和C#用，现在的 AddComponent 是编辑器单独实现的

这一轮修改花费了很多时间和精力来确保重构之后功能一切正常

## 6157dc39df73192f7d4d61fdc68e5c1fe68b89ec ecs refactor2

最近几轮在真正实现scenedata资源化之前要先解决C#脚本 EntityScript 的序列化/反序列化和状态同步问题，主要有几点：
- 场景加载的时候需要从C++侧加载脚本状态到C#
- 编辑器需要在C++的UI面板和C#之间双向同步
- 编辑场景保存的时候需要从C#到C++的同步

需要以下能力：
- 为 C# 的 EntityScript 生成元数据
- C++ 侧能够识别元数据，在 JSON 转内存二进制和编辑器双向同步的场景下需要用到元数据

所以需要以下几步：
- C# 侧实现元数据的反射和导出
- C++ 按元数据把 JSON 转成内存二进制格式，能够直接传指针给C#反序列化
- C++ 编辑器端按元数据生成UI控件，并且与C#间进行双向同步

这一轮基本实现了第一步

## 3b7d1d61a57c03d93a924e8197ed130fd42feb8c C# metadata export

这一轮需要把 EntityScript 反序列化从 JSON 转成二进制的方式

需要从 ScriptManager 先加载脚本的信息，然后在加载的时候按名字匹配

这里还有一个问题，C#会引用一些C++的类型信息，但是C#侧反射是运行时通过json解析动态加载进来，所以需要一个表来记录C++侧的类型信息，然后能够用字符串按名称动态索引

具体的设计文档写在 docs\type_info.md 和 docs\interop.md 里

## 09cdd851f475a9bf491c9c47e49139e9419687fe C# entity script binary packing & script manager type info management

之前C#端scenemanager的start会调用所有注册的start，但是现在SceneManager::loadEntitiesToEngine会被多次调用，其中调用C#的start就会导致场景中所有注册的start被多次调用，这一版先修了这个

还发现了一个问题，C#的dispatch会先把callback复制到list里，避免回调里改动集合。现在实体的延迟销毁已经实现了，但Dispose仍会直接注销脚本，所以暂时还不能去掉这个复制，后面要把注册和注销的时机一起理一下，细节在docs\script\lifecycle.md 里

## e6ceb1cda0733b8a418a7b481a9ef8eccd895a4e assetref

实现了scenedata的资源化以及实例化接口，相关设计细节比较多，在 `docs\script\lifecycle.md` 和 `docs\scene_data.md` 里

## 20dc77a0ad7b227ed8dd8aecb5aaa800f304821b scenedata resource & prefab instantiate

这一轮把增删组件的功能收回了 scenemanager 进行统一管理。但是还遗留了一些问题：
- 删除 RenderableObject 和 SkeletonAnimator 还需要 vke_render::Renderer::WaitIdle() 这个很有可能会导致卡死
- Camera和Transform组件用 static constexpr bool in_place_delete = true 因为有些其他部分依赖了这两个组件的真实地址，但是entt可能在删除某些组件的时候挪动其他组件的位置
- C# 部分还没有接通创建空实体以及增删组件的接口

## ee0658e27d8c00fbc0ddc1183d624baf694e26c5 simplify light and physics component state ownership

此后的更新分以下几步

- 实现各个系统的reset，这一reset不需要考虑里面组件的实际状态，不是逐个实体或组件调用unload，仅仅就是暂停系统更新，清空系统内一系列已注册的资源句柄并释放资源，复位到引擎的初始状态，这个reset功能后面会被编辑器用到，编辑器从运行状态切换回编辑状态时会reset，但是这一步里不需要实现这些
- 把组件的loadtoengine合并进组件的构造函数里
- 统一C++组件和C#脚本组件的生命周期和相关函数
  - 在构造时接受对应的data作为参数
  - 导出当前状态到对应的data
  - start（引擎从编辑模式切换到运行模式时调用，或者运行模式时被创建/添加时调用，这一部分由引擎对应的系统控制）
  - 与C#脚本组件的一系列update一致的更新函数
  - 从引擎中卸载
- 明确系统和组件的关系，组件的生命周期由系统控制，组件自身不需要感知引擎状态，start和一系列update的时机由系统控制。组件也不需要频繁校验自身所属实体的有效性，这一部分同样由系统保证
- 实现编辑器对脚本组件的编辑
  - 去掉scenemanager里的csharpScriptStates，C#组件状态只有C#中的唯一版本
  - 编辑器编辑面板在编辑C#组件状态时用数据导出功能导出二进制状态给C++编辑器，编辑更新后再将二进制状态同步回C#
- 完善编辑器状态管理
  - 只有编辑状态才能编辑（增删/修改组件状态和资源），但不允许系统进行update
  - 运行状态可以update，但是不允许编辑
  - 从编辑状态开始运行时，导出一份所有状态的快照
  - 从运行时回到编辑状态，reset系统，并且应用之前导出的快照

这一轮实现各个系统的reset，在这过程中还发现了一些问题：
- 音频系统的音频加载没接通到资源管理系统，而是自行完成
- Spatial2DLayerManager 的算法和数据结构设计还存在问题
- 描述符集可以封装成RAII的形式

## 73e436c836b98fca82adbc29c7166ff122c5c2e5 Unify component lifecycles and add runtime script state serialization

按上面的计划实际执行的时候因为用了codex改代码，做完“明确系统和组件的关系”这一步之后代码有些乱，尤其是新增了一些在C#和C++之间传递数据的代码，接口设计和名称都乱糟糟的，这一轮主要改了这些：
- ScriptStateData 从保存 JSON 改为持有二进制数据，本来运行时只需要二进制数据，但是之前写的方式导致C++和C#之间来回传的时候会多两轮二进制数据和JSON之间的转换
- 把C#和C++之间实体数据传输相关函数的名字和顺序都捋顺了。

最近九次提交用的是astra的high，开发体验比以前（GPT-5.6-sol）顺畅了一些，但主要还是存在几个问题：代码方面，一个是一些代码喜欢暴力枚举，即使已经有了对应的map或者set，另外就是喜欢在函数里加一大堆检查，但没有考虑这个函数在整个流程里的位置（之前是否检查过或者逻辑上不存在出问题的可能）。文档方面：喜欢在我的文档里乱涂乱画，尤其是在某些设计A变动为B的时候，会特别在文档里加一句“不采用A”，即使设计A本身特别离谱（可能是AI之前的产物），而且代码里也找不到一丝A的痕迹，非常影响阅读体验。还有一个很有意思的现象就是它有的时候会在我这个文档里乱拉乱尿，但是没有一次是按我自己很明显的命名方式（SHA+commit message）来的。从这个版本开始我把这些问题整理到AGENTS.md里，不知道之后会不会好一些

不知道在现在这个时候还用我自己人力审阅代码还有没有意义，但迄今为止我还是坚持这么做，而且基本每个版本的提交能一眼看出2-3出明显的暴力枚举，我现在会让AI自己先找一遍有没有复杂度或者废代码的问题，自己先改顺了以后我再大致扫一遍。也许不看代码顺其自然让AI自己改是一种更快的方式，但是在我这个项目里还是放不下，果然还是希望写出来的东西能按我的想法来。


后面可能需要专门的重构，来检查AI引入的这些乱七八糟的校验