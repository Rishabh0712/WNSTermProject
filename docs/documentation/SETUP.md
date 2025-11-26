# Setup Instructions for Multi-Party TLS Implementation

## Prerequisites

This C++ implementation requires:
1. **C++17 compatible compiler** (g++ 7.0+, clang++ 5.0+, or MSVC 2017+)
2. **OpenSSL development libraries** (for cryptographic operations)

## Installation Guide

### Option 1: Ubuntu/Debian (including WSL)

```bash
# Update package lists
sudo apt-get update

# Install build essentials (g++, make, etc.)
sudo apt-get install build-essential

# Install OpenSSL development libraries
sudo apt-get install libssl-dev

# Verify installation
g++ --version
```

### Option 2: Fedora/RHEL/CentOS

```bash
# Install development tools
sudo dnf groupinstall "Development Tools"

# Install OpenSSL development libraries
sudo dnf install openssl-devel

# Verify installation
g++ --version
```

### Option 3: macOS

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install OpenSSL via Homebrew
brew install openssl

# You may need to set compiler flags for OpenSSL location
export CPPFLAGS="-I/usr/local/opt/openssl/include"
export LDFLAGS="-L/usr/local/opt/openssl/lib"
```

### Option 4: Windows (Native)

#### Using MSYS2/MinGW

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 terminal and run:
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-openssl
```

#### Using Visual Studio

1. Install Visual Studio 2017 or later
2. Install vcpkg package manager:
```powershell
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install openssl:x64-windows
```

3. Compile with Visual Studio:
```powershell
cl /EHsc /std:c++17 /I"vcpkg\installed\x64-windows\include" ^
   shamir_secret_sharing.cpp tls_multiparty.cpp test_tls_multiparty.cpp ^
   /link /LIBPATH:"vcpkg\installed\x64-windows\lib" libssl.lib libcrypto.lib
```

## Building the Project

### Quick Start (Linux/macOS/WSL)

```bash
# Make the build script executable
chmod +x build.sh

# Run the build script
./build.sh
```

### Manual Build

#### Using Makefile (if Make is installed)

```bash
make
```

#### Using g++ directly

```bash
g++ -std=c++17 -Wall -Wextra -O2 -g \
    shamir_secret_sharing.cpp \
    tls_multiparty.cpp \
    test_tls_multiparty.cpp \
    -o test_tls_multiparty \
    -lssl -lcrypto
```

#### Using clang++

```bash
clang++ -std=c++17 -Wall -Wextra -O2 -g \
    shamir_secret_sharing.cpp \
    tls_multiparty.cpp \
    test_tls_multiparty.cpp \
    -o test_tls_multiparty \
    -lssl -lcrypto
```

## Running the Tests

After successful compilation:

```bash
./test_tls_multiparty
```

Expected output:
```
╔════════════════════════════════════════════════════════╗
║  Multi-Party Authorization in TLS - Implementation    ║
║  Approach 1: Shamir's Secret Sharing                  ║
╚════════════════════════════════════════════════════════╝

========================================
TEST 1: Shamir's Secret Sharing
========================================
...
✓ All tests passed
```

## Troubleshooting

### Error: "openssl/ssl.h: No such file or directory"

**Solution**: Install OpenSSL development libraries
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# Fedora/RHEL
sudo dnf install openssl-devel

# macOS
brew install openssl
```

### Error: "undefined reference to `HMAC'"

**Solution**: Add `-lssl -lcrypto` linker flags
```bash
g++ ... -lssl -lcrypto
```

### Error: "This file requires compiler and library support for the ISO C++ 2017 standard"

**Solution**: Ensure you're using `-std=c++17` flag and a compatible compiler
```bash
g++ --version  # Should be 7.0 or higher
```

### Error: "__uint128_t is not defined" (on some platforms)

**Solution**: The code uses 128-bit arithmetic for modular multiplication. On platforms without native 128-bit support, you can:

1. Replace `__uint128_t` with a library like GMP:
```cpp
#include <gmp.h>
// Use mpz_t for arbitrary precision arithmetic
```

2. Or use a safer multiplication with overflow checking:
```cpp
BigInt mod_mul(BigInt a, BigInt b) const {
    a = a % prime_;
    b = b % prime_;
    BigInt result = 0;
    while (b > 0) {
        if (b & 1) result = (result + a) % prime_;
        a = (a << 1) % prime_;
        b >>= 1;
    }
    return result;
}
```

## WSL-Specific Setup

If you're using Windows Subsystem for Linux (WSL):

```bash
# Update WSL
wsl --update

# Inside WSL, update packages
sudo apt-get update
sudo apt-get upgrade

# Install compilers and libraries
sudo apt-get install build-essential libssl-dev

# Navigate to your Windows directory
cd /mnt/c/Users/YOUR_USERNAME/Documents/WNS

# Build
./build.sh

# Run
./test_tls_multiparty
```

## Docker Option

For a clean, isolated environment:

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY *.cpp *.hpp ./

RUN g++ -std=c++17 -O2 \
    shamir_secret_sharing.cpp \
    tls_multiparty.cpp \
    test_tls_multiparty.cpp \
    -o test_tls_multiparty \
    -lssl -lcrypto

CMD ["./test_tls_multiparty"]
```

Build and run:
```bash
docker build -t tls-multiparty .
docker run tls-multiparty
```

## File Structure

After setup, your directory should contain:

```
├── shamir_secret_sharing.hpp    # Header for secret sharing
├── shamir_secret_sharing.cpp    # Implementation of Shamir's scheme
├── tls_multiparty.hpp           # Header for TLS multi-party
├── tls_multiparty.cpp           # TLS implementation
├── test_tls_multiparty.cpp      # Test suite
├── Makefile                     # Build configuration
├── build.sh                     # Build script
├── TLS_MULTIPARTY_README.md     # Documentation
├── SETUP.md                     # This file
└── test_tls_multiparty          # Compiled executable (after build)
```

## Next Steps

1. **Build the project**: `./build.sh` or `make`
2. **Run tests**: `./test_tls_multiparty`
3. **Review code**: Start with `test_tls_multiparty.cpp` to see usage examples
4. **Modify parameters**: Change threshold/num_parties in test cases
5. **Extend implementation**: Add additional features or integrate with existing TLS libraries

## Support

For issues specific to:
- **Compilation errors**: Check compiler version and OpenSSL installation
- **Runtime errors**: Ensure all libraries are correctly linked
- **Mathematical errors**: Verify the prime modulus is large enough for your use case

## Production Deployment

**⚠ WARNING**: This is a research/educational implementation. For production use:

1. Replace simplified encryption with proper RSA-OAEP
2. Use arbitrary precision arithmetic (GMP, NTL)
3. Implement distributed key generation (DKG)
4. Add secure communication channels between parties
5. Implement proper error handling and logging
6. Use hardware security modules (HSMs) for key storage
7. Conduct security audit and penetration testing
