#!/usr/bin/env python3
import argparse
import base64
import glob
import io
import os
import re
import subprocess
import sys

from PIL import Image, ImageChops

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAF_FRAME_DUMP = os.path.join(REPO, "scripts", "caf_frame_dump.exe")
CAF_DUMP = os.path.join(REPO, "scripts", "caf_dump.exe")
BG = (40, 40, 40) 

PLAYER_LABELS = {
    0: "idle (default / one-handed)", 1: "walk", 2: "run",
    3: "fiery collapse -> death?", 4: "44-frame sequence",
    5: "1H attack swing 1", 6: "1H attack swing 2", 7: "1H combo / unarmed",
    8: "1H attack finisher", 9: "single frame", 10: "attack (ranged/blunt?)",
    11: "attack", 12: "single frame", 13: "attack", 14: "single frame",
    15: "2H attack swing 1", 16: "2H attack swing 2", 17: "2H combo 2",
    18: "2H combo 3", 19: "axe swing 1", 20: "axe swing 2",
    21: "axe combo windup", 22: "axe spin loop (dir 8)", 23: "axe combo recovery",
    24: "idle (axe/blunt)", 25: "idle (two-handed)", 26: "walk (heavy)",
    27: "run (axe/blunt)", 28: "run (two-handed)",
}

PRESETS = {
    "player": [
        ("Male", "ShadowFlare/Player/Male", "Animation00"),
        ("Female", "ShadowFlare/Player/Female", "Animation00"),
    ],
}


def chart_count(char_dir, stem):
    caf = os.path.join(char_dir, stem + ".Caf")
    out = subprocess.run([CAF_DUMP, caf], capture_output=True, text=True).stdout
    m = re.search(r"charts=(\d+)", out)
    return int(m.group(1)) if m else 0


def render_chart(char_dir, stem, chart, out_prefix, directions, max_frames=60):
    for d in directions:
        for f in glob.glob(out_prefix + "_*.bmp"):
            os.remove(f)
        subprocess.run([CAF_FRAME_DUMP, char_dir, str(chart), str(d), out_prefix,
                        str(max_frames), stem],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        files = sorted(glob.glob(out_prefix + "_*.bmp"))
        if files:
            return files, d
    return [], None


def content_bbox(frames):
    bb = None
    for im in frames:
        bg = Image.new("RGB", im.size, BG)
        b = ImageChops.difference(im.convert("RGB"), bg).getbbox()
        if b:
            bb = b if bb is None else (min(bb[0], b[0]), min(bb[1], b[1]),
                                       max(bb[2], b[2]), max(bb[3], b[3]))
    return bb


def build_gif(frame_paths, target_h=150, pad=6, frame_ms=110):
    frames = [Image.open(f).convert("RGB") for f in frame_paths]
    bb = content_bbox(frames) or (0, 0, frames[0].width, frames[0].height)
    W, H = frames[0].size
    bb = (max(0, bb[0] - pad), max(0, bb[1] - pad), min(W, bb[2] + pad), min(H, bb[3] + pad))
    frames = [im.crop(bb) for im in frames]
    scale = max(1, round(target_h / max(1, frames[0].height)))  # nearest-upscale tiny sprites
    if scale > 1:
        frames = [im.resize((im.width * scale, im.height * scale), Image.NEAREST) for im in frames]
    buf = io.BytesIO()
    frames[0].save(buf, format="GIF", save_all=True, append_images=frames[1:],
                   duration=frame_ms if len(frames) > 1 else 1000, loop=0, disposal=2)
    return base64.b64encode(buf.getvalue()).decode()


def build_tab(label, char_dir, stem, directions, workdir):
    abs_dir = char_dir if os.path.isabs(char_dir) else os.path.join(REPO, char_dir)
    n = chart_count(abs_dir, stem)
    if n == 0:
        raise SystemExit(f"no charts found for {abs_dir}/{stem}.Caf (missing game files or bad path?)")
    labels = PLAYER_LABELS if stem == "Animation00" else {}
    cards = []
    for c in range(n):
        files, used_dir = render_chart(abs_dir, stem, c, os.path.join(workdir, f"c{c}"), directions)
        if not files:
            continue
        b64 = build_gif(files)
        role = labels.get(c, "")
        dirnote = "" if used_dir == directions[0] else f" · dir {used_dir}"
        cards.append(
            f'<figure class="card"><img src="data:image/gif;base64,{b64}" alt="chart {c}"/>'
            f'<figcaption><span class="idx">Chart {c}</span>'
            f'<span class="fc">{len(files)} frame{"s" if len(files) != 1 else ""}{dirnote}</span>'
            f'<span class="lab">{role}</span></figcaption></figure>')
    return label, "".join(cards)


PAGE = """<!doctype html><html><head><meta charset="utf-8"><title>{title}</title>
<style>
 body{{margin:0;background:#15151a;color:#e8e8ee;font:14px/1.4 system-ui,Segoe UI,Arial,sans-serif;padding:18px}}
 h1{{font-size:19px;margin:0 0 4px}}
 p.note{{color:#a9a9b6;margin:0 0 14px;max-width:960px}}
 .tabs{{display:flex;gap:6px;margin-bottom:14px;flex-wrap:wrap}}
 .tabs button{{background:#20202a;color:#cfcfe0;border:1px solid #33333f;border-radius:8px;
   padding:7px 16px;font:inherit;font-weight:600;cursor:pointer}}
 .tabs button.active{{background:#3a5bd9;border-color:#3a5bd9;color:#fff}}
 .panel{{display:none}} .panel.active{{display:block}}
 .grid{{display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:12px}}
 .card{{margin:0;background:#20202a;border:1px solid #33333f;border-radius:10px;padding:8px;text-align:center}}
 .card img{{width:100%;height:170px;object-fit:contain;background:#282828;border-radius:6px;image-rendering:pixelated}}
 figcaption{{display:flex;flex-direction:column;gap:1px;margin-top:6px}}
 .idx{{font-weight:700;font-size:15px}} .fc{{color:#8f8fa0;font-size:12px}} .lab{{color:#cfcfe0;font-size:12px;min-height:1em}}
</style></head><body>
<h1>{title}</h1>
<p class="note">{note}</p>
<div class="tabs">{tabbtns}</div>
{panels}
<script>
 document.querySelectorAll('.tabs button').forEach(function(b){{
   b.onclick=function(){{
     document.querySelectorAll('.tabs button').forEach(x=>x.classList.remove('active'));
     document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));
     b.classList.add('active');
     document.getElementById('panel-'+b.dataset.i).classList.add('active');
   }};
 }});
</script>
</body></html>"""


def main():
    ap = argparse.ArgumentParser(description="Render a character's CAF charts into a HTML gallery.")
    args = ap.parse_args()

    if not os.path.exists(CAF_FRAME_DUMP) or not os.path.exists(CAF_DUMP):
        raise SystemExit("scripts/caf_frame_dump.exe and scripts/caf_dump.exe are required "
                         "(build them from scripts/*.c first).")

    if args.source:
        sources = []
        for s in args.source:
            parts = s.split(":")
            if len(parts) < 3:
                raise SystemExit(f"--source must be LABEL:PATH:STEM, got {s!r} "
                                 "()")
            label = parts[0]
            stem = parts[-1]
            path = ":".join(parts[1:-1]) 
            sources.append((label, path, stem))
        preset_name = "custom"
    else:
        sources = PRESETS[args.preset]
        preset_name = args.preset

    out = args.out or os.path.join(REPO, "tools", "generated", f"{preset_name}_anim_gallery.html")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    workdir = os.path.join(REPO, "tools", "generated", "_frames")
    os.makedirs(workdir, exist_ok=True)

    directions = [args.direction] + [d for d in (0, 8) if d != args.direction]

    tabbtns, panels = [], []
    for i, (label, path, stem) in enumerate(sources):
        print(f"[{i+1}/{len(sources)}] rendering {label} ({path}/{stem}.Caf) ...", flush=True)
        lab, grid = build_tab(label, path, stem, directions, workdir)
        active = " active" if i == 0 else ""
        tabbtns.append(f'<button class="tabbtn{active}" data-i="{i}">{lab}</button>')
        panels.append(f'<div class="panel{active}" id="panel-{i}"><div class="grid">{grid}</div></div>')

    for f in glob.glob(os.path.join(workdir, "c*_*.bmp")):
        os.remove(f)

    title = "Animation charts — " + " / ".join(l for l, _, _ in sources)
    note = ""
    html = PAGE.format(title=title, note=note, tabbtns="".join(tabbtns), panels="".join(panels))
    with open(out, "w", encoding="utf-8") as f:
        f.write(html)
    print("wrote", out)


if __name__ == "__main__":
    main()
