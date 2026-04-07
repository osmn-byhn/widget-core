# WidgetCore-Linux 🐧

**@osmn-byhn/widget-core-linux** is a high-performance, Linux-native library designed for developers who want to create beautiful, interactive, and persistent desktop widgets without the overhead of heavy frameworks. 

Built with **Node.js** and **C++**, it leverages **WebKitGTK** to provide a lightweight alternative to Electron, optimized specifically for the Linux desktop experience.

---

## 🏗️ Architecture: Hybrid Power

WidgetCore uses a sophisticated hybrid model to ensure both developer productivity and runtime efficiency.

```mermaid
graph TD
    A[Node.js Runtime] --> B[DesktopWidget API]
    B --> C[WidgetRegistry (Persistence)]
    B --> D[AutostartManager (Systemd/XDG)]
    B --> E[Native C++ Addon (N-API)]
    E --> F[GTK3 Windowing]
    E --> G[WebKitGTK Rendering]
    F --> H[Wayland (gtk-layer-shell)]
    F --> I[X11 (EWMH/Xlib)]
```

---

## ⚡ Why WidgetCore-Linux?

Unlike generic cross-platform frameworks, WidgetCore-Linux is built from the ground up for the Linux ecosystem.

| Feature | WidgetCore-Linux | Electron |
| :--- | :--- | :--- |
| **Memory Footprint** | ~15MB - 30MB | 150MB+ |
| **Windowing** | Native GTK Layers | Chromium Windowing |
| **Wayland Support** | Native `gtk-layer-shell` | XWayland / Ozone |
| **System Integration** | Linux-specific Autostart | Generic |
| **Security** | Hardware-level Sandboxing | Software-level Isolation |

---

## ✨ Features at a Glance

- **🚀 Performance First**: Direct access to GTK widgets and WebKit rendering without Chromium's overhead.
- **🖼️ Aggressive Transparency**: Deep integration with window managers to ensure widgets blend perfectly with any wallpaper.
- **🖇️ Universal Linux Support**: Binary compatibility for Wayland and X11 (including GNOME, KDE, LXQt, and Tiling WMs).
- **🖱️ Flexible Interactivity**: Seamlessly toggle between "Click-through" passive mode and "Interactive" desktop utility mode.
- **💾 Zero-Config Persistence**: Built-in registry that remembers your widgets' positions, sizes, and states across reboots.
- **🔒 Security Shield**: A built-in security layer that sandboxes the widget's JS environment, blocking sensitive Node.js APIs by default.

---

## 🔒 Security Shield Architecture

Security is non-negotiable for desktop widgets that might load external content. Our "Security Shield" provides:

1. **Protocol Filtering**: Restricts content to `http:`, `https:`, or vetted `file:` paths.
2. **Context Isolation**: Each widget runs its JS in a separate WebKit context.
3. **Keyword Inspection**: Prevents execution of strings containing dangerous keywords like `require`, `process`, or `child_process`.
4. **API Freezing**: Injects a protective script that locks down sensitive globals before the widget content loads.

---

## 🚀 Getting Started

### Installation

```bash
npm install @osmn-byhn/widget-core-linux
```

### Basic Usage: Remote Widget

```typescript
import { DesktopWidget } from '@osmn-byhn/widget-core-linux';

// Load a remote dashboard
const widget = new DesktopWidget('https://your-dashboard.com', {
  width: 400,
  height: 600,
  x: 100,
  y: 100,
  opacity: 0.9,
  interactive: true
});

// Ensure it starts when the system boots
await widget.makePersistent({ name: 'My Dashboard' });
```

### Advanced Usage: Local HTML with Blur

```typescript
const clockHTML = `
  <div style="font-size: 50px; color: white; filter: drop-shadow(0 0 10px blue);">Time: <span id="t"></span></div>
  <script>
    setInterval(() => document.getElementById('t').innerText = new Date().toLocaleTimeString(), 1000);
  </script>
`;

const clock = new DesktopWidget("", {
  html: clockHTML,
  width: 400,
  height: 200,
  x: 800,
  y: 10,
  blur: true, // Enable backdrop-blur via WebKit
  scroll: false
});
```

---

## 🛠️ API Documentation

### `DesktopWidget(url, options)`

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `width` | `number` | **Required** | Window width. |
| `height` | `number` | **Required** | Window height. |
| `x` | `number` | **Required** | Initial X position. |
| `y` | `number` | **Required** | Initial Y position. |
| `opacity` | `number` | `1.0` | Global widget transparency (0-1). |
| `blur` | `boolean` | `false` | Apply CSS backdrop-blur to the window. |
| `sticky` | `boolean` | `true` | Pin to desktop (always below other windows). |
| `interactive` | `boolean` | `true` | Allow clicks/scrolls. |
| `scroll` | `boolean` | `true` | Show/hide scrollbars. |
| `permissions` | `string[]` | `[]` | List of allowed system capabilities. |

---

## 🐧 Distribution & Environment

WidgetCore-Linux handles the complexity of Linux desktop protocols automatically.

- **Wayland Support**: Uses `gtk-layer-shell` to anchor widgets to the desktop background layer.
- **X11 Support**: Manages `_NET_WM_STATE_BELOW` atoms and sends periodic heartbeats to ensure the window stays at the bottom of the stack.
- **Autostart**: Implements XDG Autostart specification by generating `.desktop` files in `~/.config/autostart/`.

### System Requirements

Ensure you have the following development libraries installed:

- **Debian/Ubuntu**: `libwebkit2gtk-4.0-dev`, `libgtk-3-dev`, `libgtk-layer-shell-dev`
- **Arch**: `webkit2gtk`, `gtk3`, `gtk-layer-shell`
- **Fedora**: `webkit2gtk4.0-devel`, `gtk3-devel`, `gtk-layer-shell-devel`

---

## 🧪 Development & Testing

We use **Vitest** for unit testing and **Xvfb** for headless GUI verification.

```bash
# Install and build
npm install
npm run build

# Run tests
npm test

# Headless GUI test (requires xvfb)
xvfb-run -a node test_cyberpunk.js
```

---

## 📝 License & Authors

**MIT License.** Copyright (c) 2026 **Osman Beyhan**.

Special thanks to the [Web Widgets Community](https://github.com/web-widgets-community) for inspiring the widget engine.
