#include "WidgetManager.h"
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>

// Simple argument parser
std::map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.substr(0, 2) == "--") {
            size_t pos = arg.find('=');
            if (pos != std::string::npos) {
                args[arg.substr(2, pos - 2)] = arg.substr(pos + 1);
            } else if (i + 1 < argc) {
                args[arg.substr(2)] = argv[++i];
            }
        }
    }
    return args;
}

int main(int argc, char* argv[]) {
    std::map<std::string, std::string> args = parse_args(argc, argv);

    if (args.find("url") == args.end()) {
        std::cerr << "Usage: widget_host --url <url> [options]" << std::endl;
        return 1;
    }

    std::string url = args["url"];
    WidgetOptions options;
    options.width = args.count("width") ? std::stoi(args["width"]) : 400;
    options.height = args.count("height") ? std::stoi(args["height"]) : 400;
    options.x = args.count("x") ? std::stoi(args["x"]) : 100;
    options.y = args.count("y") ? std::stoi(args["y"]) : 100;
    options.opacity = args.count("opacity") ? std::stod(args["opacity"]) : 1.0;
    options.blur = args.count("blur") ? (args["blur"] == "true") : false;
    options.sticky = args.count("sticky") ? (args["sticky"] == "true") : true;
    options.interactive = args.count("interactive") ? (args["interactive"] == "true") : true;
    options.scroll = args.count("scroll") ? (args["scroll"] == "true") : true;

    std::cout << "[Host] Starting widget for: " << url << std::endl;

    // Direct call to CreateWidget (which starts the loop if needed)
    void* handle = WidgetManager::CreateWidget(url, options);

    if (!handle) {
        std::cerr << "[Host] Failed to create widget." << std::endl;
        return 1;
    }

    // In standalone mode, we need to keep the main thread alive 
    // since the loop runs in a detached thread in WidgetManager.cpp.
    // However, it's better if we just wait for the window to close.
    
    // For now, let's just sleep or wait for a signal.
    // Ideally, WidgetManager should provide a way to wait for all windows to close.
    
    std::cout << "[Host] Widget active. Press Ctrl+C to terminate host." << std::endl;
    
    // Simple wait loop
    while (true) {
        Sleep(1000);
    }

    return 0;
}
