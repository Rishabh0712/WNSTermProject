#include "shamir_secret_sharing.hpp"
#include <algorithm>
#include <iostream>

ShamirSecretSharing::ShamirSecretSharing(size_t threshold, size_t num_shares, BigInt prime)
    : threshold_(threshold), num_shares_(num_shares), prime_(prime), rng_(rd_()) {
    
    if (threshold < 2) {
        throw std::invalid_argument("Threshold must be at least 2");
    }
    if (num_shares < threshold) {
        throw std::invalid_argument("Number of shares must be >= threshold");
    }
    if (prime < 2) {
        throw std::invalid_argument("Prime must be >= 2");
    }
}

std::vector<ShamirSecretSharing::Share> ShamirSecretSharing::split(BigInt secret) {
    if (secret >= prime_) {
        throw std::invalid_argument("Secret must be less than prime");
    }
    
    // Generate random polynomial coefficients
    // f(x) = a_0 + a_1*x + a_2*x^2 + ... + a_(t-1)*x^(t-1)
    // where a_0 = secret
    std::vector<BigInt> coefficients(threshold_);
    coefficients[0] = secret;  // a_0 is the secret
    
    std::uniform_int_distribution<BigInt> dist(1, prime_ - 1);
    for (size_t i = 1; i < threshold_; ++i) {
        coefficients[i] = dist(rng_);
    }
    
    // Generate shares by evaluating polynomial at points 1, 2, ..., n
    std::vector<Share> shares;
    shares.reserve(num_shares_);
    
    for (size_t i = 1; i <= num_shares_; ++i) {
        Share share;
        share.id = i;
        share.value = evaluate_polynomial(coefficients, i);
        shares.push_back(share);
    }
    
    return shares;
}

ShamirSecretSharing::BigInt ShamirSecretSharing::reconstruct(const std::vector<Share>& shares) {
    if (shares.size() < threshold_) {
        throw std::invalid_argument("Need at least threshold shares to reconstruct");
    }
    
    // Validate share IDs are unique
    std::map<size_t, bool> seen;
    for (const auto& share : shares) {
        if (seen[share.id]) {
            throw std::invalid_argument("Duplicate share IDs detected");
        }
        seen[share.id] = true;
    }
    
    // Use Lagrange interpolation to find f(0) = secret
    return lagrange_interpolate(shares);
}

ShamirSecretSharing::BigInt ShamirSecretSharing::mod_add(BigInt a, BigInt b) const {
    return (a % prime_ + b % prime_) % prime_;
}

ShamirSecretSharing::BigInt ShamirSecretSharing::mod_sub(BigInt a, BigInt b) const {
    a = a % prime_;
    b = b % prime_;
    if (a >= b) {
        return (a - b) % prime_;
    } else {
        return (prime_ - ((b - a) % prime_)) % prime_;
    }
}

ShamirSecretSharing::BigInt ShamirSecretSharing::mod_mul(BigInt a, BigInt b) const {
    // Use 128-bit arithmetic to prevent overflow
    __uint128_t result = (__uint128_t)(a % prime_) * (b % prime_);
    return result % prime_;
}

ShamirSecretSharing::BigInt ShamirSecretSharing::mod_pow(BigInt base, BigInt exp) const {
    BigInt result = 1;
    base = base % prime_;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = mod_mul(result, base);
        }
        exp = exp >> 1;
        base = mod_mul(base, base);
    }
    
    return result;
}

int64_t ShamirSecretSharing::extended_gcd(BigInt a, BigInt b, BigInt& x, BigInt& y) const {
    // Extended Euclidean algorithm using signed arithmetic
    int64_t old_r = static_cast<int64_t>(a), r = static_cast<int64_t>(b);
    int64_t old_s = 1, s = 0;
    int64_t old_t = 0, t = 1;
    
    while (r != 0) {
        int64_t quotient = old_r / r;
        
        int64_t temp = r;
        r = old_r - quotient * r;
        old_r = temp;
        
        temp = s;
        s = old_s - quotient * s;
        old_s = temp;
        
        temp = t;
        t = old_t - quotient * t;
        old_t = temp;
    }
    
    // Store results (will be interpreted as signed by caller)
    x = static_cast<BigInt>(old_s);
    y = static_cast<BigInt>(old_t);
    return old_r;
}

ShamirSecretSharing::BigInt ShamirSecretSharing::mod_inv(BigInt a) const {
    // Use Fermat's Little Theorem: a^(p-1) ≡ 1 (mod p)
    // Therefore: a^(-1) ≡ a^(p-2) (mod p) for prime p
    if (a == 0) {
        throw std::runtime_error("Modular inverse of 0 does not exist");
    }
    
    a = a % prime_;
    // Compute a^(prime-2) mod prime using modular exponentiation
    return mod_pow(a, prime_ - 2);
}

ShamirSecretSharing::BigInt ShamirSecretSharing::evaluate_polynomial(
    const std::vector<BigInt>& coefficients, BigInt x) const {
    
    x = x % prime_;  // Ensure x is in the field
    BigInt result = 0;
    BigInt x_power = 1;  // x^0 = 1
    
    for (size_t i = 0; i < coefficients.size(); ++i) {
        BigInt term = mod_mul(coefficients[i], x_power);
        result = mod_add(result, term);
        x_power = mod_mul(x_power, x);  // x^(i+1)
    }
    
    return result;
}

ShamirSecretSharing::BigInt ShamirSecretSharing::lagrange_interpolate(
    const std::vector<Share>& shares) const {
    
    // Compute f(0) using Lagrange interpolation
    // f(0) = Σ(y_i * L_i(0)) where L_i(0) = Π((0-x_j)/(x_i-x_j)) for j≠i
    
    // Use only the first 'threshold_' shares
    size_t num_shares_to_use = std::min(shares.size(), threshold_);
    
    BigInt secret = 0;
    
    for (size_t i = 0; i < num_shares_to_use; ++i) {
        BigInt numerator = 1;
        BigInt denominator = 1;
        
        for (size_t j = 0; j < num_shares_to_use; ++j) {
            if (i != j) {
                // numerator *= (0 - x_j) = -x_j
                // Since we want (0 - x_j), this is equivalent to prime - x_j in modular arithmetic
                numerator = mod_mul(numerator, mod_sub(0, shares[j].id));
                
                // denominator *= (x_i - x_j)
                denominator = mod_mul(denominator, mod_sub(shares[i].id, shares[j].id));
            }
        }
        
        // L_i(0) = numerator / denominator
        BigInt lagrange_coeff = mod_mul(numerator, mod_inv(denominator));
        
        // Add y_i * L_i(0) to result
        BigInt term = mod_mul(shares[i].value, lagrange_coeff);
        secret = mod_add(secret, term);
    }
    
    return secret;
}
