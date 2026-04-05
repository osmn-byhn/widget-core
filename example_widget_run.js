import { DesktopWidget } from './dist/index.js';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const widgetUrl = `file:///${path.resolve(__dirname, 'example_widget.html').replace(/\\/g, '/')}`;

console.log('-------------------------------------------');
console.log('WIDGET CORE TEST RUN');
console.log('-------------------------------------------');
console.log('Launching widget with URL:', widgetUrl);

try {
    const widget = new DesktopWidget(widgetUrl, {
        width: 350,
        height: 350,
        x: 100,
        y: 100,
        opacity: 0.95,
        blur: true,
        sticky: true,
        interactive: true,
        overflow: true
    });

    console.log('✅ Widget launched successfully!');
    console.log('Widget ID:', widget.id);

    // Simulate some updates
    setTimeout(() => {
        console.log('🔄 Moving widget to (300, 300)...');
        widget.setPosition(300, 300);
    }, 2000);

    setTimeout(() => {
        console.log('✅ Setup completed. Widget is now active.');
        console.log('Press Ctrl+C to close this process.');
    }, 4000);

    // Keep process alive indefinitely
    setInterval(() => { }, 1000 * 60 * 60);

} catch (error) {
    console.error('❌ Failed to launch widget:', error);
    process.exit(1);
}
