#!/bin/bash
set -e

PLUGIN_BUNDLE="aac_encoder_plugin.dvcp.bundle"
PLUGIN_BUNDLE_FULL_PATH="$PLUGIN_BUNDLE/Contents/Linux-x86-64/aac_encoder_plugin.dvcp"
PLUGIN_PATH="/opt/resolve/IOPlugins/$PLUGIN_BUNDLE/Contents/Linux-x86-64"
IOPLUGINS_DIR="/opt/resolve/IOPlugins"

# Check ffmpeg presence
if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "Error: ffmpeg is not installed or not in PATH. Please install ffmpeg."
  exit 10
fi

if [ ! -f "$PLUGIN_BUNDLE_FULL_PATH" ]; then
  echo "Error: $PLUGIN_BUNDLE_FULL_PATH not found. Please build or extract the plugin bundle first."
  exit 2
fi

# Try to create IOPlugins dir if not exists, check write access
if [ ! -d "$IOPLUGINS_DIR" ]; then
  if ! mkdir -p "$IOPLUGINS_DIR" 2>/dev/null; then
    echo "No write access to $IOPLUGINS_DIR. Trying with sudo..."
    sudo mkdir -p "$IOPLUGINS_DIR"
  fi
fi

# Try to create target path and copy plugin, fallback to sudo if needed
if ! mkdir -p "$PLUGIN_PATH" 2>/dev/null; then
  echo "No write access to $PLUGIN_PATH. Trying with sudo..."
  sudo mkdir -p "$PLUGIN_PATH"
fi

if ! cp "$PLUGIN_BUNDLE_FULL_PATH" "$PLUGIN_PATH/" 2>/dev/null; then
  echo "No write access to $PLUGIN_PATH. Trying with sudo..."
  sudo cp "$PLUGIN_BUNDLE_FULL_PATH" "$PLUGIN_PATH/"
fi

echo "Plugin successfully installed to $PLUGIN_PATH/aac_encoder_plugin.dvcp"
