### loadScene流程

- 加载所有gameobject数据并初始化组件
- 收集所有脚本状态 Scene::csharpScriptStates，并调用 ScriptManager::Load一次性传递给C#做反序列化，
  - 传递的第一个为const char **data, 是一个C风格字符串的数组，每个串表示一个需要反序列化的C#对象的信息
  - uint32_t cnt，表示字符串数组长度
- C# 端SceneManager.Load遍历每个字符串做JSON解析，并通过反射获取并构造对应的C#对象，将其中的字段进行赋值

### Script组件的存储格式

每个脚本状态在场景描述文件中表示为GameObject下的一个Component，但是没有对应的C++类与之对应，而是直接打包为一个JSON字符串供C#端解析，其组件格式为

``` json
{
    "type"："script",
    "className" : "XXX", // 表示对应的C#脚本类（具体格式为 namespace.clasname）
    "data": "XXX" //见下方C#对象状态格式
}   
```

### 传递给C#的数据格式

``` json
{
    "entity" ：1, // 表示要挂载到的entity
    "className" : "XXX", // 表示对应的C#脚本类（具体格式为 namespace.clasname）
    "data": "XXX" //见下方C#对象状态格式
}   
```

### C#对象状态格式

C#对象表示为JSON格式：

``` json
{   
    "fields" : [ //表示需要反序列化的字段
        {
            "name" : "name1", //字段名
            "val" : "val1" //对应取值，可以是字符串或数值
        },
        {
            "name" : "name2", //字段名
            "val" : "val2" //对应取值，可以是字符串或数值
        } 
        //, ...
    ]
}
```

C#端SceneManager.Load使用固定名称"Game.dll"找到游戏项目的程序集，并在其中通过反射查找className


### EntityScript 元数据生成

游戏项目在`dotnet build`完成后运行`EntityScriptMetadataExporter`，扫描生成的`Game.dll`。导出器只读取程序集元数据，不实例化脚本；所有继承`EntityScript`的非抽象类都会写入`generated/Game.entityscripts.json`。`generated/`由`tests/csharp/.gitignore`忽略。

使用`[Export]`标记需要导出的public字段或property：

```csharp
[Export]
public float MoveSpeed = 2.5f;

[Export("speed")]
public float Speed { get; set; }
```

每个脚本对象包含类名、命名空间、完整类型名，以及被标记成员的名称、导出名称、成员种类、C#完整类型名和读写能力。类型不会递归展开；C++端应优先根据`System.Single`等C#类型名完成基础类型映射。只有特殊类型或需要覆盖默认映射时才声明C++类型：

```csharp
[Export(CppType = "vke_common::Transform")]
public Transform transform { get; set; }
```
