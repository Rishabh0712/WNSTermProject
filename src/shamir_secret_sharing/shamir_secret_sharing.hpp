#ifndef SHAMIR_SECRET_SHARING_HPP
#define SHAMIR_SECRET_SHARING_HPP

#include <vector>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <map>

/**
 * Shamir's Secret Sharing Implementation
 * Implements (t,n)-threshold secret sharing scheme
 */
class ShamirSecretSharing {
public:
    using BigInt = uint64_t;  // Simplified for demonstration; use GMP/NTL for production
    
    struct Share {
        size_t id;      // Party identifier (x-coordinate)
        BigInt value;   // Share value (y-coordinate)
    };
    
    /**
     * Constructor
     * @param threshold Minimum number of shares needed to reconstruct (t)
     * @param num_shares Total number of shares to generate (n)
     * @param prime Large prime number for finite field operations
     */
    ShamirSecretSharing(size_t threshold, size_t num_shares, BigInt prime);
    
    /**
     * Split a secret into n shares
     * @param secret The secret value to split
     * @return Vector of shares
     */
    std::vector<Share> split(BigInt secret);
    
    /**
     * Reconstruct secret from t or more shares
     * @param shares Vector of at least t shares
     * @return Reconstructed secret
     */
    BigInt reconstruct(const std::vector<Share>& shares);
    
    /**
     * Get the threshold value
     */
    size_t getThreshold() const { return threshold_; }
    
    /**
     * Get the total number of shares
     */
    size_t getNumShares() const { return num_shares_; }

private:
    size_t threshold_;    // Minimum shares needed (t)
    size_t num_shares_;   // Total shares (n)
    BigInt prime_;        // Prime modulus for finite field
    
    std::random_device rd_;
    std::mt19937_64 rng_;
    
    /**
     * Modular arithmetic operations
     */
    BigInt mod_add(BigInt a, BigInt b) const;
    BigInt mod_sub(BigInt a, BigInt b) const;
    BigInt mod_mul(BigInt a, BigInt b) const;
    BigInt mod_pow(BigInt base, BigInt exp) const;
    BigInt mod_inv(BigInt a) const;  // Modular multiplicative inverse
    
    /**
     * Polynomial evaluation at point x
     * f(x) = coefficients[0] + coefficients[1]*x + ... + coefficients[t-1]*x^(t-1)
     */
    BigInt evaluate_polynomial(const std::vector<BigInt>& coefficients, BigInt x) const;
    
    /**
     * Lagrange interpolation to find f(0)
     */
    BigInt lagrange_interpolate(const std::vector<Share>& shares) const;
    
    /**
     * Extended Euclidean algorithm for modular inverse
     */
    int64_t extended_gcd(BigInt a, BigInt b, BigInt& x, BigInt& y) const;
};

#endif // SHAMIR_SECRET_SHARING_HPP
