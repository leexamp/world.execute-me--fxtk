# DeepSeek-chan — 矢量娘化形象说明（蓝鲸女仆版）

- 文件：`mv/pv/assets/char/deepseek_girl.svg`
- 画布：`viewBox="0 0 512 512"`，深黑底，胸像构图
- 体积：**28246 bytes**（约 27.6 KB），纯矢量；无外部字体、无位图、无远程引用
- 角色依据：角色立绘（本地私有素材，未入库）
- 渲染：Chromium / playwright-core，`deviceScaleFactor: 2` 渲染后 Lanczos 缩到 512 与 176
- 验证图：`/tmp/svg_check/512.png`、`/tmp/svg_check/176.png`
- 本轮重做迭代：**7 轮**（每次渲染 → read_image 目检 → 对照官方立绘检查发色、眼睛、头饰、鳍耳、尾巴、围裙鲸鱼标 → 修正）

> 上一版误做成了橙发猫耳角色；本版按官方 DeepSeek-chan 重新设计为**蓝发、蓝眼、白荷叶边女仆头饰、鲸鱼鳍耳、鲸鱼尾、藏蓝/白色女仆装、白围裙鲸鱼标**。

---

## 1. 色板

| 用途 | 色值 | 说明 |
|---|---|---|
| 背景 | `#05070B` | 近黑，带一点冷蓝 |
| 主轮廓 | `#0D1726` | 深海军黑，所有大色块共用 |
| 头发主色 | `#3F7DAE` | 钢蓝，官方发色方向 |
| 头发中间层 | `#356C9C` | 侧发 / 次级发束 |
| 头发底层 | `#2C527F` | 后发大剪影 |
| 头发暗部 | `#24446D` | 内层、鬓角 |
| 头发深阴影 | `#1B3557` | 发缝、发根 |
| 头发高光 | `#6FA8D0 / #8FC2DF` | 亮发丝、蓝发高光 |
| 皮肤 | `#F8E5D6` | 暖米白肤色 |
| 皮肤阴影 | `#E7C5B7` | 刘海下、下颌 |
| 颈部阴影 | `#E0B5A8` | 下巴投影 |
| 腮红 | `#F2A6A0`（38%） | 淡粉 |
| 米白布料 | `#F7F6F2` | 头饰、衬衫、围裙、袖口 |
| 布料暗部 | `#DDE0E7` | 衣褶、荷叶边阴影 |
| 藏蓝布料 | `#1E2C4A` | 胸衣、泡泡袖、颈结 |
| 藏蓝亮部 | `#31466B` | 袖褶、衣褶 |
| 金扣 / 金边 | `#C9A66B` | 胸衣扣子、围裙边 |
| 鲸鳍外缘 | `#7E90A9` | 鳍耳外膜 |
| 鲸鳍内面 | `#DDE3EA` | 鳍耳内面 |
| 鳍纹 | `#647A96` | 鳍条 |
| 眼白 | `#FFFDF8` | 高亮米白 |
| 虹膜渐变 | `#8AD8F4 → #4CA8D8 → #1F5F9E` | 上亮下深的蓝眼 |
| 瞳孔 | `#122A45` | 深海军蓝 |
| 睫毛 / 眼线 | `#0D1726` | 与主轮廓统一 |
| 口腔 | `#7E3341` | 张口笑 |
| 舌头 | `#E98C8B` | 下唇内侧 |
| 蓝蝴蝶结 | `#69B1D0 → #3A7799` | 头侧发结 |
| 蓝宝石 | `#6EC6E6` | 颈结宝石 |
| 鲸尾渐变 | `#4E709C → #36547E → #243B5C` | 比头发更暗、更冷，形成层次 |

---

## 2. 分组（绘制顺序 = 从后到前）

| id | 内容 | 动画建议 |
|---|---|---|
| `#glow` | 冷蓝径向光晕 + 水泡/漂浮微粒 | opacity 0.55→1、scale 1→1.04；水泡独立上下漂浮 6–10s |
| `#hair-back` | 后发大剪影、外层波浪、亮发丝、发缝、外侧小翘发 | 整体慢摆 rotate ±1.2°，origin 约 50% 88%，6–8s ease-in-out |
| `#body` | 颈部、白衬衫、藏蓝胸衣、金扣、前胸白面板、白色荷叶领、肩部荷叶边、泡泡袖、白袖口、白围裙 + 鲸鱼标 | 呼吸：translateY 1–2px + scale 1.005；围裙与胸口宝石可做极轻浮动 |
| `#face` | 脸、刘海阴影、下颌阴影、腮红、鼻 | 与 eyes/mouth 一起做头部轻点头/侧倾；静态 1.06 缩放已放在内层 g |
| `#eyes` | 左右大蓝眼、虹膜渐变、瞳孔、双高光、睫毛、下眼线 | 眨眼：scaleY 1→0.08→1（0.12s），每 4–6s 一次；瞳孔可做 1px 漂移 |
| `#mouth` | 张口笑、牙齿、舌头、嘴角 | 说话/唱歌时 scaleY 1→1.35；也可以逐帧替换 d |
| `#ears` | 左右鲸鱼鳍耳（灰蓝外膜 + 米白内面 + 鳍条） | 以鳍根为轴 rotate ±5–7°，偶发快速抖动；两只错开相位 |
| `#hair-side-l` | 左鬓发、深色内层、亮发丝、垂到胸前的发尾 | 与 hair-back 反相慢摆 ±2°，延迟 80–150ms |
| `#hair-side-r` | 右鬓发、深色内层、亮发丝、垂到胸前的发尾 | 同上，与左鬓错开相位 |
| `#tail` | 鲸鱼尾：粗尾柄 + 双叶尾鳍 + 浅蓝边缘高光（为胸像可读性，画在侧发之上） | 以尾根为轴 rotate ±5°/3.2s；尾鳍尖端可加 0.5–1px 延迟 |
| `#hair-front` | 头顶、三层刘海、鬓角、亮发丝、发根阴影、**呆毛（ahoge）** | 刘海滞后摆动 ±0.8°；呆毛可单独做更大的 ±4° 弹性摆动 |
| `#maid-cap` | 白色荷叶边女仆头饰：荷叶边、蕾丝圆点、压线、白色高光 | 跟随头部运动；额外 1–2px 弹跳，蕾丝边可做极轻 scaleY |
| `#flower` | 头饰左侧的五瓣小白花 + 金色花心（因任务要求保留 `#flower` 分组；官方立绘无花） | 缓慢 rotate ±3°，4–6s；花心可做高光脉冲 |
| `#bow` | 头侧蓝色蝴蝶结 + 藏蓝颈结 + 蓝宝石 + 飘带 | 发结以结心为轴 rotate ±3°；颈结随呼吸 scale 1→1.02；宝石高光闪烁 |

> `#face`、`#eyes`、`#mouth`、`#hair-side-l/r`、`#flower` 的静态缩放/位移已经下沉到**内层 `<g>`**。外层 id 分组本身不带 transform，外部 CSS / WAAPI 直接对 id 做 transform 不会覆盖原构图。

---

## 3. 动画用法示例

### 眨眼 + 头部呼吸

```css
#eyes {
  transform-box: fill-box;
  transform-origin: 50% 52%;
  animation: ds-blink 5.2s infinite;
}
@keyframes ds-blink {
  0%, 91%, 100% { transform: scaleY(1); }
  93%, 95%      { transform: scaleY(0.08); }
}
#body, #face { animation: ds-breathe 4.8s ease-in-out infinite; }
@keyframes ds-breathe {
  0%, 100% { transform: translateY(0); }
  50%      { transform: translateY(-1.5px); }
}
```

### 鲸尾摆动

```css
#tail {
  transform-box: fill-box;
  transform-origin: 70% 96%; /* 尾根在画面右下 */
  animation: ds-tail 3.2s ease-in-out infinite;
}
@keyframes ds-tail {
  0%, 100% { transform: rotate(-5deg); }
  50%      { transform: rotate(5deg); }
}
```

### 头发跟随

```css
#hair-back   { animation: ds-hair 7.2s ease-in-out infinite; }
#hair-side-l { animation: ds-hair 7.2s ease-in-out -0.12s infinite; }
#hair-side-r { animation: ds-hair 7.2s ease-in-out -0.24s infinite; }
@keyframes ds-hair {
  0%, 100% { transform: rotate(-1.2deg); }
  50%      { transform: rotate(1.2deg); }
}
```

### 水泡

```css
#glow circle { animation: ds-bubble 7s ease-in-out infinite; }
@keyframes ds-bubble {
  0%, 100% { transform: translateY(0); opacity: .35; }
  50%      { transform: translateY(-6px); opacity: .8; }
}
```

### 逐帧动效顺序建议

1. 先让 `#body` / `#face` / `#eyes` / `#mouth` 做整体呼吸与点头；
2. 再让 `#hair-back`、`#hair-side-*`、`#hair-front` 以不同相位跟随；
3. 最后叠加 `#ears` 鳍耳抖动、`#tail` 摆尾、`#bow` / `#flower` 次级摆动与 `#glow` 水泡。
所有 id 分组都是独立可驱动单元，便于 GSAP / WAAPI / SMIL / 序列帧导出。

---

## 4. 与官方参考图的差异

1. **构图**：官方是 1024×1536 全身立绘、白底；本图按任务要求为 512×512、黑底胸像，只保留头、肩、胸、围裙上半与部分尾鳍。
2. **画风**：官方是细笔触、柔和渐变的日系插画；本图是有限色板 + 大色块 + 深色轮廓的扁平矢量，细节（蕾丝、发丝、衣褶）被简化。
3. **发色**：官方是低饱和钢蓝渐变；本图用 4 层蓝 + 亮发丝模拟层次，整体更饱和、更“图形化”。
4. **头饰与服装**：官方头饰的花边层数更多、女仆装裙摆/围裙更大；本图压缩为白荷叶边 bonnet、藏蓝胸衣、金扣、白围裙和鲸鱼标，便于 176px 缩略图识别。
5. **鳍耳 / 尾鳍**：官方鳍耳更小、尾鳍在裙后；本图为了黑底可读性，把鳍耳略放大并左右对称化，尾鳍画在侧发之上、靠近画面右下。
6. **额外元素**：官方没有花饰；本图按任务要求加入 `#flower` 小白花（头饰左侧）。表情按官方做成**大蓝眼 + 张口笑**，不再沿用上一版的困倦半闭眼。
7. **背景**：官方白底；本图按原任务保持黑底，并用冷蓝光晕/水泡补足鲸鱼主题。

---

## 5. 最终自评（5 条）

1. **像**：长蓝发、蓝眼、白色荷叶边女仆头饰、藏蓝+白女仆装、白围裙，第一眼能认出是 DeepSeek-chan。
2. **像**：鲸鱼鳍耳、鲸鱼尾鳍、围裙上的小鲸鱼标、头顶呆毛、蓝色发结与金扣，这些官方识别点都在。
3. **像**：蓝发/蓝眼/藏蓝服装/白围裙的配色关系与官方一致，黑底水泡也强化了鲸鱼主题。
4. **不像**：脸更 chibi、眼睛比例更大、发型更整块，缺少官方立绘的细碎发丝、柔和渐变和繁复蕾丝。
5. **不像**：尾鳍为了胸像可读性被移到侧发上方且更靠近画面边缘，头饰/围裙层次少于官方；额外的小白花是任务分组要求，官方原图没有。

---

## 6. 验收检查

```bash
# 纯矢量（应为 0）
grep -c "image/" mv/pv/assets/char/deepseek_girl.svg

# 无外部引用（应为 0）
grep -cE 'href=|url\(http|@font-face' mv/pv/assets/char/deepseek_girl.svg

# XML 合法
xmllint --noout mv/pv/assets/char/deepseek_girl.svg

# 体积
wc -c mv/pv/assets/char/deepseek_girl.svg   # 28246
```

14 个必需分组 id 均存在且唯一：
`hair-back`、`hair-front`、`hair-side-l`、`hair-side-r`、`ears`、`face`、`eyes`、`mouth`、`maid-cap`、`flower`、`bow`、`body`、`tail`、`glow`。

验证图：
- `/tmp/svg_check/512.png`（512×512）
- `/tmp/svg_check/176.png`（176×176；蓝发、双蓝眼、白 bonnet、鳍耳、尾鳍、白围裙鲸鱼标仍可辨）
