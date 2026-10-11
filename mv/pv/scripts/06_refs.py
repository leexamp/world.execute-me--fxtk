#!/usr/bin/env python3
"""06_refs.py — 收集用户放进 mv/ 的"每句歌词对应参考图", 并和 STORYBOARD.md 对上。

规则:
  1. 扫 mv/ 下的图片 (png/jpg/jpeg/webp/gif/bmp), 排除 mv/pv (我自己的素材) 和 mv/.venv;
  2. 解析 mv/STORYBOARD.md 里的图片引用: ![](path) / <img src=...> / 裸路径;
  3. 给每张图找到它**前面最近的小节标题** (### / #### / **`歌词`**) 作为它对应的歌词节点;
  4. 没被 doc 引用的图, 按文件名顺序保留 (页码/编号), 不丢;
  5. 输出 mv/pv/lyrics/refs.json + 一张总览图 mv/pv/build/refs_sheet.png

跑法: python3 mv/pv/scripts/06_refs.py
"""
from __future__ import annotations
import json, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]          # 仓库根
MV = ROOT / 'mv'
PV = MV / 'pv'
IMG_EXT = {'.png', '.jpg', '.jpeg', '.webp', '.gif', '.bmp'}


def collect():
    out = []
    for p in sorted(MV.rglob('*')):
        if p.suffix.lower() not in IMG_EXT or not p.is_file():
            continue
        rel = p.relative_to(MV).as_posix()
        if rel.startswith(('pv/', '.venv/', 'audio/')):
            continue
        out.append(p)
    return out


def storyboard_sections():
    doc = MV / 'STORYBOARD.md'
    if not doc.exists():
        return None, []
    txt = doc.read_text(encoding='utf-8')
    # 小节标题: markdown 标题 或 **`歌词`** 这种加粗行
    heads = []
    for m in re.finditer(r'^(#{2,4}\s*(.+?)|\*\*(`[^`]*`[^*]*)\*\*)\s*$', txt, re.M):
        heads.append((m.start(), (m.group(2) or m.group(3) or '').strip()))
    refs = []
    for m in re.finditer(r'!\[[^\]]*\]\(([^)]+)\)|<img[^>]+src=["\']([^"\']+)["\']|(?<![\w/])([\w\u4e00-\u9fff./\-]+\.(?:png|jpg|jpeg|webp|gif|bmp))', txt):
        path = next(g for g in m.groups() if g)
        head = ''
        for pos, h in heads:
            if pos < m.start():
                head = h
        refs.append({'path': path.strip(), 'at': m.start(), 'section': head})
    return txt, refs


def main():
    imgs = collect()
    txt, refs = storyboard_sections()
    by_name = {p.name: p for p in imgs}
    items, used = [], set()
    for r in refs:
        name = Path(r['path']).name
        p = by_name.get(name)
        if p is None:
            # doc 里引用了但文件还没到 —— 记下来, 别假装有
            items.append({'section': r['section'], 'path': r['path'], 'file': None, 'state': '文档已引用·文件未到'})
            continue
        used.add(p)
        items.append({'section': r['section'], 'path': r['path'], 'file': p.relative_to(ROOT).as_posix(), 'state': 'ok'})
    for p in imgs:
        if p in used:
            continue
        if p.name.startswith('v25_demo_'):
            continue
        items.append({'section': '(文档未引用)', 'path': p.name, 'file': p.relative_to(ROOT).as_posix(), 'state': '仅文件'})
    out = PV / 'lyrics' / 'refs.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({'count': len(items), 'items': items}, ensure_ascii=False, indent=1), encoding='utf-8')
    miss = [i for i in items if i['state'] != 'ok']
    print(f'参考图 {len(items)} 张 -> {out}')
    for i in items:
        print(f"  [{i['state']}] {i['section']}  <-  {i['path']}")

    # 总览图: 一次性看完所有参考图
    try:
        from PIL import Image
        files = [ROOT / i['file'] for i in items if i.get('file')]
        if files:
            cols = min(5, len(files)); rows = (len(files) + cols - 1) // cols
            CW, CH = 300, 170
            sheet = Image.new('RGB', (cols * CW, rows * CH), (12, 14, 20))
            for k, f in enumerate(files):
                try:
                    im = Image.open(f).convert('RGB'); im.thumbnail((CW - 8, CH - 8))
                    sheet.paste(im, ((k % cols) * CW + 4, (k // cols) * CH + 4))
                except Exception as e:
                    print('  缩略失败', f, e)
            sheet_dir = PV / 'build'; sheet_dir.mkdir(parents=True, exist_ok=True)
            sheet_path = sheet_dir / 'refs_sheet.png'
            sheet.save(sheet_path)
            print('总览图 ->', sheet_path)
    except ImportError:
        pass


if __name__ == '__main__':
    main()