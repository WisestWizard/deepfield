#!/bin/sh
# base/tools/publish_site.sh for deepfield.fog.baby: GitHub Pages drops a custom domain whenever a push replaces the
# gh-pages branch without a CNAME file, and the base script replaces the branch on every publish.
#   deepfield/tools/publish_site.sh VERSION
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="$ROOT/deepfield"
V="${1:?usage: deepfield/tools/publish_site.sh VERSION}"
PY="${PYTHON:-/opt/homebrew/bin/python3.14}"
DOMAIN=deepfield.fog.baby
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}" AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
"$PY" "$ROOT/base/tools/build.py" "$APP" --release "$V"
"$PY" "$ROOT/base/tools/build.py" "$APP" --web --release "$V"
"$PY" "$ROOT/base/tools/make_site.py" "$APP" "$APP/build/deepfield-$V.fwsc" "$V" "$APP/build/site"
mkdir -p "$APP/build/site/cheatsheet"
sed "s/{{VERSION}}/$V/g" "$APP/site/cheatsheet/index.html" > "$APP/build/site/cheatsheet/index.html"
printf '%s\n' "$DOMAIN" > "$APP/build/site/CNAME"
REMOTE="$(git -C "$APP" remote get-url origin)"
SHA="$(git -C "$APP" rev-parse --short HEAD)"
cd "$APP/build/site"
rm -rf .git
git init -q -b gh-pages
git add -A
git commit -q -m "Site for deepfield $V (from $SHA)"
git push -q -f "$REMOTE" gh-pages
rm -rf .git
REPO="$(printf %s "$REMOTE" | sed -E 's#(git@github.com:|https://github.com/)##; s#\.git$##')"
gh api -X POST "repos/$REPO/pages/builds" >/dev/null 2>&1 || echo "(could not ask GitHub Pages to build: check the repository's Pages settings)"
echo "pushed the site for deepfield $V to gh-pages: https://$DOMAIN"
