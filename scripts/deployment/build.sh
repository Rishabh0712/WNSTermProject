#!/bin/bash

# Build script for Multi-Party TLS Implementation
# This script checks for dependencies and compiles the project

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Multi-Party TLS - Build Script                       ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for C++ compiler
echo "[1/4] Checking for C++ compiler..."
if command_exists g++; then
    COMPILER="g++"
    echo "✓ Found g++"
elif command_exists clang++; then
    COMPILER="clang++"
    echo "✓ Found clang++"
else
    echo "✗ No C++ compiler found!"
    echo ""
    echo "Please install a C++ compiler:"
    echo "  Ubuntu/Debian: sudo apt-get install build-essential"
    echo "  Fedora/RHEL:   sudo dnf install gcc-c++"
    echo "  macOS:         xcode-select --install"
    exit 1
fi

# Check for OpenSSL
echo ""
echo "[2/4] Checking for OpenSSL development libraries..."
if [ -f "/usr/include/openssl/ssl.h" ] || [ -f "/usr/local/include/openssl/ssl.h" ]; then
    echo "✓ OpenSSL headers found"
else
    echo "✗ OpenSSL development libraries not found!"
    echo ""
    echo "Please install OpenSSL:"
    echo "  Ubuntu/Debian: sudo apt-get install libssl-dev"
    echo "  Fedora/RHEL:   sudo dnf install openssl-devel"
    echo "  macOS:         brew install openssl"
    exit 1
fi

# Compile
echo ""
echo "[3/4] Compiling source files..."
echo "Compiler: $COMPILER"
echo "Flags: -std=c++17 -Wall -Wextra -O2 -g"
echo ""

$COMPILER -std=c++17 -Wall -Wextra -O2 -g \
    shamir_secret_sharing.cpp \
    tls_multiparty.cpp \
    test_tls_multiparty.cpp \
    -o test_tls_multiparty \
    -lssl -lcrypto

if [ $? -eq 0 ]; then
    echo ""
    echo "[4/4] Build successful!"
    echo "✓ Executable created: ./test_tls_multiparty"
    echo ""
    echo "To run the tests:"
    echo "  ./test_tls_multiparty"
    echo ""
else
    echo ""
    echo "[4/4] Build failed!"
    echo "✗ Compilation errors occurred"
    exit 1
fi
