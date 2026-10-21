<p align="right"><strong>简体中文</strong> · <a href="GROK_REMAINING.md">English</a></p>

# 给 Grok：四级、六级、进阶、毕业，共 12 条视频

四张起始图已完成几何和身份检查，头部与小学 A 逐像素相同，均为 1024×1280、纯绿底、同一条水平桌沿。见[四套预览](../preview/outfit_remaining.png)。视频尚未生成或验收。

| 学段 | 必须使用的起始图 | 外观锁定 |
| --- | --- | --- |
| cet4 四级 | [cet4.png](../start/cet4.png) | 灰连帽卫衣，耳机挂脖子 |
| cet6 六级 | [cet6.png](../start/cet6.png) | 白衬衫，深色背包肩带 |
| adv 进阶 | [adv.png](../start/adv.png) | 驼色风衣，相机挂胸前 |
| grad 毕业 | [grad.png](../start/grad.png) | 黑金学士服，手持卷起证书，仍戴白色 NABO 棒球帽 |

## 可直接复制给 Grok

请用这四张图片分别生成 NABO 的四个学段视频：cet4、cet6、adv、grad。每个学段做 01 待机、02 开心、03 沮丧，共 12 条。每条只用对应学段的起始图，不能混用服装。

通用要求：每条 3–4 秒；固定正面机位，不推拉、不平移、不旋转；人物整体大小、绿背景和水平书桌不变；保持同一个 NABO 的脸、发型、圆眼镜、白帽与正向 NABO 帽标，不能镜像、变脸、换帽或中途变装。桌面保持空，不添加摆件、字幕、水印。动作自然，结尾回到各自起始图的姿势和表情，并短暂停稳。手和配件始终不出画。

动作逐条生成：

1. `01_idle`：轻微呼吸，眨一次眼，头微微歪一下再回正；首尾姿势尽量完全一致，方便循环。
2. `02_happy`：眼睛笑弯，双手在胸前小幅握拳举起，身体轻轻一弹，然后放下回到起始姿势。毕业版双手始终拿着同一卷证书，以轻轻举起证书代替握拳，证书不能复制、消失或变形。
3. `03_sad`：肩膀垮一点，头低一点，轻轻叹气，再打起精神回正。毕业版证书始终在手里。

配件锁定：四级耳机一直挂在脖子上，不改成戴在头上；进阶相机一直挂在胸前，不凭空消失；毕业只有一卷证书，仍戴原白色 NABO 帽，绝对不换学士帽。

输出原始 MP4，不再压缩，命名如下；多版用 `_v2`、`_v3`，不要覆盖旧文件：

```
art/video/cet4/cet4_01_idle.mp4
art/video/cet4/cet4_02_happy.mp4
art/video/cet4/cet4_03_sad.mp4
art/video/cet6/cet6_01_idle.mp4
art/video/cet6/cet6_02_happy.mp4
art/video/cet6/cet6_03_sad.mp4
art/video/adv/adv_01_idle.mp4
art/video/adv/adv_02_happy.mp4
art/video/adv/adv_03_sad.mp4
art/video/grad/grad_01_idle.mp4
art/video/grad/grad_02_happy.mp4
art/video/grad/grad_03_sad.mp4
```

这些路径相对 `nabo-words/`。旧的小学、初中、高中源视频不需要重做。以上仅为视频制作交接，并不表示视频已生成或循环验收通过。
