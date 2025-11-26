# Secure Syslog Integration with AMF-2

## Overview
This project implements secure syslog forwarding from a second AMF instance (AMF-2) to a centralized syslog server using TLS encryption.

## Architecture
- **Syslog Server**: Running on WSL host (Ubuntu 24.04) with rsyslog + rsyslog-gnutls
- **AMF-2**: Second AMF instance (192.168.71.136) configured to forward logs via TLS
- **Encryption**: TLS 1.2+ with x509 certificate authentication
- **Port**: 6514 (secure syslog over TCP)

## Components

### 1. TLS Certificates (in \certs/\ directory)
- \ca-cert.pem\ / \ca-key.pem\ - Certificate Authority
- \server-cert.pem\ / \server-key.pem\ - Syslog server certificates
- \client-cert.pem\ / \client-key.pem\ - AMF-2 client certificates

### 2. Rsyslog Server Configuration (\syslog-server.conf\)
- Listens on port 6514 with TLS
- Accepts connections from clients with CN=amf-2
- Stores logs in \/var/log/amf2/amf2-YYYY-MM-DD.log
### 3. AMF-2 Configuration
- **Container**: rfsim5g-oai-amf-2
- **IP Address**: 192.168.71.136
- **Config**: \openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator/amf2_config.yaml- **Entrypoint**: \mf2_entrypoint.sh\ (installs rsyslog, configures forwarding)

## Deployment Steps

### 1. Generate TLS Certificates
\\ash
# Already generated in certs/ directory
# CA certificate, server certificate, and client certificate for AMF-2
\
### 2. Configure Rsyslog Server
\\ash
sudo cp rsyslog-server.conf /etc/rsyslog.d/99-amf-secure.conf
sudo mkdir -p /var/log/amf2
sudo chown syslog:adm /var/log/amf2
sudo systemctl restart rsyslog
\
### 3. Deploy AMF-2
\\ash
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
sudo docker-compose up -d oai-amf-2
\
## Verification

### Check Syslog Server
\\ash
# Verify rsyslog is listening on port 6514
sudo ss -tlnp | grep 6514

# Check received logs
sudo ls -lh /var/log/amf2/
sudo tail -f /var/log/amf2/amf2-*.log
\
### Check AMF-2 Container
\\ash
# Verify AMF-2 is running
sudo docker ps | grep amf-2

# Check rsyslog inside container
sudo docker exec rfsim5g-oai-amf-2 ps aux | grep rsyslog

# View rsyslog configuration
sudo docker exec rfsim5g-oai-amf-2 cat /etc/rsyslog.d/50-amf2-forward.conf
\
## Security Features
-  TLS 1.2+ encryption for log transmission
-  x509 certificate-based mutual authentication
-  Certificate CN validation (only amf-2 client accepted)
-  Separate log directory with proper permissions
-  Private keys protected with 600 permissions

## Log Format
Logs are stored with timestamp, container ID, and log message:
\2025-11-10T18:35:30+00:00 e76a5742b8b8 rsyslogd: [origin software=
