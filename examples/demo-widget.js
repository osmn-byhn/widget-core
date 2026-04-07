import path from 'path';
import { fileURLToPath } from 'url';

// FIX: Set environment variables BEFORE importing the native module
// Signal 10 (SIGUSR1) conflict fix: tell WebKit to use SIGUSR2 (12)
process.env.JSC_SIGNAL_FOR_GC = '12';
// Ensure X11/XWayland for GNOME desktop compatibility
process.env.GDK_BACKEND = 'x11';

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
