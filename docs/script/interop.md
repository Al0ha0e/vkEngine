# EntityScript 场景加载互操作

## loadScene 流程

1. `ScriptManager::init` 在加载 `gameAssemblyPath` 后，读取
   `gameScriptTypeInfoPath`，将导出的脚本 `TypeInfo` 按完整类名加载到 map。
2. `SceneManager::Instantiate` 接受 Ready 状态的 SceneData，根据 `className` 查找
   `TypeInfo`，再调用 `TypeInfo::EncodeBinaryFromJson` 把场景 JSON 中的
   `data` 编码为二进制。编码失败直接返回错误。
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
   `vkEngine.Generated.EntityScriptBinaryReaders.Parse` 同步解析，不复制整个缓冲区，也不保留原生指针。生成的 reader 直接构造
   脚本并按确定的布局解析字段；加载热路径不再解析 JSON，也不再逐字段使用反射。
6. `SceneManager.Load` 返回 void，不捕获加载异常。加载完成后，运行模式仅启动本批实体；编辑模式由切换到运行的入口显式调用全场景 Start。生命周期钩子异常记录后继续分派。

启动由 C++ `SceneManager::Instantiate` 显式调度，`loadScripts` 只负责加载。批次 `Start(entities, count)` 只传带脚本的实体 ID，每个实体一次；全场景启动使用无参数 `StartAll()`，直接从托管端已注册脚本生成快照。`SceneManagerFunctions` 在 `FillData` 后追加 `StartAll` 函数指针，原生与 EngineCore 需要一起重新编译、部署。

## Script 组件的场景格式

脚本仍然以 JSON 保存在场景文件中，`data` 是与导出的脚本 TypeInfo 对应的
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
`EntityScriptMetadataExporter` 扫描生成的 `Game.dll`；导出器不会实例化脚本，只生成：

- 递归 TypeInfo 文件 `generated/Game.entityscripts.json`，供 C++ 编码场景数据。

Roslyn source generator 生成的 reader 只作为 compiler-generated source 参与编译，
不写入项目的 `generated/` 目录。

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

示例项目只需执行一次构建：source generator 在本次编译中生成并编译 reader，
随后 exporter 输出 metadata JSON。导出器在内容未变化时不会改写文件，因此后续
构建不会形成时间戳循环。`generated/` 属于构建产物，不提交版本库。

## 当前状态导出

生成器同时生成 `EntityScriptBinaryReaders.FillData(EntityScript) -> byte[]`。它与 reader 共用字段排序、类型布局及对齐规则，导出成员名别名由 TypeInfo 映射到 JSON。脚本调用 `FillData()` 得到 `EntityScriptData`。

`SceneManagerFunctions` 在 Reset 之后增加 FillData 函数指针：

```cpp
int32_t (*fillData)(entt::entity entity, void *context,
    void (*receive)(void *context, const char *className, const std::byte *data, int32_t dataSize));
```

返回 1 表示成功（无脚本也成功），0 表示托管导出失败。每个脚本调用一次 receive；className 是零结尾 UTF-8，缓冲区只在同步回调内有效。托管端先生成该实体全部快照，再执行回调。原生端通过 `ValueView::Parse` 验证，再调用 `ToJSON()` 形成 ScriptStateData。任一错误使 `ScriptManager::FillData` 及整场景导出返回错误，不回退到加载时的旧状态。

导出不调用任何生命周期钩子。该函数表变更需要原生与 EngineCore 一起重新编译、部署。
