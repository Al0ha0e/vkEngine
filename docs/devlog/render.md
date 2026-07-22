# render 分支的修改日志

不是正式的设计文档，语言组织比较随意，版本和日志按时间顺序排列，标题下方的日志**不是**标题所示版本进行的修改

## f0297deb85cf742697d1f5675510db6c070707e1 glyph data refactor

让codex在frame graph里加了个profile，实现相当顺利，结果发现gen_sky_lut和gen_sky_ibl_lut这两个task占了运行时间的大头，这两个task加上light_culling是仅有的三个占用compute queue的，结果里面两个占了大头，确实实现的时候没咋注意shader本身和调用时机的优化。

某一帧test_render的原始profile日志：
```
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] frame=1318 task_count=14
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=gen sky lut queue=1 gpu_task=0.113 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.029 ms
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=light culling queue=1 gpu_task=0.008 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.007 ms
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=shadow pass queue=0 gpu_task=0.082 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.290 ms
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=gbuffer pass queue=0 gpu_task=0.027 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.094 ms
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=gen sky ibl lut queue=1 gpu_task=1.179 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.028 ms
[2026-07-22 01:48:01.832] [vkEngine] [info] [FrameGraph Profile] task=ssao queue=0 gpu_task=0.173 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.040 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=ssao blur queue=0 gpu_task=0.153 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.024 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=deferred lighting queue=0 gpu_task=0.212 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.042 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=skybox render queue=0 gpu_task=0.014 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.046 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=atmosphere queue=0 gpu_task=0.282 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.038 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=transparent queue=0 gpu_task=0.015 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.043 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=bloom queue=0 gpu_task=0.044 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.021 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=tone mapping queue=0 gpu_task=0.012 ms pre_barrier=0.001 ms post_barrier=0.000 ms cpu_record=0.017 ms
[2026-07-22 01:48:01.833] [vkEngine] [info] [FrameGraph Profile] task=layered 2D render queue=0 gpu_task=0.004 ms pre_barrier=0.000 ms post_barrier=0.000 ms cpu_record=0.028 ms
```

让codex总结了下几帧的：
| Task              |     队列 |      GPU | CPU 录制 |
| ----------------- | -------: | -------: | -------: |
| gen sky ibl lut   |  Compute | 1.154 ms | 0.029 ms |
| atmosphere        | Graphics | 0.365 ms | 0.040 ms |
| ssao              | Graphics | 0.293 ms | 0.040 ms |
| deferred lighting | Graphics | 0.223 ms | 0.044 ms |
| ssao blur         | Graphics | 0.196 ms | 0.025 ms |
| gen sky lut       |  Compute | 0.113 ms | 0.030 ms |
| shadow pass       | Graphics | 0.082 ms | 0.289 ms |
| bloom             | Graphics | 0.044 ms | 0.023 ms |
| gbuffer pass      | Graphics | 0.033 ms | 0.096 ms |


有profile真的能很直接的看出很多问题