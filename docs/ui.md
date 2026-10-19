# UI 系统设计

## 职责与渲染接口

UI 系统负责 Canvas、节点层级、RectTransform 布局、文字和图片内容，以及场景保存、编辑器和脚本接口。底层 [2D 渲染系统](2d.md) 负责 Quad 存储、材质状态、渲染单元和 GPU 绘制。UI 系统将布局结果和几何数据提交给 2D 渲染器。

`UIRenderableComponent` 创建时向 `Layered2DRenderer` 分配渲染单元，持有材质和 Quad ID；布局更新时通过 `updateRenderTransform` 同步矩阵和绘制顺序。`UIText` 重建文字几何后通过 `setGeometry` 更新 Quad 与渲染单元的实例 ID 列表，颜色修改直接更新 Quad 颜色。节点卸载时释放渲染单元和 Quad 槽位。

运行时颜色、UV 和纹理索引以 `QuadManager` 中的 Quad 为准，属性读取和 `ToData()` 从 Quad 取值。`UIText` 的所有字形共享颜色，重建时读取首个 Quad 的颜色；没有可见字形时保留一个零面积 Quad，保存颜色供后续修改和重建使用。`UIImage` 在修改 UV、切换纹理或重新布局时读取现有 Quad，只更新受影响的字段。`UITextData` 和 `UIImageData` 保存用于创建节点和序列化的属性快照。

## Canvas 与 UI 节点

`vke_component::Canvas` 是挂在场景实体上的组件，拥有一棵独立的 UI 树。场景组件接口使用 `ComponentType::Canvas`。UI 节点由 `unique_ptr<UIComponent>` 持有，父指针为非拥有引用；场景的 Transform 层级管理实体，Canvas 的层级管理 UI。

```text
Canvas（场景组件，拥有根节点列表）
└─ children: unique_ptr<UIComponent>
   └─ 每个节点拥有自己的 children 列表

UIComponent（节点基类）
├─ UIContainer
└─ UIRenderableComponent
   ├─ UIText
   └─ UIImage
```

`include/ui/` 下的节点类型、RectTransform 和节点数据属于 `vke_ui` 命名空间；`Canvas` 与 `CanvasData` 位于 `include/component/canvas.hpp`，属于 `vke_component` 命名空间。

`ui.hpp` 定义公共数据基类 `UINodeData`、运行时基类 `UIComponent`，以及容器的 `UIContainerData` 和 `UIContainer`；`text.hpp` 定义 `UITextData` 和 `UIText`，`image.hpp` 定义 `UIImageData` 和 `UIImage`。各数据类型与对应的 UI 组件位于同一头文件。节点类与 Canvas 的成员函数实现在各自头文件中；`src/ui/ui.cpp` 实现按节点类型分派的 JSON 校验和数据构造。`ui_manager.hpp` 定义 `UIManager`，其类内实现的静态 `Update` 入口负责收集、排序 Canvas 并调用画布更新；布局状态由 Canvas 和节点持有。

`UIComponent` 保存名称、RectTransform、父子关系和最近一次布局结果。`UIContainer` 提供布局容器；`UIRenderableComponent` 管理材质、渲染单元和 Quad；`UIText` 负责文字几何与颜色，`UIImage` 负责图片纹理、UV、颜色和适配布局的矩形。每个节点都可以有子节点。节点创建后地址稳定，Canvas 在 EnTT 存储中移动不会移动内部节点。

原生调用方直接访问节点的 `name`、`handle`、`parent`、`children`、`rectTransform` 和 `resolvedSize`。`name` 可直接修改；`handle` 由 Canvas 分配，层级修改使用 Canvas 的操作函数，以同步父子关系、句柄索引和资源释放。布局修改通过 `SetRectTransform` 校验并标脏，直接赋值 `rectTransform` 不会触发布局重算；`resolvedSize` 由布局系统写入。

Canvas 通过哈希表把 `UIHandle` 映射到节点指针，句柄查找平均 O(1)。句柄为进程内单调分配的 64 位编号，0 表示 Canvas 根。删除、移除 Canvas、销毁实体和 Reset 会使对应节点句柄失效，新节点使用新的编号。句柄不参与序列化，场景恢复后须重新查询。原生节点指针只在节点存活期间有效。

## 屏幕空间

当前支持 `screenSpace` 模式。Canvas 可以挂在任意场景实体上，宿主 Transform 不参与屏幕布局。所有 Canvas 使用当前渲染目标的像素尺寸，左上原点，X 向右，Y 向下。

`referenceResolution` 默认 `(1920, 1080)`，两个分量必须为正。画布等比适配，宽高比变化时扩大对应方向的逻辑范围：

```text
scale = min(viewportWidth / referenceWidth, viewportHeight / referenceHeight)
logicalSize = viewportPixelSize / scale
canvasMatrix = Scale(scale, scale, 1)
```

JSON 中的 `mode` 仅支持 `screenSpace`，其他值会返回错误。

## RectTransform

`include/ui/rect_transform.hpp` 定义布局和局部视觉变换：

- `anchorMin`、`anchorMax`：相对于父矩形的归一化锚点，每轴满足 `0 <= min <= max <= 1`，默认 `(0, 0)`。
- `offsetMin`、`offsetMax`：相对锚点的逻辑单位偏移，允许负数，默认 `(0, 0)`。
- `pivot`：自身矩形中的旋转、缩放中心，每轴范围 `[0, 1]`，默认 `(0.5, 0.5)`。
- `scale`：二维局部缩放，默认 `(1, 1)`，允许零和负数。
- `rotation`：绕 pivot 的局部 Z 轴旋转，单位弧度，默认 0。

求解公式为：

```text
minimum = anchorMin * parentSize + offsetMin
maximum = anchorMax * parentSize + offsetMax
size = max(maximum - minimum, 0)
position = minimum + pivot * size - parentPivot * parentSize
layoutMatrix = parentMatrix * Translate(position) * Rotate(rotation) * Scale(scale)
geometryMatrix = layoutMatrix * Translate(-pivot * size)
```

Canvas 的父 pivot 为 `(0, 0)`。子节点按父节点未缩放的逻辑尺寸布局，视觉变换沿 UI 层级组合。零尺寸合法，负尺寸收敛为零。布局矩形不裁剪文字，也不随文字内容自动调整大小。

`UIComponent::SetRectTransform` 校验输入并标脏；`Canvas::SetReferenceResolution` 标记整棵树需要重算。`Engine::Update` 和 `Editor::Update` 通过公共入口 `Engine::UpdatePresentation()` 调度绘制前更新：先将场景 registry 和当前渲染目标尺寸传给 `vke_ui::UIManager::Update(registry, viewportSize)`，再调用 `Renderer::Update()` 执行渲染。编辑、暂停和运行状态均执行这条路径。如果随后渲染时发生交换链重建，UI 布局在下一帧适配新尺寸。零尺寸目标保留上次结果。

每帧遍历节点，复杂度 O(n)，只重新计算脏节点及受父节点变化影响的子树布局。节点的 `resolvedSize` 保存最近一次求解结果，首次求解前为零。需要提前读取布局时，可以显式调用 `vke_ui::UIManager::Update(registry, viewportSize)`。

## UIImage

`UIImage` 使用共享的内置图片材质，通过 Quad 中的纹理索引访问 2D Renderer 的 bindless 图片纹理表。每个 UIImage 使用一个 Quad，图片和文字按同一 Canvas 顺序绘制，每个节点独立提交 draw。尺寸变化调用 `onSizeChanged()`，图片原位更新 Quad；移动、旋转和缩放通过渲染单元矩阵更新。

`UIImageData` 的属性包括：

- `texture`：`AssetRef<Texture2D>`，JSON 保存资源句柄；0 表示内置白色纹理，可配合颜色绘制纯色背景。
- `color`：RGBA 值，默认全 1；shader 输出为纹理采样值乘颜色，使用普通 Alpha 混合。
- `uvRect`：`(minU, minV, maxU, maxV)`，默认 `(0, 0, 1, 1)`。各分量在 `[0, 1]` 内，每轴 minimum 不大于 maximum；坐标原点为图片左上角。
- `preserveAspect`：默认 false，填满布局矩形；true 时根据纹理尺寸和 UV 区域计算宽高比，居中等比缩放到布局矩形内。空 UV 区域在等比模式下生成零尺寸几何。子节点始终按完整布局矩形定位。

原生 `SetTexture(AssetHandle)` 校验并加载纹理，注册成功后更新图片，失败返回错误并保留原来的纹理和几何。`SetColor` 写入 Quad 颜色，`SetUVRect` 校验 UV 范围并更新 Quad，两者返回 `SceneResult<void>`；`SetPreserveAspect(bool)` 更新显示方式。资源校验和加载沿用节点数据的 `ValidateAssets` / `LoadAssets` 流程。

纹理表容纳 1024 个槽位，其中槽位 0 为白色纹理。同一纹理对象共享索引并按注册次数计数，删除节点或成功换图时注销旧纹理；最后一个使用者注销后，CPU 槽位立即可复用。各在飞行帧通过自己的纹理引用快照保留仍在使用的资源，待该帧完成并同步描述符后释放旧引用。换图先注册新纹理，表满且新纹理尚未注册时返回错误并保留原图。构造时注册失败的节点记录错误，使用零面积 Quad 保存颜色和 UV，可通过成功的 `SetTexture` 恢复。Quad 容量不足时记录分配错误并归还纹理注册；没有分配到 Quad 的图片节点读取默认颜色和 UV，换图、修改颜色和 UV 返回错误。帧描述符同步和资源生命周期见 [2D 渲染系统](2d.md)。

```json
{
  "type": "uiImage",
  "name": "Background",
  "texture": 0,
  "color": [0.1, 0.15, 0.2, 0.8],
  "uvRect": [0, 0, 1, 1],
  "preserveAspect": false,
  "rectTransform": {"offsetMax": [400, 200]},
  "children": []
}
```

## 顺序、操作与生命周期

Canvas 按 `sortOrder` 从小到大排列，相同值按运行时实体 ID 排列。每棵 UI 树按先父后子、兄弟数组顺序遍历，后访问的节点后绘制。系统将遍历顺序写入渲染单元矩阵的 Z 平移，供现有 2D renderer 排序；该值不参与 UI 平面布局。`SetSiblingIndex` 调整同级节点顺序。

`Canvas::Add(const UINodeData &, UIHandle parent = 0)` 接收具体类型的数据，校验节点子树、预检全部资源，再深复制数据、加载资源并创建节点；调用方的数据保持不变。`SetParent` 只允许同一 Canvas 内的节点，拒绝环，保留 RectTransform 输入并在新父矩形中重新求解；parent=0 表示画布根。`Find(name, parent)` 在指定父节点的直接子节点中查找，重复名称返回第一个匹配。`Get(handle)` 直接查找句柄。

`Remove(handle)` 销毁完整子树，递归释放渲染单元和 Quad，并使子树句柄失效。场景移除 Canvas 或销毁实体时调用 `Canvas::Unload`。移动赋值先卸载目标资源，再转移源树和句柄索引。所有树修改都在主线程进行，不允许在遍历受影响的 children 容器时修改层级。

场景 Reset 先由 Renderer 批量清理渲染资源，再销毁 Canvas 与 UI 树；UI 节点析构不访问 Renderer。原生调用方必须按场景生命周期在单独销毁 Canvas 前调用 `Unload`。

## 保存与预制体

`CanvasData` 保存参考分辨率、排序值和有序根节点列表。根节点及 `UINodeData::children` 使用 `unique_ptr<UINodeData>` 拥有具体类型的数据对象。`UINodeData` 保存名称、RectTransform 和子节点；`UIContainerData` 表示布局容器，`UITextData` 保存文字、颜色和通过 `AssetRef` 引用的材质，`UIImageData` 保存纹理引用、颜色、UV 和等比显示选项。数据对象的 `GetType()` 返回其具体类型，校验、资源预检、加载和 JSON 导出通过虚函数分派，并递归处理子节点。

JSON 输入先经过 `UINodeData::ValidateJSON` 校验，再由 `UINodeData::FromJSON` 根据 `type` 构造具体数据对象；两个入口都递归处理子节点。`CanvasData::ValidateJSON` 和 JSON 构造函数分别调用这两个阶段。原生代码可以直接构造 `UIContainerData`、`UITextData` 或 `UIImageData`，子节点列表中的空指针会被 `Validate()` 拒绝。

`UINodeData::Clone()` 保留具体类型并深复制子树；具体数据类型和 `CanvasData` 的复制构造、复制赋值同样深复制节点，已解析的资源仍通过 `AssetRef` 共享所有权。运行时节点通过 `ToData()` 返回对应的具体数据对象，`Canvas::FillData` 汇集完整数据树。场景导出、复制、预制体实例化和编辑器运行快照包含完整 UI 树；布局结果和运行时句柄不保存。

```json
{
  "type": "canvas",
  "mode": "screenSpace",
  "referenceResolution": [1920, 1080],
  "sortOrder": 0,
  "children": [{
    "type": "uiText",
    "name": "Title",
    "text": "Hello",
    "color": [1, 1, 1, 1],
    "material": 0,
    "rectTransform": {
      "anchorMin": [0.5, 0], "anchorMax": [0.5, 0],
      "offsetMin": [-200, 20], "offsetMax": [200, 100],
      "pivot": [0.5, 0.5], "scale": [1, 1], "rotation": 0
    },
    "children": []
  }]
}
```

`uiContainer` 使用相同的公共字段，不包含文字与材质字段。UI 系统在场景实体的 `components` 数组中以 `canvas` 表示，`uiText`、`uiImage` 和 `uiContainer` 只允许出现在 Canvas 节点树内。

## 编辑器和 C#

编辑器通过 `Scene > New Object > Empty` 创建实体，再通过 Inspector 的 `Add Component > Canvas` 添加 Canvas。Canvas Inspector 展示内部 UI 树，支持添加容器、文字和图片、删除子树、修改父节点、调整同级顺序，以及编辑布局、文字和颜色。图片节点可选择纹理、编辑 UV 区域和等比显示选项。旋转输入显示角度，存储与 API 使用弧度。

C# 的 Canvas 通过实体 ID 定位，UIComponent 保存 Canvas 引用和节点 Handle。UIContainer 继承 UIComponent，UIText 和 UIImage 通过 UIRenderableComponent 继承 UIComponent。`GetComponent<Canvas>()` 获取场景组件，节点通过 `Find`、`Get`、`Children`、`CreateContainer`、`CreateText` 和 `CreateImage` 访问。

```csharp
var canvas = GetComponent<Canvas>()!;
var panel = canvas.CreateContainer("Panel");
var rect = panel.RectTransform;
rect.AnchorMin = rect.AnchorMax = new NVec2(0.5f, 0.5f);
rect.OffsetMin = new NVec2(-200, -100);
rect.OffsetMax = new NVec2(200, 100);
panel.RectTransform = rect;
var title = canvas.CreateText("Title", panel);
title.Text = "Hello";
title.SetParent(null); // 移至画布根，保留布局输入
```

`Find` 只查直接子节点，`Get` 和 `Find` 找不到时返回 null。`IsValid` 检查句柄是否仍有效；依赖原生节点的属性读取和修改失败时抛出 `InvalidOperationException`，跨 Canvas 指定父节点抛出 `ArgumentException`，`SetSiblingIndex` 的负数索引抛出 `ArgumentOutOfRangeException`。Children 返回本次查询的托管包装列表，元素仍引用原生节点，删除后须重新查询。原生和托管函数表需要一起构建部署。

C# `UIImage.Texture` 使用 `UInt64` 资源句柄（0 为白色），`Color` 和 `UVRect` 使用 `NVec4`，`PreserveAspect` 使用 bool。属性修改失败时抛出 `InvalidOperationException`，图片颜色和 UV 存在 Quad 中。

```csharp
var background = canvas.CreateImage("Background", panel);
var imageRect = background.RectTransform;
imageRect.OffsetMax = new NVec2(400, 200);
background.RectTransform = imageRect;
background.Texture = 0;
background.Color = new NVec4(0.1f, 0.15f, 0.2f, 0.8f);
background.UVRect = new NVec4(0, 0, 1, 1);
background.PreserveAspect = false;
background.SetSiblingIndex(0);
```

## 未实现功能

世界空间 Canvas 尚未实现。当前文字顶点着色器直接把像素位置转换为 NDC，固定输出 Z=0、W=1；2D pass 只绑定颜色附件，关闭深度测试和深度写入。世界空间模式需要相机投影、透视除法以及场景深度集成。
