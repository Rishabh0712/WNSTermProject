#!/bin/bash

# Script: Setup Secure Syslog for AMF-2
# Purpose: Configure AMF-2 with rsyslog-gnutls for TLS-secured syslog forwarding
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 12, 2025
# Reference: RFC 5425 - Transport Layer Security (TLS) Transport Mapping for Syslog

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "=========================================="
echo "Secure Syslog Setup for AMF-2"
echo "RFC 5425 TLS Transport"
echo "=========================================="
echo ""

AMF2_CONTAINER="rfsim5g-oai-amf-2"
SYSLOG_PORT=6514
CERTS_DIR="./syslog_certs"

# Check if AMF-2 is running
echo -e "${BLUE}[Step 1] Checking AMF-2 Container${NC}"
echo "=========================================="
if docker ps | grep -q "$AMF2_CONTAINER"; then
    echo -e "${GREEN}✓ AMF-2 is running${NC}"
    AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$AMF2_CONTAINER")
    echo -e "  AMF-2 IP: ${YELLOW}$AMF2_IP${NC}"
else
    echo -e "${RED}✗ AMF-2 is not running${NC}"
    echo "Starting AMF-2 from docker-compose..."
    cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
    docker-compose start oai-amf-2
    sleep 5
    AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$AMF2_CONTAINER" 2>/dev/null)
    if [ -z "$AMF2_IP" ]; then
        echo -e "${RED}Failed to start AMF-2${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ AMF-2 started${NC}"
fi
echo ""

# Create certificates directory
echo -e "${BLUE}[Step 2] Setting up TLS Certificates${NC}"
echo "=========================================="
mkdir -p "$CERTS_DIR"
cd "$CERTS_DIR"

# Generate CA certificate
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

# Generate server certificate
if [ ! -f "server-cert.pem" ]; then
    echo "Generating server certificate..."
    openssl req -newkey rsa:4096 -nodes \
        -keyout server-key.pem \
        -out server-req.pem \
        -subj "/C=IN/ST=Telangana/L=Hyderabad/O=WNS-Project/OU=5G-Security/CN=syslog-server" \
        2>/dev/null
    
    openssl x509 -req -in server-req.pem -days 365 \
        -CA ca-cert.pem -CAkey ca-key.pem -CAcreateserial \
        -out server-cert.pem \
        2>/dev/null
    echo -e "${GREEN}✓ Server certificate created${NC}"
else
    echo -e "${YELLOW}Server certificate already exists${NC}"
fi

# Generate client certificate
if [ ! -f "client-cert.pem" ]; then
    echo "Generating client certificate..."
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

cd ..
echo ""

# Create rsyslog configuration
echo -e "${BLUE}[Step 3] Creating Rsyslog Configuration${NC}"
echo "=========================================="

cat > "${CERTS_DIR}/rsyslog-client.conf" << 'EOF'
# AMF-2 Rsyslog Client Configuration
# RFC 5425 - TLS Transport for Syslog

# Load modules
module(load="imuxsock")    # Local system logging support
module(load="imklog")      # Kernel logging support
module(load="omfwd")       # Forwarding support

# Global directives
$ActionFileDefaultTemplate RSYSLOG_TraditionalFileFormat
$FileOwner root
$FileGroup adm
$FileCreateMode 0640
$DirCreateMode 0755
$Umask 0022

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
mail.*                                       -/var/log/maillog
cron.*                                       /var/log/cron
*.emerg                                      :omusrmsg:*
EOF

echo -e "${GREEN}✓ Client configuration created${NC}"
echo ""

# Create syslog server configuration
cat > "${CERTS_DIR}/rsyslog-server.conf" << 'EOF'
# Syslog Server Configuration
# RFC 5425 - TLS Transport for Syslog

# Load modules
module(load="imtcp")
module(load="imuxsock")

# TLS driver settings
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

echo -e "${GREEN}✓ Server configuration created${NC}"
echo ""

# Display setup instructions
echo -e "${BLUE}[Step 4] Setup Instructions${NC}"
echo "=========================================="
echo ""
echo -e "${YELLOW}To enable secure syslog on AMF-2:${NC}"
echo ""
echo "1. Copy certificates to AMF-2 container:"
echo "   docker cp $CERTS_DIR $AMF2_CONTAINER:/etc/rsyslog.d/"
echo ""
echo "2. Install rsyslog-gnutls in AMF-2:"
echo "   docker exec -it $AMF2_CONTAINER apt-get update"
echo "   docker exec -it $AMF2_CONTAINER apt-get install -y rsyslog rsyslog-gnutls"
echo ""
echo "3. Configure rsyslog client in AMF-2:"
echo "   docker exec -it $AMF2_CONTAINER bash -c 'sed \"s/HOST_IP/\$(hostname -I | awk \"{print \\\$1}\")/g\" /etc/rsyslog.d/certs/rsyslog-client.conf > /etc/rsyslog.conf'"
echo ""
echo "4. Start rsyslog service:"
echo "   docker exec -it $AMF2_CONTAINER service rsyslog start"
echo ""
echo "5. On syslog server (host or separate container):"
echo "   sudo cp $CERTS_DIR/rsyslog-server.conf /etc/rsyslog.d/"
echo "   sudo sed -i \"s/AMF2_IP/$AMF2_IP/g\" /etc/rsyslog.d/rsyslog-server.conf"
echo "   sudo service rsyslog restart"
echo ""
echo -e "${YELLOW}To capture TLS handshake:${NC}"
echo "   ./capture_syslog_tls.sh"
echo ""
echo "=========================================="
echo -e "${GREEN}Setup files ready in: $CERTS_DIR/${NC}"
echo "=========================================="
echo ""
echo "Certificate files:"
ls -lh "$CERTS_DIR"/*.pem 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "Configuration files:"
ls -lh "$CERTS_DIR"/*.conf 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
