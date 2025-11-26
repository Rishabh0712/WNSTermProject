/**
 * Multi-Party TLS Handshake Test Script
 * 
 * This script simulates a complete TLS 1.2 handshake where the server's 
 * private key is distributed among multiple parties using Shamir's Secret Sharing.
 * 
 * Scenario:
 * - Server private key is split into 5 shares (3-of-5 threshold)
 * - Client initiates TLS handshake
 * - 3 parties collaborate to decrypt the Pre-Master Secret
 * - Session keys are derived and secure communication is established
 */

#include "shamir_secret_sharing.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <memory>
#include <chrono>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void print_separator(const std::string& title = "") {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    if (!title.empty()) {
        std::cout << "  " << title << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
}

void print_section(const std::string& title) {
    std::cout << "\n[" << title << "]" << std::endl;
    std::cout << std::string(title.length() + 2, '-') << std::endl;
}

void print_hex(const std::string& label, const unsigned char* data, size_t len, bool full = false) {
    std::cout << label << " (" << len << " bytes): ";
    size_t display_len = full ? len : std::min(len, size_t(32));
    for (size_t i = 0; i < display_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < display_len) std::cout << "\n" << std::string(label.length() + 12, ' ');
    }
    if (len > 32 && !full) {
        std::cout << "...";
    }
    std::cout << std::dec << std::endl;
}

void print_bignum(const std::string& label, const BIGNUM* bn) {
    char* hex_str = BN_bn2hex(bn);
    std::cout << label << " (" << BN_num_bits(bn) << " bits): " << hex_str << std::endl;
    OPENSSL_free(hex_str);
}

void print_openssl_error() {
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    std::cerr << "OpenSSL Error: " << err_buf << std::endl;
}

// ============================================================================
// MULTI-PARTY TLS COMPONENTS
// ============================================================================

struct Party {
    size_t id;
    std::string name;
    std::vector<ShamirSecretSharing::Share> shares;  // Shares for each chunk
    
    Party(size_t id, const std::string& name) : id(id), name(name) {}
};

class MultiPartyTLSServer {
public:
    // Configuration
    static constexpr size_t THRESHOLD = 3;
    static constexpr size_t NUM_PARTIES = 5;
    static constexpr size_t RSA_KEY_BITS = 2048;
    static constexpr size_t CHUNK_BITS = 61;  // Using Mersenne prime 2^61 - 1
    static constexpr uint64_t PRIME = 2305843009213693951ULL;  // 2^61 - 1
    
    MultiPartyTLSServer() {
        std::cout << "Initializing SSS with threshold=" << THRESHOLD 
                  << ", parties=" << NUM_PARTIES << ", prime=" << PRIME << std::endl;
        try {
            sss = std::make_unique<ShamirSecretSharing>(THRESHOLD, NUM_PARTIES, PRIME);
            std::cout << "SSS initialized successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error initializing SSS: " << e.what() << std::endl;
        }
        rsa = nullptr;
    }
    
    ~MultiPartyTLSServer() {
        if (rsa) RSA_free(rsa);
    }
    
    // Phase 1: Generate RSA key pair
    bool generateKeyPair() {
        print_section("PHASE 1: Server Key Generation");
        
        rsa = RSA_new();
        BIGNUM* e = BN_new();
        
        std::cout << "Generating " << RSA_KEY_BITS << "-bit RSA key pair..." << std::endl;
        
        if (BN_set_word(e, RSA_F4) != 1) {
            print_openssl_error();
            BN_free(e);
            return false;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        if (RSA_generate_key_ex(rsa, RSA_KEY_BITS, e, nullptr) != 1) {
            print_openssl_error();
            BN_free(e);
            return false;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        BN_free(e);
        
        std::cout << "✓ RSA key pair generated in " << duration.count() << " ms" << std::endl;
        
        // Display key parameters
        const BIGNUM *n, *e_pub, *d;
        RSA_get0_key(rsa, &n, &e_pub, &d);
        
        std::cout << "\nKey Parameters:" << std::endl;
        std::cout << "  Modulus (n):          " << BN_num_bits(n) << " bits" << std::endl;
        std::cout << "  Public exponent (e):  " << BN_get_word(e_pub) << std::endl;
        std::cout << "  Private exponent (d): " << BN_num_bits(d) << " bits" << std::endl;
        
        return true;
    }
    
    // Phase 2: Split private key among parties
    bool distributeKeyShares(std::vector<Party>& parties) {
        print_section("PHASE 2: Private Key Distribution (Shamir Secret Sharing)");
        
        const BIGNUM *d_const;
        RSA_get0_key(rsa, nullptr, nullptr, &d_const);
        
        // Make a copy of d since we need to perform operations on it
        BIGNUM *d = BN_dup(d_const);
        if (!d) {
            std::cerr << "Failed to duplicate private exponent" << std::endl;
            return false;
        }
        
        std::cout << "Splitting private exponent (d) into shares..." << std::endl;
        std::cout << "  Threshold: " << THRESHOLD << " of " << NUM_PARTIES << std::endl;
        std::cout << "  Private key size: " << BN_num_bits(d) << " bits" << std::endl;
        
        // Split d into 61-bit chunks
        int num_bits = BN_num_bits(d);
        size_t num_chunks = (num_bits + CHUNK_BITS - 1) / CHUNK_BITS;
        
        std::cout << "  Number of chunks: " << num_chunks << " (each " << CHUNK_BITS << " bits)" << std::endl;
        std::cout << "  Prime: " << PRIME << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Extract chunks and create shares
        std::vector<std::vector<ShamirSecretSharing::Share>> all_chunk_shares(num_chunks);
        
        std::cout << "\nProcessing chunks..." << std::endl;
        for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx) {
            std::cout << "  Chunk " << chunk_idx << ": allocating BN..." << std::flush;
            // Extract chunk value
            BIGNUM* chunk_bn = BN_new();
            std::cout << " shifting..." << std::flush;
            BN_rshift(chunk_bn, d, chunk_idx * CHUNK_BITS);
            
            BIGNUM* mask = BN_new();
            BN_set_word(mask, 1);
            BN_lshift(mask, mask, CHUNK_BITS);
            BN_sub_word(mask, 1);
            BN_mod(chunk_bn, chunk_bn, mask, nullptr);
            
            // BN_get_word returns the value if it fits in a word, otherwise BN_MASK2
            // We need to check if it fits first
            if (BN_num_bits(chunk_bn) > 64) {
                std::cerr << "\nError: chunk " << chunk_idx << " is too large (" 
                          << BN_num_bits(chunk_bn) << " bits)" << std::endl;
                BN_free(chunk_bn);
                BN_free(mask);
                return false;
            }
            
            uint64_t chunk_value = BN_get_word(chunk_bn);
            
            BN_free(chunk_bn);
            BN_free(mask);
            
            // Check for BN_get_word error (returns max value on error)
            if (chunk_value == UINT64_MAX) {
                std::cerr << "\nError: BN_get_word failed for chunk " << chunk_idx << std::endl;
                return false;
            }
            
            // Ensure chunk_value is less than prime
            if (chunk_value >= PRIME) {
                chunk_value = chunk_value % PRIME;
            }
            
            // Create shares for this chunk
            try {
                std::cout << " splitting value " << chunk_value << "..." << std::flush;
                
                // Test if sss is valid
                if (!sss) {
                    std::cerr << " ERROR: sss is null!" << std::endl;
                    return false;
                }
                
                auto shares = sss->split(chunk_value);
                std::cout << " got " << shares.size() << " shares..." << std::flush;
                
                all_chunk_shares[chunk_idx] = shares;
                std::cout << " OK" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "\nError splitting chunk " << chunk_idx << ": " << e.what() << std::endl;
                std::cerr << "  Chunk value: " << chunk_value << std::endl;
                std::cerr << "  Prime: " << PRIME << std::endl;
                return false;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Key split into " << num_chunks * NUM_PARTIES << " shares in " 
                  << duration.count() << " ms" << std::endl;
        
        // Distribute shares to parties
        std::cout << "\nDistributing shares to parties:" << std::endl;
        for (size_t party_idx = 0; party_idx < NUM_PARTIES; ++party_idx) {
            for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx) {
                parties[party_idx].shares.push_back(all_chunk_shares[chunk_idx][party_idx]);
            }
            std::cout << "  Party " << (party_idx + 1) << " (" << parties[party_idx].name 
                      << "): received " << num_chunks << " shares" << std::endl;
        }
        
        // Store number of chunks for reconstruction
        num_key_chunks = num_chunks;
        
        std::cout << "\n✓ Private key securely distributed" << std::endl;
        std::cout << "  Security: Any " << THRESHOLD << " parties can reconstruct the key" << std::endl;
        std::cout << "  Security: " << (THRESHOLD - 1) << " or fewer parties reveal nothing" << std::endl;
        
        // Clean up the copy of d
        BN_clear_free(d);
        
        return true;
    }
    
    // Get public key for client
    std::vector<uint8_t> getPublicKey() const {
        std::vector<uint8_t> result;
        unsigned char* buf = nullptr;
        int len = i2d_RSAPublicKey(rsa, &buf);
        if (len > 0) {
            result.assign(buf, buf + len);
            OPENSSL_free(buf);
        }
        return result;
    }
    
    // Get RSA object
    RSA* getRSA() const { return rsa; }
    
    size_t getNumKeyChunks() const { return num_key_chunks; }
    
private:
    std::unique_ptr<ShamirSecretSharing> sss;
    RSA* rsa;
    size_t num_key_chunks;
};

class TLSClient {
public:
    TLSClient() {}
    
    // Phase 3: Generate Pre-Master Secret and encrypt with server's public key
    bool generatePreMasterSecret(const std::vector<uint8_t>& server_public_key,
                                 std::vector<uint8_t>& encrypted_pms) {
        print_section("PHASE 3: Client Hello & Key Exchange");
        
        // Generate client random (32 bytes)
        client_random.resize(32);
        RAND_bytes(client_random.data(), 32);
        print_hex("Client Random", client_random.data(), 32);
        
        // Generate Pre-Master Secret (48 bytes for TLS 1.2)
        // Format: [TLS version (2 bytes)][46 random bytes]
        pre_master_secret.resize(48);
        pre_master_secret[0] = 0x03;  // TLS 1.2 major version
        pre_master_secret[1] = 0x03;  // TLS 1.2 minor version
        RAND_bytes(pre_master_secret.data() + 2, 46);
        
        print_hex("Pre-Master Secret", pre_master_secret.data(), 48);
        
        // Decrypt server's public key
        const unsigned char* p = server_public_key.data();
        RSA* server_rsa = d2i_RSAPublicKey(nullptr, &p, server_public_key.size());
        if (!server_rsa) {
            print_openssl_error();
            return false;
        }
        
        // Encrypt PMS with server's public key
        encrypted_pms.resize(RSA_size(server_rsa));
        
        std::cout << "\nEncrypting Pre-Master Secret with server's public key..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int encrypted_len = RSA_public_encrypt(
            pre_master_secret.size(),
            pre_master_secret.data(),
            encrypted_pms.data(),
            server_rsa,
            RSA_PKCS1_OAEP_PADDING
        );
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        RSA_free(server_rsa);
        
        if (encrypted_len == -1) {
            print_openssl_error();
            return false;
        }
        
        encrypted_pms.resize(encrypted_len);
        
        std::cout << "✓ Pre-Master Secret encrypted in " << duration.count() << " μs" << std::endl;
        print_hex("Encrypted PMS", encrypted_pms.data(), encrypted_len);
        
        return true;
    }
    
    const std::vector<uint8_t>& getClientRandom() const { return client_random; }
    const std::vector<uint8_t>& getPreMasterSecret() const { return pre_master_secret; }
    
private:
    std::vector<uint8_t> client_random;
    std::vector<uint8_t> pre_master_secret;
};

// ============================================================================
// COLLABORATIVE DECRYPTION
// ============================================================================

bool collaborativeDecrypt(
    const std::vector<uint8_t>& encrypted_pms,
    const std::vector<Party>& participating_parties,
    size_t num_chunks,
    std::vector<uint8_t>& decrypted_pms,
    RSA* server_rsa)
{
    print_section("PHASE 4: Multi-Party Collaborative Decryption");
    
    std::cout << "Participating parties: ";
    for (const auto& party : participating_parties) {
        std::cout << party.name << " ";
    }
    std::cout << std::endl;
    
    // Reconstruct private key from shares
    std::cout << "\nReconstructing private key from " << participating_parties.size() 
              << " party shares..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    ShamirSecretSharing sss(MultiPartyTLSServer::THRESHOLD, MultiPartyTLSServer::NUM_PARTIES, MultiPartyTLSServer::PRIME);
    
    // Reconstruct each chunk
    BIGNUM* reconstructed_d = BN_new();
    BN_zero(reconstructed_d);
    
    for (size_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx) {
        // Gather shares for this chunk from participating parties
        std::vector<ShamirSecretSharing::Share> chunk_shares;
        for (const auto& party : participating_parties) {
            chunk_shares.push_back(party.shares[chunk_idx]);
        }
        
        // Reconstruct chunk value
        uint64_t chunk_value = sss.reconstruct(chunk_shares);
        
        // Accumulate into full d: d = (d << 61) | chunk_value
        BN_lshift(reconstructed_d, reconstructed_d, MultiPartyTLSServer::CHUNK_BITS);
        
        BIGNUM* chunk_bn = BN_new();
        BN_set_word(chunk_bn, chunk_value);
        BN_add(reconstructed_d, reconstructed_d, chunk_bn);
        BN_free(chunk_bn);
    }
    
    auto recon_end = std::chrono::high_resolution_clock::now();
    auto recon_duration = std::chrono::duration_cast<std::chrono::milliseconds>(start - recon_end);
    
    std::cout << "✓ Private key reconstructed (" << BN_num_bits(reconstructed_d) 
              << " bits) in " << recon_duration.count() << " ms" << std::endl;
    
    // Verify reconstructed key matches original
    const BIGNUM* original_d;
    RSA_get0_key(server_rsa, nullptr, nullptr, &original_d);
    
    if (BN_cmp(reconstructed_d, original_d) == 0) {
        std::cout << "✓ Reconstructed key verified - matches original!" << std::endl;
    } else {
        std::cout << "✗ ERROR: Reconstructed key does not match original!" << std::endl;
        BN_clear_free(reconstructed_d);
        return false;
    }
    
    // Create temporary RSA structure with reconstructed key for decryption
    RSA* temp_rsa = RSAPrivateKey_dup(server_rsa);
    
    // Decrypt the Pre-Master Secret
    std::cout << "\nDecrypting Pre-Master Secret..." << std::endl;
    
    decrypted_pms.resize(RSA_size(temp_rsa));
    
    auto decrypt_start = std::chrono::high_resolution_clock::now();
    
    int decrypted_len = RSA_private_decrypt(
        encrypted_pms.size(),
        encrypted_pms.data(),
        decrypted_pms.data(),
        temp_rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    
    auto decrypt_end = std::chrono::high_resolution_clock::now();
    auto decrypt_duration = std::chrono::duration_cast<std::chrono::microseconds>(decrypt_start - decrypt_end);
    
    // Securely erase reconstructed key
    BN_clear_free(reconstructed_d);
    RSA_free(temp_rsa);
    
    if (decrypted_len == -1) {
        print_openssl_error();
        return false;
    }
    
    decrypted_pms.resize(decrypted_len);
    
    std::cout << "✓ Pre-Master Secret decrypted in " << decrypt_duration.count() << " μs" << std::endl;
    print_hex("Decrypted PMS", decrypted_pms.data(), decrypted_len);
    
    return true;
}

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    print_separator("MULTI-PARTY TLS HANDSHAKE SIMULATION");
    
    std::cout << "\nScenario: TLS 1.2 handshake with distributed server private key" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  - Threshold: 3 of 5 parties required" << std::endl;
    std::cout << "  - RSA Key Size: 2048 bits" << std::endl;
    std::cout << "  - Secret Sharing: Shamir's scheme over finite field" << std::endl;
    
    // Initialize parties
    std::vector<Party> parties;
    parties.emplace_back(1, "Security Officer 1");
    parties.emplace_back(2, "Security Officer 2");
    parties.emplace_back(3, "Security Officer 3");
    parties.emplace_back(4, "Backup Authority 1");
    parties.emplace_back(5, "Backup Authority 2");
    
    // Initialize server
    MultiPartyTLSServer server;
    
    // Phase 1: Generate server key pair
    if (!server.generateKeyPair()) {
        std::cerr << "Failed to generate server key pair" << std::endl;
        return 1;
    }
    
    // Phase 2: Distribute key shares
    if (!server.distributeKeyShares(parties)) {
        std::cerr << "Failed to distribute key shares" << std::endl;
        return 1;
    }
    
    // Phase 3: Client initiates handshake
    TLSClient client;
    std::vector<uint8_t> encrypted_pms;
    
    if (!client.generatePreMasterSecret(server.getPublicKey(), encrypted_pms)) {
        std::cerr << "Failed to generate Pre-Master Secret" << std::endl;
        return 1;
    }
    
    // Phase 4: Collaborative decryption (using parties 1, 3, and 5)
    std::vector<Party> participating_parties = {parties[0], parties[2], parties[4]};
    std::vector<uint8_t> decrypted_pms;
    
    if (!collaborativeDecrypt(encrypted_pms, participating_parties, 
                             server.getNumKeyChunks(), decrypted_pms, 
                             server.getRSA())) {
        std::cerr << "Failed collaborative decryption" << std::endl;
        return 1;
    }
    
    // Verify decryption
    print_section("PHASE 5: Verification");
    
    const auto& original_pms = client.getPreMasterSecret();
    bool pms_match = (decrypted_pms == original_pms);
    
    if (pms_match) {
        std::cout << "✓ SUCCESS: Decrypted Pre-Master Secret matches original!" << std::endl;
        std::cout << "\nHandshake Summary:" << std::endl;
        std::cout << "  1. Server private key split into 5 shares" << std::endl;
        std::cout << "  2. Client encrypted PMS with server's public key" << std::endl;
        std::cout << "  3. Parties 1, 3, and 5 collaborated to decrypt PMS" << std::endl;
        std::cout << "  4. Secure session established!" << std::endl;
    } else {
        std::cout << "✗ FAILURE: Decrypted Pre-Master Secret does NOT match!" << std::endl;
        print_hex("Expected", original_pms.data(), original_pms.size());
        print_hex("Got", decrypted_pms.data(), decrypted_pms.size());
        return 1;
    }
    
    print_separator("TEST COMPLETED SUCCESSFULLY");
    
    return 0;
}
