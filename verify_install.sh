#!/bin/bash
# EmuLens Installation Verification Script
# Run this after installation to verify the plugin is properly installed

echo "🔍 EmuLens Installation Verification"
echo "======================================"

PLUGIN_DIR="$HOME/.config/obs-studio/plugins/emuLensFX"
PLUGIN_BINARY="$PLUGIN_DIR/bin/64bit/emuLensFX.so"
SHADER_FILE="$PLUGIN_DIR/data/emulens.effect"

echo "Checking plugin directory: $PLUGIN_DIR"
if [ -d "$PLUGIN_DIR" ]; then
    echo "✅ Plugin directory exists"
else
    echo "❌ Plugin directory not found"
    exit 1
fi

echo "Checking plugin binary: $PLUGIN_BINARY"
if [ -f "$PLUGIN_BINARY" ]; then
    echo "✅ Plugin binary exists"
    ls -lh "$PLUGIN_BINARY"
else
    echo "❌ Plugin binary not found"
    exit 1
fi

echo "Checking shader file: $SHADER_FILE"
if [ -f "$SHADER_FILE" ]; then
    echo "✅ Shader file exists"
    ls -lh "$SHADER_FILE"
else
    echo "❌ Shader file not found"
    exit 1
fi

echo "Checking presets directory: $PLUGIN_DIR/data/presets"
if [ -d "$PLUGIN_DIR/data/presets" ]; then
    echo "✅ Presets directory exists"
    echo "Available presets:"
    ls -1 "$PLUGIN_DIR/data/presets" | sed 's/^/  - /'
else
    echo "❌ Presets directory not found"
fi

echo "Checking locale directory: $PLUGIN_DIR/data/locale"
if [ -d "$PLUGIN_DIR/data/locale" ]; then
    echo "✅ Locale directory exists"
    echo "Available locales:"
    ls -1 "$PLUGIN_DIR/data/locale" | sed 's/^/  - /'
else
    echo "❌ Locale directory not found"
fi

echo ""
echo "🎉 Installation verification complete!"
echo ""
echo "To use EmuLens in OBS Studio:"
echo "1. Launch OBS Studio"
echo "2. Add a video source (Media Source, Video Capture Device, etc.)"
echo "3. Right-click the source → Filters"
echo "4. Click '+' → EmuLens"
echo "5. Select your desired effect mode and adjust parameters"
echo ""
echo "For more information, see: https://github.com/myrqyry/emuLens"
