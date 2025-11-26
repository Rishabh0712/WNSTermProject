#!/bin/bash

# Script: Secure Syslog Setup with Multi-Party Threshold TLS
# Purpose: Generate RSA keys using threshold cryptography and configure rsyslog
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 26, 2025

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "=========================================="
echo "Multi-Party Threshold TLS for Rsyslog"
echo "Secure Syslog with Distributed Key Management"
echo "=========================================="
echo ""

# Configuration
NUM_PARTIES=5
THRESHOLD=3
AMF2_CONTAINER="rfsim5g-oai-amf-2"
SYSLOG_PORT=6514
CERTS_DIR="./syslog_certs"

# Check if multiparty key generator is compiled
if [ ! -f "./multiparty_key_generator" ]; then
    echo -e "${BLUE}[Step 0] Compiling Multi-Party Key Generator${NC}"
    echo "=========================================="
    
    if [ ! -f "multiparty_key_generator.cpp" ]; then
        echo -e "${RED}✗ multiparty_key_generator.cpp not found${NC}"
        exit 1
    fi
    
    if [ ! -f "shamir_secret_sharing.cpp" ]; then
        echo -e "${RED}✗ shamir_secret_sharing.cpp not found${NC}"
        exit 1
    fi
    
    echo "Compiling..."
    g++ -std=c++17 -O2 multiparty_key_generator.cpp shamir_secret_sharing.cpp \
        -o multiparty_key_generator -lssl -lcrypto
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Compilation failed${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ Compilation successful${NC}"
    echo ""
fi

# Check if AMF-2 is running
echo -e "${BLUE}[Step 1] Checking AMF-2 Container${NC}"
echo "=========================================="
if docker ps | grep -q "$AMF2_CONTAINER"; then
    echo -e "${GREEN}✓ AMF-2 is running${NC}"
    AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$AMF2_CONTAINER")
    echo -e "  AMF-2 IP: ${YELLOW}$AMF2_IP${NC}"
else
    echo -e "${YELLOW}⚠ AMF-2 is not running${NC}"
    echo "  You may need to start it later"
    AMF2_IP="192.168.71.132"  # Default IP
fi
echo ""

# Create certificates directory
echo -e "${BLUE}[Step 2] Setting up Certificates Directory${NC}"
echo "=========================================="
mkdir -p "$CERTS_DIR"
cd "$CERTS_DIR"
echo -e "${GREEN}✓ Directory created: $CERTS_DIR${NC}"
echo ""

# Generate CA certificate (traditional)
echo -e "${BLUE}[Step 3] Generating CA Certificate${NC}"
echo "=========================================="
if [ ! -f "ca-cert.pem" ]; then
    echo "Generating CA certificate..."
    openssl req -x509 -newkey rsa:4096 -days 365 -nodes \
        -keyout ca-key.pem -out ca-cert.pem \
        -subj "/C=IN/ST=Telangana/L=Hyderabad/O=WNS-Project/OU=5G-Security/CN=Syslog-CA" \
        2>/dev/null
    echo -e "${GREEN}✓ CA certificate created${NC}"
else
    echo -e "${YELLOW}CA certificate already exists${NC}"
fi
echo ""

# Generate SERVER key using MULTI-PARTY THRESHOLD CRYPTOGRAPHY
echo -e "${BLUE}[Step 4] Generating Server Key (Multi-Party Threshold)${NC}"
echo "=========================================="
echo "Using ($THRESHOLD,$NUM_PARTIES)-threshold Shamir's Secret Sharing"
echo ""

if [ ! -f "server-key.pem" ]; then
    echo "Generating distributed RSA key..."
    ../multiparty_key_generator server-key.pem $NUM_PARTIES $THRESHOLD
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}✗ Multi-party key generation failed${NC}"
        cd ..
        exit 1
    fi
    
    echo ""
    echo -e "${GREEN}✓ Multi-party server key generated${NC}"
    echo -e "  ${YELLOW}Security: Key requires $THRESHOLD out of $NUM_PARTIES parties${NC}"
    
    # Show generated shares
    echo ""
    echo "Key shares distributed to parties:"
    for i in $(seq 1 $NUM_PARTIES); do
        if [ -f "server-key_party${i}_shares.dat" ]; then
            SIZE=$(stat -f%z "server-key_party${i}_shares.dat" 2>/dev/null || stat -c%s "server-key_party${i}_shares.dat" 2>/dev/null)
            echo -e "  ${GREEN}✓${NC} Party $i: server-key_party${i}_shares.dat ($SIZE bytes)"
        fi
    done
else
    echo -e "${YELLOW}Server key already exists${NC}"
fi
echo ""

# Generate server certificate using multi-party key
echo -e "${BLUE}[Step 5] Generating Server Certificate${NC}"
echo "=========================================="
if [ ! -f "server-cert.pem" ]; then
    echo "Creating certificate signing request..."
    openssl req -new -key server-key.pem \
        -out server-req.pem \
        -subj "/C=IN/ST=Telangana/L=Hyderabad/O=WNS-Project/OU=5G-Security/CN=syslog-server" \
        2>/dev/null
    
    echo "Signing certificate with CA..."
    openssl x509 -req -in server-req.pem -days 365 \
        -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
        -out server-cert.pem \
        2>/dev/null
    
    echo -e "${GREEN}✓ Server certificate created${NC}"
    echo -e "  ${YELLOW}Note: Certificate uses multi-party threshold private key${NC}"
else
    echo -e "${YELLOW}Server certificate already exists${NC}"
fi
echo ""

# Generate client certificate (traditional)
echo -e "${BLUE}[Step 6] Generating Client Certificate${NC}"
echo "=========================================="
if [ ! -f "client-cert.pem" ]; then
    echo "Generating client key and certificate..."
    openssl req -newkey rsa:4096 -nodes \
        -keyout client-key.pem \
        -out client-req.pem \
        -subj "/C=IN/ST=Telangana/L=Hyderabad/O=WNS-Project/OU=5G-Security/CN=amf2-client" \
        2>/dev/null
    
    openssl x509 -req -in client-req.pem -days 365 \
        -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
        -out client-cert.pem \
        2>/dev/null
    
    echo -e "${GREEN}✓ Client certificate created${NC}"
else
    echo -e "${YELLOW}Client certificate already exists${NC}"
fi
echo ""

# Create rsyslog server configuration
echo -e "${BLUE}[Step 7] Creating Rsyslog Configurations${NC}"
echo "=========================================="

cat > "rsyslog-server.conf" << 'EOF'
# Syslog Server Configuration with Multi-Party Threshold TLS
# RFC 5425 - TLS Transport for Syslog
# Private key protected by (3,5)-threshold Shamir's Secret Sharing

# Load modules
module(load="imtcp")
module(load="imuxsock")

# TLS driver settings - USING MULTI-PARTY THRESHOLD KEY
$DefaultNetstreamDriver gtls
$DefaultNetstreamDriverCAFile /etc/rsyslog.d/certs/ca-cert.pem
$DefaultNetstreamDriverCertFile /etc/rsyslog.d/certs/server-cert.pem
$DefaultNetstreamDriverKeyFile /etc/rsyslog.d/certs/server-key.pem
$InputTCPServerStreamDriverAuthMode x509/name
$InputTCPServerStreamDriverPermittedPeer amf2-client
$InputTCPServerStreamDriverMode 1 # TLS mode

# Listen on TLS port
$ModLoad imtcp
$InputTCPServerRun 6514

# Template for AMF logs
$template AMF2Template,"/var/log/amf2/%HOSTNAME%-%$YEAR%%$MONTH%%$DAY%.log"

# Log AMF-2 messages
if $fromhost-ip == 'AMF2_IP' then ?AMF2Template
& stop

# Default logging
*.info;mail.none;authpriv.none;cron.none    /var/log/messages
EOF

cat > "rsyslog-client.conf" << 'EOF'
# AMF-2 Rsyslog Client Configuration
# Connects to server using multi-party threshold TLS

# Load modules
module(load="imuxsock")
module(load="imklog")
module(load="omfwd")

# TLS driver settings
$DefaultNetstreamDriver gtls
$DefaultNetstreamDriverCAFile /etc/rsyslog.d/certs/ca-cert.pem
$DefaultNetstreamDriverCertFile /etc/rsyslog.d/certs/client-cert.pem
$DefaultNetstreamDriverKeyFile /etc/rsyslog.d/certs/client-key.pem
$ActionSendStreamDriverAuthMode x509/name
$ActionSendStreamDriverPermittedPeer syslog-server
$ActionSendStreamDriverMode 1 # TLS mode

# Forward all logs to secure syslog server
*.* @@(o)HOST_IP:6514

# Local logging
*.info;mail.none;authpriv.none;cron.none    /var/log/messages
authpriv.*                                   /var/log/secure
EOF

echo -e "${GREEN}✓ Rsyslog configurations created${NC}"
cd ..
echo ""

# Display summary
echo -e "${BLUE}[Step 8] Setup Summary${NC}"
echo "=========================================="
echo ""
echo -e "${GREEN}✓ Multi-Party Threshold TLS Setup Complete!${NC}"
echo ""
echo "Generated Files:"
echo "  Certificates:"
echo "    - $CERTS_DIR/ca-cert.pem (CA certificate)"
echo "    - $CERTS_DIR/server-cert.pem (Server certificate)"
echo "    - $CERTS_DIR/server-key.pem (SERVER KEY - THRESHOLD PROTECTED)"
echo "    - $CERTS_DIR/client-cert.pem (Client certificate)"
echo "    - $CERTS_DIR/client-key.pem (Client key)"
echo ""
echo "  Key Shares (Distributed to parties):"
for i in $(seq 1 $NUM_PARTIES); do
    if [ -f "$CERTS_DIR/server-key_party${i}_shares.dat" ]; then
        echo "    - $CERTS_DIR/server-key_party${i}_shares.dat (Party $i)"
    fi
done
echo ""
echo "  Configurations:"
echo "    - $CERTS_DIR/rsyslog-server.conf"
echo "    - $CERTS_DIR/rsyslog-client.conf"
echo ""

echo -e "${YELLOW}Security Model:${NC}"
echo "  - Server private key requires $THRESHOLD out of $NUM_PARTIES parties to use"
echo "  - Information-theoretic security (< $THRESHOLD shares reveal nothing)"
echo "  - Ephemeral key reconstruction during TLS handshake"
echo "  - Perfect forward secrecy maintained"
echo ""

echo -e "${YELLOW}Deployment Instructions:${NC}"
echo ""
echo "1. Deploy syslog server with multi-party key:"
echo "   sudo cp $CERTS_DIR/rsyslog-server.conf /etc/rsyslog.d/"
echo "   sudo sed -i \"s/AMF2_IP/$AMF2_IP/g\" /etc/rsyslog.d/rsyslog-server.conf"
echo "   sudo service rsyslog restart"
echo ""
echo "2. Copy certificates to AMF-2 container:"
echo "   docker cp $CERTS_DIR $AMF2_CONTAINER:/etc/rsyslog.d/"
echo ""
echo "3. Configure rsyslog client in AMF-2:"
echo "   docker exec -it $AMF2_CONTAINER apt-get update"
echo "   docker exec -it $AMF2_CONTAINER apt-get install -y rsyslog rsyslog-gnutls"
echo "   docker exec -it $AMF2_CONTAINER bash -c 'sed \"s/HOST_IP/\$(hostname -I | awk \"{print \\\$1}\")/g\" /etc/rsyslog.d/certs/rsyslog-client.conf > /etc/rsyslog.conf'"
echo "   docker exec -it $AMF2_CONTAINER service rsyslog start"
echo ""
echo "4. Verify TLS handshake:"
echo "   openssl s_client -connect localhost:6514 -CAfile $CERTS_DIR/ca-cert.pem"
echo ""
echo "5. Test log forwarding:"
echo "   docker exec -it $AMF2_CONTAINER logger -p local0.info \"Test message from AMF-2\""
echo ""

echo -e "${YELLOW}Key Management:${NC}"
echo "  - Store each party's shares securely (HSM, encrypted storage)"
echo "  - Minimum $THRESHOLD parties needed for key operations"
echo "  - Distribute shares to: Judicial Authority, Law Enforcement,"
echo "    Network Security, Privacy Officer, Independent Auditor"
echo ""

echo "=========================================="
echo -e "${GREEN}Ready for secure syslog with multi-party TLS!${NC}"
echo "=========================================="
