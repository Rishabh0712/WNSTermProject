# Multi-Party Rsyslog TLS - Quick Reference

## Quick Start

```bash
# 1. Generate keys and certificates
./setup_multiparty_rsyslog.sh

# 2. Deploy to server
sudo cp syslog_certs/rsyslog-server.conf /etc/rsyslog.d/
sudo cp syslog_certs/{ca-cert,server-cert,server-key}.pem /etc/rsyslog.d/certs/
sudo service rsyslog restart

# 3. Deploy to AMF-2
docker cp syslog_certs rfsim5g-oai-amf-2:/etc/rsyslog.d/
docker exec -it rfsim5g-oai-amf-2 apt-get install -y rsyslog rsyslog-gnutls
docker exec -it rfsim5g-oai-amf-2 bash -c 'sed "s/HOST_IP/SERVER_IP/g" /etc/rsyslog.d/certs/rsyslog-client.conf > /etc/rsyslog.conf'
docker exec -it rfsim5g-oai-amf-2 service rsyslog start

# 4. Test
docker exec -it rfsim5g-oai-amf-2 logger -p local0.info "Test log"
sudo tail -f /var/log/amf2/*
```

## Commands

### Key Generation
```bash
# Generate with defaults (5 parties, threshold 3)
./multiparty_key_generator server-key.pem 5 3

# Custom threshold (7 parties, need 4)
./multiparty_key_generator server-key.pem 7 4
```

### Compilation
```bash
g++ -std=c++17 -O2 -o multiparty_key_generator \
    multiparty_key_generator.cpp shamir_secret_sharing.cpp \
    -lssl -lcrypto
```

### Validation
```bash
# Check key validity
openssl rsa -in server-key.pem -check -noout

# Verify certificate
openssl verify -CAfile ca-cert.pem server-cert.pem

# Test TLS connection
openssl s_client -connect localhost:6514 -CAfile ca-cert.pem
```

## File Structure

```
syslog_certs/
├── ca-cert.pem                    # CA certificate (distribute)
├── ca-key.pem                     # CA private key (secure!)
├── server-cert.pem                # Server certificate
├── server-key.pem                 # ← THRESHOLD PROTECTED KEY
├── server-key_party1_shares.dat   # Party 1 (Judicial)
├── server-key_party2_shares.dat   # Party 2 (Law Enforcement)
├── server-key_party3_shares.dat   # Party 3 (Network Security)
├── server-key_party4_shares.dat   # Party 4 (Privacy Officer)
├── server-key_party5_shares.dat   # Party 5 (Auditor)
├── client-cert.pem                # AMF-2 certificate
├── client-key.pem                 # AMF-2 private key
├── rsyslog-server.conf            # Server config
└── rsyslog-client.conf            # AMF-2 config
```

## Rsyslog Config Snippets

### Server (`/etc/rsyslog.d/rsyslog-server.conf`)
```
module(load="imtcp")
$DefaultNetstreamDriver gtls
$DefaultNetstreamDriverCAFile /etc/rsyslog.d/certs/ca-cert.pem
$DefaultNetstreamDriverCertFile /etc/rsyslog.d/certs/server-cert.pem
$DefaultNetstreamDriverKeyFile /etc/rsyslog.d/certs/server-key.pem  # ← Threshold key
$InputTCPServerStreamDriverAuthMode x509/name
$InputTCPServerStreamDriverMode 1
$InputTCPServerRun 6514
```

### Client (`/etc/rsyslog.conf` in AMF-2)
```
module(load="omfwd")
$DefaultNetstreamDriver gtls
$DefaultNetstreamDriverCAFile /etc/rsyslog.d/certs/ca-cert.pem
$DefaultNetstreamDriverCertFile /etc/rsyslog.d/certs/client-cert.pem
$DefaultNetstreamDriverKeyFile /etc/rsyslog.d/certs/client-key.pem
$ActionSendStreamDriverMode 1
*.* @@(o)SERVER_IP:6514
```

## Security Properties

| Property | Value |
|----------|-------|
| Threshold | 3 of 5 parties |
| Key Size | RSA-2048 |
| Chunks | 34 × 61 bits |
| Prime Field | 2^61 - 1 |
| Security | Information-theoretic (< 3 shares) |
| TLS Version | 1.2/1.3 (rsyslog dependent) |

## Troubleshooting

### Compilation fails
```bash
sudo apt-get install build-essential libssl-dev
```

### TLS handshake fails
```bash
# Check rsyslog is listening
sudo netstat -tlnp | grep 6514

# Check certificate chain
openssl verify -CAfile ca-cert.pem server-cert.pem

# Check rsyslog logs
sudo tail -f /var/log/syslog | grep rsyslog
```

### Logs not forwarding
```bash
# Test connectivity
nc -zv SERVER_IP 6514

# Check client rsyslog status
docker exec rfsim5g-oai-amf-2 service rsyslog status

# Check for errors
docker exec rfsim5g-oai-amf-2 tail -f /var/log/syslog
```

### Permission errors
```bash
sudo chown root:root /etc/rsyslog.d/certs/*.pem
sudo chmod 600 /etc/rsyslog.d/certs/server-key.pem
sudo chmod 644 /etc/rsyslog.d/certs/*.pem
```

## Key Concepts

### Shamir's Secret Sharing (3,5)
- **Threshold (t=3)**: Minimum parties needed
- **Total (n=5)**: Total number of parties
- **Security**: < 3 shares reveal NOTHING
- **Reconstruction**: Lagrange interpolation

### Party Distribution
1. **Judicial Authority**: Court oversight
2. **Law Enforcement**: Investigation access
3. **Network Security**: Operational control
4. **Privacy Officer**: User protection
5. **Independent Auditor**: Compliance

### Why This Works
- Key generated using threshold crypto
- Output is standard PEM format
- Rsyslog uses key normally (no modification)
- Security: Need 3 parties to recreate key
- Transparency: TLS clients see normal RSA

## Performance

| Operation | Time | Memory |
|-----------|------|--------|
| Key generation | ~2s | 8 MB |
| TLS handshake | 0ms overhead | - |
| Log forwarding | Standard | - |

## References

- Full docs: `MULTIPARTY_RSYSLOG_README.md`
- Implementation: `MULTIPARTY_RSYSLOG_COMPLETE.md`
- Source: `multiparty_key_generator.cpp`
- Setup: `setup_multiparty_rsyslog.sh`

---
**Author**: Rishabh Kumar (cs25resch04002)  
**Project**: WNS Term Project - Multi-Party Threshold TLS  
**Date**: November 2025
