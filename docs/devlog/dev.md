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