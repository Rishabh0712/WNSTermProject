#include "tls_multiparty.hpp"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <iostream>

TLSMultiParty::TLSMultiParty(size_t threshold, size_t num_parties)
    : threshold_(threshold), num_parties_(num_parties) {
    
    sss_ = std::make_unique<ShamirSecretSharing>(threshold, num_parties, PRIME);
}

std::pair<TLSMultiParty::Bytes, TLSMultiParty::PrivateKeyShares> 
TLSMultiParty::generateAndDistributeKeys() {
    // For demonstration, generate a simplified "private key" as a random number
    // In production, this would be an actual RSA private exponent
    
    Bytes random_key = generateRandom(32);  // 256-bit key
    ShamirSecretSharing::BigInt private_key = bytesToBigInt(random_key);
    
    // Ensure private key is within field
    private_key = private_key % PRIME;
    
    std::cout << "[Key Generation] Original private key: " << private_key << std::endl;
    
    // Split private key into shares
    PrivateKeyShares shares = sss_->split(private_key);
    
    std::cout << "[Key Distribution] Generated " << shares.size() << " shares:" << std::endl;
    for (const auto& share : shares) {
        std::cout << "  Party " << share.id << " receives share: " << share.value << std::endl;
    }
    
    // Generate corresponding public key (simplified)
    Bytes public_key = bigIntToBytes(private_key, 32);  // In practice, this would be e, n for RSA
    
    return {public_key, shares};
}

TLSMultiParty::Bytes TLSMultiParty::encryptPreMasterSecret(
    const Bytes& pms, const Bytes& public_key) {
    
    // Simplified encryption: In production, use RSA-PKCS1 or RSA-OAEP
    // For demonstration, we'll just XOR with public key (NOT SECURE - for illustration only)
    
    std::cout << "[Client] Encrypting Pre-Master Secret with server's public key" << std::endl;
    
    Bytes encrypted = pms;
    for (size_t i = 0; i < encrypted.size() && i < public_key.size(); ++i) {
        encrypted[i] ^= public_key[i];
    }
    
    return encrypted;
}

TLSMultiParty::Bytes TLSMultiParty::collaborativeDecryption(
    const Bytes& encrypted_pms,
    const PrivateKeyShares& shares,
    const std::vector<size_t>& share_ids) {
    
    if (shares.size() < threshold_) {
        throw std::invalid_argument("Insufficient shares for decryption");
    }
    
    std::cout << "\n[Multi-Party Decryption] Starting collaborative decryption..." << std::endl;
    std::cout << "[Multi-Party Decryption] " << shares.size() << " parties participating" << std::endl;
    
    // Step 1: Each party contributes their share
    std::vector<ShamirSecretSharing::Share> active_shares;
    for (size_t i = 0; i < std::min(shares.size(), threshold_); ++i) {
        std::cout << "  Party " << shares[i].id << " contributes share: " << shares[i].value << std::endl;
        active_shares.push_back(shares[i]);
    }
    
    // Step 2: Reconstruct the complete private key using Lagrange interpolation
    std::cout << "\n[Key Reconstruction] Using Lagrange interpolation..." << std::endl;
    ShamirSecretSharing::BigInt reconstructed_key = sss_->reconstruct(active_shares);
    
    std::cout << "[Key Reconstruction] Reconstructed private key: " << reconstructed_key << std::endl;
    std::cout << "[SECURITY WARNING] Complete private key exists in memory temporarily!" << std::endl;
    
    // Step 3: Decrypt the PMS using reconstructed private key
    Bytes private_key_bytes = bigIntToBytes(reconstructed_key, 32);
    
    Bytes decrypted_pms = encrypted_pms;
    for (size_t i = 0; i < decrypted_pms.size() && i < private_key_bytes.size(); ++i) {
        decrypted_pms[i] ^= private_key_bytes[i];
    }
    
    // Step 4: CRITICAL - Securely erase the reconstructed private key
    std::cout << "[Security] Securely erasing reconstructed private key from memory" << std::endl;
    secureErase(private_key_bytes);
    
    std::cout << "[Multi-Party Decryption] Pre-Master Secret successfully decrypted\n" << std::endl;
    
    return decrypted_pms;
}

TLSMultiParty::Bytes TLSMultiParty::deriveMasterSecret(
    const Bytes& pms,
    const Bytes& client_random,
    const Bytes& server_random) {
    
    std::cout << "[Key Derivation] Deriving master secret from PMS..." << std::endl;
    
    // Concatenate client_random + server_random
    Bytes seed = client_random;
    seed.insert(seed.end(), server_random.begin(), server_random.end());
    
    // master_secret = PRF(pms, "master secret", client_random + server_random)[0..47]
    Bytes master_secret = tls_prf(pms, "master secret", seed, 48);
    
    std::cout << "[Key Derivation] Master secret derived (48 bytes)" << std::endl;
    
    return master_secret;
}

TLSMultiParty::Bytes TLSMultiParty::deriveKeyBlock(
    const Bytes& master_secret,
    const Bytes& client_random,
    const Bytes& server_random,
    size_t length) {
    
    std::cout << "[Key Derivation] Deriving key block for session keys..." << std::endl;
    
    // Concatenate server_random + client_random (note: reversed order)
    Bytes seed = server_random;
    seed.insert(seed.end(), client_random.begin(), client_random.end());
    
    // key_block = PRF(master_secret, "key expansion", server_random + client_random)
    Bytes key_block = tls_prf(master_secret, "key expansion", seed, length);
    
    std::cout << "[Key Derivation] Key block derived (" << length << " bytes)" << std::endl;
    
    return key_block;
}

void TLSMultiParty::secureErase(Bytes& data) {
    // Securely zero out memory
    if (!data.empty()) {
        OPENSSL_cleanse(data.data(), data.size());
        data.clear();
    }
}

TLSMultiParty::Bytes TLSMultiParty::generateRandom(size_t length) {
    Bytes result(length);
    if (RAND_bytes(result.data(), length) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return result;
}

ShamirSecretSharing::BigInt TLSMultiParty::bytesToBigInt(const Bytes& bytes) {
    ShamirSecretSharing::BigInt result = 0;
    for (size_t i = 0; i < bytes.size() && i < 8; ++i) {
        result = (result << 8) | bytes[i];
    }
    return result;
}

TLSMultiParty::Bytes TLSMultiParty::bigIntToBytes(
    ShamirSecretSharing::BigInt value, size_t length) {
    
    Bytes result(length, 0);
    for (int i = std::min(length, size_t(8)) - 1; i >= 0; --i) {
        result[i] = value & 0xFF;
        value >>= 8;
    }
    return result;
}

TLSMultiParty::Bytes TLSMultiParty::tls_prf(
    const Bytes& secret,
    const std::string& label,
    const Bytes& seed,
    size_t output_length) {
    
    // TLS 1.2 PRF: PRF(secret, label, seed) = P_SHA256(secret, label + seed)
    Bytes label_and_seed(label.begin(), label.end());
    label_and_seed.insert(label_and_seed.end(), seed.begin(), seed.end());
    
    return p_hash(secret, label_and_seed, output_length);
}

TLSMultiParty::Bytes TLSMultiParty::hmac_sha256(const Bytes& key, const Bytes& data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;
    
    HMAC(EVP_sha256(), key.data(), key.size(),
         data.data(), data.size(), result, &result_len);
    
    return Bytes(result, result + result_len);
}

TLSMultiParty::Bytes TLSMultiParty::p_hash(
    const Bytes& secret,
    const Bytes& seed,
    size_t output_length) {
    
    // P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
    //                        HMAC_hash(secret, A(2) + seed) + ...
    // where A(0) = seed, A(i) = HMAC_hash(secret, A(i-1))
    
    Bytes result;
    Bytes a = seed;  // A(0) = seed
    
    while (result.size() < output_length) {
        // A(i) = HMAC(secret, A(i-1))
        a = hmac_sha256(secret, a);
        
        // Concatenate A(i) + seed
        Bytes a_and_seed = a;
        a_and_seed.insert(a_and_seed.end(), seed.begin(), seed.end());
        
        // HMAC(secret, A(i) + seed)
        Bytes hmac_result = hmac_sha256(secret, a_and_seed);
        
        result.insert(result.end(), hmac_result.begin(), hmac_result.end());
    }
    
    // Truncate to required length
    result.resize(output_length);
    return result;
}
