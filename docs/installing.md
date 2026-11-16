# Installing Glimmer

Prebuilt downloads are published on the
[Releases page](https://github.com/sSseeth/Glimmer/releases) for:

| Platform | Download | Requirements |
|---|---|---|
| Windows | `glimmer-vX.Y.Z-windows-x64.zip` | Windows 10 or 11, 64-bit |
| macOS | `glimmer-vX.Y.Z-macos-arm64.dmg` | macOS 13.3+ on Apple Silicon (M1 or later) |
| Linux | `glimmer-vX.Y.Z-linux-x86_64.AppImage` | x86-64 with glibc 2.35+ (Ubuntu 22.04, Debian 12, Fedora 36 or newer) |

Voice transcription is built into every download. It needs a 64-bit CPU
with AVX2 (Intel and AMD chips from about 2013 onwards) or Apple Silicon.

The installers below download the latest release, **check it against the
release's `SHA256SUMS`**, and install it for your user only, with no admin
rights needed.

---

## Windows

Open **PowerShell** and run:

```powershell
irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1 | iex
```

This installs to `%LOCALAPPDATA%\Programs\Glimmer`, adds **Glimmer** to the
Start menu, and registers it under **Settings → Apps** so you can uninstall
it like any other app.

Options (install a specific version, add to PATH, or uninstall):

```powershell
$installer = [scriptblock]::Create((irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1))
& $installer -Version v0.2.0     # a specific release
& $installer -AddToPath          # also put glimmer.exe on your PATH
& $installer -Uninstall          # remove it (your diary is kept)
```

**Manual install:** download the `.zip` from Releases, extract it anywhere,
and run `glimmer.exe`. Windows SmartScreen may say "Windows protected your
PC" because the build isn't code-signed. Click **More info → Run anyway**.

## macOS

Open **Terminal** and run:

```sh
curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash
```

This copies **Glimmer.app** into `/Applications` (or `~/Applications` if you
can't write to `/Applications`) and adds a `glimmer` command in
`~/.local/bin`.

**Manual install:** open the `.dmg` and drag **Glimmer** onto
**Applications**. The app isn't notarized by Apple yet, so the first time,
**right-click Glimmer → Open → Open**. Or go to **System Settings → Privacy &
Security** and click **Open Anyway**. If macOS says the app "is damaged", run:

```sh
xattr -dr com.apple.quarantine /Applications/Glimmer.app
```

The first time you record a voice entry, macOS asks for microphone access.
Allow it, or enable it later under **System Settings → Privacy & Security →
Microphone**.

Intel Macs: there's no prebuilt download yet. Follow
[Build from source](../README.md#build-from-source).

## Linux

```sh
curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash
```

This unpacks the AppImage into `~/.local/opt/glimmer`, adds a `glimmer`
command in `~/.local/bin`, and adds **Glimmer** to your application menu.
It doesn't need FUSE or root.

**Manual install:** download the `.AppImage`, then:

```sh
chmod +x glimmer-*-linux-x86_64.AppImage
./glimmer-*-linux-x86_64.AppImage
```

Running an AppImage directly needs FUSE 2. On Ubuntu 22.04 and later that's
`sudo apt install libfuse2t64` (or `libfuse2` on 22.04). Without it, run it
with `--appimage-extract-and-run`.

---

## Installer options (macOS and Linux)

Pass options after `bash -s --`:

```sh
curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash -s -- --version v0.2.0
curl -fsSL https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh | bash -s -- --uninstall
```

## Prefer to read the script first?

Piping a script from the internet into a shell runs it straight away. To
check it first:

```sh
curl -fsSLO https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.sh
less install.sh
bash install.sh
```

```powershell
irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1 -OutFile install.ps1
notepad install.ps1
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

## Verifying a manual download

Every release includes `SHA256SUMS`. Compare the hash of your download with
the matching line:

```sh
sha256sum glimmer-*-linux-x86_64.AppImage   # Linux
shasum -a 256 glimmer-*-macos-arm64.dmg     # macOS
```

```powershell
Get-FileHash .\glimmer-*-windows-x64.zip -Algorithm SHA256   # Windows
```

## Updating

Run the installer again. It replaces the app and leaves your diary alone.

## Uninstalling

| Platform | How |
|---|---|
| Windows | Settings → Apps → Glimmer → Uninstall, or `& $installer -Uninstall` |
| macOS / Linux | `curl -fsSL …/install.sh \| bash -s -- --uninstall` |

Uninstalling **never deletes your diary**.

## Where your diary is stored

| Platform | Folder |
|---|---|
| Windows | `%APPDATA%\glimmer\glimmer\` |
| macOS | `~/Library/Application Support/glimmer/glimmer/` |
| Linux | `~/.local/share/glimmer/glimmer/` |

It holds `glimmer.sqlite` (your entries) and `media/` (images and
recordings). To back up your diary, copy this folder while Glimmer is
closed.

## After installing

- **Voice transcription:** download a speech model once, e.g.
  [ggml-base.en.bin](https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin)
  (about 150 MB, English) or `ggml-base.bin` (multilingual). Then choose it
  in **File → Settings → Whisper model**.
- **AI companion (optional):** see [AI companion](../README.md#ai-companion-omniroute).

## Troubleshooting

| Problem | Fix |
|---|---|
| Linux: `glimmer: command not found` | Add `~/.local/bin` to your PATH, e.g. `echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc` |
| Linux: blank window or "could not load the Qt platform plugin xcb" | `sudo apt install libxcb-cursor0` (Debian/Ubuntu) |
| macOS: "Glimmer is damaged and can't be opened" | `xattr -dr com.apple.quarantine /Applications/Glimmer.app` |
| Windows: installer says Glimmer is running | Close Glimmer (save your entry first) and run it again |
| Any: "couldn't find release 'latest'" | No release has been published yet. Build from source in the meantime. |

Still stuck? [Open an issue](https://github.com/sSseeth/Glimmer/issues/new/choose).
