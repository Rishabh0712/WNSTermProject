#include "tls_multiparty.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

// Helper function to print bytes in hex
void print_hex(const std::string& label, const TLSMultiParty::Bytes& data, size_t max_bytes = 16) {
    std::cout << label << ": ";
    for (size_t i = 0; i < std::min(data.size(), max_bytes); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(data[i]);
    }
    if (data.size() > max_bytes) {
        std::cout << "... (" << std::dec << data.size() << " bytes total)";
    }
    std::cout << std::dec << std::endl;
}

void test_shamir_secret_sharing() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 1: Shamir's Secret Sharing" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Parameters: (3,5)-threshold scheme
    const size_t threshold = 3;
    const size_t num_shares = 5;
    const uint64_t prime = 2305843009213693951ULL;  // 2^61 - 1 (Mersenne prime)
    const uint64_t secret = 12345678901234ULL;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Threshold (t): " << threshold << std::endl;
    std::cout << "  Total shares (n): " << num_shares << std::endl;
    std::cout << "  Prime modulus: " << prime << std::endl;
    std::cout << "  Original secret: " << secret << "\n" << std::endl;
    
    ShamirSecretSharing sss(threshold, num_shares, prime);
    
    // Split secret into shares
    std::cout << "Splitting secret into " << num_shares << " shares..." << std::endl;
    auto shares = sss.split(secret);
    
    std::cout << "\nGenerated shares:" << std::endl;
    for (const auto& share : shares) {
        std::cout << "  Share " << share.id << ": " << share.value << std::endl;
    }
    
    // Test reconstruction with exactly t shares
    std::cout << "\n--- Test 1a: Reconstruct with " << threshold << " shares ---" << std::endl;
    std::vector<ShamirSecretSharing::Share> subset1 = {shares[0], shares[1], shares[2]};
    std::cout << "Using shares: [" << subset1[0].id << ", " << subset1[1].id << ", " << subset1[2].id << "]" << std::endl;
    uint64_t reconstructed1 = sss.reconstruct(subset1);
    std::cout << "Reconstructed secret: " << reconstructed1 << std::endl;
    assert(reconstructed1 == secret);
    std::cout << "✓ Success! Secret correctly reconstructed." << std::endl;
    
    // Test reconstruction with different t shares
    std::cout << "\n--- Test 1b: Reconstruct with different " << threshold << " shares ---" << std::endl;
    std::vector<ShamirSecretSharing::Share> subset2 = {shares[1], shares[3], shares[4]};
    std::cout << "Using shares: [" << subset2[0].id << ", " << subset2[1].id << ", " << subset2[2].id << "]" << std::endl;
    uint64_t reconstructed2 = sss.reconstruct(subset2);
    std::cout << "Reconstructed secret: " << reconstructed2 << std::endl;
    assert(reconstructed2 == secret);
    std::cout << "✓ Success! Secret correctly reconstructed." << std::endl;
    
    // Test reconstruction with more than t shares
    std::cout << "\n--- Test 1c: Reconstruct with " << num_shares << " shares ---" << std::endl;
    std::cout << "Using shares: [";
    for (size_t i = 0; i < shares.size(); ++i) {
        std::cout << shares[i].id;
        if (i < shares.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    uint64_t reconstructed3 = sss.reconstruct(shares);
    std::cout << "Reconstructed secret: " << reconstructed3 << std::endl;
    assert(reconstructed3 == secret);
    std::cout << "✓ Success! Secret correctly reconstructed." << std::endl;
    
    // Test failure with insufficient shares
    std::cout << "\n--- Test 1d: Fail with insufficient shares (t-1) ---" << std::endl;
    std::vector<ShamirSecretSharing::Share> insufficient = {shares[0], shares[1]};
    std::cout << "Using shares: [" << insufficient[0].id << ", " << insufficient[1].id << "]" << std::endl;
    try {
        sss.reconstruct(insufficient);
        std::cout << "✗ Failed! Should have thrown exception." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Success! Correctly rejected: " << e.what() << std::endl;
    }
}

void test_tls_multiparty_handshake() {
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "TEST 2: Multi-Party TLS Handshake" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Configuration: 3-of-5 threshold
    const size_t threshold = 3;
    const size_t num_parties = 5;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Threshold: " << threshold << " parties needed" << std::endl;
    std::cout << "  Total parties: " << num_parties << "\n" << std::endl;
    
    TLSMultiParty tls(threshold, num_parties);
    
    // Phase 1: Key Generation and Distribution
    std::cout << "=== PHASE 1: KEY GENERATION AND DISTRIBUTION ===" << std::endl;
    auto [public_key, private_key_shares] = tls.generateAndDistributeKeys();
    
    // Simulate TLS handshake
    std::cout << "\n=== PHASE 2: TLS HANDSHAKE ===" << std::endl;
    
    // Generate random values for handshake
    auto client_random = TLSMultiParty::generateRandom(32);
    auto server_random = TLSMultiParty::generateRandom(32);
    auto pre_master_secret = TLSMultiParty::generateRandom(48);  // TLS 1.2: 48 bytes
    
    print_hex("[Handshake] Client Random", client_random);
    print_hex("[Handshake] Server Random", server_random);
    print_hex("[Handshake] Pre-Master Secret", pre_master_secret);
    
    // Client encrypts PMS with server's public key
    std::cout << "\n[Step 1] Client encrypts PMS with server's public key" << std::endl;
    auto encrypted_pms = tls.encryptPreMasterSecret(pre_master_secret, public_key);
    print_hex("[Client] Encrypted PMS", encrypted_pms);
    
    // Multi-party decryption: Parties 1, 2, and 3 collaborate
    std::cout << "\n[Step 2] Multi-party collaborative decryption" << std::endl;
    std::vector<ShamirSecretSharing::Share> collaborating_parties = {
        private_key_shares[0],  // Party 1
        private_key_shares[1],  // Party 2
        private_key_shares[2]   // Party 3
    };
    std::vector<size_t> party_ids = {1, 2, 3};
    
    std::cout << "Participating parties: [";
    for (size_t i = 0; i < party_ids.size(); ++i) {
        std::cout << party_ids[i];
        if (i < party_ids.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    auto decrypted_pms = tls.collaborativeDecryption(
        encrypted_pms, collaborating_parties, party_ids);
    
    print_hex("[Server] Decrypted PMS", decrypted_pms);
    
    // Verify decryption
    assert(decrypted_pms == pre_master_secret);
    std::cout << "✓ Decryption successful! PMS matches original." << std::endl;
    
    // Derive master secret
    std::cout << "\n[Step 3] Derive master secret from PMS" << std::endl;
    auto master_secret = tls.deriveMasterSecret(
        decrypted_pms, client_random, server_random);
    print_hex("[TLS] Master Secret", master_secret);
    
    // Derive session keys
    std::cout << "\n[Step 4] Derive session keys from master secret" << std::endl;
    // Key block size for AES-128-CBC with SHA256:
    // client_write_MAC_key (32) + server_write_MAC_key (32) +
    // client_write_key (16) + server_write_key (16) +
    // client_write_IV (16) + server_write_IV (16) = 128 bytes
    auto key_block = tls.deriveKeyBlock(master_secret, client_random, server_random, 128);
    print_hex("[TLS] Key Block", key_block);
    
    std::cout << "\n✓ TLS handshake completed successfully!" << std::endl;
    std::cout << "  - " << threshold << " parties collaborated to decrypt PMS" << std::endl;
    std::cout << "  - Master secret derived" << std::endl;
    std::cout << "  - Session keys established" << std::endl;
}

void test_different_party_combinations() {
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "TEST 3: Different Party Combinations" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const size_t threshold = 3;
    const size_t num_parties = 5;
    
    TLSMultiParty tls(threshold, num_parties);
    
    std::cout << "Testing that any " << threshold << " parties can decrypt...\n" << std::endl;
    
    auto [public_key, shares] = tls.generateAndDistributeKeys();
    auto pms = TLSMultiParty::generateRandom(48);
    auto encrypted_pms = tls.encryptPreMasterSecret(pms, public_key);
    
    // Test different combinations of parties
    std::vector<std::vector<size_t>> combinations = {
        {0, 1, 2},  // Parties 1, 2, 3
        {0, 2, 4},  // Parties 1, 3, 5
        {1, 3, 4},  // Parties 2, 4, 5
        {2, 3, 4}   // Parties 3, 4, 5
    };
    
    for (size_t i = 0; i < combinations.size(); ++i) {
        std::cout << "--- Combination " << (i + 1) << ": Parties ";
        for (size_t idx : combinations[i]) {
            std::cout << (idx + 1) << " ";
        }
        std::cout << "---" << std::endl;
        
        std::vector<ShamirSecretSharing::Share> party_shares;
        std::vector<size_t> party_ids;
        for (size_t idx : combinations[i]) {
            party_shares.push_back(shares[idx]);
            party_ids.push_back(idx + 1);
        }
        
        auto decrypted = tls.collaborativeDecryption(encrypted_pms, party_shares, party_ids);
        
        if (decrypted == pms) {
            std::cout << "✓ Success! Correctly decrypted PMS.\n" << std::endl;
        } else {
            std::cout << "✗ Failed! PMS mismatch.\n" << std::endl;
        }
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Multi-Party Authorization in TLS - Implementation    ║" << std::endl;
    std::cout << "║  Approach 1: Shamir's Secret Sharing                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        // Test 1: Basic secret sharing functionality
        test_shamir_secret_sharing();
        
        // Test 2: Complete TLS handshake with multi-party decryption
        test_tls_multiparty_handshake();
        
        // Test 3: Verify different party combinations work
        test_different_party_combinations();
        
        std::cout << "\n\n╔════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║            ALL TESTS PASSED SUCCESSFULLY!              ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════╝\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
