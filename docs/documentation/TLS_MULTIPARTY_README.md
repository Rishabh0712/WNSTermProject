# Multi-Party Authorization in TLS - C++ Implementation

This directory contains a complete C++ implementation of **Approach 1** from the proposal: **Secret Sharing for Key Reconstruction** using Shamir's Secret Sharing scheme.

## Overview

The implementation demonstrates how multiple parties can collaboratively decrypt a TLS Pre-Master Secret without any single party having access to the complete private key, except temporarily during reconstruction.

## Files

### Core Implementation
- **`shamir_secret_sharing.hpp/cpp`**: Implements Shamir's (t,n)-threshold secret sharing scheme
  - Polynomial generation with random coefficients
  - Share generation and distribution
  - Lagrange interpolation for secret reconstruction
  - Modular arithmetic operations in finite fields

- **`tls_multiparty.hpp/cpp`**: Multi-party TLS implementation
  - Key generation and distribution
  - Collaborative decryption of Pre-Master Secret
  - TLS 1.2 PRF (Pseudo-Random Function)
  - Master secret and session key derivation
  - Secure memory erasure

### Testing
- **`test_tls_multiparty.cpp`**: Comprehensive test suite
  - Basic secret sharing tests
  - Complete TLS handshake simulation
  - Different party combination scenarios

### Build System
- **`Makefile`**: Build configuration for compilation

## Key Features

### 1. Shamir's Secret Sharing
```cpp
ShamirSecretSharing sss(threshold, num_shares, prime);
auto shares = sss.split(secret);              // Split secret
auto reconstructed = sss.reconstruct(shares);  // Reconstruct from t+ shares
```

### 2. Multi-Party TLS Handshake
```cpp
TLSMultiParty tls(threshold, num_parties);

// Phase 1: Generate and distribute keys
auto [public_key, shares] = tls.generateAndDistributeKeys();

// Phase 2: Collaborative decryption
auto decrypted_pms = tls.collaborativeDecryption(
    encrypted_pms, collaborating_shares, party_ids);

// Phase 3: Standard TLS key derivation
auto master_secret = tls.deriveMasterSecret(pms, client_random, server_random);
auto key_block = tls.deriveKeyBlock(master_secret, client_random, server_random, 128);
```

## Building

### Prerequisites
- C++17 compatible compiler (g++ 7.0+, clang++ 5.0+)
- OpenSSL development libraries

#### Install OpenSSL on Ubuntu/WSL:
```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

#### Install OpenSSL on macOS:
```bash
brew install openssl
```

### Compilation

Using Make:
```bash
make
```

Manual compilation:
```bash
g++ -std=c++17 -Wall -O2 -o test_tls_multiparty \
    shamir_secret_sharing.cpp \
    tls_multiparty.cpp \
    test_tls_multiparty.cpp \
    -lssl -lcrypto
```

## Running

Execute the test suite:
```bash
make run
```

Or directly:
```bash
./test_tls_multiparty
```

## Test Cases

### Test 1: Shamir's Secret Sharing
- Splits secret into n shares
- Reconstructs with exactly t shares (minimum threshold)
- Reconstructs with different combinations of t shares
- Reconstructs with more than t shares
- Correctly rejects insufficient shares (< t)

### Test 2: Multi-Party TLS Handshake
- Key generation and distribution to n parties
- Client encrypts Pre-Master Secret
- t parties collaborate to decrypt
- Derives master secret using TLS 1.2 PRF
- Derives session keys (key block)

### Test 3: Different Party Combinations
- Verifies any subset of t parties can decrypt
- Tests multiple combinations: {1,2,3}, {1,3,5}, {2,4,5}, {3,4,5}

## Example Output

```
========================================
TEST 1: Shamir's Secret Sharing
========================================

Configuration:
  Threshold (t): 3
  Total shares (n): 5
  Prime modulus: 18446744073709551557
  Original secret: 12345678901234

Generated shares:
  Share 1: 7234567890123
  Share 2: 9876543210987
  Share 3: 4567891234567
  Share 4: 8901234567890
  Share 5: 3456789012345

--- Test 1a: Reconstruct with 3 shares ---
Reconstructed secret: 12345678901234
✓ Success! Secret correctly reconstructed.

========================================
TEST 2: Multi-Party TLS Handshake
========================================

=== PHASE 1: KEY GENERATION AND DISTRIBUTION ===
[Key Generation] Original private key: 12345...
[Key Distribution] Generated 5 shares

=== PHASE 2: TLS HANDSHAKE ===
[Client] Encrypting Pre-Master Secret...

[Multi-Party Decryption] Starting collaborative decryption...
  Party 1 contributes share: 7234567890123
  Party 2 contributes share: 9876543210987
  Party 3 contributes share: 4567891234567

[Key Reconstruction] Using Lagrange interpolation...
[Key Reconstruction] Reconstructed private key: 12345...
[SECURITY WARNING] Complete private key exists in memory temporarily!
[Security] Securely erasing reconstructed private key from memory

✓ Decryption successful! PMS matches original.

[TLS] Master Secret: a3b2c1d4...
[TLS] Key Block: e5f6a7b8...

✓ TLS handshake completed successfully!
```

## Security Considerations

### ✓ Implemented
- **Secure Memory Erasure**: Uses `OPENSSL_cleanse()` to zero out sensitive data
- **Threshold Security**: Any t parties needed to reconstruct private key
- **Modular Arithmetic**: Operations in finite field modulo large prime
- **TLS 1.2 PRF**: Standard HMAC-SHA256 based PRF

### ⚠ Limitations (Simplified for Demonstration)
- **Simplified Encryption**: Uses XOR for demo; production should use RSA-OAEP
- **64-bit Arithmetic**: Uses `uint64_t`; production needs arbitrary precision (GMP/NTL)
- **No Network Layer**: Simulates local computation; production needs secure channels
- **No Certificate Validation**: Focuses on key exchange mechanism only
- **Temporary Key Exposure**: Complete private key exists briefly during reconstruction

## Production Considerations

For a production implementation, consider:

1. **Use Proper RSA**: Replace simplified encryption with RSA-2048/4096 with OAEP padding
2. **Arbitrary Precision**: Use GMP, NTL, or Boost.Multiprecision for large numbers
3. **Distributed Key Generation**: Implement DKG to avoid single point of trust
4. **Secure Channels**: Use TLS for communication between parties during reconstruction
5. **Hardware Security**: Use HSMs or secure enclaves for share storage
6. **Audit Logging**: Log all reconstruction attempts with party identities
7. **Time-Limited Reconstruction**: Set strict time limits for key reconstruction
8. **Zero-Knowledge Proofs**: Verify shares without revealing them

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    TLS Client                           │
│  - Generates Pre-Master Secret (48 bytes)              │
│  - Encrypts with server's public key                   │
└─────────────────────┬───────────────────────────────────┘
                      │ Encrypted PMS
                      ▼
┌─────────────────────────────────────────────────────────┐
│             Multi-Party TLS Server                      │
│                                                         │
│  Party 1: Share s₁  Party 2: Share s₂  Party 3: Share s₃│
│       │                  │                  │           │
│       └──────────────────┼──────────────────┘           │
│                          │                              │
│               Lagrange Interpolation                    │
│                          │                              │
│                Reconstructed Private Key                │
│                          │                              │
│                   Decrypt PMS                           │
│                          │                              │
│                  [SECURE ERASE]                         │
│                          │                              │
│              Derive Master Secret (PRF)                 │
│                          │                              │
│             Derive Session Keys (PRF)                   │
└─────────────────────────────────────────────────────────┘
```

## References

- Shamir, A. (1979). "How to share a secret". Communications of the ACM.
- RFC 5246: The Transport Layer Security (TLS) Protocol Version 1.2
- RFC 2104: HMAC: Keyed-Hashing for Message Authentication

## License

This implementation is for educational and research purposes.
