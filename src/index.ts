import { createRequire } from "module";
import { spawn } from "child_process";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { WidgetRegistry } from "./registry.js";
import { AutostartManager } from "./autostart.js";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const nodeRequire = createRequire(import.meta.url);

let native: any;
try {
  native = nodeRequire("../build/Release/widget_shield_native");
} catch (e) {
  console.warn("Native module not found, falling back to mock for testing.");
  native = {
    createWidget: (url: string, opts: any) => {
        console.log(`[Mock Shield] Creating widget for ${url} (Interactive: ${opts.interactive})`);
        return { id: 1 };
    },
    updateOpacity: (h: any, v: number) => console.log(`[Mock Shield] Opacity -> ${v}`),
    updatePosition: (h: any, x: number, y: number) => console.log(`[Mock Shield] Position -> (${x}, ${y})`)
  };
}

export interface WidgetOptions {
  width: number;
  height: number;
  x: number;
  y: number;
  opacity?: number;
  blur?: boolean;
  sticky?: boolean;
  interactive?: boolean;
  html?: string;
  scroll?: boolean;
  permissions?: string[]; // e.g., ['network', 'disk-read']
  detached?: boolean;
}

export class DesktopWidget {
  private nativeHandle: any;
  public readonly id: string;
  private static readonly BLOCKED_KEYWORDS = ["shell", "process", "eval", "fs", "child_process"];
  private registry = new WidgetRegistry();
  private autostart = new AutostartManager();

  constructor(
    private url: string = "",
    options: WidgetOptions,
  ) {
    this.id = Math.random().toString(36).substring(2, 11);
    if (url) {
        this.validateURL(url);
    } else if (!options.html) {
        throw new Error("Either url or html content must be provided.");
    }
    this.applySecurityShield(options);

    // Native tarafında pencereyi oluştur
    try {
      if (options.detached) {
        this.spawnNativeHost(url, options);
      } else {
        this.nativeHandle = native.createWidget(url, options);
      }
    } catch (e) {
      console.error("Failed to create native widget:", e);
    }
  }

  private spawnNativeHost(url: string, options: WidgetOptions) {
    const isWin = process.platform === 'win32';
    const hostName = isWin ? 'widget_host.exe' : 'widget_host';
    // Look in build/Release
    const hostPath = path.resolve(__dirname, '..', 'build', 'Release', hostName);
    
    if (!fs.existsSync(hostPath)) {
        console.warn(`Native host not found at ${hostPath}. Falling back to in-process widget.`);
        this.nativeHandle = native.createWidget(url, options);
        return;
    }

    const args = [
        `--url=${url}`,
        `--width=${options.width}`,
        `--height=${options.height}`,
        `--x=${options.x}`,
        `--y=${options.y}`,
        `--opacity=${options.opacity ?? 1.0}`,
        `--blur=${options.blur ? 'true' : 'false'}`,
        `--sticky=${options.sticky !== false ? 'true' : 'false'}`,
        `--interactive=${options.interactive !== false ? 'true' : 'false'}`,
        `--scroll=${options.scroll !== false ? 'true' : 'false'}`
    ];

    console.log(`[DesktopWidget] Spawning detached native host: ${hostName}`);
    const child = spawn(hostPath, args, {
        detached: true,
        stdio: 'ignore'
    });
    
    child.unref();
  }

  public setOpacity(value: number) {
    if (value < 0 || value > 1) throw new Error("Opacity must be between 0 and 1");
    native.updateOpacity(this.nativeHandle, value);
    const existing = this.registry.getWidget(this.id);
    if (existing) {
        this.registry.updateWidget(this.id, { options: { ...existing.options, opacity: value } });
    }
  }

  public setPosition(x: number, y: number) {
    native.updatePosition(this.nativeHandle, x, y);
    const existing = this.registry.getWidget(this.id);
    if (existing) {
        this.registry.updateWidget(this.id, { options: { ...existing.options, x, y } });
    }
  }

  public setPersistent(value: boolean) {
    if (value) {
        if (!this.id) {
            // Options used to create this widget might not be exactly what we want to persist if they were modified
            // For now, we use the ones that work.
            // Note: We'd need to track current options in the class.
            // Simplified for this implementation.
        }
    }
  }

  public async makePersistent(options: WidgetOptions): Promise<boolean> {
    try {
        const result = this.registry.addWidget(this.url, options, this.id);
        if (!result.success) return false;
        
        const runnerPath = path.join(__dirname, "runner.js");
        const command = `node ${runnerPath} ${this.id}`;
        return this.autostart.enable(this.id, this.url, command);
    } catch (e) {
        return false;
    }
  }

  public stopPersistence(): boolean {
    try {
        const autostartSuccess = this.autostart.disable(this.id);
        const registrySuccess = this.registry.removeWidget(this.id);
        const killSuccess = DesktopWidget.killProcess(this.id);
        return autostartSuccess && registrySuccess && killSuccess;
    } catch (e) {
        return false;
    }
  }

  public activate(): boolean {
    try {
        if (!this.id) return false;
        const widget = this.registry.getWidget(this.id);
        if (!widget) return false;

        const regSuccess = this.registry.activateWidget(this.id);
        const runnerPath = path.join(__dirname, "runner.js");
        const command = `node ${runnerPath} ${this.id}`;
        const autoSuccess = this.autostart.enable(this.id, widget.url, command);
        return regSuccess && autoSuccess;
    } catch (e) {
        return false;
    }
  }

  public deactivate(): boolean {
    try {
        const regSuccess = this.registry.deactivateWidget(this.id);
        const autoSuccess = this.autostart.disable(this.id);
        const killSuccess = DesktopWidget.killProcess(this.id);
        return regSuccess && autoSuccess && killSuccess;
    } catch (e) {
        return false;
    }
  }

  public static listWidgets() {
    return new WidgetRegistry().getWidgets();
  }

  public static listActiveWidgets() {
    return new WidgetRegistry().getWidgets().filter(w => w.active);
  }

  public static listPassiveWidgets() {
    return new WidgetRegistry().getWidgets().filter(w => !w.active);
  }

  public static activateById(id: string): boolean {
    try {
        const registry = new WidgetRegistry();
        const autostart = new AutostartManager();
        const widget = registry.getWidget(id);
        if (!widget) return false;

        const regSuccess = registry.activateWidget(id);
        const runnerPath = path.join(__dirname, "runner.js");
        const command = `node ${runnerPath} ${id}`;
        const autoSuccess = autostart.enable(id, widget.url, command);
        return regSuccess && autoSuccess;
    } catch (e) {
        return false;
    }
  }

  public static deactivateById(id: string): boolean {
    try {
        const registry = new WidgetRegistry();
        const autostart = new AutostartManager();
        const regSuccess = registry.deactivateWidget(id);
        const autoSuccess = autostart.disable(id);
        const killSuccess = DesktopWidget.killProcess(id);
        return regSuccess && autoSuccess && killSuccess;
    } catch (e) {
        return false;
    }
  }

  public static removeById(id: string): boolean {
    try {
        const registry = new WidgetRegistry();
        const autostart = new AutostartManager();
        const regSuccess = registry.removeWidget(id);
        const autoSuccess = autostart.disable(id);
        const killSuccess = DesktopWidget.killProcess(id);
        return regSuccess && autoSuccess && killSuccess;
    } catch (e) {
        return false;
    }
  }

  public static stopAll(): boolean {
    try {
        const registry = new WidgetRegistry();
        const autostart = new AutostartManager();
        const widgets = registry.getWidgets();
        
        let success = true;
        for (const w of widgets) {
            const regSuccess = registry.deactivateWidget(w.id);
            const autoSuccess = autostart.disable(w.id);
            if (!regSuccess || !autoSuccess) success = false;
        }
        
        const killSuccess = DesktopWidget.killAllProcesses();
        return success && killSuccess;
    } catch (e) {
        return false;
    }
  }

  public static killAllProcesses(): boolean {
    const { execSync } = createRequire(import.meta.url)("child_process");
    try {
        if (process.platform === 'win32') {
            execSync(`wmic process where "CommandLine like '%runner.js%'" delete`);
        } else {
            execSync(`pkill -f "runner.js"`);
        }
        return true;
    } catch (e) {
        return true; // Still okay if none found
    }
  }

  private static killProcess(id: string): boolean {
    const { execSync } = createRequire(import.meta.url)("child_process");
    try {
        if (process.platform === 'win32') {
            try {
                execSync(`wmic process where "CommandLine like '%runner.js %${id}%'" delete`);
            } catch (e) { }
        } else {
            execSync(`pkill -f "runner.js ${id}"`);
        }
        return true;
    } catch (e) {
        return true; // pkill returns non-zero if no process matched, which is fine
    }
  }

  public launchStandalone(): boolean {
    try {
        if (!this.id) return false;
        
        const runnerPath = path.join(__dirname, "runner.js");
        const child = spawn("node", [runnerPath, this.id], {
            detached: true,
            stdio: 'ignore'
        });
        child.unref();
        return true;
    } catch (e) {
        return false;
    }
  }

  private validateURL(url: string) {
    try {
      const parsed = new URL(url);
      if (parsed.protocol !== "http:" && parsed.protocol !== "https:" && parsed.protocol !== "file:") {
        throw new Error(`Security Shield: Protocol ${parsed.protocol} is blocked.`);
      }
      
      // Localhost or specific allowed domains could be enforced here
    } catch (e) {
      if (e instanceof Error && e.message.includes("Security Shield")) throw e;
      throw new Error("Security Shield: Invalid URL format.");
    }
  }

  private applySecurityShield(options: WidgetOptions) {
    // Block sensitive node-like options if they were passed
    const permissions = options.permissions || [];
    if (permissions.includes("all")) {
        console.warn("Security Shield: 'all' permissions granted. Use with extreme caution.");
    }
    
    // In a real implementation, we would pass these flags to the native webview
    // to disable Node.js integration, context isolation, etc.
  }
}
