#!/bin/bash

# widget-core-linux: Universal multi-distribution dependency installer
# Supports: apt, dnf, yum, pacman, zypper, apk, xbps-install, eopkg, swupd, emerge, nix

echo "🔍 Detecting Linux distribution..."

if [ ! -f /etc/os-release ]; then
    echo "❌ Error: /etc/os-release not found. Cannot determine distribution."
    exit 1
fi

. /etc/os-release

# ─── Track installation status ───────────────────────────────────────────────
MISSING=0
LAYER_SHELL_OK=0
WEBKIT_VER=""

# ─── Check if already satisfied ──────────────────────────────────────────────
check_installed() {
    # Check webkit2gtk version
    if pkg-config --exists webkit2gtk-4.1 2>/dev/null; then
        WEBKIT_VER="4.1"
    elif pkg-config --exists webkit2gtk-4.0 2>/dev/null; then
        WEBKIT_VER="4.0"
        echo "ℹ️  webkit2gtk-4.0 detected (4.1 preferred; some features may be limited)"
    else
        echo "⚠️  Missing: webkit2gtk"
        MISSING=1
    fi

    for pkg in gtk+-3.0 x11; do
        if ! pkg-config --exists "$pkg" 2>/dev/null; then
            echo "⚠️  Missing: $pkg"
            MISSING=1
        fi
    done

    if pkg-config --exists gtk-layer-shell-0 2>/dev/null; then
        LAYER_SHELL_OK=1
    else
        echo "ℹ️  Optional: gtk-layer-shell not found (Wayland layer support disabled)"
    fi
}

check_installed

if [ $MISSING -eq 0 ]; then
    echo "✅ All required dependencies are already installed."
    [ $LAYER_SHELL_OK -eq 1 ] && echo "✅ gtk-layer-shell (Wayland layer support) available."
    exit 0
fi

# ─── Detect package manager ───────────────────────────────────────────────────
detect_pkg_manager() {
    command -v apt-get      &>/dev/null && echo "apt"     && return
    command -v dnf          &>/dev/null && echo "dnf"     && return
    command -v pacman       &>/dev/null && echo "pacman"  && return
    command -v zypper       &>/dev/null && echo "zypper"  && return
    command -v apk          &>/dev/null && echo "apk"     && return
    command -v xbps-install &>/dev/null && echo "xbps"    && return
    command -v eopkg        &>/dev/null && echo "eopkg"   && return
    command -v swupd        &>/dev/null && echo "swupd"   && return
    command -v emerge       &>/dev/null && echo "emerge"  && return
    command -v nix-env      &>/dev/null && echo "nix"     && return
    command -v yum          &>/dev/null && echo "yum"     && return
    echo "unknown"
}

PKG_MANAGER=$(detect_pkg_manager)
echo "📦 Package manager: $PKG_MANAGER"
echo "📦 Installing dependencies for $NAME..."

# ─── Helper: try installing a package, return 0 on success ───────────────────
try_pkg() { "$@" 2>/dev/null; return $?; }

# ─── Installation ─────────────────────────────────────────────────────────────
case "$PKG_MANAGER" in

    # ── Debian / Ubuntu / Mint / Pop!_OS / Kali / Elementary / Zorin ─────────
    apt)
        sudo apt-get update -qq
        # Required base
        sudo apt-get install -y build-essential pkg-config libgtk-3-dev libx11-dev

        # webkit2gtk: 4.1 preferred, 4.0 fallback
        if apt-cache show libwebkit2gtk-4.1-dev &>/dev/null 2>&1; then
            sudo apt-get install -y libwebkit2gtk-4.1-dev
        elif apt-cache show libwebkit2gtk-4.0-dev &>/dev/null 2>&1; then
            echo "ℹ️  Using webkit2gtk-4.0 (4.1 not in repos)"
            sudo apt-get install -y libwebkit2gtk-4.0-dev
        else
            echo "❌ webkit2gtk dev package not found in apt repos"
            exit 1
        fi

        # Optional: gtk-layer-shell (Ubuntu 22.04+ / Debian 12+)
        if apt-cache show libgtk-layer-shell-dev &>/dev/null 2>&1; then
            sudo apt-get install -y libgtk-layer-shell-dev && LAYER_SHELL_OK=1
        else
            echo "ℹ️  libgtk-layer-shell-dev not available — Wayland layer support disabled"
        fi
        ;;

    # ── Fedora / RHEL 8+ / Rocky / AlmaLinux / CentOS Stream ─────────────────
    dnf)
        sudo dnf install -y gcc-c++ make pkg-config gtk3-devel libX11-devel

        # webkit2gtk: try multiple package name variants
        WEBKIT_INSTALLED=0
        for wk in webkit2gtk4.1-devel webkit2gtk4.0-devel webkit2gtk3-devel; do
            if try_pkg sudo dnf install -y "$wk"; then
                WEBKIT_INSTALLED=1
                break
            fi
        done
        if [ $WEBKIT_INSTALLED -eq 0 ]; then
            echo "❌ No webkit2gtk package found via dnf"
            exit 1
        fi

        # Optional: gtk-layer-shell
        if try_pkg sudo dnf install -y gtk-layer-shell-devel; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell-devel not available — Wayland layer support disabled"
        fi
        ;;

    # ── Legacy yum (old RHEL / CentOS 7) ──────────────────────────────────────
    yum)
        sudo yum install -y gcc-c++ make pkgconfig gtk3-devel libX11-devel
        if ! try_pkg sudo yum install -y webkit2gtk3-devel; then
            echo "❌ webkit2gtk not available via yum. Consider upgrading to a newer RHEL/CentOS."
            exit 1
        fi
        echo "ℹ️  gtk-layer-shell not available on legacy yum systems"
        ;;

    # ── Arch / Manjaro / EndeavourOS / Garuda / Artix ─────────────────────────
    pacman)
        sudo pacman -Sy --needed --noconfirm base-devel pkgconf libx11

        # webkit2gtk: 4.1 in main repos (Arch current)
        if pacman -Si webkit2gtk-4.1 &>/dev/null 2>&1; then
            sudo pacman -S --needed --noconfirm gtk3 webkit2gtk-4.1
        else
            echo "ℹ️  webkit2gtk-4.1 not found, using webkit2gtk"
            sudo pacman -S --needed --noconfirm gtk3 webkit2gtk
        fi

        # Optional: gtk-layer-shell
        if pacman -Si gtk-layer-shell &>/dev/null 2>&1; then
            sudo pacman -S --needed --noconfirm gtk-layer-shell && LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell not found in repos — Wayland layer support disabled"
        fi
        ;;

    # ── openSUSE Tumbleweed / openSUSE Leap ───────────────────────────────────
    zypper)
        sudo zypper --non-interactive install gcc-c++ make pkg-config gtk3-devel libX11-devel

        # webkit2gtk: openSUSE uses underscore for dots in version names
        # Tumbleweed: webkit2gtk-4_1-devel  |  Leap: webkit2gtk3-devel
        WEBKIT_INSTALLED=0
        for wk_pkg in webkit2gtk-4_1-devel webkit2gtk3-soup2-devel webkit2gtk3-devel; do
            if try_pkg sudo zypper --non-interactive install "$wk_pkg"; then
                WEBKIT_INSTALLED=1
                echo "✅ Installed $wk_pkg"
                break
            fi
        done
        if [ $WEBKIT_INSTALLED -eq 0 ]; then
            echo "❌ No webkit2gtk package found for openSUSE"
            echo "   Tried: webkit2gtk-4_1-devel, webkit2gtk3-soup2-devel, webkit2gtk3-devel"
            exit 1
        fi

        # Optional: gtk-layer-shell
        if try_pkg sudo zypper --non-interactive install gtk-layer-shell-devel; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell not found in openSUSE repos — Wayland layer support disabled"
        fi
        ;;

    # ── Alpine Linux ───────────────────────────────────────────────────────────
    apk)
        sudo apk add --no-cache build-base pkgconf gtk+3.0-dev libx11-dev

        # webkit2gtk: use edge/testing if needed
        WEBKIT_INSTALLED=0
        for wk_pkg in webkit2gtk-4.1-dev webkit2gtk-6.0-dev webkit2gtk-dev; do
            if try_pkg sudo apk add --no-cache "$wk_pkg"; then
                WEBKIT_INSTALLED=1
                break
            fi
        done
        if [ $WEBKIT_INSTALLED -eq 0 ]; then
            echo "❌ webkit2gtk not found. Try enabling Alpine edge repos:"
            echo "   echo '@edge https://dl-cdn.alpinelinux.org/alpine/edge/community' >> /etc/apk/repositories"
            echo "   apk add webkit2gtk-dev@edge"
            exit 1
        fi

        # Optional
        if try_pkg sudo apk add --no-cache gtk-layer-shell-dev; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell not found in Alpine repos — Wayland layer support disabled"
        fi
        ;;

    # ── Void Linux ────────────────────────────────────────────────────────────
    xbps)
        sudo xbps-install -Sy gcc make pkg-config gtk+3-devel libX11-devel

        WEBKIT_INSTALLED=0
        for wk_pkg in webkit2gtk-devel webkit2gtk41-devel; do
            if try_pkg sudo xbps-install -Sy "$wk_pkg"; then
                WEBKIT_INSTALLED=1
                break
            fi
        done
        if [ $WEBKIT_INSTALLED -eq 0 ]; then
            echo "❌ webkit2gtk-devel not found in Void repos"
            exit 1
        fi

        if try_pkg sudo xbps-install -Sy gtk-layer-shell-devel; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell not in Void repos — Wayland layer support disabled"
        fi
        ;;

    # ── Solus ─────────────────────────────────────────────────────────────────
    eopkg)
        sudo eopkg install -y gcc g++ make pkgconf-devel gtk3-devel libx11-devel

        if try_pkg sudo eopkg install -y webkit2gtk-devel; then
            : # success
        else
            echo "❌ webkit2gtk-devel not found in Solus repos"
            exit 1
        fi

        if try_pkg sudo eopkg install -y gtk-layer-shell-devel; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell not in Solus repos — Wayland layer support disabled"
        fi
        ;;

    # ── Clear Linux ───────────────────────────────────────────────────────────
    swupd)
        sudo swupd bundle-add c-basic devpkg-gtk3 devpkg-libX11
        if sudo swupd bundle-add devpkg-webkit2gtk; then
            : # success
        else
            echo "❌ webkit2gtk bundle not found in Clear Linux"
            exit 1
        fi
        if try_pkg sudo swupd bundle-add devpkg-gtk-layer-shell; then
            LAYER_SHELL_OK=1
        else
            echo "ℹ️  gtk-layer-shell bundle not found — Wayland layer support disabled"
        fi
        ;;

    # ── Gentoo ────────────────────────────────────────────────────────────────
    emerge)
        echo ""
        echo "🧩 Gentoo detected. Please run as root:"
        echo "   emerge --ask x11-libs/gtk+:3 net-libs/webkit-gtk:4.1 x11-libs/libX11"
        echo ""
        echo "   Optional (Wayland wlr-layer-shell support):"
        echo "   emerge --ask gui-libs/gtk-layer-shell"
        echo ""
        echo "ℹ️  Automated installation skipped for Gentoo — please run manually."
        exit 0
        ;;

    # ── NixOS ─────────────────────────────────────────────────────────────────
    nix)
        echo ""
        echo "🧊 NixOS detected."
        echo "   Add to your shell.nix / devShell:"
        echo "     pkgs.gtk3  pkgs.webkitgtk_4_1  pkgs.libX11  pkgs.pkg-config  pkgs.gcc"
        echo ""
        echo "   Optional (Wayland): pkgs.gtk-layer-shell"
        echo ""
        echo "   Or run one-shot:"
        echo "     nix-shell -p gtk3 webkitgtk_4_1 libX11 pkg-config gcc"
        echo ""
        echo "ℹ️  Automated installation skipped for NixOS — please configure your environment."
        exit 0
        ;;

    # ── Unknown ───────────────────────────────────────────────────────────────
    *)
        echo ""
        echo "❌ Unknown package manager for: $NAME ($ID)"
        echo ""
        echo "Please install these packages manually:"
        echo "  Required : GTK3 dev, WebKit2GTK 4.1 dev (or 4.0), libX11 dev, pkg-config, gcc/g++"
        echo "  Optional : gtk-layer-shell dev (Wayland wlr-layer-shell support)"
        echo ""
        echo "Then re-run: npm install"
        exit 1
        ;;
esac

# ─── Summary ──────────────────────────────────────────────────────────────────
echo ""
echo "─────────────────────────────────────────────────────────"
echo "✨ Installation complete!"
if [ $LAYER_SHELL_OK -eq 1 ]; then
    echo "  ✅ gtk-layer-shell : Available (Wayland wlr-layer-shell enabled)"
else
    echo "  ⚠️  gtk-layer-shell : Not installed (X11 / XWayland fallback will be used)"
fi
echo "─────────────────────────────────────────────────────────"
