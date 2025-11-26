#include "shamir_secret_sharing.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>

// Helper function to print OpenSSL errors
void print_openssl_error() {
    char err_buf[256];
    ERR_error_string_n(ERR_get_error(), err_buf, sizeof(err_buf));
    std::cerr << "OpenSSL Error: " << err_buf << std::endl;
}

// Helper to print hex
void print_hex(const std::string& label, const unsigned char* data, size_t len) {
    std::cout << label << ": ";
    for (size_t i = 0; i < std::min(len, size_t(16)); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(data[i]);
    }
    if (len > 16) {
        std::cout << "... (" << std::dec << len << " bytes)";
    }
    std::cout << std::dec << std::endl;
}

// Generate RSA key pair
RSA* generate_rsa_keypair(int bits = 2048) {
    std::cout << "Generating " << bits << "-bit RSA key pair..." << std::endl;
    
    RSA* rsa = RSA_new();
    BIGNUM* bne = BN_new();
    
    if (BN_set_word(bne, RSA_F4) != 1) {
        print_openssl_error();
        BN_free(bne);
        RSA_free(rsa);
        return nullptr;
    }
    
    if (RSA_generate_key_ex(rsa, bits, bne, nullptr) != 1) {
        print_openssl_error();
        BN_free(bne);
        RSA_free(rsa);
        return nullptr;
    }
    
    BN_free(bne);
    std::cout << "RSA key pair generated successfully." << std::endl;
    return rsa;
}

// Save RSA keys to files
bool save_rsa_keys(RSA* rsa, const std::string& private_key_file, const std::string& public_key_file) {
    // Save private key
    FILE* priv_file = fopen(private_key_file.c_str(), "wb");
    if (!priv_file) {
        std::cerr << "Cannot open private key file for writing" << std::endl;
        return false;
    }
    
    if (PEM_write_RSAPrivateKey(priv_file, rsa, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        print_openssl_error();
        fclose(priv_file);
        return false;
    }
    fclose(priv_file);
    std::cout << "Private key saved to: " << private_key_file << std::endl;
    
    // Save public key
    FILE* pub_file = fopen(public_key_file.c_str(), "wb");
    if (!pub_file) {
        std::cerr << "Cannot open public key file for writing" << std::endl;
        return false;
    }
    
    if (PEM_write_RSAPublicKey(pub_file, rsa) != 1) {
        print_openssl_error();
        fclose(pub_file);
        return false;
    }
    fclose(pub_file);
    std::cout << "Public key saved to: " << public_key_file << std::endl;
    
    return true;
}

// Extract RSA private exponent and other parameters for sharing
struct RSAComponents {
    BIGNUM* n;  // Modulus
    BIGNUM* e;  // Public exponent
    BIGNUM* d;  // Private exponent
    BIGNUM* p;  // Prime p
    BIGNUM* q;  // Prime q
    BIGNUM* dmp1;  // d mod (p-1)
    BIGNUM* dmq1;  // d mod (q-1)
    BIGNUM* iqmp;  // q^-1 mod p
};

RSAComponents extract_rsa_components(RSA* rsa) {
    std::cout << "Extracting RSA components..." << std::endl;
    
    RSAComponents comp;
    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    const BIGNUM* d = nullptr;
    const BIGNUM* p = nullptr;
    const BIGNUM* q = nullptr;
    const BIGNUM* dmp1 = nullptr;
    const BIGNUM* dmq1 = nullptr;
    const BIGNUM* iqmp = nullptr;
    
    RSA_get0_key(rsa, &n, &e, &d);
    RSA_get0_factors(rsa, &p, &q);
    RSA_get0_crt_params(rsa, &dmp1, &dmq1, &iqmp);
    
    // Duplicate all components
    comp.n = BN_dup(n);
    comp.e = BN_dup(e);
    comp.d = BN_dup(d);
    comp.p = p ? BN_dup(p) : nullptr;
    comp.q = q ? BN_dup(q) : nullptr;
    comp.dmp1 = dmp1 ? BN_dup(dmp1) : nullptr;
    comp.dmq1 = dmq1 ? BN_dup(dmq1) : nullptr;
    comp.iqmp = iqmp ? BN_dup(iqmp) : nullptr;
    
    char* d_str = BN_bn2hex(comp.d);
    std::cout << "Private exponent (d): " << std::string(d_str).substr(0, 32) << "... (" 
              << BN_num_bits(comp.d) << " bits)" << std::endl;
    OPENSSL_free(d_str);
    
    return comp;
}

void free_rsa_components(RSAComponents& comp) {
    if (comp.n) BN_free(comp.n);
    if (comp.e) BN_free(comp.e);
    if (comp.d) BN_free(comp.d);
    if (comp.p) BN_free(comp.p);
    if (comp.q) BN_free(comp.q);
    if (comp.dmp1) BN_free(comp.dmp1);
    if (comp.dmq1) BN_free(comp.dmq1);
    if (comp.iqmp) BN_free(comp.iqmp);
}

// Split BIGNUM into 61-bit chunks (to fit in our prime field)
std::vector<uint64_t> split_bignum_to_chunks(const BIGNUM* bn) {
    std::vector<uint64_t> chunks;
    int num_bits = BN_num_bits(bn);
    int chunk_bits = 61;  // Our Mersenne prime is 2^61 - 1
    int num_chunks = (num_bits + chunk_bits - 1) / chunk_bits;
    
    std::cout << "Splitting " << num_bits << "-bit BIGNUM into " 
              << num_chunks << " chunks of " << chunk_bits << " bits each" << std::endl;
    
    BIGNUM* temp = BN_dup(bn);
    BIGNUM* mask = BN_new();
    BIGNUM* chunk_bn = BN_new();
    
    // Create mask for 61 bits: 2^61 - 1
    BN_set_word(mask, 1);
    BN_lshift(mask, mask, chunk_bits);
    BN_sub_word(mask, 1);
    
    for (int i = 0; i < num_chunks; i++) {
        // Extract chunk: (bn >> (i * chunk_bits)) & mask
        BN_copy(chunk_bn, temp);
        BN_rshift(chunk_bn, chunk_bn, i * chunk_bits);
        BN_mask_bits(chunk_bn, chunk_bits);
        
        uint64_t chunk_value = BN_get_word(chunk_bn);
        chunks.push_back(chunk_value);
        
        std::cout << "  Chunk " << i << ": " << chunk_value 
                  << " (" << BN_num_bits(chunk_bn) << " bits)" << std::endl;
    }
    
    BN_free(temp);
    BN_free(mask);
    BN_free(chunk_bn);
    
    return chunks;
}

// Reconstruct BIGNUM from chunks
BIGNUM* reconstruct_bignum_from_chunks(const std::vector<uint64_t>& chunks) {
    BIGNUM* result = BN_new();
    BN_zero(result);
    
    int chunk_bits = 61;
    
    std::cout << "Reconstructing BIGNUM from " << chunks.size() << " chunks" << std::endl;
    
    for (size_t i = 0; i < chunks.size(); i++) {
        BIGNUM* chunk_bn = BN_new();
        BN_set_word(chunk_bn, chunks[i]);
        BN_lshift(chunk_bn, chunk_bn, i * chunk_bits);
        BN_add(result, result, chunk_bn);
        BN_free(chunk_bn);
    }
    
    std::cout << "Reconstructed BIGNUM: " << BN_num_bits(result) << " bits" << std::endl;
    
    return result;
}

// Reconstruct RSA key from components with reconstructed private exponent
RSA* reconstruct_rsa_from_components(const RSAComponents& comp, const BIGNUM* reconstructed_d) {
    std::cout << "Reconstructing RSA key from components..." << std::endl;
    
    RSA* new_rsa = RSA_new();
    
    // Verify the reconstructed private exponent matches
    if (BN_cmp(reconstructed_d, comp.d) == 0) {
        std::cout << "✓ Reconstructed private exponent matches original!" << std::endl;
    } else {
        std::cout << "✗ Warning: Reconstructed private exponent doesn't match" << std::endl;
    }
    
    // Create new BIGNUMs using reconstructed private exponent
    BIGNUM* new_n = BN_dup(comp.n);
    BIGNUM* new_e = BN_dup(comp.e);
    BIGNUM* new_d = BN_dup(reconstructed_d);  // Use reconstructed private exponent
    
    if (RSA_set0_key(new_rsa, new_n, new_e, new_d) != 1) {
        print_openssl_error();
        BN_free(new_n);
        BN_free(new_e);
        BN_free(new_d);
        RSA_free(new_rsa);
        return nullptr;
    }
    
    // Set CRT parameters if available (improves performance)
    if (comp.p && comp.q && comp.dmp1 && comp.dmq1 && comp.iqmp) {
        BIGNUM* new_p = BN_dup(comp.p);
        BIGNUM* new_q = BN_dup(comp.q);
        BIGNUM* new_dmp1 = BN_dup(comp.dmp1);
        BIGNUM* new_dmq1 = BN_dup(comp.dmq1);
        BIGNUM* new_iqmp = BN_dup(comp.iqmp);
        
        if (RSA_set0_factors(new_rsa, new_p, new_q) != 1) {
            print_openssl_error();
            BN_free(new_p);
            BN_free(new_q);
            BN_free(new_dmp1);
            BN_free(new_dmq1);
            BN_free(new_iqmp);
        } else if (RSA_set0_crt_params(new_rsa, new_dmp1, new_dmq1, new_iqmp) != 1) {
            print_openssl_error();
            BN_free(new_dmp1);
            BN_free(new_dmq1);
            BN_free(new_iqmp);
        }
    }
    
    std::cout << "RSA key reconstructed successfully." << std::endl;
    return new_rsa;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Multi-Party TLS with Real OpenSSL RSA Certificates   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Initialize OpenSSL
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    
    // Step 1: Generate RSA key pair
    std::cout << "=== STEP 1: Generate RSA Key Pair ===" << std::endl;
    RSA* rsa = generate_rsa_keypair(2048);
    if (!rsa) {
        std::cerr << "Failed to generate RSA key pair" << std::endl;
        return 1;
    }
    
    // Save keys to files
    if (!save_rsa_keys(rsa, "rsa_private.pem", "rsa_public.pem")) {
        RSA_free(rsa);
        return 1;
    }
    
    std::cout << "\n=== STEP 2: Split Full Private Exponent using Shamir's Secret Sharing ===" << std::endl;
    
    // Extract RSA components including private exponent (d)
    RSAComponents rsa_comp = extract_rsa_components(rsa);
    
    // Split the full private exponent into 61-bit chunks
    std::cout << "\nSplitting full private exponent into chunks..." << std::endl;
    std::vector<uint64_t> d_chunks = split_bignum_to_chunks(rsa_comp.d);
    
    // Configure Shamir's Secret Sharing
    const size_t threshold = 3;
    const size_t num_parties = 5;
    const uint64_t prime = 2305843009213693951ULL;  // 2^61 - 1
    
    std::cout << "\nShamir's Secret Sharing configuration:" << std::endl;
    std::cout << "  Threshold: " << threshold << std::endl;
    std::cout << "  Total parties: " << num_parties << std::endl;
    std::cout << "  Prime modulus: " << prime << " (2^61 - 1)" << std::endl;
    std::cout << "  Number of chunks: " << d_chunks.size() << std::endl;
    
    // Split each chunk using Shamir's Secret Sharing
    // Each party will receive one share of each chunk
    std::vector<std::vector<ShamirSecretSharing::Share>> all_chunk_shares;
    
    std::cout << "\nSplitting each chunk into " << num_parties << " shares..." << std::endl;
    for (size_t chunk_idx = 0; chunk_idx < d_chunks.size(); chunk_idx++) {
        uint64_t chunk = d_chunks[chunk_idx];
        ShamirSecretSharing sss(threshold, num_parties, prime);
        auto shares = sss.split(chunk);
        all_chunk_shares.push_back(shares);
        
        std::cout << "  Chunk " << chunk_idx << " (value=" << chunk << ") → " 
                  << shares.size() << " shares created" << std::endl;
    }
    
    std::cout << "\nEach party now holds " << d_chunks.size() 
              << " shares (one per chunk):" << std::endl;
    for (size_t party = 0; party < num_parties; party++) {
        std::cout << "  Party " << (party + 1) << ": [";
        for (size_t chunk_idx = 0; chunk_idx < d_chunks.size(); chunk_idx++) {
            if (chunk_idx > 0) std::cout << ", ";
            std::cout << "chunk" << chunk_idx << "_share";
        }
        std::cout << "]" << std::endl;
    }
    
    std::cout << "\n=== STEP 3: Encrypt Pre-Master Secret with Public Key ===" << std::endl;
    
    // Create a 48-byte Pre-Master Secret (TLS 1.2 standard)
    unsigned char pms[48];
    if (RAND_bytes(pms, sizeof(pms)) != 1) {
        print_openssl_error();
        RSA_free(rsa);
        return 1;
    }
    
    print_hex("Pre-Master Secret", pms, sizeof(pms));
    
    // Encrypt PMS with public key
    unsigned char encrypted_pms[RSA_size(rsa)];
    int encrypted_len = RSA_public_encrypt(
        sizeof(pms),
        pms,
        encrypted_pms,
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    
    if (encrypted_len == -1) {
        print_openssl_error();
        RSA_free(rsa);
        return 1;
    }
    
    std::cout << "PMS encrypted with RSA public key (OAEP padding)" << std::endl;
    print_hex("Encrypted PMS", encrypted_pms, encrypted_len);
    
    std::cout << "\n=== STEP 4: Multi-Party Collaborative Decryption ===" << std::endl;
    
    // Simulate: 3 parties collaborate (parties 1, 2, and 3)
    std::cout << "\nParties 1, 2, and 3 collaborate to reconstruct the full private key..." << std::endl;
    const std::vector<size_t> collaborating_parties = {0, 1, 2};  // Party indices
    
    for (size_t party_idx : collaborating_parties) {
        std::cout << "  Party " << (party_idx + 1) << " contributes all their shares" << std::endl;
    }
    
    // Reconstruct each chunk from the collaborating parties' shares
    std::cout << "\nReconstructing each chunk using Lagrange interpolation..." << std::endl;
    std::vector<uint64_t> reconstructed_chunks;
    
    for (size_t chunk_idx = 0; chunk_idx < d_chunks.size(); chunk_idx++) {
        // Gather shares from collaborating parties for this chunk
        std::vector<ShamirSecretSharing::Share> chunk_shares;
        for (size_t party_idx : collaborating_parties) {
            chunk_shares.push_back(all_chunk_shares[chunk_idx][party_idx]);
        }
        
        // Reconstruct this chunk
        ShamirSecretSharing sss(threshold, num_parties, prime);
        uint64_t reconstructed_chunk = sss.reconstruct(chunk_shares);
        reconstructed_chunks.push_back(reconstructed_chunk);
        
        std::cout << "  Chunk " << chunk_idx << ": ";
        std::cout << "original=" << d_chunks[chunk_idx] << ", ";
        std::cout << "reconstructed=" << reconstructed_chunk;
        
        if (reconstructed_chunk == d_chunks[chunk_idx]) {
            std::cout << " ✓" << std::endl;
        } else {
            std::cout << " ✗ MISMATCH!" << std::endl;
            free_rsa_components(rsa_comp);
            RSA_free(rsa);
            return 1;
        }
    }
    
    std::cout << "\n✓ All " << d_chunks.size() << " chunks successfully reconstructed!" << std::endl;
    
    // Reconstruct the full private exponent from chunks
    std::cout << "\nReassembling full private exponent from chunks..." << std::endl;
    BIGNUM* reconstructed_d_bn = reconstruct_bignum_from_chunks(reconstructed_chunks);
    
    // Verify reconstruction matches original
    if (BN_cmp(reconstructed_d_bn, rsa_comp.d) == 0) {
        std::cout << "✓ Full private exponent successfully reconstructed!" << std::endl;
        std::cout << "  Original:      " << BN_num_bits(rsa_comp.d) << " bits" << std::endl;
        std::cout << "  Reconstructed: " << BN_num_bits(reconstructed_d_bn) << " bits" << std::endl;
    } else {
        std::cout << "✗ Private exponent reconstruction failed!" << std::endl;
        BN_free(reconstructed_d_bn);
        free_rsa_components(rsa_comp);
        RSA_free(rsa);
        return 1;
    }
    
    std::cout << "\n=== STEP 5: Decrypt Pre-Master Secret ===" << std::endl;
    
    // Reconstruct the full RSA key from components and reconstructed private exponent
    std::cout << "\nReconstructing full RSA key from RSA components..." << std::endl;
    std::cout << "  Using fully reconstructed private exponent (" 
              << BN_num_bits(reconstructed_d_bn) << " bits)" << std::endl;
    RSA* reconstructed_rsa = reconstruct_rsa_from_components(rsa_comp, reconstructed_d_bn);
    if (!reconstructed_rsa) {
        std::cerr << "Failed to reconstruct RSA key from components" << std::endl;
        BN_free(reconstructed_d_bn);
        free_rsa_components(rsa_comp);
        RSA_free(rsa);
        return 1;
    }
    
    // Use the reconstructed RSA key for decryption
    std::cout << "\nDecrypting PMS with reconstructed multi-party RSA key..." << std::endl;
    std::cout << "  This demonstrates that the key was reconstructed from threshold shares." << std::endl;
    unsigned char decrypted_pms[RSA_size(reconstructed_rsa)];
    int decrypted_len = RSA_private_decrypt(
        encrypted_len,
        encrypted_pms,
        decrypted_pms,
        reconstructed_rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    
    if (decrypted_len == -1) {
        print_openssl_error();
        BN_free(reconstructed_d_bn);
        free_rsa_components(rsa_comp);
        RSA_free(rsa);
        RSA_free(reconstructed_rsa);
        return 1;
    }
    
    print_hex("Decrypted PMS", decrypted_pms, decrypted_len);
    
    // Verify decryption
    if (decrypted_len == sizeof(pms) && memcmp(pms, decrypted_pms, sizeof(pms)) == 0) {
        std::cout << "✓ Pre-Master Secret successfully decrypted!" << std::endl;
        std::cout << "✓ Decrypted PMS matches original!" << std::endl;
        std::cout << "\n✓✓✓ FULL PRIVATE KEY RECONSTRUCTION SUCCESSFUL ✓✓✓" << std::endl;
        std::cout << "    The complete " << BN_num_bits(reconstructed_d_bn) 
                  << "-bit private key was:" << std::endl;
        std::cout << "    1. Split into " << d_chunks.size() << " chunks" << std::endl;
        std::cout << "    2. Each chunk shared among " << num_parties << " parties" << std::endl;
        std::cout << "    3. Reconstructed from " << threshold << " parties' shares" << std::endl;
        std::cout << "    4. Used for successful decryption" << std::endl;
    } else {
        std::cout << "✗ PMS decryption failed or mismatch!" << std::endl;
        BN_free(reconstructed_d_bn);
        free_rsa_components(rsa_comp);
        RSA_free(rsa);
        RSA_free(reconstructed_rsa);
        return 1;
    }
    
    std::cout << "\n=== STEP 6: Security - Erase Reconstructed Key ===" << std::endl;
    std::cout << "Securely erasing reconstructed private exponent from memory..." << std::endl;
    
    // Clear the reconstructed chunks vector
    for (auto& chunk : reconstructed_chunks) {
        OPENSSL_cleanse(&chunk, sizeof(chunk));
    }
    
    // Clear the BIGNUM
    BN_clear_free(reconstructed_d_bn);
    std::cout << "✓ Reconstructed key and all " << reconstructed_chunks.size() 
              << " chunks erased." << std::endl;
    
    // Cleanup
    free_rsa_components(rsa_comp);
    RSA_free(rsa);
    RSA_free(reconstructed_rsa);
    EVP_cleanup();
    ERR_free_strings();
    
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              TEST COMPLETED SUCCESSFULLY!              ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝\n" << std::endl;
    
    std::cout << "Generated files:" << std::endl;
    std::cout << "  - rsa_private.pem (RSA private key)" << std::endl;
    std::cout << "  - rsa_public.pem (RSA public key)" << std::endl;
    
    return 0;
}
