#include <iostream>
#include <cstdint>

using BigInt = uint64_t;
const BigInt PRIME = 18446744073709551557ULL;

BigInt mod_add(BigInt a, BigInt b) {
    return (a % PRIME + b % PRIME) % PRIME;
}

BigInt mod_sub(BigInt a, BigInt b) {
    a = a % PRIME;
    b = b % PRIME;
    if (a >= b) {
        return (a - b) % PRIME;
    } else {
        return (PRIME - ((b - a) % PRIME)) % PRIME;
    }
}

BigInt mod_mul(BigInt a, BigInt b) {
    __uint128_t result = (__uint128_t)(a % PRIME) * (b % PRIME);
    return result % PRIME;
}

BigInt mod_pow(BigInt base, BigInt exp) {
    BigInt result = 1;
    base = base % PRIME;
    
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = mod_mul(result, base);
        }
        exp = exp >> 1;
        base = mod_mul(base, base);
    }
    
    return result;
}

BigInt mod_inv(BigInt a) {
    if (a == 0) {
        throw std::runtime_error("Modular inverse of 0 does not exist");
    }
    
    a = a % PRIME;
    return mod_pow(a, PRIME - 2);
}

int main() {
    // Test simple case: secret = 5, threshold = 2, shares = 3
    // f(x) = 5 + 3x  (random coefficient a1 = 3)
    BigInt secret = 5;
    BigInt a1 = 3;
    
    // Generate shares: f(1) = 8, f(2) = 11, f(3) = 14
    BigInt s1 = mod_add(secret, mod_mul(a1, 1)); // 5 + 3*1 = 8
    BigInt s2 = mod_add(secret, mod_mul(a1, 2)); // 5 + 3*2 = 11
    BigInt s3 = mod_add(secret, mod_mul(a1, 3)); // 5 + 3*3 = 14
    
    std::cout << "Secret: " << secret << std::endl;
    std::cout << "Share 1 (x=1): " << s1 << std::endl;
    std::cout << "Share 2 (x=2): " << s2 << std::endl;
    std::cout << "Share 3 (x=3): " << s3 << std::endl;
    std::cout << std::endl;
    
    // Reconstruct using shares 1 and 2
    // f(0) = s1 * L1(0) + s2 * L2(0)
    // L1(0) = (0-2)/(1-2) = -2/-1 = 2
    // L2(0) = (0-1)/(2-1) = -1/1 = -1
    
    BigInt num1 = mod_sub(0, 2);  // -2
    BigInt den1 = mod_sub(1, 2);  // -1
    BigInt L1 = mod_mul(num1, mod_inv(den1));
    
    BigInt num2 = mod_sub(0, 1);  // -1
    BigInt den2 = mod_sub(2, 1);  // 1
    BigInt L2 = mod_mul(num2, mod_inv(den2));
    
    std::cout << "L1(0) numerator (0-2): " << num1 << std::endl;
    std::cout << "L1(0) denominator (1-2): " << den1 << std::endl;
    std::cout << "L1(0): " << L1 << std::endl;
    std::cout << std::endl;
    
    std::cout << "L2(0) numerator (0-1): " << num2 << std::endl;
    std::cout << "L2(0) denominator (2-1): " << den2 << std::endl;
    std::cout << "L2(0): " << L2 << std::endl;
    std::cout << std::endl;
    
    BigInt reconstructed = mod_add(mod_mul(s1, L1), mod_mul(s2, L2));
    
    std::cout << "Reconstructed: " << reconstructed << std::endl;
    std::cout << "Expected: " << secret << std::endl;
    std::cout << "Match: " << (reconstructed == secret ? "YES" : "NO") << std::endl;
    
    return 0;
}
