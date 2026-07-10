#!/bin/bash

# Compile all GLSL shaders to SPIR-V

set -e

SHADER_DIR="src/main/resources/shaders"
OUTPUT_DIR="build/shaders"

# Check if glslangValidator is available
if ! command -v glslangValidator &> /dev/null; then
    echo "Error: glslangValidator not found. Please install Vulkan SDK."
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Find all shader files
VERT_SHADERS=$(find "$SHADER_DIR" -name "*.vert")
FRAG_SHADERS=$(find "$SHADER_DIR" -name "*.frag")

echo "Compiling vertex shaders..."
for shader in $VERT_SHADERS; do
    filename=$(basename "$shader")
    output="$OUTPUT_DIR/${filename}.spv"
    echo "  Compiling $shader -> $output"
    glslangValidator -V "$shader" -o "$output"
done

echo "Compiling fragment shaders..."
for shader in $FRAG_SHADERS; do
    filename=$(basename "$shader")
    output="$OUTPUT_DIR/${filename}.spv"
    echo "  Compiling $shader -> $output"
    glslangValidator -V "$shader" -o "$output"
done

echo "Shader compilation complete!"
echo "SPIR-V files are in: $OUTPUT_DIR"