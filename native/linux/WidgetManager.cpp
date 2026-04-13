#include "WidgetManager.h"
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <thread>
#include <mutex>
#include <condition_variable>

// Try to use gtk-layer-shell for Wayland support
#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#endif

// X11 backend
#include <gdk/gdk.h>
#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#endif

struct GtkWidgetContext {
    GtkWidget* window;
    GtkWidget* webview;
    bool wayland;
};

struct PosTask {
    GtkWidget* w;
    int x;
    int y;
    bool wayland;
};

static GMainContext* gtk_context = nullptr;

void run_gtk_loop() {
    // Note: Environment variables like GDK_BACKEND must be set in the process 
    // before the GTK thread starts. setenv() is NOT thread-safe for background threads.
    if (!gtk_init_check(NULL, NULL)) {
        return;
    }
    gtk_context = g_main_context_default();
    gtk_main();
}

struct CreateTask {
    const std::string* url;
    const WidgetOptions* options;
    void* result;
    std::mutex mutex;
    std::condition_variable cv;
    bool done;
    CreateTask(const std::string* u, const WidgetOptions* o)
        : url(u), options(o), result(nullptr), done(false) {}
};

#ifdef GDK_WINDOWING_X11
// Proper EWMH way: send _NET_WM_STATE_BELOW client message to the WM
static void send_wm_below(Display* dpy, Window xid) {
    XClientMessageEvent ev = {};
    ev.type = ClientMessage;
    ev.display = dpy;
    ev.window = xid;
    ev.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
    ev.format = 32;
    ev.data.l[0] = 1; // _NET_WM_STATE_ADD
    ev.data.l[1] = (long)XInternAtom(dpy, "_NET_WM_STATE_BELOW", False);
    ev.data.l[2] = 0;
    ev.data.l[3] = 1;
    ev.data.l[4] = 0;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureRedirectMask | SubstructureNotifyMask,
               (XEvent*)&ev);
    XFlush(dpy);
}

struct KeepBelowData {
    Display* dpy;
    Window xid;
};
#endif

// Auto-allow all permissions (Geolocation, Media, Notifications, etc.)
static gboolean permission_request_cb(WebKitWebView *web_view, WebKitPermissionRequest *request, gpointer user_data) {
    webkit_permission_request_allow(request);
    return TRUE;
}

gboolean create_widget_idle(gpointer data) {
    CreateTask* task = (CreateTask*)data;

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), task->options->width, task->options->height);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(window), task->options->interactive);
    gtk_window_set_focus_on_map(GTK_WINDOW(window), task->options->interactive);

    bool wayland_pinned = false;

    // === WAYLAND: Use gtk-layer-shell ===
#ifdef HAVE_GTK_LAYER_SHELL
    if (gtk_layer_is_supported()) {
        gtk_layer_init_for_window(GTK_WINDOW(window));
        gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_BACKGROUND);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, task->options->x);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, task->options->y);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(window), -1);
        gtk_layer_set_keyboard_mode(GTK_WINDOW(window), task->options->interactive ? GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND : GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
        wayland_pinned = true;
    }
#endif

    // === X11 / XWayland: EWMH keep-below ===
    if (!wayland_pinned && task->options->sticky) {
        gtk_window_stick(GTK_WINDOW(window));
        // Set keep_below hint: WM respects this on map
        gtk_window_set_keep_below(GTK_WINDOW(window), TRUE);
        // NORMAL hint: Safest way to ensure visibility; we use set_decorated(FALSE) to keep it widget-like
        gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);
    }

    GdkScreen* screen = gtk_widget_get_screen(window);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (visual) gtk_widget_set_visual(window, visual);

    GtkWidget* webview = webkit_web_view_new();
    
    // Auto-allow all permission requests (Geolocation, Camera, Microphone, etc.)
    g_signal_connect(webview, "permission-request", G_CALLBACK(permission_request_cb), NULL);
    
    // Enable advanced web APIs
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_webgl(settings, TRUE);
    webkit_settings_set_enable_webaudio(settings, TRUE);
    webkit_settings_set_enable_media_stream(settings, TRUE);
    webkit_settings_set_enable_mediasource(settings, TRUE);
    webkit_settings_set_enable_javascript(settings, TRUE);
    
    gtk_container_add(GTK_CONTAINER(window), webview);
    
    if (!task->options->html.empty()) {
        webkit_web_view_load_html(WEBKIT_WEB_VIEW(webview), task->options->html.c_str(), task->url->empty() ? NULL : task->url->c_str());
    } else {
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), task->url->c_str());
    }

    // Apply opacity from options
    if (task->options->opacity < 1.0) {
        gtk_widget_set_opacity(window, task->options->opacity);
    }
    
    gtk_widget_set_app_paintable(window, TRUE);
    
    // Apply blur: inject CSS into webview for backdrop blur effect
    if (task->options->blur) {
        const char* blur_css = "body { backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px); }";
        WebKitUserContentManager* ucm = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
        WebKitUserStyleSheet* sheet = webkit_user_style_sheet_new(
            blur_css,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_STYLE_LEVEL_USER,
            NULL, NULL);
        webkit_user_content_manager_add_style_sheet(ucm, sheet);
    }
    
    // Hide Scrollbars and handle Overflow
    {
        std::string scroll_css = "::-webkit-scrollbar { display: none !important; width: 0 !important; } ";
        if (!task->options->scroll) {
            scroll_css += "body { overflow: hidden !important; }";
        }
        WebKitUserContentManager* ucm = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
        WebKitUserStyleSheet* sheet = webkit_user_style_sheet_new(
            scroll_css.c_str(),
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_STYLE_LEVEL_USER,
            NULL, NULL);
        webkit_user_content_manager_add_style_sheet(ucm, sheet);
    }
    
    // Aggressive Transparency using CSS
    GdkRGBA rgba = {0, 0, 0, 0};
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), &rgba);
    
    // Modern transparency: apply via CSS provider
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "window, .main, body { background-color: rgba(0,0,0,0); }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(window),
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_widget_show_all(window);

    // Move AFTER show so the WM respects the position
    gtk_window_move(GTK_WINDOW(window), task->options->x, task->options->y);

    // X11: after show, send proper EWMH below request and start periodic re-enforcement
    if (!wayland_pinned) {
#ifdef GDK_WINDOWING_X11
        GdkWindow* gdk_win = gtk_widget_get_window(window);
        if (GDK_IS_X11_WINDOW(gdk_win)) {
            Display* dpy = GDK_WINDOW_XDISPLAY(gdk_win);
            Window xid = GDK_WINDOW_XID(gdk_win);

            // Force exact position via X11 (more reliable than GTK on XWayland)
            XMoveWindow(dpy, xid, task->options->x, task->options->y);

            // Initial below request
            send_wm_below(dpy, xid);
            XLowerWindow(dpy, xid);
            XFlush(dpy);

            // Periodic timer: re-send every 500ms to fight any WM restacking
            auto* kd = new KeepBelowData{dpy, xid};
            g_timeout_add(500, [](gpointer d) -> gboolean {
                auto* data = static_cast<KeepBelowData*>(d);
                send_wm_below(data->dpy, data->xid);
                XLowerWindow(data->dpy, data->xid);
                XFlush(data->dpy);
                return G_SOURCE_CONTINUE;
            }, kd);
        }
#endif
    }

    GtkWidgetContext* ctx = new GtkWidgetContext();
    ctx->window = window;
    ctx->webview = webview;
    ctx->wayland = wayland_pinned;

    task->result = ctx;
    {
        std::lock_guard<std::mutex> lock(task->mutex);
        task->done = true;
    }
    task->cv.notify_one();
    return FALSE;
}

void* WidgetManager::CreateWidget(const std::string& url, const WidgetOptions& options) {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
        std::thread(run_gtk_loop).detach();
        while (!gtk_context) std::this_thread::yield();
    });

    CreateTask task(&url, &options);
    g_main_context_invoke(gtk_context, create_widget_idle, &task);

    std::unique_lock<std::mutex> lock(task.mutex);
    task.cv.wait(lock, [&]{ return task.done; });

    return task.result;
}

void WidgetManager::UpdateOpacity(void* handle, double opacity) {
    if (!handle) return;
    g_main_context_invoke(gtk_context, [](gpointer data) -> gboolean {
        auto pair = (std::pair<GtkWidget*, double>*)data;
        gtk_widget_set_opacity(pair->first, pair->second);
        delete pair;
        return FALSE;
    }, new std::pair<GtkWidget*, double>(((GtkWidgetContext*)handle)->window, opacity));
}

void WidgetManager::UpdatePosition(void* handle, int x, int y) {
    if (!handle) return;
    auto* ctx = static_cast<GtkWidgetContext*>(handle);
    if (!ctx->window) return;

    g_main_context_invoke(gtk_context, [](gpointer data) -> gboolean {
        auto t = static_cast<PosTask*>(data);
        if (!t->w) {
            delete t;
            return FALSE;
        }

        if (t->wayland) {
#ifdef HAVE_GTK_LAYER_SHELL
            gtk_layer_set_margin(GTK_WINDOW(t->w), GTK_LAYER_SHELL_EDGE_LEFT, t->x);
            gtk_layer_set_margin(GTK_WINDOW(t->w), GTK_LAYER_SHELL_EDGE_TOP, t->y);
#endif
        } else {
            gtk_window_move(GTK_WINDOW(t->w), t->x, t->y);
#ifdef GDK_WINDOWING_X11
            GdkWindow* gdk_win = gtk_widget_get_window(t->w);
            if (gdk_win && GDK_IS_X11_WINDOW(gdk_win)) {
                Display* dpy = GDK_WINDOW_XDISPLAY(gdk_win);
                Window xid = GDK_WINDOW_XID(gdk_win);
                XMoveWindow(dpy, xid, t->x, t->y);
                XFlush(dpy);
            }
#endif
        }
        delete t;
        return FALSE;
    }, new PosTask{ctx->window, x, y, ctx->wayland});
}
