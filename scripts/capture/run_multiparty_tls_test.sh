#!/bin/bash

# Multi-Party TLS Handshake Test Script
# Compiles and runs the multi-party TLS simulation

echo "=========================================="
echo "Multi-Party TLS Handshake Test"
echo "=========================================="
echo ""

# Check if we're in WSL
if grep -qi microsoft /proc/version; then
    echo "✓ Running in WSL"
else
    echo "Note: This script is designed for WSL/Linux"
fi

# Clean previous builds
echo "Cleaning previous builds..."
rm -f test_multiparty_tls_handshake test_multiparty_tls_handshake.o

# Compile
echo ""
echo "Compiling test_multiparty_tls_handshake.cpp..."
g++ -std=c++17 -O2 \
    test_multiparty_tls_handshake.cpp \
    shamir_secret_sharing.cpp \
    -o test_multiparty_tls_handshake \
    -lssl -lcrypto \
    -I. \
    -Wall

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ Compilation failed!"
    exit 1
fi

echo "✓ Compilation successful!"

# Run the test
echo ""
echo "=========================================="
echo "Running Multi-Party TLS Handshake Test"
echo "=========================================="
echo ""

./test_multiparty_tls_handshake

TEST_RESULT=$?

echo ""
echo "=========================================="
if [ $TEST_RESULT -eq 0 ]; then
    echo "✓ ALL TESTS PASSED"
else
    echo "✗ TESTS FAILED (exit code: $TEST_RESULT)"
fi
echo "=========================================="

exit $TEST_RESULT
