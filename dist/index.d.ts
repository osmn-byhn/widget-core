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
    permissions?: string[];
    detached?: boolean;
}
export declare class DesktopWidget {
    private url;
    private nativeHandle;
    readonly id: string;
    private static readonly BLOCKED_KEYWORDS;
    private registry;
    private autostart;
    constructor(url: string | undefined, options: WidgetOptions);
    private spawnNativeHost;
    setOpacity(value: number): void;
    setPosition(x: number, y: number): void;
    setPersistent(value: boolean): void;
    makePersistent(options: WidgetOptions): Promise<boolean>;
    stopPersistence(): boolean;
    activate(): boolean;
    deactivate(): boolean;
    static listWidgets(): import("./registry.js").RegisteredWidget[];
    static listActiveWidgets(): import("./registry.js").RegisteredWidget[];
    static listPassiveWidgets(): import("./registry.js").RegisteredWidget[];
    static activateById(id: string): boolean;
    static deactivateById(id: string): boolean;
    static removeById(id: string): boolean;
    static stopAll(): boolean;
    static killAllProcesses(): boolean;
    private static killProcess;
    launchStandalone(): boolean;
    private validateURL;
    private applySecurityShield;
}
//# sourceMappingURL=index.d.ts.map