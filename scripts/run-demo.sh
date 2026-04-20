#!/bin/bash
# scripts/run-demo.sh
# Smart demo launcher: auto-detects display environment and sets appropriate flags

echo "🔍 Detecting display environment..."

# Detect display backend
if [ -n "${WAYLAND_DISPLAY:-}" ]; then
    DE="${XDG_CURRENT_DESKTOP:-${DESKTOP_SESSION:-unknown}}"
    DE_LOWER=$(echo "$DE" | tr '[:upper:]' '[:lower:]')

    # Check if it's a wlr compositor (supports layer-shell natively)
    IS_WLR=0
    for wlr_de in sway hyprland wayfire river labwc dwl niri cage hikari; do
        if echo "$DE_LOWER" | grep -q "$wlr_de"; then
            IS_WLR=1
            break
        fi
    done

    if [ $IS_WLR -eq 1 ]; then
        echo "🖥️  wlr Wayland compositor ($DE) → native Wayland + layer-shell"
        # No extra flags needed — widget-core handles it natively
    else
        echo "🖥️  Non-wlr Wayland compositor ($DE) → XWayland fallback"
        export GDK_BACKEND=x11
        export LIBGL_ALWAYS_SOFTWARE=1
        export WEBKIT_DISABLE_ACCELERATED_COMPOSITING=1
        export WEBKIT_DISABLE_COMPOSITING_MODE=1
    fi
else
    echo "🖥️  X11 display → native X11"
    # Software rendering helps on VMs and bare X11 without GPU acceleration
    export LIBGL_ALWAYS_SOFTWARE=1
    export WEBKIT_DISABLE_ACCELERATED_COMPOSITING=1
    export WEBKIT_DISABLE_COMPOSITING_MODE=1
fi

# Always disable client-side decorations (we want frameless widgets)
export GTK_CSD=0

echo "🚀 Launching demo widget..."
exec node examples/demo-widget.js "$@"
