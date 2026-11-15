#!/usr/bin/env bash
# Installs (or uninstalls) Glimmer on Linux and macOS from GitHub Releases.
#
#   curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash
#
# Options (pass after `bash -s --` when piping):
#   --version vX.Y.Z   install a specific release instead of the latest
#   --from DIR         install from already-downloaded release files in DIR
#   --uninstall        remove Glimmer (your diary is kept)
#   --help             show this help
#
# What it does
#   Linux (x86_64): unpacks the AppImage into ~/.local/opt/glimmer, links
#                   ~/.local/bin/glimmer, adds a menu entry and icon. No root,
#                   no FUSE needed.
#   macOS (Apple Silicon): copies Glimmer.app into /Applications (or
#                   ~/Applications if that isn't writable) and links
#                   ~/.local/bin/glimmer.
# Every download is checked against the release's SHA256SUMS before use.
set -euo pipefail

REPO="sSseeth/Glimmer"
API="https://api.github.com/repos/${REPO}/releases"

version="latest"
from_dir=""
action="install"

say() { printf '\033[1m==>\033[0m %s\n' "$*"; }
die() { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "'$1' is required but not installed"; }

usage() {
  cat <<'USAGE'
Install Glimmer from GitHub Releases (Linux x86_64, macOS Apple Silicon).

  install.sh [--version vX.Y.Z] [--from DIR] [--uninstall]

  --version vX.Y.Z   install a specific release instead of the latest
  --from DIR         install from already-downloaded release files in DIR
  --uninstall        remove Glimmer (your diary is kept)
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --version) version="${2:?--version needs a tag, e.g. v0.2.0}"; shift 2 ;;
    --from) from_dir="${2:?--from needs a directory}"; shift 2 ;;
    --uninstall) action="uninstall"; shift ;;
    -h | --help) usage; exit 0 ;;
    *) die "unknown option: $1 (try --help)" ;;
  esac
done

os="$(uname -s)"
arch="$(uname -m)"
bin_dir="${HOME}/.local/bin"

case "$os" in
  Linux)
    [ "$arch" = "x86_64" ] || die "prebuilt Linux builds are x86_64 only (you have $arch); see README to build from source"
    asset_suffix="-linux-x86_64.AppImage"
    app_dir="${HOME}/.local/opt/glimmer"
    desktop_file="${XDG_DATA_HOME:-$HOME/.local/share}/applications/glimmer.desktop"
    icon_file="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/512x512/apps/glimmer.png"
    data_dir="${XDG_DATA_HOME:-$HOME/.local/share}/glimmer"
    ;;
  Darwin)
    [ "$arch" = "arm64" ] || die "prebuilt macOS builds are Apple Silicon only (you have $arch); see README to build from source"
    asset_suffix="-macos-arm64.dmg"
    if [ -w /Applications ]; then app_dir="/Applications/Glimmer.app"; else app_dir="${HOME}/Applications/Glimmer.app"; fi
    data_dir="${HOME}/Library/Application Support/glimmer"
    ;;
  *) die "unsupported OS: $os (on Windows use install.ps1)" ;;
esac

# --- uninstall ----------------------------------------------------------------

if [ "$action" = "uninstall" ]; then
  say "Removing Glimmer"
  if [ "$os" = "Linux" ]; then
    rm -rf "$app_dir"
    rm -f "$desktop_file" "$icon_file"
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$(dirname "$desktop_file")" || true
  else
    rm -rf "/Applications/Glimmer.app" "${HOME}/Applications/Glimmer.app"
  fi
  if [ -L "${bin_dir}/glimmer" ] || grep -qs 'glimmer-launcher' "${bin_dir}/glimmer"; then
    rm -f "${bin_dir}/glimmer"
  fi
  say "Done. Your diary was NOT deleted; it's in: ${data_dir}"
  exit 0
fi

# --- download and verify --------------------------------------------------------

need uname
if command -v sha256sum >/dev/null 2>&1; then
  sha256() { sha256sum "$1" | cut -d' ' -f1; }
elif command -v shasum >/dev/null 2>&1; then
  sha256() { shasum -a 256 "$1" | cut -d' ' -f1; }
else
  die "need sha256sum or shasum to verify downloads"
fi

work="$(mktemp -d)"
cleanup() {
  if [ -n "${mounted:-}" ]; then hdiutil detach -quiet "$mounted" || true; fi
  rm -rf "$work"
}
trap cleanup EXIT

if [ -n "$from_dir" ]; then
  [ -d "$from_dir" ] || die "--from: $from_dir is not a directory"
  package="$(find "$from_dir" -maxdepth 1 -name "glimmer-*${asset_suffix}" | sort | tail -n1)"
  [ -n "$package" ] || die "no glimmer-*${asset_suffix} in $from_dir"
  sums="${from_dir}/SHA256SUMS"
  tag="$(basename "$package" | sed "s/^glimmer-\(.*\)${asset_suffix}$/\1/")"
else
  need curl
  if [ "$version" = "latest" ]; then url="${API}/latest"; else url="${API}/tags/${version}"; fi
  say "Looking up ${version} release of ${REPO}"
  release="$(curl -fsSL -H 'Accept: application/vnd.github+json' "$url")" ||
    die "couldn't find release '${version}' (has one been published yet?)"
  tag="$(printf '%s' "$release" | grep -o '"tag_name": *"[^"]*"' | head -n1 | sed 's/.*"\([^"]*\)"$/\1/')"
  asset_url="$(printf '%s' "$release" | grep -o '"browser_download_url": *"[^"]*"' |
    sed 's/.*"\([^"]*\)"$/\1/' | grep -- "${asset_suffix}\$" | head -n1)" ||
    die "release ${tag} has no ${asset_suffix} download"
  sums_url="$(printf '%s' "$release" | grep -o '"browser_download_url": *"[^"]*"' |
    sed 's/.*"\([^"]*\)"$/\1/' | grep '/SHA256SUMS$' | head -n1)" ||
    die "release ${tag} has no SHA256SUMS file"
  package="${work}/$(basename "$asset_url")"
  sums="${work}/SHA256SUMS"
  say "Downloading Glimmer ${tag}"
  curl -fL --progress-bar -o "$package" "$asset_url"
  curl -fsSL -o "$sums" "$sums_url"
fi

[ -f "$sums" ] || die "SHA256SUMS not found; refusing to install an unverified download"
expected="$(grep " \*\{0,1\}$(basename "$package")\$" "$sums" | cut -d' ' -f1 || true)"
[ -n "$expected" ] || die "$(basename "$package") is not listed in SHA256SUMS"
actual="$(sha256 "$package")"
[ "$expected" = "$actual" ] || die "checksum mismatch for $(basename "$package") (expected $expected, got $actual)"
say "Checksum OK"

mkdir -p "$bin_dir"

# --- install: Linux -------------------------------------------------------------

if [ "$os" = "Linux" ]; then
  say "Installing to ${app_dir}"
  cp "$package" "${work}/glimmer.AppImage"
  chmod +x "${work}/glimmer.AppImage"
  # Extracting avoids needing FUSE (missing by default on Ubuntu 22.04+).
  (cd "$work" && ./glimmer.AppImage --appimage-extract >/dev/null)
  rm -rf "$app_dir"
  mkdir -p "$(dirname "$app_dir")"
  mv "${work}/squashfs-root" "$app_dir"
  ln -sf "${app_dir}/AppRun" "${bin_dir}/glimmer"

  mkdir -p "$(dirname "$desktop_file")" "$(dirname "$icon_file")"
  cp "${app_dir}/glimmer.png" "$icon_file" 2>/dev/null || true
  sed -e "s|^Exec=.*|Exec=${app_dir}/AppRun %F|" -e "s|^Icon=.*|Icon=glimmer|" \
    "${app_dir}/glimmer.desktop" >"$desktop_file"
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$(dirname "$desktop_file")" || true
fi

# --- install: macOS -------------------------------------------------------------

if [ "$os" = "Darwin" ]; then
  say "Installing to ${app_dir}"
  mounted="${work}/mnt"
  mkdir -p "$mounted"
  hdiutil attach -quiet -nobrowse -readonly -mountpoint "$mounted" "$package"
  [ -d "${mounted}/Glimmer.app" ] || die "Glimmer.app not found inside $(basename "$package")"
  mkdir -p "$(dirname "$app_dir")"
  rm -rf "$app_dir"
  ditto "${mounted}/Glimmer.app" "$app_dir"
  hdiutil detach -quiet "$mounted"
  mounted=""
  # The build isn't notarized by Apple; clear the quarantine flag so macOS
  # doesn't refuse to open it. (curl downloads usually don't set it anyway.)
  xattr -dr com.apple.quarantine "$app_dir" 2>/dev/null || true
  # A wrapper rather than a symlink: Qt finds its bundled plugins relative
  # to the real executable inside Glimmer.app.
  rm -f "${bin_dir}/glimmer"
  printf '#!/bin/sh\n# glimmer-launcher (created by install.sh)\nexec "%s/Contents/MacOS/Glimmer" "$@"\n' \
    "$app_dir" >"${bin_dir}/glimmer"
  chmod +x "${bin_dir}/glimmer"
fi

say "Installed Glimmer ${tag}"
case ":${PATH}:" in
  *":${bin_dir}:"*) ;;
  *) echo "    Note: ${bin_dir} isn't on your PATH. Add it to run 'glimmer' from a terminal." ;;
esac
if [ "$os" = "Linux" ]; then
  echo "    Launch it from your app menu, or run: glimmer"
else
  echo "    Launch it from Launchpad / Applications, or run: open -a Glimmer"
fi
echo "    Your diary will be stored in: ${data_dir}"
