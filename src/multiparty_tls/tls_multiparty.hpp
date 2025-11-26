#ifndef TLS_MULTIPARTY_HPP
#define TLS_MULTIPARTY_HPP

#include "shamir_secret_sharing.hpp"
#include <vector>
#include <string>
#include <array>
#include <memory>

/**
 * Multi-Party TLS Implementation using Shamir's Secret Sharing
 * Implements Approach 1: Secret Sharing for Key Reconstruction
 */
class TLSMultiParty {
public:
    using Bytes = std::vector<uint8_t>;
    using PrivateKeyShares = std::vector<ShamirSecretSharing::Share>;
    
    struct KeyPair {
        Bytes public_key;
        Bytes private_key;
    };
    
    struct TLSSession {
        Bytes client_random;
        Bytes server_random;
        Bytes pre_master_secret;
        Bytes master_secret;
        Bytes key_block;  // Contains client/server write keys and IVs
    };
    
    /**
     * Constructor
     * @param threshold Minimum number of parties needed (t)
     * @param num_parties Total number of parties (n)
     */
    TLSMultiParty(size_t threshold, size_t num_parties);
    
    /**
     * Phase 1: Key Generation and Distribution
     * Generate RSA key pair and split private key into shares
     * @return Pair of (public key, vector of private key shares)
     */
    std::pair<Bytes, PrivateKeyShares> generateAndDistributeKeys();
    
    /**
     * Phase 2: TLS Handshake with Distributed Key
     * Each party holds a share of the private key
     */
    
    /**
     * Step 1: Client sends encrypted PMS using server's public key
     * @param pms Pre-Master Secret (48 bytes for TLS 1.2)
     * @param public_key Server's RSA public key
     * @return Encrypted PMS
     */
    Bytes encryptPreMasterSecret(const Bytes& pms, const Bytes& public_key);
    
    /**
     * Step 2: Parties collaborate to decrypt encrypted PMS
     * @param encrypted_pms Encrypted Pre-Master Secret
     * @param shares Vector of at least t private key shares
     * @param share_ids IDs of the participating parties
     * @return Decrypted Pre-Master Secret
     */
    Bytes collaborativeDecryption(
        const Bytes& encrypted_pms,
        const PrivateKeyShares& shares,
        const std::vector<size_t>& share_ids
    );
    
    /**
     * Step 3: Derive master secret from PMS
     * master_secret = PRF(pre_master_secret, "master secret", 
     *                     client_random + server_random)[0..47]
     */
    Bytes deriveMasterSecret(
        const Bytes& pms,
        const Bytes& client_random,
        const Bytes& server_random
    );
    
    /**
     * Step 4: Derive session keys from master secret
     * key_block = PRF(master_secret, "key expansion",
     *                 server_random + client_random)
     */
    Bytes deriveKeyBlock(
        const Bytes& master_secret,
        const Bytes& client_random,
        const Bytes& server_random,
        size_t length
    );
    
    /**
     * Security: Immediately destroy private key after reconstruction
     */
    void secureErase(Bytes& data);
    
    /**
     * Generate random bytes (for testing)
     */
    static Bytes generateRandom(size_t length);
    
    /**
     * Convert between bytes and BigInt for cryptographic operations
     */
    static ShamirSecretSharing::BigInt bytesToBigInt(const Bytes& bytes);
    static Bytes bigIntToBytes(ShamirSecretSharing::BigInt value, size_t length);

private:
    size_t threshold_;
    size_t num_parties_;
    std::unique_ptr<ShamirSecretSharing> sss_;
    
    // Large prime for finite field (simplified; use proper RSA modulus in production)
    // Using Mersenne prime 2^61 - 1 for safety and efficiency
    static constexpr uint64_t PRIME = 2305843009213693951ULL;  // 2^61 - 1 (Mersenne prime)
    
    /**
     * TLS 1.2 PRF (Pseudo-Random Function)
     * PRF(secret, label, seed) = P_SHA256(secret, label + seed)
     */
    Bytes tls_prf(
        const Bytes& secret,
        const std::string& label,
        const Bytes& seed,
        size_t output_length
    );
    
    /**
     * HMAC-SHA256 for TLS PRF
     */
    Bytes hmac_sha256(const Bytes& key, const Bytes& data);
    
    /**
     * P_hash function for TLS PRF
     */
    Bytes p_hash(const Bytes& secret, const Bytes& seed, size_t output_length);
};

#endif // TLS_MULTIPARTY_HPP
