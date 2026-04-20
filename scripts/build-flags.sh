#!/bin/bash
# scripts/build-flags.sh
# Helper for binding.gyp: generates correct pkg-config cflags/libs
# Usage: build-flags.sh --cflags | --libs | --has-layer-shell

MODE=$1

# Detect webkit2gtk version (4.1 preferred, 4.0 fallback)
if pkg-config --exists webkit2gtk-4.1 2>/dev/null; then
    WK="webkit2gtk-4.1"
elif pkg-config --exists webkit2gtk-4.0 2>/dev/null; then
    WK="webkit2gtk-4.0"
else
    echo "ERROR: webkit2gtk not found (run: npm run setup)" >&2
    exit 1
fi

# Base packages always required
BASE_PKGS="gtk+-3.0 $WK"

# Optional: gtk-layer-shell
if pkg-config --exists gtk-layer-shell-0 2>/dev/null; then
    LAYER_PKGS="gtk-layer-shell-0"
else
    LAYER_PKGS=""
fi

case "$MODE" in
    --cflags)
        pkg-config --cflags $BASE_PKGS $LAYER_PKGS
        ;;
    --libs)
        LIBS=$(pkg-config --libs $BASE_PKGS $LAYER_PKGS)
        # Add X11 if available
        if pkg-config --exists x11 2>/dev/null; then
            echo "$LIBS -lX11"
        else
            echo "$LIBS"
        fi
        ;;
    --has-layer-shell)
        # Output "1" if gtk-layer-shell is present, "0" otherwise
        pkg-config --exists gtk-layer-shell-0 2>/dev/null && echo 1 || echo 0
        ;;
    --webkit-version)
        echo "$WK"
        ;;
    *)
        echo "Usage: $0 --cflags | --libs | --has-layer-shell | --webkit-version" >&2
        exit 1
        ;;
esac
