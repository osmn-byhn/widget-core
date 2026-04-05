import { DesktopWidget } from './dist/index.js';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const widgetUrl = `file:///${path.resolve(__dirname, 'example_widget.html').replace(/\\/g, '/')}`;

console.log('-------------------------------------------');
console.log('DETACHED WIDGET TEST');
console.log('-------------------------------------------');
console.log('Launching detached widget...');

try {
    const widget = new DesktopWidget(widgetUrl, {
        width: 400,
        height: 400,
        x: 150,
        y: 150,
        opacity: 0.9,
        blur: true,
        sticky: true,
        interactive: true,
        detached: true // Bu seçenek artık bağımsız süreci başlatıyor
    });

    console.log('✅ Detached widget launch command sent!');
    console.log('Current process will now exit, but the widget should STAY OPEN.');
    
    // Uygulamanın hemen kapanmasını sağlıyoruz
    process.exit(0);

} catch (error) {
    console.error('❌ Failed to launch detached widget:', error);
    process.exit(1);
}
