#!/usr/bin/env bash
# Publish the staged runtime to a fast-forward-only, build-only branch.
set -euo pipefail
build=$(cd "${1:?Pass the staged web directory}" && pwd)
for file in index.html mountain.js mountain.wasm assets.json assets/scripts/world.lua; do
  test -f "$build/$file" || { echo "Missing $file in web distribution" >&2; exit 1; }
done
repo=$(git rev-parse --show-toplevel)
version=$(git -C "$repo" rev-parse --short=12 HEAD)
remote=$(git -C "$repo" remote get-url origin)
temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT
git -C "$temp" init -q -b gh-page
git -C "$temp" remote add origin "$remote"
# Persist the checkout's authentication header for this process only.
if auth=$(git -C "$repo" config --get http.https://github.com/.extraheader); then
  export GIT_CONFIG_COUNT=1 GIT_CONFIG_KEY_0=http.https://github.com/.extraheader GIT_CONFIG_VALUE_0="$auth"
fi
head=$(git -C "$temp" ls-remote --heads origin refs/heads/gh-page)
if [[ -n "$head" ]]; then
  git -C "$temp" fetch -q --depth=1 origin refs/heads/gh-page
  git -C "$temp" reset -q --mixed FETCH_HEAD
fi
cp -a "$build/." "$temp/"
git -C "$temp" add -A
if git -C "$temp" diff --cached --quiet; then exit 0; fi
git -C "$temp" -c user.name="${GIT_AUTHOR_NAME:-Mountain builds}" -c user.email="${GIT_AUTHOR_EMAIL:-41898282+github-actions[bot]@users.noreply.github.com}" commit -qm "Publish Mountain web playtest from $version"
git -C "$temp" push origin HEAD:refs/heads/gh-page
