# TOKENIZER.md — 歌词带 token ID 的真实来源、预计算与验收

> 任务：把 `mv/pv/web/mv.html` 歌词带下面的**伪词表哈希 ID**（HF_WORDS + h32）换成
> **真实 tokenizer 的 token ID**。页面运行时仍是纯 `t` 函数、确定性、零网络。
> 本文件记录词表来源、编码口径、预计算静态表、实测数字与诚实标注。
>
> **本轮更新**：`deepseek-v4-flash-0731` 从 `deepseek-ai/deepseek-llm-7b-base` 换成
> **DeepSeek-V4 系列真实共享词表**（来源 repo `AndersonBY/deepseek-tokenizer` @
> `525044c24386953b542ef5262d0c69d309a5d67f`）；`deepseek-v4.1-flash` 因 v4.1 词表未公开，
> **保留 DeepSeek-V3 替身**。页面 `<title>` 与所有悬停 `title` 工具提示都已写明这一点。

## 0. 一句话结论

* `deepseek-v4-flash-0731` → **DeepSeek-V4 系列共享词表**（V4-Pro 与 V4-Flash 共享；
  由 `AndersonBY/deepseek-tokenizer` 的 release 提供，只取 `tokenizer.json`）；
* `deepseek-v4.1-flash` → **deepseek-ai/DeepSeek-V3 替身**（v4.1 词表未公开，页面显示名是虚构的）；
* 87 行歌词逐词按 `encode(" " + word)`（词前带空格）真实编码，结果预计算成静态表
  `RB_IDS / RB_TOKS` 嵌进 `mv.html`，页面运行时不联网、不算哈希；
* **V4 与 V3 共享同一份 128000 BPE 基表**，所以本片语料上 V4 逐词 ID 与 V3 完全一致
  （442 片全等，tokens 也全等）；V4 多出来的只是 added special tokens（V4 1283 个 vs V3 818 个）；
* 旧伪哈希已全部移除（`HF_WORDS / h32 / tokenId / chunkBPE / wordChunks / tokCount / TOKRULES` 不再存在）。

## 1. 页面显示名 → 真实词表（映射，必须按此标注）

| 页面显示名（虚构） | 真实来源词表 | 真实 `tokenizer.json` | vocab_size | 文件大小 | 来源 / commit |
|---|---|---|---|---|---|
| `deepseek-v4-flash-0731` | **DeepSeek-V4 系列共享词表**（`deepseek-ai/DeepSeek-V4-Pro` 与 `deepseek-ai/DeepSeek-V4-Flash` 共享） | `src/deepseek_tokenizer/tokenizer.json` | **129280**（model.vocab 128000 + added 1283） | **6,367,146 B** | `github.com/AndersonBY/deepseek-tokenizer` @ `525044c24386953b542ef5262d0c69d309a5d67f`（2026-06-28 "Update tokenizer for DeepSeek V4"） |
| `deepseek-v4.1-flash` | **deepseek-ai/DeepSeek-V3**（v4.1 词表未公开，暂用替身） | `tok_v3.json` | 128815（model.vocab 128000 + added 818） | 7,847,652 B | `https://huggingface.co/deepseek-ai/DeepSeek-V3/resolve/main/tokenizer.json` |

V4 词表来源 URL / clone 方式：

```bash
git clone --depth 1 https://github.com/AndersonBY/deepseek-tokenizer.git /tmp/dsk-tok
# 词表本体：/tmp/dsk-tok/src/deepseek_tokenizer/tokenizer.json
# 该 repo README 明说该 release 使用 DeepSeek-V4 系列 tokenizer，
# 由 deepseek-ai/DeepSeek-V4-Pro 与 deepseek-ai/DeepSeek-V4-Flash 共享。
```

sha256：

* V4 `tokenizer.json` = `8f9f37ca37fdc4f5fd36d5cf4d3b0e8392edb4e894fd10cc0d70b4957c8633cf`
* `tok_llm7b.json`（上一版 flash-0731） = `a08b02921f08548065a7b2ec13b2ffeed873231add60f9c3c7b08b04f2cc212a`
* `tok_v3.json`（v4.1-flash 替身） = `621ac2e32d0dba658404412318818aaa8ce8cda492e59830109d8da6b517fb41`

V4 `tokenizer_config.json` 关键项：`model_max_length = 1048576`，`add_bos_token = false`，
`add_eos_token = false`，V4 special tokens 实测：
`<think>`=128821、`</think>`=128822、`｜DSML｜`=128825、`<｜latest_reminder｜>`=128828。

### 1.1 与上一版词表的 vocab_size 对比

| 词表 | 页面显示名 | model.vocab | added_tokens | `get_vocab_size()` | 文件大小 | sha256 |
|---|---|---|---|---|---|---|
| DeepSeek-V4 系列共享（本轮新用） | `deepseek-v4-flash-0731` | 128000 | 1283 | **129280** | 6,367,146 B | `8f9f37ca37fdc4f5fd36d5cf…` |
| deepseek-ai/DeepSeek-V3（保留） | `deepseek-v4.1-flash` | 128000 | 818 | 128815 | 7,847,652 B | `621ac2e3…` |
| deepseek-ai/deepseek-llm-7b-base（上一版 flash-0731） | — | 100000 | 15 | 100015 | 4,609,778 B | `a08b0292…` |

> V4 = V3 基表（128000）+ 多 465 个 special/added token，因此**普通文本 ID 完全一致**。

## 2. 编码口径（为什么这样才和真实文本一致）

* 按空白切词：`text.split(/\s+/).filter(Boolean)`（Python 侧等价 `text.split()`，两边逐行比对 **0 处不一致**）；
* 每个词编码的输入是 `" " + word`，即"词前带空格"的形态——真实文本里词与词之间本来就有空格，
  该 tokenizer 的 pre_tokenizer 又在空白处切分，所以这个前导空格属于词本身（ByteLevel 里编码成 `Ġ`）；
* `add_special_tokens=False`，不引入 BOS/EOS 等特殊 token；
* 结果是**每个词一个真实 token ID 数组**（可能 1 片，也可能多片，如 `purr → Ġpur + r`）。

预计算表结构（嵌在 `mv.html`）：

```
RB_LINE_TEXTS[i]            // 第 i 行原文，顺序 = A.lyrics[i]
RB_IDS[model][i][j]         // 第 i 行第 j 个词的真实 token ID 数组
RB_TOKS[model][i][j]        // 对应片段字符串（仅 tooltip 展示用）
RB_SRC[model]               // 真实来源词表名 / vocab / bytes / sha256 / repo / commit / note
```

## 3. 显示层（未变）

* 词下 `.tk` 显示该词真实 ID 的十六进制：`0x` + 大写 hex，最少补到 4 位（`0x0AF5`、`0xBA6F`）；
* 最多显示 3 片，超出折成 `+N`（例如 `lo-o-ove` 有 5 片会显示 3 片 + `+2`）；
* 右侧 `#rb-sum` 仍显示行合计与 tokenizer 名：`本行 N tok · tokenizer: <页面名>`；行合计 = 各词片数之和；
* 每个词格与标签的 `title`（悬停提示）写明 **真实来源词表**、repo/commit、逐片 `片段=0xID`、
  以及"页面显示名 ≠ 词表本名"的诚实标注；页面 `<title>` 也写明了两个页面名的真实词表口径。

## 4. 验收数字

### a) V4 词表信息

vocab_size = **129280**（model.vocab 128000 + added 1283），文件 **6,367,146 B**，
sha256 = `8f9f37ca37fdc4f5fd36d5cf4d3b0e8392edb4e894fd10cc0d70b4957c8633cf`；
来源 repo = `github.com/AndersonBY/deepseek-tokenizer`，commit = `525044c24386953b542ef5262d0c69d309a5d67f`。
上一版 flash-0731 用的 llm-7b-base 是 100015 / 4,609,778 B；v4.1-flash 保留的 V3 是 128815 / 7,847,652 B。

### b) 8 行抽样（词 | 真实片段 | **V4 真实 ID(hex)** | 片数；并与上一版 llm7b 的 ID 对照）

### 行 idx 0（A.lyrics[0]）`world.execute(me); - Mili`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `world.execute(me);` | Ġworld + .execute + ( + me + ); | 0x080A + 0xEA37 + 0x000A + 0x0823 + 0x0C63 | 5 | 0x0733 + 0x000D + 0x3B55 + 0x0007 + 0x05E9 + 0x01DC |
| `-` | Ġ- | 0x0235 | 1 | 0x023A |
| `Mili` | ĠM + ili | 0x0175 + 0x0D5D | 2 | 0x016C + 0x1443 |

### 行 idx 1（A.lyrics[1]）`Switch on the power line`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `Switch` | ĠSwitch | 0x8FE4 | 1 | 0x5A21 |
| `on` | Ġon | 0x0179 | 1 | 0x014B |
| `the` | Ġthe | 0x010E | 1 | 0x00FE |
| `power` | Ġpower | 0x091D | 1 | 0x071F |
| `line` | Ġline | 0x0A02 | 1 | 0x0549 |

### 行 idx 11（A.lyrics[11]）`Then I will give you my circumference`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `Then` | ĠThen | 0x1653 | 1 | 0x0B70 |
| `I` | ĠI | 0x0156 | 1 | 0x0130 |
| `will` | Ġwill | 0x02F7 | 1 | 0x021F |
| `give` | Ġgive | 0x0D93 | 1 | 0x0836 |
| `you` | Ġyou | 0x01B8 | 1 | 0x0154 |
| `my` | Ġmy | 0x0402 | 1 | 0x0259 |
| `circumference` | Ġcircumference | 0xD80D | 1 | 0x1090E |

### 行 idx 12（A.lyrics[12]）`If I'm a sine wave`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `If` | ĠIf | 0x0804 | 1 | 0x04F9 |
| `I'm` | ĠI + 'm | 0x0156 + 0x11DB | 2 | 0x0130 + 0x0006 + 0x004C |
| `a` | Ġa | 0x0104 | 1 | 0x00F5 |
| `sine` | Ġsine | 0x4437 | 1 | 0x78C8 |
| `wave` | Ġwave | 0x2585 | 1 | 0x1AE7 |

### 行 idx 21（A.lyrics[21]）`To A.D to B.C`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `To` | ĠTo | 0x0A28 | 1 | 0x086E |
| `A.D` | ĠA + .D | 0x014E + 0x1481 | 2 | 0x0152 + 0x000D + 0x0023 |
| `to` | Ġto | 0x0130 | 1 | 0x0114 |
| `B.C` | ĠB + .C | 0x0196 + 0x0DCD | 2 | 0x017C + 0x000D + 0x0022 |

### 行 idx 37（A.lyrics[37]）`Then I will purr for your enjoyment`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `Then` | ĠThen | 0x1653 | 1 | 0x0B70 |
| `I` | ĠI | 0x0156 | 1 | 0x0130 |
| `will` | Ġwill | 0x02F7 | 1 | 0x021F |
| `purr` | Ġpur + r | 0x0AF5 + 0x0054 | 2 | 0x07A1 + 0x0051 |
| `for` | Ġfor | 0x016A | 1 | 0x0147 |
| `your` | Ġyour | 0x030E | 1 | 0x0208 |
| `enjoyment` | Ġenjoyment | 0xBA6F | 1 | 0x70FB |

### 行 idx 62（A.lyrics[62]）`Challenging your god`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `Challenging` | ĠChall + enging | 0x46CA + 0xF5C7 | 2 | 0x506B + 0x11114 |
| `your` | Ġyour | 0x030E | 1 | 0x0208 |
| `god` | Ġgod | 0x2AD1 | 1 | 0x2787 |

### 行 idx 77（A.lyrics[77]）`I've studied`

| 词 | V4 真实片段 | **V4 真实 ID(hex)** | 片数 | 上一版 llm7b 的 ID(hex) |
|---|---|---|---|---|
| `I've` | ĠI + 've | 0x0156 + 0x1496 | 2 | 0x0130 + 0x0006 + 0x0139 |
| `studied` | Ġstudied | 0x2CB2 | 1 | 0x2A7C |


> 注：`deepseek-v4.1-flash`（V3）在本页不改动；因 V4 与 V3 基表相同，上表 V4 的 ID 与页面
> `deepseek-v4.1-flash` 的 ID 逐片一致，故不再重复列。上一版 `deepseek-v4-flash-0731` 用的是
> llm-7b-base，即上表"上一版 llm7b 的 ID"列。

特别地，成片 **t≈87.4s** 的 `Then I will purr for your enjoyment`（`A.lyrics[37]`，t0=82.40s）
逐词 **V4** ID 为：`Then`=0x1653、`I`=0x0156、`will`=0x02F7、`purr`=0x0AF5+0x0054、
`for`=0x016A、`your`=0x030E、`enjoyment`=0xBA6F，行合计 `1+1+1+2+1+1+1 = 8`；
对应上一版 llm7b：`Then`=0x0B70、`I`=0x0130、`will`=0x021F、`purr`=0x07A1+0x0051、
`for`=0x0147、`your`=0x0208、`enjoyment`=0x70FB。

### c) 确定性

* 同一句连续两次 `encode` 结果完全相同（V4 与 V3 均 `True`）；
* **整行 encode == 逐词 encode 拼接**：V4 全 **87/87 行**一致（0 处不一致）；
* 同一词在不同行得到完全相同的 ID 数组：161 个去重词、差异 **0**。

### d) 行合计自洽与 ID 分布

* 行合计 = 该行各词片数之和，87/87 恒成立；全 87 行总片数 = V4 **442**（上一版 llm7b 461，V3 442）；
* ID 分布（对全部真实 ID 排序统计）：

| 页面名 | 真实词表 | min | median | max | 总片数 |
|---|---|---|---|---|---|
| deepseek-v4-flash-0731 | V4共享 | min 10 (0xa) | median 2042 (0x7fa) | max 122938 (0x1e03a) | 442 |
| deepseek-v4-flash-0731(上一版) | llm-7b-base | min 6 (0x6) | median 768 (0x300) | max 94580 (0x17174) | 461 |
| deepseek-v4.1-flash | DeepSeek-V3 | min 10 (0xa) | median 2042 (0x7fa) | max 122938 (0x1e03a) | 442 |

每词片数分布：

* V4：1 片 364 词 / 2 片 27 词 / 3 片 1 词 / 4 片 4 词 / 5 片 1 词；
* llm-7b（上一版）：1 片 365 词 / 2 片 11 词 / 3 片 16 词 / 5 片 4 词 / 6 片 1 词；
* V3：1 片 364 词 / 2 片 27 词 / 3 片 1 词 / 4 片 4 词 / 5 片 1 词。

### e) 页面核验

* `cd mv/pv/web && node dbg2.mjs` → `t=0/3/8/20/40/70/100/130/160/190/205` **全部 ok**，无 PAGEERROR；
* `MV_URL=http://127.0.0.1:8901/web/mv.html node render.mjs /tmp/v4work/frame 1 87.4 87.4 --png`
  抽 1 帧（t=87.4），画面显示 `Then I will purr for your enjoyment`，词下为
  `0x1653 0x0156 0x02F7 0x0AF5 0x0054 0x016A 0x030E 0xBA6F`，
  右下 `本行 8 tok · tokenizer: deepseek-v4-flash-0731`；
* Playwright 全量核对：87 行 × 2 模型 = **174** 次 `window.rbAuditLine`，与 Python tokenizers 结果逐片比对
  **badIds = 0 / badSum = 0**（显示合计 badShown = 0），无 `pageerror`；
  `renderFrame(130)` 切到 `deepseek-v4.1-flash`，title 写明"暂用 DeepSeek-V3 替身"。

### f) 最终文件 md5 / size

编辑前后 md5（编辑窗口前已确认 `mv.html` 连续 120s 无改动）：

| 时刻 | md5 | size |
|---|---|---|
| 打补丁前（静默基线） | `95c3cf1ff1ef174acee067cc9547eb50` | 138,716 B |
| 打补丁后（最终） | `54f9f921e5a9c631baa86338fbd7dcf2` | 139,894 B |

* 改动文件：`mv/pv/web/mv.html`；
* 更新文件：`mv/pv/TOKENIZER.md`（本文件）；
* 工具版本：tokenizers 0.23.3，Python 3.12.3。

## 5. 诚实标注（重要）

* `deepseek-v4-flash-0731`、`deepseek-v4.1-flash` 是页面剧情里的**虚构模型名**，**不是**词表本名；
* `deepseek-v4-flash-0731` 绑定 **DeepSeek-V4 系列共享词表**（来源 `AndersonBY/deepseek-tokenizer`
  @ `525044c`，README 声明由 `deepseek-ai/DeepSeek-V4-Pro` 与 `deepseek-ai/DeepSeek-V4-Flash` 共享）；
* `deepseek-v4.1-flash` 因 **v4.1 词表未公开**，**暂用 deepseek-ai/DeepSeek-V3 替身**；
* 页面里每个词格的 `title`、`#rb-sum` 的 `title`、`#rb-tk` 的 `title`、页面 `<title>` 都写清了上述口径；
  不要把真实词表说成就是那个虚构模型。
