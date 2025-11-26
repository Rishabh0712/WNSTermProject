// Quick test with smaller prime
#include "shamir_secret_sharing.hpp"
#include <iostream>

int main() {
    // Use smaller prime: 257 (first prime > 256)
    const uint64_t small_prime = 257;
    const size_t threshold = 3;
    const size_t num_shares = 5;
    const uint64_t secret = 123;
    
    std::cout << "Testing with small prime: " << small_prime << std::endl;
    std::cout << "Secret: " << secret << std::endl;
    std::cout << "Threshold: " << threshold << ", Shares: " << num_shares << "\n" << std::endl;
    
    ShamirSecretSharing sss(threshold, num_shares, small_prime);
    
    auto shares = sss.split(secret);
    
    std::cout << "Shares generated:" << std::endl;
    for (const auto& share : shares) {
        std::cout << "  Share " << share.id << ": " << share.value << std::endl;
    }
    
    std::cout << "\nReconstruct with first " << threshold << " shares:" << std::endl;
    std::vector<ShamirSecretSharing::Share> subset = {shares[0], shares[1], shares[2]};
    uint64_t reconstructed = sss.reconstruct(subset);
    
    std::cout << "Reconstructed: " << reconstructed << std::endl;
    std::cout << "Expected: " << secret << std::endl;
    std::cout << "Match: " << (reconstructed == secret ? "YES ✓" : "NO ✗") << std::endl;
    
    return 0;
}
