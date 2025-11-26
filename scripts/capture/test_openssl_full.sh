#!/bin/bash

echo "╔════════════════════════════════════════════════════════╗"
echo "║  Multi-Party TLS - OpenSSL Certificate Verification   ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Run the C++ program to generate keys and test
echo "=== Step 1: Generate RSA Keys and Test Multi-Party Decryption ==="
./test_openssl_rsa

if [ $? -ne 0 ]; then
    echo "✗ Test failed!"
    exit 1
fi

echo ""
echo "=== Step 2: Verify Generated Certificates with OpenSSL ==="

# Check if files exist
if [ ! -f rsa_private.pem ] || [ ! -f rsa_public.pem ]; then
    echo "✗ Certificate files not found!"
    exit 1
fi

echo ""
echo "--- Private Key Information ---"
openssl rsa -in rsa_private.pem -text -noout | head -20

echo ""
echo "--- Public Key Information ---"
openssl rsa -pubin -in rsa_public.pem -text -noout

echo ""
echo "=== Step 3: Test Encryption/Decryption with OpenSSL CLI ==="

# Create test message
echo "Test Pre-Master Secret: $(openssl rand -hex 24)" > test_pms.txt
echo "Original message:"
cat test_pms.txt

echo ""
echo "Encrypting with public key..."
openssl pkeyutl -encrypt -pubin -inkey rsa_public.pem -in test_pms.txt -out test_pms.enc

if [ $? -eq 0 ]; then
    echo "✓ Encryption successful"
    echo "Encrypted size: $(stat -c%s test_pms.enc) bytes"
else
    echo "✗ Encryption failed"
    exit 1
fi

echo ""
echo "Decrypting with private key..."
openssl pkeyutl -decrypt -inkey rsa_private.pem -in test_pms.enc -out test_pms_decrypted.txt

if [ $? -eq 0 ]; then
    echo "✓ Decryption successful"
    echo "Decrypted message:"
    cat test_pms_decrypted.txt
else
    echo "✗ Decryption failed"
    exit 1
fi

echo ""
echo "Verifying messages match..."
if cmp -s test_pms.txt test_pms_decrypted.txt; then
    echo "✓ Messages match perfectly!"
else
    echo "✗ Messages do not match"
    exit 1
fi

echo ""
echo "=== Step 4: Create Self-Signed Certificate for TLS ==="

# Generate certificate request and self-signed cert
openssl req -new -key rsa_private.pem -out rsa_cert.csr -subj "/C=US/ST=State/L=City/O=Organization/CN=tls-multiparty.local"

openssl x509 -req -days 365 -in rsa_cert.csr -signkey rsa_private.pem -out rsa_cert.pem

if [ $? -eq 0 ]; then
    echo "✓ Self-signed certificate created"
    echo ""
    echo "--- Certificate Details ---"
    openssl x509 -in rsa_cert.pem -text -noout | head -25
else
    echo "✗ Certificate creation failed"
    exit 1
fi

echo ""
echo "=== Step 5: Verify Certificate Chain ==="
openssl verify -CAfile rsa_cert.pem rsa_cert.pem

echo ""
echo "╔════════════════════════════════════════════════════════╗"
echo "║         ALL VERIFICATION TESTS PASSED!                 ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""
echo "Generated files:"
echo "  - rsa_private.pem     (2048-bit RSA private key)"
echo "  - rsa_public.pem      (RSA public key)"
echo "  - rsa_cert.csr        (Certificate signing request)"
echo "  - rsa_cert.pem        (Self-signed X.509 certificate)"
echo "  - test_pms.txt        (Test pre-master secret)"
echo "  - test_pms.enc        (Encrypted PMS)"
echo "  - test_pms_decrypted.txt (Decrypted PMS)"
echo ""
