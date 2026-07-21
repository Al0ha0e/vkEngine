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