# EntityScript 场景加载互操作

## 场景加载流程

1. `ScriptManager::init` 在加载 `gameAssemblyPath` 后，读取
   `gameScriptTypeInfoPath`，将导出的脚本 `TypeInfo` 按完整类名加载到 map。
2. 场景文件读取时，`ScriptStateData::FromJSON` 根据 `className` 查找
   `TypeInfo`，调用 `TypeInfo::EncodeBinaryFromJson` 把 `data` 编码为二进制；
   编码失败直接返回错误。`SceneData`、预制体和 SceneData 快照均持有这些字节。
   `SceneManager::Instantiate` 使用已校验的脚本数据，在创建实体前检查所需的脚本管理器是否可用，加载时直接借用其缓冲区。
3. C++ 创建本批实体、建立层级，并初始化和注册原生组件。
4. C++ 将 `CSharpScriptLoadData[]` 一次性传给 `SceneManager.Load`：

   ```cpp
   struct CSharpScriptLoadData
   {
       uint32_t entity;
       const char *className;  // UTF-8、以零结尾
       const std::byte *data;
       int32_t dataSize;
   };
   ```

   数组、类名和二进制缓冲区只保证在同步 `Load` 调用期间有效，C# 不得保存指针。
5. C# 解码类名并校验缓冲区后，将原生字段缓冲区直接交给游戏程序集内生成的
   `vkEngine.Generated.EntityScriptBinaryReaders.Parse` 同步解析。生成的 reader 构造
   脚本并按确定的二进制布局读取字段。
6. `SceneManager.Load` 返回 void，不捕获加载异常。加载完成后，运行模式仅启动本批实体；编辑模式由切换到运行的入口显式调用全场景 Start。生命周期钩子异常记录后继续分派。

启动由 C++ `SceneManager::Instantiate` 显式调度，`loadScripts` 负责加载。批次 `Start(entities, count)` 传入带脚本的实体 ID，每个实体一次；全场景启动使用无参数 `StartAll()`，从托管端已注册脚本生成快照。`SceneManagerFunctions` 将 `Start` 与 `StartAll` 相邻排列在生命周期分组中；原生与托管端的函数表布局须保持一致，修改布局时必须一起构建、部署。

## Script 组件的场景格式

脚本以 JSON 保存在场景文件中，`data` 是与导出的脚本 TypeInfo 对应的
JSON 值。Struct 使用对象，字段必须完整且不能包含未知字段；数组使用 JSON
数组。详细映射和校验规则见 [type_info.md](type_info.md)。

```json
{
  "type": "script",
  "className": "TestProj.TestCameraScript",
  "data": {
    "JumpSpeed": 5.0,
    "MoveSpeed": 4.0,
    "RotateSpeed": 2.0
  }
}
```

## EntityScript 元数据和 reader 生成

游戏项目在编译期间运行 Roslyn `EntityScriptSourceGenerator`，分析所有继承
`EntityScript` 的非抽象类，并把二进制 reader 直接加入当前 `Game.dll`。编译完成后，
`EntityScriptMetadataExporter` 扫描生成的 `Game.dll`，输出递归 TypeInfo 文件
`generated/Game.entityscripts.json`，供 C++ 编码场景数据。

Roslyn source generator 生成的 reader 作为 compiler-generated source 参与编译。

TypeInfo JSON 的完整格式见 [type_info.md](type_info.md)。生成的 reader 遵守
[binary.md](binary.md) 的 Data 布局，包括小端编码、类型对齐、零 padding、长度
和资源限制、严格 UTF-8，以及根值必须完整消费。

当前支持 `byte`、`int`、`long`、`float`、`double`、`string`、一维数组、
`NVec2/NVec3/NVec4/NQuat`，以及由这些类型组成的值类型 Struct。被 `[Export]`
标记的成员必须公开可读写，脚本必须提供接受单个 `UInt32 entity` 的 public 或 internal
构造函数。

```csharp
[Export]
public float MoveSpeed = 2.5f;

[Export("speed")]
public float Speed { get; set; }
```

示例项目的构建先由 source generator 生成并编译 reader，随后由 exporter 输出
metadata JSON。导出器仅在内容变化时写入文件。`generated/` 属于构建产物，不提交版本库。

## 当前状态导出

生成器同时生成 `EntityScriptBinaryReaders.FillData(EntityScript) -> byte[]`。它与 reader 共用字段排序、类型布局及对齐规则，导出成员名别名由 TypeInfo 映射到 JSON。脚本调用 `FillData()` 得到 `EntityScriptData`。

`SceneManagerFunctions.GetEntityScriptsData` 获取一个实体上全部脚本的类名与二进制数据，与 `GetScriptData` / `SetScriptData` 一起位于数据读写分组：

```cpp
int32_t (*getEntityScriptsData)(entt::entity entity, void *context,
    void (*receive)(void *context, const char *className, const std::byte *data, int32_t dataSize));
```

返回 1 表示成功（无脚本也成功），0 表示托管导出失败。每个脚本调用一次 receive；className 是零结尾 UTF-8，缓冲区只在同步回调内有效。托管端先生成该实体全部快照，再执行回调。原生端通过 `ValueView::Parse` 验证并复制为二进制 `ScriptStateData`。任一错误使 `ScriptManager::GetEntityScriptsData` 及整场景导出返回错误。

SceneData 快照持有二进制脚本状态。`ScriptStateData::ToJSON` 在 `SceneData::ToJSON` 文件输出边界将二进制转换回 JSON；类型缺失或二进制无效会返回错误，编辑器完成转换后才打开保存文件。

导出仅采集字段数据，保持脚本生命周期状态不变。

## 当前状态写回

`ScriptManager::SetScriptData(entity, className, data)` 接受由 `ValueEditor` 或场景数据读取、导出入口校验过的二进制，调用方须保持数据格式和互操作长度约束。C++ 将缓冲区同步借给 C#；托管端按实体和完整类名查找现有实例，再调用生成的 `EntityScriptBinaryReaders.SetScriptData`。

生成的 applier 先解码全部字段并确认缓冲区完整消费，再向现有实例赋值。`SceneManager.SetScriptData` 捕获解析错误并返回 0，实例字段保持不变；成功返回 1。写回仅涉及导出成员，保持实例身份、未导出成员及生命周期状态。导出属性的 setter 会执行；setter 抛异常时写回返回失败，已经完成的赋值和外部副作用不会回滚。

Inspector 在完成本帧字段绘制后提交二进制修改，仅在修改成功时写回实例。原生程序、EngineCore 和游戏程序集须一起构建，以保持函数表和生成代码一致。
