#!/bin/bash

# widget-core-linux: Multi-distribution dependency installer
set -e

echo "🔍 Detecting Linux distribution..."

if [ ! -f /etc/os-release ]; then
    echo "❌ Error: /etc/os-release not found. Unknown Linux distribution."
    exit 1
fi

. /etc/os-release

# Check if required libraries are already installed via pkg-config
CHECK_PKGS="gtk+-3.0 webkit2gtk-4.1 gtk-layer-shell-0 x11"
MISSING=0

for pkg in $CHECK_PKGS; do
    if ! pkg-config --exists $pkg; then
        echo "⚠️  Missing: $pkg"
        MISSING=1
    fi
done

if [ $MISSING -eq 0 ]; then
    echo "✅ All system dependencies are already installed."
    exit 0
fi

echo "📦 Dependencies missing. Installing for $NAME..."

case $ID in
    ubuntu|debian|kali|linuxmint|pop)
        sudo apt update
        sudo apt install -y build-essential pkg-config libgtk-3-dev libwebkit2gtk-4.1-dev libgtk-layer-shell-dev libx11-dev
        ;;
    fedora|rhel|centos|rocky)
        sudo dnf install -y gcc-c++ make pkg-config gtk3-devel webkit2gtk4.1-devel gtk-layer-shell-devel libX11-devel
        ;;
    arch|manjaro|endeavouros)
        sudo pacman -S --needed --noconfirm base-devel pkgconf gtk3 webkit2gtk-4.1 gtk-layer-shell libx11
        ;;
    opensuse*|suse)
        sudo zypper install -y gcc-c++ make pkg-config gtk3-devel webkit2gtk3-devel gtk-layer-shell-devel libX11-devel
        ;;
    *)
        echo "❌ Unsupported distribution: $ID ($NAME)"
        echo "Please install the development files for GTK3, WebKit2GTK, and GTK-Layer-Shell manually."
        exit 1
        ;;
esac

echo "✨ Dependencies installed successfully!"
