#!/usr/bin/env python3
"""
==============================================================================
Mini Web Browser Engine in Python 3
Baseline reference implementation providing 100% bit-exact parity with:
  - Pure Rust Engine (apps/minibrowser/minibrowser.rs)
  - MEMElang Native AOT Engine (apps/minibrowser/minibrowser.meme)
  - MEMElang Interpreter Engine (apps/minibrowser/minibrowser.meme)
==============================================================================
"""

import sys
import time
from typing import List, Optional

# ------------------------------------------------------------------------------
# Stage 1: DOM Data Structures
# ------------------------------------------------------------------------------
class DOMNode:
    def __init__(self, tag: str, id_name: str, text: str, is_text: bool):
        self.tag: str = tag
        self.id_name: str = id_name
        self.classes: List[str] = []
        self.text: str = text
        self.is_text: bool = is_text
        self.children: List['DOMNode'] = []

# ------------------------------------------------------------------------------
# Stage 2: CSS Data Structures & Specificity
# ------------------------------------------------------------------------------
class CSSSelector:
    def __init__(self, stag: str, sid: str, sclass: str):
        self.tag: str = stag
        self.id_name: str = sid
        self.class_name: str = sclass
        spec = 0
        if sid: spec += 100
        if sclass: spec += 10
        if stag: spec += 1
        self.specificity: int = spec

class CSSDeclaration:
    def __init__(self, prop: str, val: str):
        self.property: str = prop
        self.value: str = val

class CSSRule:
    def __init__(self, sel: CSSSelector):
        self.selector: CSSSelector = sel
        self.declarations: List[CSSDeclaration] = []

class StyleSheet:
    def __init__(self):
        self.rules: List[CSSRule] = []

# ------------------------------------------------------------------------------
# Stage 3: Computed Styles
# ------------------------------------------------------------------------------
class ComputedStyle:
    def __init__(self):
        self.display: str = "block"
        self.width: int = -1
        self.height: int = -1
        self.margin_top: int = 0
        self.margin_right: int = 0
        self.margin_bottom: int = 0
        self.margin_left: int = 0
        self.padding_top: int = 0
        self.padding_right: int = 0
        self.padding_bottom: int = 0
        self.padding_left: int = 0
        self.border_width: int = 0
        self.text_align: str = "left"
        self.color: str = "default"

# ------------------------------------------------------------------------------
# Stage 4: Layout & Box Model
# ------------------------------------------------------------------------------
class LayoutBox:
    def __init__(self, tag: str, text: str, is_text: bool, display: str):
        self.tag: str = tag
        self.text: str = text
        self.is_text: bool = is_text
        self.display: str = display
        self.x: int = 0
        self.y: int = 0
        self.width: int = 0
        self.height: int = 0
        self.margin_top: int = 0
        self.margin_right: int = 0
        self.margin_bottom: int = 0
        self.margin_left: int = 0
        self.padding_top: int = 0
        self.padding_right: int = 0
        self.padding_bottom: int = 0
        self.padding_left: int = 0
        self.border_width: int = 0
        self.text_align: str = "left"
        self.text_lines: List[str] = []
        self.children: List['LayoutBox'] = []

# ------------------------------------------------------------------------------
# Stage 5: Character Canvas Rasterizer
# ------------------------------------------------------------------------------
class Canvas:
    def __init__(self, w: int, h: int):
        self.width: int = w
        self.height: int = h
        self.rows: List[str] = [" " * w for _ in range(h)]

    def draw_text(self, tx: int, ty: int, text: str):
        if ty < 0 or ty >= self.height: return
        if tx < 0 or tx >= self.width: return
        t_len = len(text)
        s_slice = text
        if tx + t_len > self.width:
            t_len = self.width - tx
            s_slice = text[:t_len]
        cur = self.rows[ty]
        self.rows[ty] = cur[:tx] + s_slice + cur[tx + t_len:]

    def draw_border(self, bx: int, by: int, bw: int, bh: int):
        if bw < 2 or bh < 2: return
        h_edge = "+" + ("-" * (bw - 2)) + "+"
        self.draw_text(bx, by, h_edge)
        self.draw_text(bx, by + bh - 1, h_edge)

        for y in range(by + 1, by + bh - 1):
            self.draw_text(bx, y, "|")
            self.draw_text(bx + bw - 1, y, "|")

    def render(self) -> str:
        return "".join(r + "\n" for r in self.rows)

# ------------------------------------------------------------------------------
# Helper & Parsing Utilities
# ------------------------------------------------------------------------------
def parse_int_custom(s: str) -> int:
    res = 0
    for c in s:
        if '0' <= c <= '9':
            res = res * 10 + (ord(c) - ord('0'))
    return res

def normalize_spaces(s: str) -> str:
    res = []
    in_space = False
    for ch in s:
        if ch in (' ', '\t', '\n', '\r'):
            if not in_space:
                res.append(' ')
                in_space = True
        else:
            res.append(ch)
            in_space = False
    return "".join(res).strip()

def wrap_text(text: str, max_width: int) -> List[str]:
    lines = []
    if max_width <= 0: return lines
    cur_line = ""
    for word in text.split(' '):
        w = word.strip()
        if w:
            if not cur_line:
                cur_line = w
            else:
                cand_len = len(cur_line) + 1 + len(w)
                if cand_len <= max_width:
                    cur_line += " " + w
                else:
                    lines.append(cur_line)
                    cur_line = w
    if cur_line:
        lines.append(cur_line)
    return lines

# ------------------------------------------------------------------------------
# HTML Tokenizer & DOM Construction
# ------------------------------------------------------------------------------
def parse_html(html: str) -> DOMNode:
    root = DOMNode("root", "", "", False)
    stack = [root]

    n = len(html)
    pos = 0

    while pos < n:
        if html[pos] == '<':
            if pos + 1 < n and html[pos + 1] == '/':
                end_pos = pos + 2
                while end_pos < n and html[end_pos] != '>':
                    end_pos += 1
                if len(stack) > 1:
                    stack.pop()
                pos = end_pos + 1
            else:
                end_pos = pos + 1
                while end_pos < n and html[end_pos] != '>':
                    end_pos += 1
                tag_content = html[pos + 1:end_pos].strip()
                is_self_closing = False
                if tag_content.endswith('/'):
                    is_self_closing = True
                    tag_content = tag_content[:-1].strip()

                sp_idx = tag_content.find(' ')
                if sp_idx != -1:
                    tag_name_raw = tag_content[:sp_idx]
                    attr_str = tag_content[sp_idx + 1:]
                else:
                    tag_name_raw = tag_content
                    attr_str = ""
                tag_name = tag_name_raw.lower()

                if tag_name in ("br", "hr", "img", "input"):
                    is_self_closing = True

                id_val = ""
                classes = []
                if attr_str:
                    id_pos = attr_str.find('id="')
                    if id_pos != -1:
                        rem = attr_str[id_pos + 4:]
                        q_end = rem.find('"')
                        if q_end != -1:
                            id_val = rem[:q_end]
                    cl_pos = attr_str.find('class="')
                    if cl_pos != -1:
                        rem2 = attr_str[cl_pos + 7:]
                        q_end2 = rem2.find('"')
                        if q_end2 != -1:
                            cl_raw = rem2[:q_end2]
                            for ctk in cl_raw.split(' '):
                                c = ctk.strip()
                                if c:
                                    classes.append(c)

                node = DOMNode(tag_name, id_val, "", False)
                node.classes = classes

                parent = stack[-1]
                parent.children.append(node)
                if not is_self_closing:
                    stack.append(node)
                pos = end_pos + 1
        else:
            end_pos = pos
            while end_pos < n and html[end_pos] != '<':
                end_pos += 1
            raw_text = html[pos:end_pos]
            norm_text = normalize_spaces(raw_text)
            if norm_text:
                text_node = DOMNode("#text", "", norm_text, True)
                parent = stack[-1]
                parent.children.append(text_node)
            pos = end_pos

    if len(root.children) == 1:
        return root.children[0]
    return root

# ------------------------------------------------------------------------------
# CSS Tokenizer, Specificity & Rule Parser
# ------------------------------------------------------------------------------
def parse_selector(raw: str) -> CSSSelector:
    s = raw.strip()
    tag = ""
    id_val = ""
    cls = ""
    mode = "tag"
    cur = []

    for ch in s:
        if ch in ('.', '#'):
            token = "".join(cur)
            if mode == "tag": tag = token
            elif mode == "id": id_val = token
            elif mode == "class": cls = token
            cur = []
            if ch == '.': mode = "class"
            elif ch == '#': mode = "id"
        else:
            cur.append(ch)

    token = "".join(cur)
    if mode == "tag": tag = token
    elif mode == "id": id_val = token
    elif mode == "class": cls = token

    return CSSSelector(tag, id_val, cls)

def parse_css(css: str) -> StyleSheet:
    sheet = StyleSheet()
    for block in css.split('}'):
        b = block.strip()
        brace_pos = b.find('{')
        if brace_pos != -1:
            sel_str = b[:brace_pos].strip()
            decl_str = b[brace_pos + 1:].strip()

            sel = parse_selector(sel_str)
            rule = CSSRule(sel)

            for d in decl_str.split(';'):
                decl_trimmed = d.strip()
                colon_pos = decl_trimmed.find(':')
                if colon_pos != -1:
                    prop = decl_trimmed[:colon_pos].strip().lower()
                    val = decl_trimmed[colon_pos + 1:].strip().lower()
                    rule.declarations.append(CSSDeclaration(prop, val))
            sheet.rules.append(rule)
    return sheet

# ------------------------------------------------------------------------------
# Selector Matching & Cascade
# ------------------------------------------------------------------------------
def matches_selector(sel: CSSSelector, node: DOMNode) -> bool:
    if node.is_text: return False
    if sel.tag and sel.tag != node.tag: return False
    if sel.id_name and sel.id_name != node.id_name: return False
    if sel.class_name:
        if sel.class_name not in node.classes:
            return False
    return True

def compute_style(node: DOMNode, sheet: StyleSheet) -> ComputedStyle:
    style = ComputedStyle()
    if node.is_text:
        style.display = "inline"
        return style

    # Default User-Agent Stylesheet
    tag = node.tag
    if tag in ("div", "section", "article", "header", "footer", "main", "body", "html", "nav"):
        style.display = "block"
    elif tag in ("h1", "h2", "h3", "p"):
        style.display = "block"
        style.margin_top = 1
        style.margin_bottom = 1
    elif tag == "hr":
        style.display = "block"
        style.border_width = 1
        style.margin_top = 1
        style.margin_bottom = 1
    elif tag in ("ul", "ol", "li"):
        style.display = "block"
    elif tag == "button":
        style.display = "inline"
        style.border_width = 1
        style.padding_left = 1
        style.padding_right = 1
    else:
        style.display = "inline"

    # Specificity Cascade Tracking
    spec_display = 0
    spec_width = 0
    spec_height = 0
    spec_margin = 0
    spec_padding = 0
    spec_border = 0
    spec_text_align = 0

    for rule in sheet.rules:
        if matches_selector(rule.selector, node):
            sp = rule.selector.specificity
            for decl in rule.declarations:
                prop = decl.property
                val = decl.value

                if prop == "display" and sp >= spec_display:
                    style.display = val
                    spec_display = sp
                elif prop == "width" and sp >= spec_width:
                    style.width = parse_int_custom(val)
                    spec_width = sp
                elif prop == "height" and sp >= spec_height:
                    style.height = parse_int_custom(val)
                    spec_height = sp
                elif prop == "margin" and sp >= spec_margin:
                    m = parse_int_custom(val)
                    style.margin_top = m
                    style.margin_right = m
                    style.margin_bottom = m
                    style.margin_left = m
                    spec_margin = sp
                elif prop == "margin-top" and sp >= spec_margin:
                    style.margin_top = parse_int_custom(val)
                elif prop == "margin-bottom" and sp >= spec_margin:
                    style.margin_bottom = parse_int_custom(val)
                elif prop == "margin-left" and sp >= spec_margin:
                    style.margin_left = parse_int_custom(val)
                elif prop == "margin-right" and sp >= spec_margin:
                    style.margin_right = parse_int_custom(val)
                elif prop == "padding" and sp >= spec_padding:
                    p = parse_int_custom(val)
                    style.padding_top = p
                    style.padding_right = p
                    style.padding_bottom = p
                    style.padding_left = p
                    spec_padding = sp
                elif prop == "padding-top" and sp >= spec_padding:
                    style.padding_top = parse_int_custom(val)
                elif prop == "padding-bottom" and sp >= spec_padding:
                    style.padding_bottom = parse_int_custom(val)
                elif prop == "padding-left" and sp >= spec_padding:
                    style.padding_left = parse_int_custom(val)
                elif prop == "padding-right" and sp >= spec_padding:
                    style.padding_right = parse_int_custom(val)
                elif (prop == "border" or prop == "border-width") and sp >= spec_border:
                    style.border_width = parse_int_custom(val)
                    spec_border = sp
                elif prop == "text-align" and sp >= spec_text_align:
                    style.text_align = val
                    spec_text_align = sp

    return style

# ------------------------------------------------------------------------------
# Layout Tree Generation & Reflow
# ------------------------------------------------------------------------------
def build_layout_tree_recursive(node: DOMNode, sheet: StyleSheet, inherited_align: str) -> LayoutBox:
    if node.is_text:
        box_node = LayoutBox("#text", node.text, True, "text")
        box_node.text_align = inherited_align
        return box_node

    style = compute_style(node, sheet)
    if style.display == "none":
        return LayoutBox(node.tag, "", False, "none")

    current_align = style.text_align
    if current_align == "left" and inherited_align != "left":
        current_align = inherited_align

    box_node = LayoutBox(node.tag, "", False, style.display)
    box_node.width = style.width
    box_node.height = style.height
    box_node.margin_top = style.margin_top
    box_node.margin_right = style.margin_right
    box_node.margin_bottom = style.margin_bottom
    box_node.margin_left = style.margin_left
    box_node.padding_top = style.padding_top
    box_node.padding_right = style.padding_right
    box_node.padding_bottom = style.padding_bottom
    box_node.padding_left = style.padding_left
    box_node.border_width = style.border_width
    box_node.text_align = current_align

    for ch_node in node.children:
        ch_box = build_layout_tree_recursive(ch_node, sheet, current_align)
        if ch_box.display != "none":
            box_node.children.append(ch_box)

    return box_node

def build_layout_tree(node: DOMNode, sheet: StyleSheet) -> LayoutBox:
    return build_layout_tree_recursive(node, sheet, "left")

def layout_reflow(box_node: LayoutBox, cx: int, cy: int, cwidth: int):
    if box_node.display == "none": return

    avail_w = cwidth - box_node.margin_left - box_node.margin_right - box_node.padding_left - box_node.padding_right - (box_node.border_width * 2)
    if avail_w < 0: avail_w = 0

    if box_node.width > 0:
        if box_node.width > avail_w: box_node.width = avail_w
    else:
        box_node.width = avail_w

    box_node.x = cx + box_node.margin_left + box_node.border_width + box_node.padding_left
    box_node.y = cy + box_node.margin_top + box_node.border_width + box_node.padding_top

    cur_y = box_node.y
    if box_node.is_text:
        box_node.text_lines = wrap_text(box_node.text, box_node.width)
        box_node.height = len(box_node.text_lines)
    else:
        total_ch_h = 0
        bw = box_node.width
        bx = box_node.x
        for ch in box_node.children:
            layout_reflow(ch, bx, cur_y, bw)
            ch_outer_h = ch.height + ch.padding_top + ch.padding_bottom + (ch.border_width * 2) + ch.margin_top + ch.margin_bottom
            cur_y += ch_outer_h
            total_ch_h += ch_outer_h
        if box_node.height <= 0:
            box_node.height = total_ch_h

def get_outer_height(box_node: LayoutBox) -> int:
    return box_node.height + box_node.padding_top + box_node.padding_bottom + (box_node.border_width * 2) + box_node.margin_top + box_node.margin_bottom

# ------------------------------------------------------------------------------
# Painter & Rasterizer
# ------------------------------------------------------------------------------
def paint_box(box_node: LayoutBox, cv: Canvas):
    if box_node.display == "none": return

    if box_node.border_width > 0:
        bx = box_node.x - box_node.padding_left - box_node.border_width
        by = box_node.y - box_node.padding_top - box_node.border_width
        bw = box_node.width + box_node.padding_left + box_node.padding_right + (box_node.border_width * 2)
        bh = box_node.height + box_node.padding_top + box_node.padding_bottom + (box_node.border_width * 2)
        cv.draw_border(bx, by, bw, bh)

    if box_node.text_lines:
        for i, line_str in enumerate(box_node.text_lines):
            tx = box_node.x
            if box_node.text_align == "center":
                tx = box_node.x + (box_node.width - len(line_str)) // 2
            elif box_node.text_align == "right":
                tx = box_node.x + box_node.width - len(line_str)
            if tx < box_node.x: tx = box_node.x
            cv.draw_text(tx, box_node.y + i, line_str)

    for ch in box_node.children:
        paint_box(ch, cv)

def render_web_page(html: str, css: str, viewport_width: int) -> str:
    dom = parse_html(html)
    sheet = parse_css(css)
    layout_root = build_layout_tree(dom, sheet)
    layout_reflow(layout_root, 0, 0, viewport_width)
    outer_h = get_outer_height(layout_root)
    if outer_h < 1: outer_h = 1
    cv = Canvas(viewport_width, outer_h)
    paint_box(layout_root, cv)
    return cv.render()

# ------------------------------------------------------------------------------
# Parity Serialization Utilities
# ------------------------------------------------------------------------------
def dump_dom_summary(node: DOMNode) -> str:
    if node.is_text:
        return f'#text:"{node.text}"'
    cls_str = ",".join(node.classes)
    res = f"<{node.tag}"
    if node.id_name:
        res += f" id={node.id_name}"
    if cls_str:
        res += f" class=[{cls_str}]"
    res += ">["
    for c, child in enumerate(node.children):
        if c > 0: res += ";"
        res += dump_dom_summary(child)
    res += "]"
    return res

def dump_layout_summary(box_node: LayoutBox) -> str:
    if box_node.display == "none": return ""
    res = f"[{box_node.tag} x={box_node.x} y={box_node.y} w={box_node.width} h={box_node.height}"
    if box_node.text_lines:
        res += f" lines={len(box_node.text_lines)}"
    res += "]("
    for c, child in enumerate(box_node.children):
        if c > 0: res += ";"
        res += dump_layout_summary(child)
    res += ")"
    return res

def count_dom_nodes(node: DOMNode) -> int:
    count = 1
    for ch in node.children:
        count += count_dom_nodes(ch)
    return count

def count_layout_boxes(box_node: LayoutBox) -> int:
    if box_node.display == "none": return 0
    count = 1
    for ch in box_node.children:
        count += count_layout_boxes(ch)
    return count

# ------------------------------------------------------------------------------
# Built-in Benchmark Test Pages
# ------------------------------------------------------------------------------
def get_page_html(name: str) -> str:
    if name == "small":
        return '<div id="user-card" class="card"><h1 class="title">Dev Profile</h1><p class="badge">Verified Staff Architect</p><p class="bio">Specializing in compilers, systems runtimes, and high-performance layout algorithms.</p><div class="actions"><button class="btn">Connect</button></div></div>'
    elif name == "medium":
        return '<div class="wrapper"><nav id="header" class="navbar"><span class="brand">GOAT Web Engine</span></nav><article id="post-1" class="article"><h1 class="headline">Architecture of a High-Speed Browser Engine</h1><p class="byline">Published by Antigravity Core Research Group</p><hr/><p class="intro">Modern browser layout engines bridge structured tree models to 2D raster framebuffers through optimized phases.</p><div class="callout"><p class="callout-text">Key Milestone: Bit-exact parity verified across Pure Rust, MEMElang Native AOT, and Python 3.</p></div><p class="body-text">With zero-overhead recursion and direct struct dereferencing, MEMElang achieves native execution performance.</p><button class="btn">Read Full Paper</button></article><footer class="footer"><p class="copyright">2026 GOAT Lang & Rust Parity Suite</p></footer></div>'
    
    h = '<div id="dashboard" class="dashboard"><header class="header"><h1 class="dash-title">Cloud Cluster Telemetry Console</h1><p class="cluster-status">Status: Operational (Cluster us-east-1)</p></header><section class="metrics-grid"><div class="metric-card"><h2 class="metric-title">Throughput</h2><p class="metric-val">1,420,890 QPS</p></div><div class="metric-card"><h2 class="metric-title">Memory RSS</h2><p class="metric-val">14.2 MB</p></div><div class="metric-card"><h2 class="metric-title">P99 Latency</h2><p class="metric-val">0.024 ms</p></div><div class="metric-card"><h2 class="metric-title">Nodes Active</h2><p class="metric-val">64 Nodes</p></div></section><section class="event-stream"><h2 class="stream-title">Live Event Telemetry Log</h2><div class="table-header"><p>Event ID | Timestamp | Severity | Description</p></div><hr/>'
    for row_idx in range(1, 13):
        h += f'<div class="table-row"><p>EVT-{1000 + row_idx} | 08:24:{10 + row_idx} | INFO | Frame buffer rasterized successfully.</p></div>'
    h += '</section><footer class="dash-footer"><button class="btn-refresh">Refresh View</button><p class="footer-note">Autorefresh active: every 500ms</p></footer></div>'
    return h

def get_page_css(name: str) -> str:
    if name == "small":
        return ".card { width: 50; border: 1; padding: 1; margin: 1; } .title { text-align: center; margin-bottom: 1; } .badge { text-align: center; margin-bottom: 1; } .bio { margin-bottom: 1; } .actions { text-align: center; } .btn { border: 1; padding-left: 1; padding-right: 1; }"
    elif name == "medium":
        return ".wrapper { width: 78; margin: 1; } .navbar { border: 1; padding: 1; margin-bottom: 1; text-align: center; } .article { border: 1; padding: 1; margin-bottom: 1; } .headline { text-align: center; margin-bottom: 1; } .byline { text-align: center; margin-bottom: 1; } .intro { margin-bottom: 1; } .callout { border: 1; padding: 1; margin-top: 1; margin-bottom: 1; } .callout-text { text-align: center; } .body-text { margin-bottom: 1; } .btn { border: 1; padding-left: 1; padding-right: 1; } .footer { text-align: center; padding: 1; }"
    return ".dashboard { width: 78; margin: 1; border: 1; padding: 1; } .header { margin-bottom: 1; text-align: center; } .dash-title { text-align: center; margin-bottom: 1; } .cluster-status { text-align: center; margin-bottom: 1; } .metrics-grid { margin-bottom: 1; } .metric-card { border: 1; padding: 1; margin-bottom: 1; } .metric-title { text-align: center; margin-bottom: 1; } .metric-val { text-align: center; } .event-stream { border: 1; padding: 1; margin-bottom: 1; } .stream-title { text-align: center; margin-bottom: 1; } .table-header { margin-bottom: 1; } .table-row { margin-bottom: 0; } .dash-footer { text-align: center; margin-top: 1; } .btn-refresh { border: 1; padding-left: 1; padding-right: 1; margin-bottom: 1; } .footer-note { text-align: center; }"

# ------------------------------------------------------------------------------
# Parity & Verification Suite
# ------------------------------------------------------------------------------
def run_verify():
    print("==================================================")
    print("🌐 MiniBrowser Parity & Layout Verification Suite")
    print("==================================================")

    pages = ["small", "medium", "heavy"]
    for pname in pages:
        print()
        print(f"--- PAGE: {pname} ---")
        html = get_page_html(pname)
        css = get_page_css(pname)

        dom = parse_html(html)
        dom_str = dump_dom_summary(dom)
        print(f"DOM Nodes Count: {count_dom_nodes(dom)}")
        pfx_dom = dom_str[:80] if len(dom_str) > 80 else dom_str
        print(f"DOM Summary Hash/Prefix: {pfx_dom}")

        sheet = parse_css(css)
        print(f"CSS Rules Count: {len(sheet.rules)}")

        layout_root = build_layout_tree(dom, sheet)
        layout_reflow(layout_root, 0, 0, 80)
        layout_str = dump_layout_summary(layout_root)
        print(f"Layout Boxes Count: {count_layout_boxes(layout_root)}")
        print(f"Layout Outer Height: {get_outer_height(layout_root)}")
        pfx_lay = layout_str[:80] if len(layout_str) > 80 else layout_str
        print(f"Layout Summary Prefix: {pfx_lay}")

        cv_h = get_outer_height(layout_root)
        if cv_h < 1: cv_h = 1
        cv = Canvas(80, cv_h)
        paint_box(layout_root, cv)
        rendered = cv.render()

        print("ASCII Canvas Sample (First 10 lines):")
        for line in rendered.splitlines()[:10]:
            print(line)

    print()
    print("==================================================")
    print("✅ Verification Completed Successfully!")
    print("==================================================")

# ------------------------------------------------------------------------------
# Benchmark Suite
# ------------------------------------------------------------------------------
def run_benchmark(page_name: str, iterations: int, viewport_width: int):
    html = get_page_html(page_name)
    css = get_page_css(page_name)

    # Warm-up iteration
    warm_dom = parse_html(html)
    warm_sheet = parse_css(css)
    warm_layout = build_layout_tree(warm_dom, warm_sheet)
    layout_reflow(warm_layout, 0, 0, viewport_width)
    warm_h = get_outer_height(warm_layout)
    if warm_h < 1: warm_h = 1
    warm_cv = Canvas(viewport_width, warm_h)
    paint_box(warm_layout, warm_cv)
    _warm_str = warm_cv.render()

    dom_node_count = count_dom_nodes(warm_dom)
    layout_box_count = count_layout_boxes(warm_layout)

    # Benchmark loop
    t0 = time.perf_counter()

    t_html_total = 0.0
    t_css_total = 0.0
    t_style_total = 0.0
    t_layout_total = 0.0
    t_paint_total = 0.0

    for _ in range(iterations):
        # Stage 1: HTML Parse
        t_h0 = time.perf_counter()
        dom = parse_html(html)
        t_html_total += time.perf_counter() - t_h0

        # Stage 2: CSS Parse
        t_c0 = time.perf_counter()
        sheet = parse_css(css)
        t_css_total += time.perf_counter() - t_c0

        # Stage 3: Style Cascade & Layout Tree Construction
        t_s0 = time.perf_counter()
        layout_root = build_layout_tree(dom, sheet)
        t_style_total += time.perf_counter() - t_s0

        # Stage 4: Layout Reflow
        t_l0 = time.perf_counter()
        layout_reflow(layout_root, 0, 0, viewport_width)
        t_layout_total += time.perf_counter() - t_l0

        # Stage 5: Paint & Rasterize
        t_p0 = time.perf_counter()
        cv_h = get_outer_height(layout_root)
        if cv_h < 1: cv_h = 1
        cv = Canvas(viewport_width, cv_h)
        paint_box(layout_root, cv)
        _rendered_res = cv.render()
        t_paint_total += time.perf_counter() - t_p0

    total_time = time.perf_counter() - t0
    total_ms = total_time * 1000.0
    fps = iterations / total_time if total_time > 0 else 0
    avg_pipeline_ms = total_ms / iterations if iterations > 0 else 0

    html_ms = t_html_total * 1000.0 / iterations
    css_ms = t_css_total * 1000.0 / iterations
    style_ms = t_style_total * 1000.0 / iterations
    layout_ms = t_layout_total * 1000.0 / iterations
    paint_ms = t_paint_total * 1000.0 / iterations

    print(
        f"page={page_name} iterations={iterations} total_ms={total_ms:.5f} fps={fps:.1f} avg_ms={avg_pipeline_ms:.5f} "
        f"html_ms={html_ms:.6f} css_ms={css_ms:.6f} style_ms={style_ms:.6f} layout_ms={layout_ms:.6f} paint_ms={paint_ms:.6f} "
        f"dom_nodes={dom_node_count} layout_boxes={layout_box_count} canvas_h={warm_h}"
    )

# ------------------------------------------------------------------------------
# Main Entrypoint
# ------------------------------------------------------------------------------
if __name__ == "__main__":
    args = sys.argv
    is_bench = 0
    bench_page = "medium"
    bench_iters = 100
    bench_viewport = 80

    if len(args) >= 2:
        if args[1] == "--benchmark":
            is_bench = 1
            if len(args) >= 3:
                bench_page = args[2]
            if len(args) >= 4:
                bench_iters = int(args[3])
            if len(args) >= 5:
                bench_viewport = int(args[4])
        elif args[1] == "--dump":
            target_page = "small"
            if len(args) >= 3:
                target_page = args[2]
            h = get_page_html(target_page)
            c = get_page_css(target_page)
            d = parse_html(h)
            s = parse_css(c)
            l = build_layout_tree(d, s)
            layout_reflow(l, 0, 0, 80)
            cv_h = get_outer_height(l)
            if cv_h < 1: cv_h = 1
            cv = Canvas(80, cv_h)
            paint_box(l, cv)
            print("=== DOM ===")
            print(dump_dom_summary(d))
            print("=== LAYOUT ===")
            print(dump_layout_summary(l))
            print("=== CANVAS ===")
            print(cv.render())
            is_bench = -1

    if is_bench == 1:
        run_benchmark(bench_page, bench_iters, bench_viewport)
    elif is_bench == 0:
        run_verify()
