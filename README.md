<div align="center">
  <img src="content/Uphonic.svg" alt="Logo", width="50%">
</div>

**Uphonic** is a digital audio workstation written entirely within C.\
Built to be simple, fast, and customizable.

> [!WARNING]  
> Uphonic is in an early development stage. Many features are incomplete, experimental, or subject to change.

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

You will also need to install [lua 5.5](https://sourceforge.net/projects/luabinaries/files/5.5.0/)  or newer and add it to path.

### Linux

Install the required packages for your distribution.

#### Ubuntu / Debian

```bash
sudo apt update
sudo apt install clang lua5.4 libx11-dev libegl-dev xwayland
```

#### Fedora

```bash
sudo dnf install clang lua libX11-devel mesa-libEGL-devel xorg-x11-server-Xwayland
```

#### Arch Linux

```bash
sudo pacman -S clang lua libx11 libegl xorg-xwayland
```

#### openSUSE

```bash
sudo zypper install clang lua54 libX11-devel Mesa-libEGL-devel xwayland
```
> [!IMPORTANT]
>`xwayland` is only required if you're running a Wayland session.

---

## Building

```bash
lua build.lua <mode> <configuration>
```

Modes:
 - run = Only runs an allready built binary.
 - build = Builds the project.
 - build_run = Builds and runs the project.

Configurations:
 - debug
 - release
