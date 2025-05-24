#!/bin/bash
set -e

# Build the plugin and package it into the correct bundle structure
PLUGIN_NAME="aac_encoder_plugin.dvcp"
BUNDLE_DIR="aac_encoder_plugin.dvcp.bundle/Contents/Linux-x86-64"

# Clean previous build
rm -rf aac_encoder_plugin.dvcp.bundle
mkdir -p "$BUNDLE_DIR"

# Build (assumes Makefile produces $PLUGIN_NAME in current dir or bin/)
make clean && make

# Copy the built plugin to the bundle
if [ -f "bin/$PLUGIN_NAME" ]; then
  cp "bin/$PLUGIN_NAME" "$BUNDLE_DIR/"
elif [ -f "$PLUGIN_NAME" ]; then
  cp "$PLUGIN_NAME" "$BUNDLE_DIR/"
else
  echo "Error: $PLUGIN_NAME not found after build."
  exit 1
fi

echo "Bundle created at $BUNDLE_DIR/$PLUGIN_NAME"