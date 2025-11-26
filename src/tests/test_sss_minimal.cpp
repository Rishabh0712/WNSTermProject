#include "shamir_secret_sharing.hpp"
#include <iostream>

int main() {
    std::cout << "Creating SSS object..." << std::endl;
    
    try {
        ShamirSecretSharing sss(3, 5, 2305843009213693951ULL);
        std::cout << "SSS created successfully" << std::endl;
        
        std::cout << "Attempting to split value 42..." << std::endl;
        auto shares = sss.split(42);
        std::cout << "Split successful! Created " << shares.size() << " shares" << std::endl;
        
        for (size_t i = 0; i < shares.size(); ++i) {
            std::cout << "  Share " << shares[i].id << ": " << shares[i].value << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Test passed!" << std::endl;
    return 0;
}
