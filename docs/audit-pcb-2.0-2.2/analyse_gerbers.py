"""Rebuild the U6 audit from the two Gerber ZIPs, without changing firmware.

Run from any directory with Python, gerbonara, shapely and matplotlib installed:
    python docs/audit-pcb-2.0-2.2/analyse_gerbers.py

Lengths describe carrier trace centerlines, not analog timing or module wiring.
"""
from pathlib import Path
from zipfile import ZipFile
import hashlib
import heapq
import json
import math
import warnings

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.path import Path as PlotPath
from matplotlib.patches import PathPatch
from gerbonara import GerberFile, ExcellonFile
from gerbonara.graphic_objects import Flash, Line as GerberLine
from gerbonara.graphic_primitives import Circle, Line
from shapely.geometry import Point, Polygon, LineString
from shapely.geometry.polygon import orient
from shapely.ops import unary_union
from shapely.strtree import STRtree

OUT = Path(__file__).resolve().parent
REPO = OUT.parent.parent


def primitive_shape(p):
    if isinstance(p, Circle):
        return Point(p.x, p.y).buffer(p.r, quad_segs=32)
    if isinstance(p, Line):
        return LineString([(p.x1, p.y1), (p.x2, p.y2)]).buffer(p.width / 2, quad_segs=16)
    try:
        return Polygon(p.to_arc_poly().approximate_arcs(max_error=.001).outline).buffer(0)
    except ZeroDivisionError:
        # Degenerate, zero-radius silkscreen strokes carry no area.
        return Polygon()


def layer_shape(g):
    result, batch, last = Polygon(), [], None
    for obj in g.objects:
        for p in obj.to_primitives():
            if last is not None and p.polarity_dark != last:
                geom = unary_union(batch)
                result = result.union(geom) if last else result.difference(geom)
                batch = []
            batch.append(primitive_shape(p))
            last = p.polarity_dark
    if batch:
        geom = unary_union(batch)
        result = result.union(geom) if last else result.difference(geom)
    return result


PADS = {
    "ESP_GND": (-18.87509, 23.83990),
    "ESP_D3_TFT_CS": (-13.79509, 40.00497),
    "ESP_D5_ETH_CS": (-8.71509, 40.00497),
    "ESP_D6_TFT_DC": (-6.17509, 40.00497),
    "ESP_D7_RESET": (-6.17509, 23.83990),
    "ESP_D8_SCLK": (-8.71509, 23.83990),
    "ESP_D9_MISO": (-11.25509, 23.83990),
    "ESP_D10_MOSI": (-13.79509, 23.83990),
    "UPWR_OUTPUT": (3.20512, 26.35501),
}
for x, name in zip([26.03515, 28.57515, 31.11515, 33.65515, 36.19515, 38.73515],
                   ["INT", "CS", "SCLK", "MOSI", "GND1", "GND2"]):
    PADS["U6_" + name] = (x, 46.35497)
for x, name in zip([26.03515, 28.57515, 31.11515, 33.65515, 36.19515, 38.73515],
                   ["MISO", "RST", "NC", "3V3_1", "3V3_2", "GND3"]):
    PADS["U6_" + name] = (x, 26.03497)
for y, name in zip([29.96867, 27.42867, 24.88867, 22.34867, 19.80867, 17.26867, 14.72867],
                   ["CS", "DC", "RST", "MOSI", "CLK", "PWR", "GND"]):
    PADS["TFT_" + name] = (76.27965, y)


def load_board(revision):
    archive = REPO / f"gerber {revision}.zip"
    objects, shapes, holes = {}, {}, []
    with ZipFile(archive) as z:
        for layer, filename in [("top", "TopLayer.GTL"), ("bottom", "BottomLayer.GBL"), ("silk", "TopSilkscreenLayer.GTO")]:
            g = GerberFile.from_string(z.read("Gerber_" + filename).decode())
            objects[layer], shapes[layer] = g.objects, layer_shape(g)
        for filename in ["Drill_PTH_Through.DRL", "Drill_PTH_Through_Via.DRL"]:
            holes += ExcellonFile.from_string(z.read(filename).decode(), plated=True).objects
    polys, layers = [], []
    for layer in ["top", "bottom"]:
        geom = shapes[layer]
        parts = list(geom.geoms) if hasattr(geom, "geoms") else [geom]
        polys += parts
        layers += [layer] * len(parts)
    tree = STRtree(polys)
    parent = list(range(len(polys)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for hole in holes:
        if hasattr(hole, "x"):
            pts = [Point(hole.x, hole.y)]
        else:
            pts = [Point(hole.x1, hole.y1), Point(hole.x2, hole.y2)]
        for pt in pts:
            ids = [int(i) for i in tree.query(pt.buffer(.01)) if polys[int(i)].distance(pt) < .01]
            for i in ids[1:]:
                parent[find(i)] = find(ids[0])
    nets = {}
    for name, xy in PADS.items():
        pt = Point(*xy)
        ids = [int(i) for i in tree.query(pt.buffer(.01))
               if polys[int(i)].distance(pt) < .01 and (name != "UPWR_OUTPUT" or layers[int(i)] == "top")]
        assert ids, (revision, name)
        assert len({find(i) for i in ids}) == 1, (revision, name, "unjoined plated pad")
        nets[name] = find(ids[0])
    groups = {}
    for name, net in nets.items():
        groups.setdefault(net, []).append(name)
    return dict(revision=revision, objects=objects, shapes=shapes, holes=holes, polys=polys,
                layers=layers, parent=[find(i) for i in range(len(polys))], nets=nets,
                groups=sorted(sorted(names) for names in groups.values()),
                sha256=hashlib.sha256(archive.read_bytes()).hexdigest())


def routes(board):
    nodes, edges, flashes, traces = [], [], [], []
    for layer in ["top", "bottom"]:
        for obj in board["objects"][layer]:
            if isinstance(obj, GerberLine):
                a = len(nodes)
                nodes += [(obj.x1, obj.y1, layer), (obj.x2, obj.y2, layer)]
                edges.append((a, a + 1, math.dist(nodes[a][:2], nodes[a+1][:2])))
                traces.append((a, a + 1, LineString([nodes[a][:2], nodes[a+1][:2]])))
            elif isinstance(obj, Flash):
                a = len(nodes)
                nodes.append((obj.x, obj.y, layer))
                for p in obj.to_primitives():
                    if p.polarity_dark:
                        flashes.append((a, primitive_shape(p)))
    points = [Point(x, y) for x, y, _ in nodes]
    tree = STRtree(points)
    for i, pt in enumerate(points):
        for jj in tree.query(pt.buffer(.02)):
            j = int(jj)
            if j > i and nodes[i][2] == nodes[j][2] and pt.distance(points[j]) < .02:
                edges.append((i, j, pt.distance(points[j])))
    for i, shape in flashes:
        for jj in tree.query(shape):
            j = int(jj)
            if i != j and nodes[i][2] == nodes[j][2] and shape.distance(points[j]) < .0001:
                edges.append((i, j, points[i].distance(points[j])))
    for a, b, line in traces:
        for ii in tree.query(line.buffer(.001)):
            i = int(ii)
            if i not in (a, b) and nodes[a][2] == nodes[i][2] and line.distance(points[i]) < .001:
                edges += [(a, i, points[a].distance(points[i])), (b, i, points[b].distance(points[i]))]
    for hole in board["holes"]:
        pt = Point(hole.x, hole.y) if hasattr(hole, "x") else Point((hole.x1+hole.x2)/2, (hole.y1+hole.y2)/2)
        ids = [int(i) for i in tree.query(pt.buffer(.02)) if points[int(i)].distance(pt) < .02]
        for a in ids:
            for b in ids:
                if a < b:
                    edges.append((a, b, 0))
    adj = {}
    for a, b, distance in edges:
        adj.setdefault(a, []).append((b, distance))
        adj.setdefault(b, []).append((a, distance))
    padnodes = {name: min((i for i in range(len(nodes)) if nodes[i][2] == "top"),
                          key=lambda i: points[i].distance(Point(*xy))) for name, xy in PADS.items()}

    def shortest(source, target, module_ground=False):
        # Optional ideal bridges show why carrier-only path lengths must not be
        # confused with the populated module's electrical equivalent.
        bridges = [padnodes["U6_GND"+str(i)] for i in [1, 2, 3]] if module_ground else []
        heap, visited = [(0, padnodes[source])], set()
        while heap:
            distance, i = heapq.heappop(heap)
            if i in visited:
                continue
            visited.add(i)
            if i == padnodes[target]:
                return round(distance, 3)
            neighbors = adj.get(i, []) + ([(j, 0) for j in bridges] if i in bridges else [])
            for j, delta in neighbors:
                if j not in visited:
                    heapq.heappush(heap, (distance+delta, j))
        raise ValueError((board["revision"], source, target, "no trace path"))

    pairs = [("ESP_D5_ETH_CS", "U6_CS"), ("ESP_D7_RESET", "U6_RST"),
             ("ESP_D8_SCLK", "U6_SCLK"), ("ESP_D9_MISO", "U6_MISO"),
             ("ESP_D10_MOSI", "U6_MOSI"), ("ESP_D8_SCLK", "TFT_CLK"),
             ("ESP_D10_MOSI", "TFT_MOSI"), ("ESP_GND", "TFT_GND"),
             ("UPWR_OUTPUT", "U6_3V3_1")]
    pairs += [("ESP_GND", "U6_GND"+str(i)) for i in [1, 2, 3]]
    return {a+" -> "+b: {"carrier_mm": shortest(a, b),
                          "ideal_module_ground_bridges_mm": shortest(a, b, True)} for a, b in pairs}


def draw(ax, geom, color, alpha=1):
    for poly in (list(geom.geoms) if hasattr(geom, "geoms") else [geom]):
        if poly.is_empty:
            continue
        vertices, codes = [], []
        poly = orient(poly)
        for ring in [poly.exterior] + list(poly.interiors):
            pts = list(ring.coords)
            vertices += pts
            codes += [PlotPath.MOVETO] + [PlotPath.LINETO]*(len(pts)-2) + [PlotPath.CLOSEPOLY]
        ax.add_patch(PathPatch(PlotPath(vertices, codes), facecolor=color, edgecolor="none", alpha=alpha))


def ground(ax, board):
    draw(ax, board["shapes"]["silk"], "#777", .6)
    gid = board["nets"]["ESP_GND"]
    for layer, color in [("bottom", "#2977c5"), ("top", "#e84937")]:
        geoms = [p for p, l, n in zip(board["polys"], board["layers"], board["parent"]) if n == gid and l == layer]
        draw(ax, unary_union(geoms), color, .9)
    ax.set_aspect("equal")
    ax.set_xlim(-25, 84)
    ax.set_ylim(-6, 53)
    ax.set_xlabel("X Gerber (mm)")
    ax.set_ylabel("Y Gerber (mm)")
    ax.grid(alpha=.12)


def label(ax, text, point, position):
    ax.annotate(text, point, xytext=position, fontsize=10, fontweight="bold",
                arrowprops={"arrowstyle": "->", "color": "#111"},
                bbox={"fc": "white", "ec": "none", "alpha": .94})


def figures(boards):
    fig, axs = plt.subplots(2, 1, figsize=(15, 16))
    for ax, board in zip(axs, boards):
        ground(ax, board)
        label(ax, "GND XIAO", PADS["ESP_GND"], (-23, 18))
        label(ax, "GND TFT", PADS["TFT_GND"], (65, 5))
        label(ax, "U6 monté : GND inférieur", PADS["U6_GND3"], (39, 34))
        if board["revision"] == "2.0":
            label(ax, "Liaison centrale présente", (24, 29.39138), (13, 19))
        else:
            label(ax, "Passage entre couches par le\ntrou oblong négatif batterie", (1.83098, 13.53741), (5, 15))
            label(ax, "Retour par U16 / U15 puis U5", (14, -.635), (19, 6))
        ax.set_title(f'PCB {board["revision"]} : cuivre GND — rouge dessus, bleu dessous', fontsize=14)
    fig.suptitle("W5500 soudé sur U6 — cuivre du support seul\nLes connexions internes du module ne sont pas représentées.", fontsize=14)
    fig.tight_layout(rect=(0, 0, 1, .96))
    fig.savefig(OUT / "comparaison-masses.png", dpi=170, bbox_inches="tight")
    plt.close(fig)

    fig, (ax, detail) = plt.subplots(1, 2, figsize=(17, 8), gridspec_kw={"width_ratios": [2.1, 1]})
    ground(ax, boards[1])
    a, b, c = PADS["ESP_GND"], PADS["U6_GND3"], PADS["TFT_GND"]
    for dest in [b, c]:
        ax.plot([a[0], dest[0]], [a[1], dest[1]], "--", color="#218a46", linewidth=2.5)
    for name, pt, pos in [("A : GND XIAO", a, (-23, 18)), ("B : GND inférieur U6", b, (39, 32)),
                           ("C : GND TFT", c, (62, 7)), ("D : trou oblong\nnégatif batterie", (1.83098, 13.53741), (6, 12))]:
        ax.scatter(*pt, s=85, facecolor="white", edgecolor="#218a46", zorder=5)
        label(ax, name, pt, pos)
    ax.set_title("2.2 — fil de test A-B en premier, puis A-C si nécessaire\nPointillés verts : liaisons GND, pose hors tension", fontsize=11)
    upper = ["INT", "CS", "SCLK", "MOSI", "GND1", "GND2"]
    lower = ["MISO", "RST", "NC", "3V3", "3V3", "GND3"]
    for y, names in [(2, upper), (0, lower)]:
        for x, name in enumerate(names):
            color = "#218a46" if name.startswith("GND") else "#bf3d2c" if name == "3V3" else "#253e6a"
            detail.scatter(x, y, s=150, color=color)
            detail.text(x, y+(.4 if y else -.4), name, ha="center", va="center", fontsize=10, rotation=45)
    detail.plot([-.6, 5.6, 5.6, -.6, -.6], [-1.2, -1.2, 3.2, 3.2, -1.2], color="#888")
    detail.text(2.5, 3.8, "U6 central, vu depuis le dessus\nRangées horizontales de six broches", ha="center", fontsize=12)
    detail.text(2.5, -2.1, "Rangée supérieure : Y = 46,355 mm\nRangée inférieure : Y = 26,035 mm\nX = 26,035 à 38,735 mm, pas 2,54 mm", ha="center", fontsize=10)
    detail.text(2.5, -3.2, "B : GND3, dernière broche à droite\nde la rangée inférieure", ha="center", fontsize=11, color="#176832")
    detail.set_aspect("equal")
    detail.set_xlim(-1, 6)
    detail.set_ylim(-4.2, 4.5)
    detail.axis("off")
    fig.tight_layout()
    fig.savefig(OUT / "points-mesure.png", dpi=180, bbox_inches="tight")
    plt.close(fig)


def main():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        boards = [load_board(v) for v in ["2.0", "2.2"]]
    assert boards[0]["groups"] == boards[1]["groups"], "Named copper connectivity changed"
    result = {b["revision"]: {"archive_sha256": b["sha256"], "named_groups": b["groups"], "paths": routes(b)} for b in boards}
    (OUT / "mesures-u6.json").write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n")
    figures(boards)
    for v, r in result.items():
        print(v, json.dumps(r["paths"], ensure_ascii=False))
    print("Named critical connectivity agrees; lengths, U6 figures and JSON regenerated.")


if __name__ == "__main__":
    main()
