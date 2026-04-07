import fs from 'fs';
import path from 'path';
import os from 'os';
export class WidgetRegistry {
    configPath;
    constructor() {
        this.configPath = path.join(os.homedir(), '.config', 'widget-core-linux', 'widgets.json');
        this.ensureConfigDir();
    }
    ensureConfigDir() {
        try {
            const dir = path.dirname(this.configPath);
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
            }
            if (!fs.existsSync(this.configPath)) {
                fs.writeFileSync(this.configPath, JSON.stringify([], null, 2));
            }
            return true;
        }
        catch (e) {
            console.error("Failed to ensure config directory:", e);
            return false;
        }
    }
    getWidgets() {
        try {
            const data = fs.readFileSync(this.configPath, 'utf-8');
            return JSON.parse(data);
        }
        catch (e) {
            console.error("Failed to read widget registry:", e);
            return [];
        }
    }
    saveWidgets(widgets) {
        try {
            fs.writeFileSync(this.configPath, JSON.stringify(widgets, null, 2));
            return true;
        }
        catch (e) {
            console.error("Failed to save widget registry:", e);
            return false;
        }
    }
    addWidget(url, options, id) {
        try {
            const widgets = this.getWidgets();
            const finalId = id || Math.random().toString(36).substring(2, 11);
            widgets.push({ id: finalId, url, options, active: true });
            const success = this.saveWidgets(widgets);
            return { success, id: finalId };
        }
        catch (e) {
            console.error("Failed to add widget:", e);
            return { success: false };
        }
    }
    removeWidget(id) {
        try {
            const widgets = this.getWidgets().filter(w => w.id !== id);
            return this.saveWidgets(widgets);
        }
        catch (e) {
            return false;
        }
    }
    updateWidget(id, updates) {
        try {
            const widgets = this.getWidgets().map(w => w.id === id ? { ...w, ...updates } : w);
            return this.saveWidgets(widgets);
        }
        catch (e) {
            return false;
        }
    }
    activateWidget(id) {
        return this.updateWidget(id, { active: true });
    }
    deactivateWidget(id) {
        return this.updateWidget(id, { active: false });
    }
    getWidget(id) {
        return this.getWidgets().find(w => w.id === id);
    }
}
//# sourceMappingURL=registry.js.map