#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 Kyle McCann
"""DEEP FIELD's look (PHOSPHOR) on a built site: deepfield/site/theme.css, with deepfield/site/beams.svg inlined, as
the last style of every page's head, so it wins over the page's own (fm1-base makes the front page, the manual, the
installer and the emulator page; base/ stays as it is).
  python3 deepfield/tools/site_theme.py deepfield/build/site       (publish_site.sh runs it)"""
import pathlib, sys, urllib.parse

APP = pathlib.Path(__file__).resolve().parents[1]
site = pathlib.Path(sys.argv[1])
beams = "data:image/svg+xml," + urllib.parse.quote((APP / "site" / "beams.svg").read_text(encoding="utf-8"), safe=" =:/,.-")
css = (APP / "site" / "theme.css").read_text(encoding="utf-8").replace("{{BEAMS}}", beams)
tag = '<style id="phosphor">\n' + css + "</style>\n"
pages = sorted(site.rglob("*.html"))
for page in pages:
    html = page.read_text(encoding="utf-8")
    if 'id="phosphor"' in html:
        continue
    if "</head>" not in html:
        sys.exit(f"{page}: no </head>")
    page.write_text(html.replace("</head>", tag + "</head>", 1), encoding="utf-8")
print(f"theme: PHOSPHOR on {len(pages)} pages: " + ", ".join(str(p.relative_to(site)) for p in pages))
