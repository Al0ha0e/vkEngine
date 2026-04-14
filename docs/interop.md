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