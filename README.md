<div align="center">
  <img src="content/UphonicLarge.svg" alt="Uphonic" width="85%">

  <p><b>A digital audio workstation written entirely in C.</b><br>Built to be simple, fast, and customizable.</p>

  <a href="https://github.com/DevDynastyStudios/Uphonic"><img src="https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white" alt="C"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-9D4EDD?style=for-the-badge" alt="License: GPL-3.0"></a>
  <a href="LICENSE-CC-BY"><img src="https://img.shields.io/badge/Assets-CC_BY_4.0-9D4EDD?style=for-the-badge" alt="Assets: CC BY 4.0"></a>
  <img src="https://img.shields.io/badge/Platform-Windows_|_Linux-555?style=for-the-badge" alt="Platform: Windows | Linux">
</div>

<br>

> [!WARNING]
> Uphonic is in an early development stage. Many features are incomplete, experimental, or subject to change.

## Table of Contents

- [Dependencies](#dependencies)
  - [Windows](#windows)
  - [Linux](#linux)
- [Building](#building)
- [License](#license)

---

## Dependencies

### Windows

Install [Visual Studio](https://visualstudio.microsoft.com/) with the following components:
 - MSVC Build Tools for x64/x86
 - Windows 11 SDK
 - MSVC v\<MSVC-Version> - VS \<Visual Studio Version> C++ x64/x86 build tools
 - C++ Clang tools for Windows

> [!IMPORTANT]  
> Visual Studio does not add these compilers to the system PATH; you will have to do that yourself. By default, they are only accessible through Developer PowerShell for Visual Studio.

You will also need to install [lua 5.5](https://sourceforge.net/projects/luabinaries/files/5.5.0/)  or newer and add it to PATH.

### Linux

Install the required packages for your distribution.

#### Ubuntu / Debian

```bash
sudo apt update
sudo apt install clang lua5.4 libx11-dev libxcursor-dev libegl-dev xwayland
```

#### Fedora

```bash
sudo dnf install clang lua libX11-devel libXcursor-devel mesa-libEGL-devel xorg-x11-server-Xwayland
```

#### Arch Linux

```bash
sudo pacman -S clang lua libx11 libxcursor libegl xorg-xwayland
```

#### openSUSE

```bash
sudo zypper install clang lua54 libX11-devel libXcursor-devel Mesa-libEGL-devel xwayland
```
> [!IMPORTANT]
>`xwayland` is only required if you're running a Wayland session.

---

## Building

```bash
lua build.lua <mode> <configuration>
```

| Mode | Description |
| --- | --- |
| `run` | Runs an already built binary. |
| `build` | Builds the project. |
| `buildrun` | Builds and runs the project. |

| Configuration | Description |
| --- | --- |
| `debug` | Debug build with symbols. |
| `release` | Optimized build. |

Example:

```bash
lua build.lua buildrun release
```

---

## License

Uphonic's source code is licensed under the [GNU General Public License v3.0](LICENSE).\
Assets are licensed under [Creative Commons Attribution 4.0](LICENSE-CC-BY).
