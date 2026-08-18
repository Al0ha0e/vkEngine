# TypeInfo

`TypeInfo` 是 C++ 侧对值类型的描述。它独立于具体值，可以缓存、复用，并按类型名注册到类型表中。

它只描述：

- 类型名称和种类；
- 类型在 Data 编码中的 alignment；
- Vector 的标量类型和分量数；
- Array 的元素类型；
- Struct 的字段名称、字段顺序和字段类型。

`TypeInfo` 不保存任何具体布局结果，包括 size、offset、Array count 和 stride。C++ 将 `TypeInfo` 与一块具体 Data 交给 `ValueView` 解析，由 `ValueView` 保存该实例中所有值、字段和元素的实际 offset 与 size，并提供取值接口。

## 类型名称

每个 `TypeInfo` 都有一个非空的 UTF-8 `name`。名称是运行时类型表中的唯一键，用于 C++ 和 C# 按名称匹配类型。

- 内建类型使用固定名称：`byte`、`int32`、`int64`、`float32`、`float64` 和 `string`。
- Struct 应使用稳定的完整名称，例如 C# 的 namespace-qualified name，不能使用只在单次运行中有效的名称。
- Vector 和 Array 可以使用规范化名称，例如 `vec<float32, 3>` 和 `array<int32>`。
- 同一个类型表内不得注册两个同名但定义不同的类型。

类型名只是查找键，不表示 C++ 对象的原生内存布局。跨语言读写使用 [binary.md](binary.md) 定义的 Data 布局。

## 概念结构

```text
TypeInfo
    name
    alignment
    payload: variant<
        ByteTypeInfo,
        Int32TypeInfo,
        Int64TypeInfo,
        Float32TypeInfo,
        Float64TypeInfo,
        VectorTypeInfo,
        StringTypeInfo,
        ArrayTypeInfo,
        StructTypeInfo>

VectorTypeInfo
    scalarType
    componentCount

ArrayTypeInfo
    elementType

StructTypeInfo
    fields[]

FieldInfo
    name
    type
```

`payload` 使用 `std::variant`，同一时间只保存一种类型专属信息。`kind` 由当前 alternative 得到，可以是 `Byte`、`Int32`、`Int64`、`Float32`、`Float64`、`Vector`、`String`、`Array` 或 `Struct`。

访问类型专属信息时使用 `GetIf<T>()`；类型不匹配时返回 `nullptr`，不使用异常或 `dynamic_cast`。类型引用应指向类型表中已经存在的 `TypeInfo`，类型引用图必须无环。

## Alignment

`alignment` 是类型编码规则的一部分，供 `ValueView` 在解析具体 Data 时对齐当前位置：

| 类型 | alignment |
| --- | ---: |
| `byte` | 1 |
| `int32`、`float32` | 4 |
| `int64`、`float64` | 8 |
| Vector | 标量类型的 alignment |
| String | 4 |
| Array | `max(4, elementType.alignment)` |
| Struct | 所有字段 alignment 的最大值；空 Struct 为 1 |

alignment 可以从类型定义推导，因此不需要重复写入二进制 Schema；C++ 构建 `TypeInfo` 时计算并缓存它。

## Vector

Vector 的 `scalarType` 必须是 `Int32`、`Int64`、`Float32` 或 `Float64`，`componentCount` 必须是 2、3 或 4。分量连续存储，不进行 SIMD 或 `vec4` 补齐。

Vector 的实际 size 由 `ValueView` 根据标量编码大小和分量数计算，不保存在 `TypeInfo` 中。

## Array

Array 只通过 `elementType` 描述唯一的元素类型。元素数量、每个元素的 offset/size、数组总 size 以及可用的 stride 都属于具体实例，由 `ValueView` 从 Data 中解析。

即使元素是固定大小类型，`TypeInfo` 也不缓存 stride。Array View 可以在解析具体 Data 后保存 `dataOffset + stride`，按下标推导元素布局；动态大小元素则保存逐元素 View。两种表示都属于 ValueView。

## Struct 和字段

Struct 的 `fields[]` 按 Data 编码顺序保存。每个 `FieldInfo` 只包含：

- `name`：Struct 内唯一的 UTF-8 字段名；
- `type`：字段类型。

字段 offset、字段 size 和 Struct 总 size 都不属于 `FieldInfo` 或 `TypeInfo`。`ValueView` 按字段顺序和 alignment 规则解析具体 Data，并为每个字段建立子 View。

## TypeInfo 的 JSON 格式

TypeInfo::FromJson(json) 从一个 JSON object 递归构造 TypeInfo。格式是严格的：
必需字段不能缺少，也不能包含下表之外的字段。kind 使用小写字符串。
alignment 是推导值，不写入 JSON。

| kind | 必需字段 | 可选字段 |
| --- | --- | --- |
| byte、int32、int64、float32、float64、string | kind | 无 |
| vector | kind、scalarType、componentCount | name |
| array | kind、elementType | name |
| struct | kind、name、fields | 无 |

内置类型使用固定名称，因此不能提供 name。Vector 和 Array 省略 name 时，
分别由 C++ 生成 vec&lt;scalarName, componentCount&gt; 和 array&lt;elementName&gt;。
componentCount 只能是 2、3 或 4，且 scalarType 只能是 int32、int64、
float32 或 float64。fields 按二进制编码顺序排列，每项严格包含 name 和 type。
类型和字段名称必须是非空、最多 1024 字节的字符串；同一个 Struct 内的字段名
不能重复。类型递归深度上限为 64。

完整示例：

    {
      "kind": "struct",
      "name": "Game.PlayerState",
      "fields": [
        {
          "name": "position",
          "type": {
            "kind": "vector",
            "name": "vkEngine.EngineCore.NVec3",
            "scalarType": {
              "kind": "float32"
            },
            "componentCount": 3
          }
        },
        {
          "name": "scores",
          "type": {
            "kind": "array",
            "name": "System.Int32[]",
            "elementType": {
              "kind": "int32"
            }
          }
        }
      ]
    }

成功时 TypeInfo::FromJson 返回 TypeInfoPtr。失败时返回 TypeInfoJsonError：
JSON 形状问题使用 TypeMismatch、MissingField 或 UnexpectedField，数值范围问题
使用 IntegerOutOfRange，违反 TypeInfo 构造约束或未知 kind 使用 InvalidType，
嵌套过深使用 DepthLimitExceeded。path 从 $ 开始指出错误位置。

### EntityScript 元数据文件

EntityScriptMetadataExporter 生成的文件使用一个很薄的程序集外层；types 中的
每一项都可以直接传给 TypeInfo::FromJson：

    {
      "assemblyName": "Game",
      "types": [
        {
          "kind": "struct",
          "name": "Game.PlayerScript",
          "fields": [
            {
              "name": "speed",
              "type": {
                "kind": "float32"
              }
            }
          ]
        }
      ]
    }

脚本 Struct 的 name 是 C# namespace-qualified type name；字段 name 是 [Export]
指定的导出名。C# 基础类型映射到内置 kind，一维数组、EngineCore Vector 和可
支持的 value struct 递归展开。复合类型的 name 默认使用 C# 完整类型名；成员
上的 CppType 可以覆盖该名称，但不能覆盖内置类型的固定名称。

## TypeInfo 与 ValueView

| 信息 | TypeInfo | ValueView |
| --- | --- | --- |
| 类型名、类型种类 | 是 | 引用 TypeInfo |
| 元素类型、字段名、字段类型 | 是 | 否 |
| alignment | 是 | 引用 TypeInfo |
| 实际 offset 和 size | 否 | 是 |
| Struct 字段 View | 否 | 是 |
| Array count 和元素 View | 否 | 是 |
| Array 实例的可用 stride | 否 | 是 |
| 从 Data 获取值 | 否 | 是 |

二进制文件中的 Schema 是 `TypeInfo` 类型图的序列化表示。读取 Schema 后，C++ 构建 `TypeInfo`；读取 Data 时再构建与该 Data 实例绑定的 `ValueView`。

C++ 的具体 View 接口定义在 `include/reflect/value_view.hpp`。ValueView 非拥有地引用 Data，因此调用方负责保证底层内存的生命周期和地址稳定性。

## 从 JSON 编码 Data

`TypeInfo::EncodeBinaryFromJson(json)` 按当前类型递归校验 JSON，先计算编码后的 Data 大小，再分配并写入 `std::vector<std::byte>`，返回 `std::unique_ptr` 所有权。Struct 使用 JSON object，Array 和 Vector 使用 JSON array，String 使用 JSON string，数值类型使用对应范围内的 JSON number。Struct 必须包含全部字段且不能包含未定义字段。

编码后的内存可以交给 `ValueView::Parse(type, std::move(data))`。这个重载会接管内存，并让根 View 及其所有字段、元素和分量 View 共享其生命周期；因此单独保留的子 View 不会因根 View 销毁而悬空。原有的 `Parse(type, span)` 仍是非拥有型接口。
