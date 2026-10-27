"""Generate the figures of docs/MATERIAL.md.

Usage (from the repository root, after building the `render_material_cell` target):

    python docs/figures/materials/make_material_figures.py --render build/render_material_cell

Writes into docs/figures/materials/:
  * sweep_*.png, presets.png, metal_roughness.png, frosted_glass.png  (path-traced sphere renders)
  * fresnel.svg, ggx_lobes.svg, model_structure.svg, local_frame.svg   (computed / drawn diagrams)

Use --skip-render to regenerate only the SVG diagrams. Requires Pillow and numpy.
"""

import argparse
import math
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
CELL, LABEL_H, GAP, ROW_LABEL_W = 180, 30, 6, 150

# (name, r, g, b, metallic, roughness, ior, transmission, er, eg, eb)
BASE = dict(color=(0.8, 0.25, 0.15), metallic=0.0, roughness=0.25, ior=1.5, transmission=0.0,
            emission=(0.0, 0.0, 0.0))


def mat(**kw):
    m = dict(BASE)
    m.update(kw)
    return m


# ---------------------------------------------------------------------------------------------
# Renders
# ---------------------------------------------------------------------------------------------

def font(size):
    for name in ("segoeui.ttf", "DejaVuSans.ttf", "arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def render_cell(exe, m, path, spp):
    c, e = m["color"], m["emission"]
    args = [str(exe), str(path), *map(str, c), str(m["metallic"]), str(m["roughness"]), str(m["ior"]),
            str(m["transmission"]), *map(str, e), str(CELL), str(spp)]
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def render_all(exe, cells, spp):
    tmp = Path(tempfile.mkdtemp())
    paths = [tmp / f"{i}.png" for i in range(len(cells))]
    with ThreadPoolExecutor(max_workers=8) as pool:
        list(pool.map(lambda a: render_cell(exe, a[0], a[1], spp), zip(cells, paths)))
    return [Image.open(p).convert("RGB") for p in paths]


def tile(name, row_titles, col_labels, rows, exe, spp, title=None):
    """rows: list of rows, each a list of materials. Writes <name>.png."""
    flat = [m for row in rows for m in row]
    images = iter(render_all(exe, flat, spp))
    left = ROW_LABEL_W if row_titles else 0
    top = LABEL_H + (28 if title else 0)
    ncol, nrow = len(rows[0]), len(rows)
    w = left + ncol * CELL + (ncol - 1) * GAP
    h = top + nrow * CELL + (nrow - 1) * GAP + (0 if col_labels else -LABEL_H)
    canvas = Image.new("RGB", (w, h), (248, 250, 252))
    d = ImageDraw.Draw(canvas)
    f, fb = font(15), font(17)
    if title:
        d.text((left, 4), title, fill=(30, 41, 59), font=fb)
    for r, row in enumerate(rows):
        y = top + r * (CELL + GAP)
        if row_titles:
            d.text((6, y + CELL // 2 - 9), row_titles[r], fill=(30, 41, 59), font=fb)
        for c in range(ncol):
            x = left + c * (CELL + GAP)
            canvas.paste(next(images), (x, y))
            if r == 0 and col_labels:
                d.text((x + 4, top - LABEL_H + 6), col_labels[c], fill=(71, 85, 105), font=f)
    canvas.save(OUT / f"{name}.png", optimize=True)
    print("wrote", OUT / f"{name}.png")


def make_renders(exe, spp):
    gold = (0.95, 0.65, 0.2)

    # One parameter at a time, everything else at its default.
    sweeps = {
        "baseColor": ("baseColor", [mat(color=c) for c in
                      [(0.8, 0.25, 0.15), (0.2, 0.6, 0.25), (0.2, 0.3, 0.8), (0.9, 0.9, 0.9), (0.12, 0.12, 0.12)]],
                      ["red", "green", "blue", "white", "near black"]),
        "metallic": ("metallic", [mat(metallic=v) for v in (0, 0.25, 0.5, 0.75, 1)],
                     ["0", "0.25", "0.5", "0.75", "1"]),
        "roughness": ("roughness", [mat(roughness=v) for v in (0, 0.15, 0.3, 0.6, 1)],
                      ["0", "0.15", "0.3", "0.6", "1"]),
        "ior": ("ior", [mat(ior=v) for v in (1.0, 1.2, 1.5, 2.0, 2.4)],
                ["1.0", "1.2", "1.5", "2.0", "2.4"]),
        "transmission": ("transmission", [mat(color=(0.75, 0.88, 1.0), roughness=0.0, transmission=v)
                                          for v in (0, 0.25, 0.5, 0.75, 1)],
                         ["0", "0.25", "0.5", "0.75", "1"]),
        "emission": ("emission", [mat(emission=tuple(k * x for x in (1.0, 0.6, 0.3))) for k in (0, 0.5, 1, 2, 4)],
                     ["0", "0.5", "1", "2", "4  (x 1.0, 0.6, 0.3)"]),
    }
    names = list(sweeps)
    rows = [sweeps[n][1] for n in names]
    # a single image with all six rows: the parameter being varied is the row title
    tile("parameter_sweeps", [sweeps[n][0] for n in names], ["low", "", "", "", "high"], rows, exe, spp)

    presets = [
        ("diffuse", mat(color=(0.75, 0.75, 0.75), roughness=1.0, ior=1.0)),
        ("mirror", mat(color=(1, 1, 1), metallic=1.0, roughness=0.0)),
        ("glass", mat(color=(1, 1, 1), roughness=0.0, ior=1.5, transmission=1.0)),
        ("metal (gold, 0.3)", mat(color=gold, metallic=1.0, roughness=0.3)),
        ("plastic (red, 0.3)", mat(color=(0.8, 0.12, 0.1), roughness=0.3)),
        ("emissive", mat(color=(0, 0, 0), ior=1.0, emission=(4.0, 3.0, 1.2))),
    ]
    tile("presets", None, [p[0] for p in presets], [[p[1] for p in presets]], exe, spp)

    tile("metal_roughness", ["metallic 0\n(plastic)", "metallic 0.5", "metallic 1\n(metal)"],
         ["roughness 0", "0.2", "0.4", "0.7", "1"],
         [[mat(color=gold, metallic=m, roughness=r) for r in (0, 0.2, 0.4, 0.7, 1.0)] for m in (0, 0.5, 1)],
         exe, spp)

    tile("frosted_glass", None, ["roughness 0", "0.1", "0.2", "0.35", "0.5"],
         [[mat(color=(0.85, 0.95, 1.0), roughness=r, transmission=1.0) for r in (0, 0.1, 0.2, 0.35, 0.5)]],
         exe, spp * 3)  # rough transmission is the noisiest case


# ---------------------------------------------------------------------------------------------
# SVG diagrams
# ---------------------------------------------------------------------------------------------

FONT = "Segoe UI, system-ui, -apple-system, sans-serif"


def svg(w, h, body, title, desc, ident):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" '
            f'font-family="{FONT}" role="img" aria-labelledby="{ident}T {ident}D">\n'
            f'<title id="{ident}T">{title}</title>\n<desc id="{ident}D">{desc}</desc>\n'
            f'<defs><marker id="{ident}A" markerWidth="8" markerHeight="8" refX="6" refY="3" orient="auto">'
            f'<path d="M0,0 L6,3 L0,6 Z" fill="#475569"/></marker></defs>\n'
            f'<rect x="4" y="4" width="{w - 8}" height="{h - 8}" rx="10" fill="#f8fafc" stroke="#e2e8f0"/>\n'
            f'{body}</svg>\n')


def polyline(pts, color, width=2.5, dash=None):
    d = f' stroke-dasharray="{dash}"' if dash else ""
    p = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
    return f'<polyline points="{p}" fill="none" stroke="{color}" stroke-width="{width}"{d}/>\n'


def text(x, y, s, size=14, color="#334155", anchor="start", weight="normal"):
    return (f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" text-anchor="{anchor}" '
            f'font-weight="{weight}">{s}</text>\n')


def fresnel_dielectric(cos_i, eta):
    """Unpolarised Fresnel reflectance for light going from index 1 into index eta."""
    sin2_t = (1 - cos_i ** 2) / eta ** 2
    cos_t = np.sqrt(np.maximum(0, 1 - sin2_t))
    rs = (cos_i - eta * cos_t) / (cos_i + eta * cos_t)
    rp = (eta * cos_i - cos_t) / (eta * cos_i + cos_t)
    return 0.5 * (rs ** 2 + rp ** 2)


def fig_fresnel():
    x0, y0, w, h = 80, 40, 520, 300
    theta = np.linspace(0, 89.5, 300)
    cos_i = np.cos(np.radians(theta))

    def X(t):
        return x0 + w * t / 90

    def Y(v):
        return y0 + h * (1 - v)

    body = ""
    for v in (0, 0.25, 0.5, 0.75, 1):
        body += f'<line x1="{x0}" y1="{Y(v)}" x2="{x0 + w}" y2="{Y(v)}" stroke="#e2e8f0"/>\n'
        body += text(x0 - 10, Y(v) + 5, f"{v:g}", 13, "#64748b", "end")
    for t in (0, 15, 30, 45, 60, 75, 90):
        body += f'<line x1="{X(t)}" y1="{y0}" x2="{X(t)}" y2="{y0 + h}" stroke="#e2e8f0"/>\n'
        body += text(X(t), y0 + h + 20, f"{t}°", 13, "#64748b", "middle")
    body += f'<rect x="{x0}" y="{y0}" width="{w}" height="{h}" fill="none" stroke="#94a3b8"/>\n'
    body += text(x0 + w / 2, y0 + h + 44, "angle of incidence θ (0° = head-on, 90° = grazing)", 14, "#334155", "middle")
    body += (f'<text transform="translate(26,{y0 + h / 2}) rotate(-90)" font-size="14" fill="#334155" '
             f'text-anchor="middle">reflectance F</text>\n')

    curves = [(1.33, "#0ea5e9", "water, ior 1.33"), (1.5, "#2563eb", "glass, ior 1.5"),
              (2.4, "#7c3aed", "diamond, ior 2.4")]
    for i, (eta, col, label) in enumerate(curves):
        F = fresnel_dielectric(cos_i, eta)
        body += polyline([(X(t), Y(f)) for t, f in zip(theta, F)], col)
        body += f'<line x1="{x0 + w + 20}" y1="{y0 + 20 + i * 24}" x2="{x0 + w + 44}" y2="{y0 + 20 + i * 24}" stroke="{col}" stroke-width="3"/>\n'
        body += text(x0 + w + 52, y0 + 25 + i * 24, label, 13)
    # Schlick approximation for ior 1.5
    f0 = ((1.5 - 1) / (1.5 + 1)) ** 2
    sch = f0 + (1 - f0) * (1 - cos_i) ** 5
    body += polyline([(X(t), Y(f)) for t, f in zip(theta, sch)], "#f97316", 2, "6 4")
    body += f'<line x1="{x0 + w + 20}" y1="{y0 + 20 + 3 * 24}" x2="{x0 + w + 44}" y2="{y0 + 20 + 3 * 24}" stroke="#f97316" stroke-width="2.5" stroke-dasharray="6 4"/>\n'
    body += text(x0 + w + 52, y0 + 25 + 3 * 24, "Schlick, ior 1.5", 13)
    # metal: Schlick from F0 = gold
    gold = 0.95
    metal = gold + (1 - gold) * (1 - cos_i) ** 5
    body += polyline([(X(t), Y(f)) for t, f in zip(theta, metal)], "#ca8a04", 2.5)
    body += f'<line x1="{x0 + w + 20}" y1="{y0 + 20 + 4 * 24}" x2="{x0 + w + 44}" y2="{y0 + 20 + 4 * 24}" stroke="#ca8a04" stroke-width="3"/>\n'
    body += text(x0 + w + 52, y0 + 25 + 4 * 24, "metal, F0 = 0.95 (red ch.)", 13)
    body += text(x0 + 8, Y(0.045) - 8, f"F0 = {f0:.3f} at ior 1.5", 13, "#1e3a8a")
    return svg(860, 400, body, "Fresnel reflectance versus angle",
               "Reflectance of dielectrics of several refractive indices rises to 1 at grazing angles; a metal starts high.",
               "fr")


def ggx_D(cos_h, alpha):
    a2 = alpha ** 2
    d = cos_h ** 2 * (a2 - 1) + 1
    return a2 / (np.pi * d * d)


def ggx_lambda(cos_w, alpha):
    tan2 = np.maximum(0, 1 - cos_w ** 2) / cos_w ** 2
    return 0.5 * (np.sqrt(1 + alpha ** 2 * tan2) - 1)


def fig_ggx_lobes():
    """Reflected radiance in the plane of incidence for several roughness values (wo at 45 degrees)."""
    cx, cy, R = 330, 330, 270
    body = ""
    for r in (0.25, 0.5, 0.75, 1.0):
        body += f'<circle cx="{cx}" cy="{cy}" r="{R * r}" fill="none" stroke="#e2e8f0"/>\n'
    for a in (-60, -30, 0, 30, 60):
        x, y = cx + R * math.sin(math.radians(a)), cy - R * math.cos(math.radians(a))
        body += f'<line x1="{cx}" y1="{cy}" x2="{x:.1f}" y2="{y:.1f}" stroke="#e2e8f0"/>\n'
    body += f'<line x1="{cx - R - 20}" y1="{cy}" x2="{cx + R + 20}" y2="{cy}" stroke="#475569" stroke-width="3"/>\n'
    body += text(cx + R - 10, cy + 22, "surface", 13, "#475569", "end")
    # incoming (wo) direction at 45 deg on the left, mirror direction on the right
    ox, oy = cx - R * 0.95 * math.sin(math.radians(45)), cy - R * 0.95 * math.cos(math.radians(45))
    body += f'<line x1="{ox:.1f}" y1="{oy:.1f}" x2="{cx}" y2="{cy}" stroke="#475569" stroke-width="2" marker-end="url(#glA)"/>\n'
    body += text(ox - 6, oy - 6, "wo", 15, "#1e293b", "end", "bold")
    mx, my = cx + R * 1.02 * math.sin(math.radians(45)), cy - R * 1.02 * math.cos(math.radians(45))
    body += f'<line x1="{cx}" y1="{cy}" x2="{mx:.1f}" y2="{my:.1f}" stroke="#94a3b8" stroke-width="1.5" stroke-dasharray="6 4"/>\n'
    body += text(mx + 6, my - 4, "mirror direction", 13, "#64748b")

    theta_o = math.radians(45)
    wo = np.array([math.sin(theta_o), math.cos(theta_o)])
    th = np.radians(np.linspace(-89.5, 89.5, 720))
    colors = [("0.05", 0.05, "#2563eb"), ("0.2", 0.2, "#0ea5e9"), ("0.4", 0.4, "#16a34a"),
              ("0.7", 0.7, "#f59e0b"), ("1.0", 1.0, "#dc2626")]
    for i, (label, rough, col) in enumerate(colors):
        alpha = rough ** 2
        # in-plane directions of wi measured from the normal, wo lies at +45 on the *left*; mirror at +45 right
        wi = np.stack([np.sin(th), np.cos(th)], axis=1)
        wo_l = np.array([-wo[0], wo[1]])
        h = wi + wo_l
        h = h / np.linalg.norm(h, axis=1)[:, None]
        cos_h = h[:, 1]
        cos_i, cos_o = wi[:, 1], wo_l[1]
        G = 1 / (1 + ggx_lambda(cos_o, alpha) + ggx_lambda(cos_i, alpha))
        f = ggx_D(cos_h, alpha) * G / (4 * cos_o * cos_i)
        lobe = f * cos_i  # what the integrator weights, f * cos
        lobe = lobe / lobe.max()
        pts = [(cx + R * r * math.sin(t), cy - R * r * math.cos(t)) for t, r in zip(th, lobe)]
        body += polyline(pts, col, 2.5)
        body += f'<line x1="640" y1="{80 + i * 26}" x2="668" y2="{80 + i * 26}" stroke="{col}" stroke-width="3"/>\n'
        body += text(676, 85 + i * 26, f"roughness {label}", 13)
    body += text(640, 58, "lobe = f · cos θi", 14, "#1e293b", weight="bold")
    body += text(640, 232, "each curve is scaled to its own", 12, "#64748b")
    body += text(640, 248, "peak; smooth surfaces keep a", 12, "#64748b")
    body += text(640, 264, "narrow spike, rough ones spread", 12, "#64748b")
    body += text(640, 280, "out and the peak moves away from", 12, "#64748b")
    body += text(640, 296, "the mirror direction.", 12, "#64748b")
    return svg(880, 370, body, "Shape of the GGX reflection lobe",
               "Polar plot in the plane of incidence of the reflected distribution for increasing roughness.", "gl")


def box(x, y, w, h, label, sub=None, fill="#eff6ff", stroke="#2563eb"):
    s = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{fill}" stroke="{stroke}" stroke-width="1.8"/>\n'
    if sub:
        s += text(x + w / 2, y + h / 2 - 3, label, 14, "#1e293b", "middle", "bold")
        s += text(x + w / 2, y + h / 2 + 15, sub, 12, "#475569", "middle")
    else:
        s += text(x + w / 2, y + h / 2 + 5, label, 14, "#1e293b", "middle", "bold")
    return s


def arrow(x1, y1, x2, y2, label=None, lx=0, ly=0):
    s = f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="#475569" stroke-width="1.8" marker-end="url(#msA)"/>\n'
    if label:
        s += text((x1 + x2) / 2 + lx, (y1 + y2) / 2 + ly, label, 12, "#64748b", "middle")
    return s


def fig_structure():
    b = ""
    b += box(300, 20, 200, 44, "light hits the surface")
    # metallic split
    b += arrow(360, 64, 190, 110, "metallic", -40, -4)
    b += arrow(440, 64, 610, 110, "1 − metallic", 50, -4)
    b += box(60, 110, 260, 56, "Conductor", "reflects, Fresnel from baseColor (F0)", "#fef9c3", "#ca8a04")
    b += box(470, 110, 300, 56, "Dielectric interface", "Fresnel F(ior, angle)", "#eff6ff", "#2563eb")
    # dielectric split
    b += arrow(570, 166, 500, 220, "F", -14, -4)
    b += arrow(670, 166, 700, 220, "1 − F", 24, -4)
    b += box(380, 220, 220, 56, "Specular reflection", "white, colourless", "#f1f5f9", "#475569")
    b += box(610, 220, 200, 56, "Enters the body", "tinted by baseColor", "#eff6ff", "#2563eb")
    # transmission split
    b += arrow(670, 276, 600, 330, "transmission", -30, -4)
    b += arrow(750, 276, 790, 330, "1 − transmission", 36, -4)
    b += box(480, 330, 220, 56, "Refraction", "microfacet / specular", "#ecfdf5", "#059669")
    b += box(710, 330, 170, 56, "Diffuse", "Lambertian, baseColor", "#fef2f2", "#dc2626")
    # roughness annotation
    b += box(60, 220, 280, 70, "roughness", "0 = delta lobes, > 0 = GGX; α = roughness²", "#f5f3ff", "#7c3aed")
    b += f'<line x1="200" y1="220" x2="200" y2="166" stroke="#7c3aed" stroke-width="1.5" stroke-dasharray="5 4"/>\n'
    b += f'<line x1="340" y1="255" x2="380" y2="250" stroke="#7c3aed" stroke-width="1.5" stroke-dasharray="5 4"/>\n'
    b += box(60, 330, 280, 56, "emission", "added on top, independent of all lobes", "#fff7ed", "#ea580c")
    return svg(900, 410, b, "How the six parameters compose a BSDF",
               "Metallic selects conductor or dielectric, Fresnel splits reflection from the body, transmission splits refraction from diffuse.",
               "ms")


def fig_frame():
    b = ""
    # medium boundary
    b += '<rect x="30" y="200" width="560" height="140" fill="#dbeafe" opacity="0.6"/>\n'
    b += '<line x1="30" y1="200" x2="590" y2="200" stroke="#1e293b" stroke-width="3"/>\n'
    b += text(44, 224, "inside the object (ior)", 14, "#1e3a8a")
    b += text(44, 184, "outside (ior 1)", 14, "#475569")
    # normal
    b += '<line x1="310" y1="200" x2="310" y2="50" stroke="#16a34a" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(318, 62, "outward normal n  (+z)", 14, "#166534")
    # case 1: wo outside, wi reflected, wt refracted
    cx, cy = 310, 200
    def pt(angle, length, up=True):
        s = -1 if up else 1
        return cx + length * math.sin(math.radians(angle)), cy + s * length * math.cos(math.radians(angle))
    ox, oy = pt(-50, 150)
    rx, ry = pt(50, 150)
    tx, ty = pt(32, 130, up=False)
    b += f'<line x1="{cx}" y1="{cy}" x2="{ox:.1f}" y2="{oy:.1f}" stroke="#2563eb" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(ox - 8, oy - 6, "wo", 16, "#1e3a8a", "end", "bold")
    b += f'<line x1="{cx}" y1="{cy}" x2="{rx:.1f}" y2="{ry:.1f}" stroke="#94a3b8" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(rx + 8, ry - 4, "reflection", 14, "#64748b")
    b += f'<line x1="{cx}" y1="{cy}" x2="{tx:.1f}" y2="{ty:.1f}" stroke="#059669" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(tx + 8, ty + 14, "wi (refracted)", 14, "#065f46")
    b += text(44, 300, "wo.z &gt; 0: the ray is outside", 14, "#1e3a8a", weight="bold")
    # right panel: ray from inside
    b += text(660, 40, "Same surface, ray coming from inside", 15, "#1e293b", weight="bold")
    b += '<rect x="640" y="200" width="300" height="140" fill="#dbeafe" opacity="0.6"/>\n'
    b += '<line x1="640" y1="200" x2="940" y2="200" stroke="#1e293b" stroke-width="3"/>\n'
    px, py = 790, 200
    b += f'<line x1="{px}" y1="{py}" x2="{px}" y2="{py - 120}" stroke="#16a34a" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(px + 8, py - 100, "n (still outward)", 14, "#166534")
    wx, wy = px - 45, py + 65
    b += f'<line x1="{px}" y1="{py}" x2="{wx}" y2="{wy}" stroke="#2563eb" stroke-width="2.5" marker-end="url(#fr2A)"/>\n'
    b += text(wx - 4, wy + 16, "wo", 16, "#1e3a8a", "end", "bold")
    b += text(655, 305, "wo.z &lt; 0: the ray is inside", 14, "#1e3a8a", weight="bold")
    b += text(655, 325, "so the BSDF refracts outwards (ior → 1)", 13, "#475569")
    b += text(30, 370, "The BSDF frame is built from the outward normal, never from the normal flipped towards the ray;", 13, "#475569")
    b += text(30, 390, "otherwise wo.z would always be positive and refraction could not tell entering from leaving.", 13, "#475569")
    return svg(970, 410, b, "The local shading frame",
               "Directions are expressed in a frame whose +z axis is the outward surface normal; the sign of wo.z says which side the ray is on.",
               "fr2")


def make_diagrams():
    for name, fn in (("fresnel", fig_fresnel), ("ggx_lobes", fig_ggx_lobes),
                     ("model_structure", fig_structure), ("local_frame", fig_frame)):
        (OUT / f"{name}.svg").write_text(fn(), encoding="utf-8")
        print("wrote", OUT / f"{name}.svg")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--render", default="build/render_material_cell", help="path to render_material_cell")
    ap.add_argument("--spp", type=int, default=128)
    ap.add_argument("--skip-render", action="store_true")
    args = ap.parse_args()
    make_diagrams()
    if not args.skip_render:
        make_renders(Path(args.render).resolve(), args.spp)


if __name__ == "__main__":
    main()
