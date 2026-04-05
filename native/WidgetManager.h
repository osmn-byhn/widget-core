#ifndef WIDGET_MANAGER_H
#define WIDGET_MANAGER_H

#include <string>

struct WidgetOptions {
    int width;
    int height;
    int x;
    int y;
    double opacity;
    bool blur;
    bool sticky;
    bool interactive;
    std::string html;
    bool scroll;
};

class WidgetManager {
public:
    static void* CreateWidget(const std::string& url, const WidgetOptions& options);
    static void UpdateOpacity(void* handle, double opacity);
    static void UpdatePosition(void* handle, int x, int y);
};

#endif
