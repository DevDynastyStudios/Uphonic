<p align="center">
  <img src="https://raw.githubusercontent.com/DevDynastyStudios/Uphonic/refs/heads/main/content/logo-large.png" alt="Naui" width="50%">
</p>

**Uphonic** is a digital audio workstation written entirely within C.\
Built to be simple, fast, and customizable.

> **Note:** Uphonic is in an early development stage.
> Many features are incomplete, experimental, or subject to change.

---

## Windows

Install the following:

* [clang](https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.8/clang+llvm-22.1.8-x86_64-pc-windows-msvc.tar.xz)
* [Lua](https://sourceforge.net/projects/luabinaries/files/5.5.0/)
* [Visual Studio](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Stable&version=VS18&source=VSLandingPage&cid=2500&passive=false)

![vsstudio](https://raw.githubusercontent.com/DevDynastyStudios/Naui/refs/heads/main/content/screenshots/vs.png)

---

## Linux

Install the required packages for your distribution.

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install clang lua5.4 libx11-dev libegl-dev xwayland
```

### Fedora

```bash
sudo dnf install clang lua libX11-devel mesa-libEGL-devel xorg-x11-server-Xwayland
```

### Arch Linux

```bash
sudo pacman -S clang lua libx11 libegl xorg-xwayland
```

### openSUSE

```bash
sudo zypper install clang lua54 libX11-devel Mesa-libEGL-devel xwayland
```

> **Note:** `xwayland` is only required if you're running a Wayland session.

---

## macOS

Support coming soon.

---

## Building

Uphonic uses a simple Lua build script:

```bash
lua build.lua release
```
