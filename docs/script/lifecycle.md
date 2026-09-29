# 实体的生命周期

## 目前实现

C++侧的 CreateEntity 和 LoadSceneData 都是直接执行的，还没有实例化队列。C#目前只提供 DestroyEntity 和 IsPendingDestroy，脚本自身也有对应的函数和属性。

销毁分两步：

1. DestroyEntity 只遍历和标记整棵子树，放进 pendingDestroy 这个set里，不断开父子关系，也不更新transform。重复请求按完整entity句柄去重。
2. 每帧在C# update之前调用 ProcessDestroyRequests，暂停时也处理。先把set复制成这一帧的vector，再批量调用C#的 UnloadEntities，传的是数组指针和数量。回调里新增的请求不在这个vector里，留到下一帧，不会直接嵌套调用 Unload。

C#按entity逐个清理，每个entity的顺序是：全部脚本 Unload → 按entity注销脚本 → Dispose → 注销物理回调。脚本清理抛异常会记录下来，然后继续，托管内存还是由GC回收。

整批C#清理完成之前，原生实体和父子关系都保留。之后C++逐个调用 UnloadFromEngine，从仍有效的父实体中移除引用，再用 registry.destroy 删除组件和entity，最后从pendingDestroy里移除。不保证父子实体的销毁顺序。

UnloadSceneData 走同一套延迟销毁流程。退出时不再允许创建实体，并处理完剩余销毁请求。

## 访问规则

- 标记待回收不会限制普通组件访问和回调，Unload里也能访问尚未释放的原生组件；脚本开始卸载或已经Dispose之后，不再参与普通生命周期分发。
- 已失效entity的组件写入跳过，读取返回默认值，body ID查询返回无效ID。接受body ID的interop直接调用Jolt，由Jolt处理句柄有效性，不额外检查和加锁。
- C#还没有修改父子关系的接口，以后加的时候要检查被移动实体和目标父实体的待回收状态，禁止修改待回收子树。C++的SetParent目前也没有这个检查。

## 后面要处理的

- C#创建实体、实例化场景的接口还没做，实例化队列也还没做。以后要处理回调里实例化实体又触发Start的情况。
- dispatch目前仍会复制callback列表，因为Dispose会直接注销脚本。要去掉这个复制，还得把注册和注销的时机一起理一下。
- 物理回调现在清理得太早，后面的Unload可能重新订阅前面已清理实体的回调，导致残留。应该等整批脚本清理完再统一注销。
- 一个脚本在Unload里Dispose同实体的另一个脚本，快照仍会调用后者的Unload和Dispose，这里还缺已释放状态的检查。
