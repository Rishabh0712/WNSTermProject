/**
 * Multi-Party Threshold TLS for Rsyslog Integration
 * 
 * This module provides threshold cryptography for TLS private keys used in rsyslog.
 * Private keys are split using Shamir's Secret Sharing (3-of-5 threshold).
 * 
 * Author: Rishabh Kumar (cs25resch04002)
 * Date: November 26, 2025
 * Reference: RFC 5425 - TLS Transport Mapping for Syslog
 */

#include "shamir_secret_sharing.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

constexpr size_t THRESHOLD = 3;          // Minimum parties needed
constexpr size_t NUM_PARTIES = 5;        // Total authorization parties
constexpr size_t CHUNK_BITS = 61;        // Bits per chunk
constexpr uint64_t PRIME = 2305843009213693951ULL;  // 2^61 - 1 (Mersenne prime)

// Authorization party identities
const char* PARTY_NAMES[NUM_PARTIES] = {
    "Judicial Authority",
    "Law Enforcement",
    "Network Security Officer",
    "Privacy Oversight Officer",
    "Independent Auditor"
};

// Party network endpoints (for distributed deployment)
struct PartyEndpoint {
    size_t id;
    std::string name;
    std::string host;
    int port;
};

// ============================================================================
// KEY SHARE STORAGE
// ============================================================================

struct KeyShareData {
    size_t party_id;
    std::string party_name;
    size_t num_chunks;
    std::vector<ShamirSecretSharing::Share> shares;
    
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        // Write header
        file.write(reinterpret_cast<const char*>(&party_id), sizeof(party_id));
        size_t name_len = party_name.size();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(party_name.c_str(), name_len);
        file.write(reinterpret_cast<const char*>(&num_chunks), sizeof(num_chunks));
        
        // Write shares
        for (const auto& share : shares) {
            file.write(reinterpret_cast<const char*>(&share.id), sizeof(share.id));
            file.write(reinterpret_cast<const char*>(&share.value), sizeof(share.value));
        }
        
        return file.good();
    }
    
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        // Read header
        file.read(reinterpret_cast<char*>(&party_id), sizeof(party_id));
        size_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        party_name.resize(name_len);
        file.read(&party_name[0], name_len);
        file.read(reinterpret_cast<char*>(&num_chunks), sizeof(num_chunks));
        
        // Read shares
        shares.clear();
        for (size_t i = 0; i < num_chunks; ++i) {
            ShamirSecretSharing::Share share;
            file.read(reinterpret_cast<char*>(&share.id), sizeof(share.id));
            file.read(reinterpret_cast<char*>(&share.value), sizeof(share.value));
            shares.push_back(share);
        }
        
        return file.good();
    }
};

// ============================================================================
// MULTI-PARTY KEY MANAGER
// ============================================================================

class MultiPartyKeyManager {
public:
    MultiPartyKeyManager() : sss_(THRESHOLD, NUM_PARTIES, PRIME) {}
    
    /**
     * Split RSA private key into shares for N parties
     */
    bool splitPrivateKey(const std::string& private_key_path, 
                        std::vector<KeyShareData>& party_shares) {
        std::cout << "[INFO] Loading private key from: " << private_key_path << std::endl;
        
        // Load RSA private key
        FILE* fp = fopen(private_key_path.c_str(), "r");
        if (!fp) {
            std::cerr << "[ERROR] Failed to open private key file" << std::endl;
            return false;
        }
        
        RSA* rsa = PEM_read_RSAPrivateKey(fp, nullptr, nullptr, nullptr);
        fclose(fp);
        
        if (!rsa) {
            std::cerr << "[ERROR] Failed to read RSA private key" << std::endl;
            return false;
        }
        
        // Get private exponent
        const BIGNUM *n, *e, *d;
        RSA_get0_key(rsa, &n, &e, &d);
        
        int d_bits = BN_num_bits(d);
        std::cout << "[INFO] Private key size: " << d_bits << " bits" << std::endl;
        
        // Calculate number of chunks needed
        size_t num_chunks = (d_bits + CHUNK_BITS - 1) / CHUNK_BITS;
        std::cout << "[INFO] Splitting into " << num_chunks << " chunks of " 
                  << CHUNK_BITS << " bits each" << std::endl;
        
        // Initialize party shares
        party_shares.resize(NUM_PARTIES);
        for (size_t i = 0; i < NUM_PARTIES; ++i) {
            party_shares[i].party_id = i + 1;
            party_shares[i].party_name = PARTY_NAMES[i];
            party_shares[i].num_chunks = num_chunks;
            party_shares[i].shares.clear();
        }
        
        // Split each chunk using SSS
        BIGNUM* chunk_bn = BN_new();
        for (size_t chunk_id = 0; chunk_id < num_chunks; ++chunk_id) {
            // Extract chunk from private key
            BN_copy(chunk_bn, d);
            BN_rshift(chunk_bn, chunk_bn, chunk_id * CHUNK_BITS);
            BN_mask_bits(chunk_bn, CHUNK_BITS);
            
            uint64_t chunk_value = BN_get_word(chunk_bn);
            
            // Split this chunk using Shamir's Secret Sharing
            auto shares = sss_.split(chunk_value);
            
            // Distribute shares to parties
            for (size_t party_id = 0; party_id < NUM_PARTIES; ++party_id) {
                party_shares[party_id].shares.push_back(shares[party_id]);
            }
            
            if ((chunk_id + 1) % 10 == 0) {
                std::cout << "  Progress: " << (chunk_id + 1) << "/" << num_chunks 
                          << " chunks processed" << std::endl;
            }
        }
        
        BN_free(chunk_bn);
        RSA_free(rsa);
        
        std::cout << "[SUCCESS] Private key split into " << num_chunks 
                  << " chunks, distributed to " << NUM_PARTIES << " parties" << std::endl;
        std::cout << "[INFO] Each party has " << num_chunks << " shares" << std::endl;
        std::cout << "[INFO] Threshold: " << THRESHOLD << " parties required for reconstruction" << std::endl;
        
        return true;
    }
    
    /**
     * Reconstruct RSA private key from threshold parties
     */
    RSA* reconstructPrivateKey(const std::vector<KeyShareData>& participating_parties,
                              const std::string& public_key_path) {
        if (participating_parties.size() < THRESHOLD) {
            std::cerr << "[ERROR] Insufficient parties: " << participating_parties.size() 
                      << " (need " << THRESHOLD << ")" << std::endl;
            return nullptr;
        }
        
        std::cout << "[INFO] Reconstructing private key from " 
                  << participating_parties.size() << " parties:" << std::endl;
        for (const auto& party : participating_parties) {
            std::cout << "  - Party " << party.party_id << ": " << party.party_name << std::endl;
        }
        
        size_t num_chunks = participating_parties[0].num_chunks;
        
        // Reconstruct each chunk
        BIGNUM* d_reconstructed = BN_new();
        BN_zero(d_reconstructed);
        
        for (size_t chunk_id = 0; chunk_id < num_chunks; ++chunk_id) {
            // Gather shares for this chunk from participating parties
            std::vector<ShamirSecretSharing::Share> chunk_shares;
            for (const auto& party : participating_parties) {
                if (chunk_id < party.shares.size()) {
                    chunk_shares.push_back(party.shares[chunk_id]);
                }
            }
            
            // Reconstruct chunk value using Lagrange interpolation
            uint64_t chunk_value = sss_.reconstruct(chunk_shares);
            
            // Add chunk to reconstructed private key
            BIGNUM* chunk_bn = BN_new();
            BN_set_word(chunk_bn, chunk_value);
            BN_lshift(chunk_bn, chunk_bn, chunk_id * CHUNK_BITS);
            BN_add(d_reconstructed, d_reconstructed, chunk_bn);
            BN_free(chunk_bn);
        }
        
        std::cout << "[INFO] Private exponent reconstructed: " 
                  << BN_num_bits(d_reconstructed) << " bits" << std::endl;
        
        // Load public key components
        FILE* fp = fopen(public_key_path.c_str(), "r");
        if (!fp) {
            std::cerr << "[ERROR] Failed to open public key file" << std::endl;
            BN_clear_free(d_reconstructed);
            return nullptr;
        }
        
        RSA* rsa_pub = PEM_read_RSA_PUBKEY(fp, nullptr, nullptr, nullptr);
        fclose(fp);
        
        if (!rsa_pub) {
            std::cerr << "[ERROR] Failed to read RSA public key" << std::endl;
            BN_clear_free(d_reconstructed);
            return nullptr;
        }
        
        // Create new RSA key with reconstructed private key
        const BIGNUM *n, *e;
        RSA_get0_key(rsa_pub, &n, &e, nullptr);
        
        BIGNUM* n_copy = BN_dup(n);
        BIGNUM* e_copy = BN_dup(e);
        
        RSA* rsa_reconstructed = RSA_new();
        RSA_set0_key(rsa_reconstructed, n_copy, e_copy, d_reconstructed);
        
        RSA_free(rsa_pub);
        
        // Verify the reconstructed key is valid
        if (RSA_check_key(rsa_reconstructed) == 1) {
            std::cout << "[SUCCESS] Private key successfully reconstructed and verified" << std::endl;
        } else {
            std::cerr << "[WARNING] Reconstructed key failed validation" << std::endl;
        }
        
        return rsa_reconstructed;
    }
    
private:
    ShamirSecretSharing sss_;
};

// ============================================================================
// PARTY SHARE SERVER (runs on each authorization party)
// ============================================================================

class PartyShareServer {
public:
    PartyShareServer(int port, const KeyShareData& shares) 
        : port_(port), shares_(shares), running_(false) {}
    
    bool start() {
        // Create socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            std::cerr << "[ERROR] Failed to create socket" << std::endl;
            return false;
        }
        
        // Set socket options
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind to port
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "[ERROR] Failed to bind to port " << port_ << std::endl;
            close(server_fd_);
            return false;
        }
        
        // Listen for connections
        if (listen(server_fd_, 5) < 0) {
            std::cerr << "[ERROR] Failed to listen on port " << port_ << std::endl;
            close(server_fd_);
            return false;
        }
        
        running_ = true;
        std::cout << "[INFO] Party " << shares_.party_id << " (" << shares_.party_name 
                  << ") listening on port " << port_ << std::endl;
        
        return true;
    }
    
    void handleRequest() {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "[ERROR] Failed to accept connection" << std::endl;
            return;
        }
        
        std::cout << "[INFO] Connection from " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        // Read request (simple protocol: send shares)
        char buffer[1024];
        ssize_t n = read(client_fd, buffer, sizeof(buffer));
        
        if (n > 0 && strncmp(buffer, "GET_SHARES", 10) == 0) {
            std::cout << "[INFO] Providing shares to requester" << std::endl;
            
            // Send shares (in production, add authentication & authorization here)
            size_t num_shares = shares_.shares.size();
            write(client_fd, &num_shares, sizeof(num_shares));
            
            for (const auto& share : shares_.shares) {
                write(client_fd, &share.id, sizeof(share.id));
                write(client_fd, &share.value, sizeof(share.value));
            }
            
            std::cout << "[SUCCESS] Shares sent (" << num_shares << " chunks)" << std::endl;
        }
        
        close(client_fd);
    }
    
    void stop() {
        running_ = false;
        close(server_fd_);
    }
    
private:
    int port_;
    KeyShareData shares_;
    bool running_;
    int server_fd_;
};

// ============================================================================
// MAIN FUNCTIONS
// ============================================================================

void printUsage(const char* program_name) {
    std::cout << "Multi-Party Threshold TLS for Rsyslog\n" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  1. Split private key:" << std::endl;
    std::cout << "     " << program_name << " split <private_key.pem> <output_dir>" << std::endl;
    std::cout << std::endl;
    std::cout << "  2. Run party share server:" << std::endl;
    std::cout << "     " << program_name << " server <party_id> <share_file> <port>" << std::endl;
    std::cout << std::endl;
    std::cout << "  3. Reconstruct key (for testing):" << std::endl;
    std::cout << "     " << program_name << " reconstruct <share_file1> <share_file2> <share_file3> <public_key.pem> <output.pem>" << std::endl;
    std::cout << std::endl;
    std::cout << "Authorization Parties:" << std::endl;
    for (size_t i = 0; i < NUM_PARTIES; ++i) {
        std::cout << "  Party " << (i+1) << ": " << PARTY_NAMES[i] << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Threshold: " << THRESHOLD << " parties required" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    MultiPartyKeyManager key_manager;
    
    if (command == "split") {
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " split <private_key.pem> <output_dir>" << std::endl;
            return 1;
        }
        
        std::string private_key_path = argv[2];
        std::string output_dir = argv[3];
        
        std::cout << "========================================" << std::endl;
        std::cout << "RSA PRIVATE KEY SPLITTING" << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::vector<KeyShareData> party_shares;
        if (!key_manager.splitPrivateKey(private_key_path, party_shares)) {
            std::cerr << "[ERROR] Failed to split private key" << std::endl;
            return 1;
        }
        
        // Save shares to files
        std::cout << "\n[INFO] Saving shares to files..." << std::endl;
        for (const auto& share_data : party_shares) {
            std::string filename = output_dir + "/party_" + std::to_string(share_data.party_id) + ".share";
            if (share_data.saveToFile(filename)) {
                std::cout << "  ✓ Party " << share_data.party_id << " shares saved to: " << filename << std::endl;
            } else {
                std::cerr << "  ✗ Failed to save shares for Party " << share_data.party_id << std::endl;
            }
        }
        
        std::cout << "\n[SUCCESS] Key splitting complete!" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "1. Distribute share files to respective authorization parties" << std::endl;
        std::cout << "2. Each party runs: " << argv[0] << " server <party_id> <share_file> <port>" << std::endl;
        std::cout << "3. Configure rsyslog to use multi-party TLS module" << std::endl;
        
    } else if (command == "server") {
        if (argc != 5) {
            std::cerr << "Usage: " << argv[0] << " server <party_id> <share_file> <port>" << std::endl;
            return 1;
        }
        
        size_t party_id = std::stoul(argv[2]);
        std::string share_file = argv[3];
        int port = std::stoi(argv[4]);
        
        // Load shares
        KeyShareData shares;
        if (!shares.loadFromFile(share_file)) {
            std::cerr << "[ERROR] Failed to load shares from: " << share_file << std::endl;
            return 1;
        }
        
        std::cout << "========================================" << std::endl;
        std::cout << "PARTY SHARE SERVER" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Party ID: " << shares.party_id << std::endl;
        std::cout << "Party Name: " << shares.party_name << std::endl;
        std::cout << "Shares: " << shares.num_chunks << " chunks" << std::endl;
        std::cout << "========================================" << std::endl;
        
        PartyShareServer server(port, shares);
        if (!server.start()) {
            return 1;
        }
        
        std::cout << "\n[INFO] Server running. Press Ctrl+C to stop." << std::endl;
        std::cout << "[INFO] Waiting for share requests..." << std::endl;
        
        // Simple event loop (in production, use proper async I/O)
        while (true) {
            server.handleRequest();
        }
        
    } else if (command == "reconstruct") {
        if (argc != 7) {
            std::cerr << "Usage: " << argv[0] << " reconstruct <share1> <share2> <share3> <public_key.pem> <output.pem>" << std::endl;
            return 1;
        }
        
        std::vector<std::string> share_files = {argv[2], argv[3], argv[4]};
        std::string public_key_path = argv[5];
        std::string output_path = argv[6];
        
        std::cout << "========================================" << std::endl;
        std::cout << "RSA PRIVATE KEY RECONSTRUCTION" << std::endl;
        std::cout << "========================================" << std::endl;
        
        // Load shares from participating parties
        std::vector<KeyShareData> participating_parties;
        for (const auto& share_file : share_files) {
            KeyShareData shares;
            if (shares.loadFromFile(share_file)) {
                participating_parties.push_back(shares);
                std::cout << "[INFO] Loaded shares from Party " << shares.party_id 
                          << " (" << shares.party_name << ")" << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to load: " << share_file << std::endl;
                return 1;
            }
        }
        
        // Reconstruct private key
        RSA* rsa_reconstructed = key_manager.reconstructPrivateKey(participating_parties, public_key_path);
        if (!rsa_reconstructed) {
            std::cerr << "[ERROR] Failed to reconstruct private key" << std::endl;
            return 1;
        }
        
        // Save reconstructed key
        FILE* fp = fopen(output_path.c_str(), "w");
        if (!fp) {
            std::cerr << "[ERROR] Failed to open output file" << std::endl;
            RSA_free(rsa_reconstructed);
            return 1;
        }
        
        if (PEM_write_RSAPrivateKey(fp, rsa_reconstructed, nullptr, nullptr, 0, nullptr, nullptr)) {
            std::cout << "[SUCCESS] Reconstructed private key saved to: " << output_path << std::endl;
            std::cout << "\n[SECURITY] Key will be destroyed from memory immediately" << std::endl;
        } else {
            std::cerr << "[ERROR] Failed to write private key" << std::endl;
        }
        
        fclose(fp);
        RSA_free(rsa_reconstructed);  // Secure erasure
        
    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
