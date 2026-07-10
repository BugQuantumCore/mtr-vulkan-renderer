#!/bin/bash

# Development environment setup script

set -e

echo "Setting up MTR Vulkan Renderer development environment..."

# Check for required tools
echo "Checking required tools..."

# Java 21
if ! command -v java &> /dev/null; then
    echo "❌ Java not found. Please install JDK 21."
    exit 1
fi

JAVA_VERSION=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}')
echo "✅ Java version: $JAVA_VERSION"

# CMake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake 3.20+."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n 1)
echo "✅ $CMAKE_VERSION"

# Vulkan SDK
if ! command -v glslangValidator &> /dev/null; then
    echo "❌ Vulkan SDK not found. Please install Vulkan SDK."
    echo "   Download from: https://vulkan.lunarg.com/sdk/home"
    exit 1
fi

echo "✅ Vulkan SDK found"

# Git
if ! command -v git &> /dev/null; then
    echo "❌ Git not found. Please install Git."
    exit 1
fi

echo "✅ Git found"

# Make gradlew executable
echo "Setting up Gradle wrapper..."
chmod +x gradlew

# Build project
echo "Building project..."
./gradlew build

# Compile shaders
echo "Compiling shaders..."
./scripts/compile-shaders.sh

echo ""
echo "✅ Development environment setup complete!"
echo ""
echo "Next steps:"
echo "  1. Place MTR JAR in libs/ folder"
echo "  2. Place VulkanMod JAR in libs/ folder"
echo "  3. Run: ./gradlew runClient"
echo ""