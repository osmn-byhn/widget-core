{
  "targets": [
    {
      "target_name": "widget_shield_native",
      "sources": [ "native/main.cpp" ],
      "include_dirs": [
        "native",
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "conditions": [
        ["OS=='mac'", {
          "sources": [ "native/macos/WidgetManager.mm" ],
          "link_settings": {
            "libraries": ["-framework AppKit", "-framework WebKit"]
          },
          "xcode_settings": {
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }],
        ["OS=='win'", {
          "sources": [ "native/win32/WidgetManager.cpp" ],
          "libraries": ["dwmapi.lib", "user32.lib", "ole32.lib", "oleaut32.lib"],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "AdditionalOptions": [ "/std:c++17" ]
            }
          }
        }],
        ["OS=='linux'", {
          "sources": [ "native/linux/WidgetManager.cpp" ],
          "cflags": [ "<!@(pkg-config --cflags gtk+-3.0 webkit2gtk-4.1 gtk-layer-shell-0 2>/dev/null || pkg-config --cflags gtk+-3.0 webkit2gtk-4.0 2>/dev/null)" ],
          "cflags_cc": [ 
            "<!@(pkg-config --cflags gtk+-3.0 webkit2gtk-4.1 gtk-layer-shell-0 2>/dev/null || pkg-config --cflags gtk+-3.0 webkit2gtk-4.0 2>/dev/null)",
            "-std=c++17" 
          ],
          "defines": [ "HAVE_GTK_LAYER_SHELL" ],
          "link_settings": {
            "libraries": [ 
              "<!@(pkg-config --libs gtk+-3.0 webkit2gtk-4.1 gtk-layer-shell-0 2>/dev/null || pkg-config --libs gtk+-3.0 webkit2gtk-4.0 2>/dev/null)",
              "-lX11"
            ]
          }
        }]
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "NODE_ADDON_API_CPP_EXCEPTIONS" ]
    },
    {
      "target_name": "widget_host",
      "type": "executable",
      "sources": [ "native/host.cpp" ],
      "include_dirs": [ "native" ],
      "conditions": [
        ["OS=='win'", {
          "sources": [ "native/win32/WidgetManager.cpp" ],
          "libraries": ["dwmapi.lib", "user32.lib", "ole32.lib", "oleaut32.lib"],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "AdditionalOptions": [ "/std:c++17" ]
            }
          }
        }]
      ]
    }
  ]
}
