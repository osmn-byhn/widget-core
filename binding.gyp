{
  "variables": {
    "has_layer_shell%": "<!(bash scripts/build-flags.sh --has-layer-shell 2>/dev/null || echo 0)"
  },
  "targets": [
    {
      "target_name": "widget_shield_native",
      "sources": [
        "native/main.cpp",
        "native/linux/WidgetManager.cpp"
      ],
      "include_dirs": [
        "native",
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "cflags": [
        "<!@(bash scripts/build-flags.sh --cflags)"
      ],
      "cflags_cc": [
        "<!@(bash scripts/build-flags.sh --cflags)"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS"
      ],
      "link_settings": {
        "libraries": [
          "<!@(bash scripts/build-flags.sh --libs)"
        ]
      },
      "conditions": [
        ["has_layer_shell==1", {
          "defines": ["HAVE_GTK_LAYER_SHELL"]
        }]
      ]
    }
  ]
}
