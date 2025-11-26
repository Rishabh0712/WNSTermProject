# Secure Syslog TLS Handshake Capture

## Overview

This directory contains scripts for setting up and capturing TLS-secured syslog communication between AMF-2 and a syslog server, following RFC 5425 standards.

## Scripts

### 1. setup_secure_syslog.sh
Sets up TLS certificates and rsyslog configuration for secure syslog forwarding.

**Features:**
- Generates CA, server, and client certificates (RSA 4096-bit)
- Creates rsyslog client configuration for AMF-2
- Creates rsyslog server configuration for log collection
- Provides step-by-step setup instructions

**Usage:**
```bash
./setup_secure_syslog.sh
```

**Output:**
- `syslog_certs/` directory with:
  - CA certificate and key
  - Server certificate and key
  - Client certificate and key
  - Rsyslog configuration files

### 2. capture_syslog_tls.sh
Captures and analyzes TLS handshake between AMF-2 and syslog server.

**Features:**
- Monitors TCP port 6514 (RFC 5425 standard)
- Captures TLS handshake messages (Client Hello, Server Hello, Certificates)
- Analyzes TLS version and cipher suites
- Generates detailed PCAP files and analysis reports

**Usage:**
```bash
./capture_syslog_tls.sh
```

**Options (edit script to modify):**
- `CAPTURE_DURATION`: Duration in seconds (default: 30)
- `SYSLOG_TLS_PORT`: Port to monitor (default: 6514)
- `OUTPUT_DIR`: Output directory (default: ./syslog_tls_captures)

**Output:**
- PCAP file for Wireshark analysis
- Log file with capture details
- Analysis report with TLS handshake breakdown

## Prerequisites

1. **tcpdump** (packet capture):
   ```bash
   sudo apt-get install tcpdump
   ```

2. **tshark** (packet analysis):
   ```bash
   sudo apt-get install tshark
   ```

3. **OpenSSL** (certificate generation):
   ```bash
   sudo apt-get install openssl
   ```

4. **rsyslog-gnutls** (TLS support for rsyslog):
   ```bash
   sudo apt-get install rsyslog rsyslog-gnutls
   ```

## Workflow

### Step 1: Setup Secure Syslog
```bash
./setup_secure_syslog.sh
```

This will:
- Check AMF-2 container status
- Generate TLS certificates (if not already present)
- Create rsyslog configuration files
- Display setup instructions

### Step 2: Configure AMF-2 (Manual)

Follow the instructions displayed by `setup_secure_syslog.sh`:

```bash
# Copy certificates to AMF-2
docker cp syslog_certs rfsim5g-oai-amf-2:/etc/rsyslog.d/

# Install rsyslog-gnutls
docker exec -it rfsim5g-oai-amf-2 apt-get update
docker exec -it rfsim5g-oai-amf-2 apt-get install -y rsyslog rsyslog-gnutls

# Configure and start rsyslog
docker exec -it rfsim5g-oai-amf-2 bash -c 'sed "s/HOST_IP/$(hostname -I | awk "{print \$1}")/g" /etc/rsyslog.d/certs/rsyslog-client.conf > /etc/rsyslog.conf'
docker exec -it rfsim5g-oai-amf-2 service rsyslog start
```

### Step 3: Configure Syslog Server (Host or Container)

```bash
# Copy server configuration
sudo cp syslog_certs/rsyslog-server.conf /etc/rsyslog.d/

# Copy certificates
sudo cp syslog_certs/*.pem /etc/rsyslog.d/certs/

# Update with AMF-2 IP
AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' rfsim5g-oai-amf-2)
sudo sed -i "s/AMF2_IP/$AMF2_IP/g" /etc/rsyslog.d/rsyslog-server.conf

# Create log directory
sudo mkdir -p /var/log/amf2

# Restart rsyslog
sudo service rsyslog restart
```

### Step 4: Capture TLS Handshake

```bash
./capture_syslog_tls.sh
```

This will:
- Verify AMF-2 is running
- Detect the network interface
- Capture packets on port 6514 for 30 seconds
- Analyze TLS handshake messages
- Generate PCAP and report files

### Step 5: Analyze Results

**View with Wireshark:**
```bash
wireshark syslog_tls_captures/syslog_tls_TIMESTAMP.pcap
```

**Analyze with tshark:**
```bash
# View all TLS handshake messages
tshark -r syslog_tls_captures/syslog_tls_TIMESTAMP.pcap -Y tls.handshake

# Filter specific handshake types
tshark -r PCAP_FILE -Y 'tls.handshake.type == 1'  # Client Hello
tshark -r PCAP_FILE -Y 'tls.handshake.type == 2'  # Server Hello
tshark -r PCAP_FILE -Y 'tls.handshake.type == 11' # Certificate
```

## TLS Handshake Messages

The capture script detects the following RFC 5425 TLS handshake types:

| Type | Message | Description |
|------|---------|-------------|
| 1 | Client Hello | AMF-2 initiates TLS connection |
| 2 | Server Hello | Syslog server responds with chosen cipher |
| 11 | Certificate | Server sends X.509 certificate |
| 12 | Server Key Exchange | Server sends key exchange parameters |
| 13 | Certificate Request | Server requests client certificate |
| 14 | Server Hello Done | Server finished its hello phase |
| 15 | Certificate Verify | Client proves certificate ownership |
| 16 | Client Key Exchange | Client sends key exchange parameters |
| 20 | Finished | Handshake completion |

## Troubleshooting

### No Packets Captured

**Possible causes:**
1. AMF-2 not sending syslog messages
   - Check: `docker logs rfsim5g-oai-amf-2 | grep syslog`
   
2. Wrong network interface
   - Manually set `DOCKER_INTERFACE` in script
   - Check: `ip addr` or `ifconfig`
   
3. Rsyslog not configured
   - Verify: `docker exec rfsim5g-oai-amf-2 service rsyslog status`

### TLS Handshake Failed

**Possible causes:**
1. Certificate mismatch
   - Verify CN matches in certificates
   - Check: `openssl x509 -in cert.pem -text -noout`
   
2. Port not listening
   - Check: `sudo netstat -tlnp | grep 6514`
   
3. Firewall blocking
   - Check: `sudo iptables -L -n | grep 6514`

### Permission Denied

Run with sudo:
```bash
sudo ./capture_syslog_tls.sh
```

## Output Files

### Generated by setup_secure_syslog.sh:
```
syslog_certs/
├── ca-cert.pem           # CA certificate
├── ca-key.pem            # CA private key
├── server-cert.pem       # Server certificate
├── server-key.pem        # Server private key
├── client-cert.pem       # Client certificate (for AMF-2)
├── client-key.pem        # Client private key
├── rsyslog-client.conf   # AMF-2 rsyslog configuration
└── rsyslog-server.conf   # Syslog server configuration
```

### Generated by capture_syslog_tls.sh:
```
syslog_tls_captures/
├── syslog_tls_TIMESTAMP.pcap           # Packet capture
├── capture_log_TIMESTAMP.txt           # Capture log
└── tls_analysis_TIMESTAMP.txt          # Analysis report
```

## Security Considerations

1. **Certificate Validation**: Uses x509/name authentication mode
2. **Mutual TLS**: Both client and server authenticate each other
3. **Strong Encryption**: RSA 4096-bit keys
4. **RFC 5425 Compliance**: Follows syslog TLS transport standard

## References

- **RFC 5425**: Transport Layer Security (TLS) Transport Mapping for Syslog
- **RFC 5424**: The Syslog Protocol
- **RFC 5246**: The Transport Layer Security (TLS) Protocol Version 1.2
- **rsyslog documentation**: https://www.rsyslog.com/doc/v8-stable/configuration/modules/omfwd.html

## Project Context

Part of **CS5553 Wireless Networks and Security** Term Project:
"Privacy-Preserving Mobile Phone Localization with Cryptographic Authorization in 5G Networks"

This demonstrates the secure transport layer (TLS) used for syslog forwarding from AMF-2, which is a component of the secure logging infrastructure for 5G network functions.

---

**Author**: Rishabh Kumar (cs25resch04002)  
**Date**: November 12, 2025  
**GitHub**: https://github.com/Rishabh0712/WNSTermProject
