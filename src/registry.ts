import fs from 'fs';
import path from 'path';
import os from 'os';
import type { WidgetOptions } from './index.js';

export interface RegisteredWidget {
    id: string;
    url: string;
    options: WidgetOptions;
    active: boolean;
}

export class WidgetRegistry {
    private configPath: string;

    constructor() {
        this.configPath = path.join(os.homedir(), '.config', 'widget-core-linux', 'widgets.json');
        this.ensureConfigDir();
    }

    private ensureConfigDir(): boolean {
        try {
            const dir = path.dirname(this.configPath);
            if (!fs.existsSync(dir)) {
                fs.mkdirSync(dir, { recursive: true });
            }
            if (!fs.existsSync(this.configPath)) {
                fs.writeFileSync(this.configPath, JSON.stringify([], null, 2));
            }
            return true;
        } catch (e) {
            console.error("Failed to ensure config directory:", e);
            return false;
        }
    }

    public getWidgets(): RegisteredWidget[] {
        try {
            const data = fs.readFileSync(this.configPath, 'utf-8');
            return JSON.parse(data);
        } catch (e) {
            console.error("Failed to read widget registry:", e);
            return [];
        }
    }

    public saveWidgets(widgets: RegisteredWidget[]): boolean {
        try {
            fs.writeFileSync(this.configPath, JSON.stringify(widgets, null, 2));
            return true;
        } catch (e) {
            console.error("Failed to save widget registry:", e);
            return false;
        }
    }

    public addWidget(url: string, options: WidgetOptions, id?: string): { success: boolean, id?: string } {
        try {
            const widgets = this.getWidgets();
            const finalId = id || Math.random().toString(36).substring(2, 11);
            widgets.push({ id: finalId, url, options, active: true });
            const success = this.saveWidgets(widgets);
            return { success, id: finalId };
        } catch (e) {
            console.error("Failed to add widget:", e);
            return { success: false };
        }
    }

    public removeWidget(id: string): boolean {
        try {
            const widgets = this.getWidgets().filter(w => w.id !== id);
            return this.saveWidgets(widgets);
        } catch (e) {
            return false;
        }
    }

    public updateWidget(id: string, updates: Partial<RegisteredWidget>): boolean {
        try {
            const widgets = this.getWidgets().map(w => w.id === id ? { ...w, ...updates } : w);
            return this.saveWidgets(widgets);
        } catch (e) {
            return false;
        }
    }

    public activateWidget(id: string): boolean {
        return this.updateWidget(id, { active: true });
    }

    public deactivateWidget(id: string): boolean {
        return this.updateWidget(id, { active: false });
    }

    public getWidget(id: string): RegisteredWidget | undefined {
        return this.getWidgets().find(w => w.id === id);
    }
}
