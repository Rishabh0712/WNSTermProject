# Multi-Party TLS Handshake Flow

## Overview

In traditional TLS, the server holds the complete private key and can unilaterally decrypt client messages. In **Multi-Party TLS**, the server's private key is split among multiple parties using Shamir's Secret Sharing, requiring collaboration of a threshold number of parties to decrypt.

## Complete Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                    SETUP PHASE (Done Once)                          │
└─────────────────────────────────────────────────────────────────────┘

Server                          Parties (1, 2, 3, 4, 5)
  │
  │  1. Generate RSA Key Pair
  │     - Public key: (n, e)
  │     - Private key: d (2048 bits)
  │
  │  2. Split Private Key (Shamir's Secret Sharing)
  │     - Threshold: t=3, Total parties: n=5
  │     - Split d into 34 chunks (61 bits each)
  │     - Each chunk → 5 shares (one per party)
  │
  ├───────────► Party 1: receives 34 shares
  ├───────────► Party 2: receives 34 shares
  ├───────────► Party 3: receives 34 shares
  ├───────────► Party 4: receives 34 shares
  └───────────► Party 5: receives 34 shares
  
  │  3. Destroy Original Private Key
  │     - Only shares remain distributed
  │     - No single entity has full key


┌─────────────────────────────────────────────────────────────────────┐
│              HANDSHAKE PHASE (Every TLS Connection)                 │
└─────────────────────────────────────────────────────────────────────┘

Client                    Server                    Parties (3 needed)
  │                         │
  │  ──ClientHello──────►  │
  │  (TLS version, ciphers) │
  │                         │
  │  ◄──ServerHello────── │
  │  (Certificate with     │
  │   public key: n, e)    │
  │                         │
  │  4. Generate Pre-Master Secret (PMS)
  │     PMS = [0x03][0x03][46 random bytes]
  │                         │
  │  5. Encrypt PMS with Server's Public Key
  │     encrypted_PMS = RSA_Encrypt(PMS, public_key)
  │                         │
  │  ──encrypted_PMS────►  │
  │                         │
  │                         │  6. Request Collaborative Decryption
  │                         ├─────────► Party 1: provide shares
  │                         ├─────────► Party 3: provide shares  
  │                         └─────────► Party 5: provide shares
  │                         │
  │                         │  7. Reconstruct Private Key
  │                         │     For each of 34 chunks:
  │                         │       - Gather 3 shares (from parties 1,3,5)
  │                         │       - Use Lagrange interpolation
  │                         │       - Reconstruct chunk value
  │                         │     Reassemble all chunks → d (private key)
  │                         │
  │                         │  8. Decrypt Pre-Master Secret
  │                         │     PMS = RSA_Decrypt(encrypted_PMS, d)
  │                         │
  │                         │  9. IMMEDIATELY DESTROY Private Key
  │                         │     Securely erase d from memory
  │                         │     (Key existed only during decryption)
  │                         │
  │                         │  ✓ PMS obtained
  │  ✓ PMS known            │
  │                         │
  │  10. Both sides derive Master Secret
  │     master_secret = PRF(PMS, "master secret", 
  │                         client_random + server_random)
  │                         │
  │  11. Derive Session Keys
  │     key_block = PRF(master_secret, "key expansion",
  │                    server_random + client_random)
  │                         │
  │     Extract from key_block:
  │     - client_write_key
  │     - server_write_key  
  │     - client_write_IV
  │     - server_write_IV
  │                         │
  │  ◄──Finished (encrypted)── │
  │  ──Finished (encrypted)──► │
  │                         │
  │  ═══════════════════════════════════
  │     SECURE COMMUNICATION
  │  ═══════════════════════════════════
  │                         │
  │  ◄──Application Data──► │
  │  (Encrypted with        │
  │   session keys)         │
```

## Detailed Step-by-Step Flow

### Setup Phase

#### Step 1: Server Key Generation
```cpp
RSA* rsa = RSA_new();
BIGNUM* e = BN_new();
BN_set_word(e, 65537);  // Standard public exponent
RSA_generate_key_ex(rsa, 2048, e, nullptr);

// Key components:
// n: 2048-bit modulus
// e: 65537 (public exponent)
// d: ~2046-bit private exponent (kept secret)
```

**Result**: Server has complete RSA key pair
- Public key (n, e) → shared openly
- Private key (d) → to be distributed

#### Step 2: Private Key Distribution (Shamir's Secret Sharing)

```cpp
// Configuration
threshold t = 3  // Minimum parties needed
total n = 5      // Total parties
prime p = 2^61 - 1  // Finite field prime

// Split process:
1. Break d into chunks of 61 bits each
   - 2046-bit key → 34 chunks
   
2. For each chunk:
   a. Create random polynomial of degree (t-1):
      f(x) = a0 + a1*x + a2*x^2
      where a0 = chunk_value
      
   b. Generate n shares by evaluating at x=1,2,...,n:
      share_i = (i, f(i) mod p)
      
3. Distribute shares:
   - Party 1 gets: {chunk0_share1, chunk1_share1, ..., chunk33_share1}
   - Party 2 gets: {chunk0_share2, chunk1_share2, ..., chunk33_share2}
   - ...
   - Party 5 gets: {chunk0_share5, chunk1_share5, ..., chunk33_share5}
```

**Security Properties**:
- Any 3 parties can reconstruct d
- Any 2 or fewer parties learn NOTHING about d
- This is information-theoretically secure

#### Step 3: Destroy Original Key
```cpp
BN_clear_free(d);  // Securely erase from memory
// Now only shares exist, distributed among parties
```

### Handshake Phase

#### Step 4: Client Generates Pre-Master Secret
```cpp
uint8_t pms[48];
pms[0] = 0x03;  // TLS 1.2 major version
pms[1] = 0x03;  // TLS 1.2 minor version
RAND_bytes(pms + 2, 46);  // 46 random bytes
```

#### Step 5: Client Encrypts PMS
```cpp
uint8_t encrypted_pms[256];  // RSA-2048 → 256 bytes
RSA_public_encrypt(
    48,                    // PMS length
    pms,                   // Plaintext
    encrypted_pms,         // Ciphertext
    server_public_key,     // Server's public key
    RSA_PKCS1_OAEP_PADDING // OAEP padding for security
);
```

**Sent to server**: encrypted_pms (256 bytes)

#### Step 6: Server Requests Collaboration
Server contacts 3 parties (e.g., parties 1, 3, and 5) and requests their shares.

Each party provides all 34 of their shares (one for each chunk).

#### Step 7: Reconstruct Private Key

```cpp
// For each chunk (0 to 33):
for (size_t chunk = 0; chunk < 34; chunk++) {
    // Gather 3 shares
    Share s1 = party1.shares[chunk];  // (x=1, y=f(1))
    Share s3 = party3.shares[chunk];  // (x=3, y=f(3))
    Share s5 = party5.shares[chunk];  // (x=5, y=f(5))
    
    // Lagrange interpolation to find f(0) = original chunk value
    chunk_value = lagrange_interpolate({s1, s3, s5}, x=0);
    
    // Accumulate into full private key
    d = (d << 61) | chunk_value;
}
```

**Lagrange Interpolation Formula**:
```
f(0) = Σ(i=1 to t) yi * Π(j=1 to t, j≠i) (0 - xj) / (xi - xj)
```

For our case (reconstructing from parties 1, 3, 5):
```
f(0) = y1 * (0-3)*(0-5)/((1-3)*(1-5)) +
       y3 * (0-1)*(0-5)/((3-1)*(3-5)) +
       y5 * (0-1)*(0-3)/((5-1)*(5-3))
     
     = y1 * 15/8 + y3 * (-5)/(-4) + y5 * 3/8  (all mod p)
```

#### Step 8: Decrypt Pre-Master Secret
```cpp
uint8_t decrypted_pms[48];
RSA_private_decrypt(
    256,                   // Ciphertext length
    encrypted_pms,         // Ciphertext
    decrypted_pms,         // Plaintext (PMS)
    reconstructed_rsa,     // RSA with reconstructed d
    RSA_PKCS1_OAEP_PADDING
);
```

#### Step 9: Destroy Private Key
```cpp
BN_clear_free(d);  // Immediately erase reconstructed key
RSA_free(temp_rsa);

// Private key existed only for ~1ms during decryption
// Now destroyed and cannot be recovered without parties
```

#### Step 10-11: Derive Session Keys

Both client and server now have the Pre-Master Secret and can independently derive the same session keys:

```cpp
// Step 10: Derive Master Secret (48 bytes)
master_secret = PRF(pre_master_secret,
                   "master secret",
                   client_random + server_random)
                   
// Step 11: Derive Key Block
key_block = PRF(master_secret,
               "key expansion",
               server_random + client_random)
               
// Extract keys from key_block:
client_write_MAC_key    = key_block[0:20]     // 20 bytes (SHA-1)
server_write_MAC_key    = key_block[20:40]
client_write_key        = key_block[40:56]    // 16 bytes (AES-128)
server_write_key        = key_block[56:72]
client_write_IV         = key_block[72:88]    // 16 bytes
server_write_IV         = key_block[88:104]
```

**TLS PRF (Pseudo-Random Function)**:
```
PRF(secret, label, seed) = P_SHA256(secret, label + seed)

P_SHA256(secret, seed) = HMAC_SHA256(secret, A(1) + seed) +
                        HMAC_SHA256(secret, A(2) + seed) +
                        ...
where:
A(0) = seed
A(i) = HMAC_SHA256(secret, A(i-1))
```

### Secure Communication Phase

Now both sides have symmetric session keys and can encrypt/decrypt application data:

```cpp
// Client → Server
encrypted_data = AES_encrypt(plaintext, client_write_key, client_write_IV)

// Server → Client  
encrypted_data = AES_encrypt(plaintext, server_write_key, server_write_IV)
```

## Security Analysis

### Threat Model

**Traditional TLS Vulnerabilities**:
1. **Single Point of Failure**: If server is compromised, attacker gets private key
2. **Insider Threat**: Single administrator has full access
3. **No Accountability**: Can't determine who performed decryption

**Multi-Party TLS Advantages**:
1. **Distributed Trust**: Need t parties to decrypt (separation of duties)
2. **No Single Point of Compromise**: Stealing from <t parties reveals nothing
3. **Accountability**: Know which parties participated in decryption
4. **Temporary Keys**: Private key exists only during decryption

### Security Properties

1. **Threshold Security**: 
   - Need exactly t parties (e.g., 3 of 5)
   - Any t-1 or fewer parties learn absolutely nothing
   
2. **Perfect Secrecy** (Information-Theoretic):
   - Based on polynomial interpolation
   - Not dependent on computational hardness
   - Secure even against quantum computers

3. **Forward Secrecy**:
   - Each session uses fresh PMS
   - Compromising one session doesn't affect others

4. **Post-Decryption Security**:
   - Private key immediately destroyed after use
   - Even if server compromised after handshake, past sessions secure

### Attack Scenarios

| Attack | Traditional TLS | Multi-Party TLS |
|--------|----------------|-----------------|
| Steal server key | ✗ Full compromise | ✓ Need t parties |
| Insider threat (1 admin) | ✗ Full access | ✓ Need t-1 more |
| Server hack during handshake | ✗ Key stolen | ✓ Key ephemeral |
| Coerce <t parties | N/A | ✓ Still secure |
| Coerce ≥t parties | N/A | ✗ Can decrypt |

## Performance Overhead

Typical overhead for 2048-bit RSA with 3-of-5 threshold:

| Operation | Time | Impact |
|-----------|------|--------|
| Key generation | ~150ms | One-time setup |
| Key splitting (34 chunks) | ~5ms | One-time setup |
| Share distribution | Network latency | One-time setup |
| Key reconstruction | ~2-3ms | Per handshake |
| RSA decryption | ~5ms | Same as traditional |
| Key destruction | ~1ms | Per handshake |
| **Total overhead** | **~8-10ms** | **Per handshake** |

For comparison:
- Traditional TLS handshake: ~50-100ms
- Multi-party overhead: ~10% increase

## Implementation Code

See files:
- `multiparty_tls_simple.cpp` - Complete working demonstration
- `shamir_secret_sharing.cpp` - Shamir's Secret Sharing implementation
- `test_openssl_rsa.cpp` - Full RSA key reconstruction with 34 chunks

## Running the Demo

```bash
# Compile
g++ -std=c++17 multiparty_tls_simple.cpp shamir_secret_sharing.cpp \
    -o multiparty_tls_simple -lssl -lcrypto

# Run
./multiparty_tls_simple
```

## Use Cases

1. **Enterprise PKI**: Root CA private key distributed among board members
2. **Financial Services**: Transaction signing requires multiple approvals
3. **Healthcare**: Patient data decryption requires medical staff + admin
4. **Government**: Classified data access requires multi-party authorization
5. **Cryptocurrency**: Wallet private keys split among custodians

## Future Enhancements

1. **Distributed Key Generation (DKG)**: Generate key shares without ever creating full key
2. **Verifiable Secret Sharing (VSS)**: Parties can verify share validity
3. **Proactive Secret Sharing**: Periodically refresh shares without changing key
4. **Threshold Signatures**: Sign without reconstructing private key
5. **Network Protocol**: Automated party communication and coordination

## References

1. Shamir, A. (1979). "How to share a secret". Communications of the ACM.
2. RFC 5246: The Transport Layer Security (TLS) Protocol Version 1.2
3. RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3
4. Gennaro, R. et al. (1996). "Robust Threshold DSS Signatures"
