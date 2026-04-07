import path from 'path';
import { fileURLToPath } from 'url';

// --- ENVIRONMENT FIXES ---
// 1. Signal Conflict Fix: Tell WebKit to use SIGUSR2 (avoids Segfault/Error)
process.env.JSC_SIGNAL_FOR_GC = 'SIGUSR2';

// 2. Visibility Fix: Disable compositing mode (essential for some GPU/Wayland drivers)
process.env.WEBKIT_DISABLE_COMPOSITING_MODE = '1';

// 3. Backend Fix: Automatic selection
const isGnome = process.env.XDG_CURRENT_DESKTOP?.toUpperCase().includes('GNOME');
const isWayland = process.env.XDG_SESSION_TYPE === 'wayland';

if (isGnome && isWayland) {
    // GNOME Wayland needs XWayland to support 'keep-below' layer
    process.env.GDK_BACKEND = 'x11';
}
// -------------------------

import { DesktopWidget } from '../dist/index.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const htmlPath = 'file://' + path.join(__dirname, 'demo.html');

console.log('🚀 Launching Widget-Core Demo...');
console.log('📁 Loading:', htmlPath);

try {
    const widget = new DesktopWidget(htmlPath, {
        width: 450,
        height: 250,
        x: 100,
        y: 100,
        opacity: 0.95,
        interactive: false, // Click through to desktop
        sticky: true,       // Stay on bottom
        blur: true          // Apply backdrop blur if supported
    });

    console.log('✅ Widget active! Check your desktop.');
    console.log('ID:', widget.id);

    // Keep the process alive
    process.on('SIGINT', () => {
        console.log('\n🛑 Shutting down...');
        process.exit(0);
    });

} catch (error) {
    console.error('❌ Failed to launch widget:', error);
    process.exit(1);
}
