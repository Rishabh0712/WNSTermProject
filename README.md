# 5G Simulation with Secure Syslog Integration

**Author:** Rishabh Kumar (cs25resch04002)  
**Email:** kumarrishabh73@gmail.com  
**Institution:** WNS  
**Project Type:** Midterm Deliverable  
**Date:** November 2025

---

## Project Overview

This project implements a comprehensive 5G network simulation using OpenAirInterface (OAI) integrated with secure syslog logging for real-time network monitoring, security analysis, and compliance logging.

### Key Features
- ✓ Full 5G Standalone (SA) network deployment
- ✓ RF Simulator for gNB and UE
- ✓ Secure syslog integration with TLS/DTLS encryption
- ✓ Real-time event capture and logging
- ✓ Network performance monitoring
- ✓ Centralized log management

---

## Architecture

The project consists of the following components:

### 5G Core Network
- **AMF** (Access and Mobility Management Function)
- **SMF** (Session Management Function)
- **UPF** (User Plane Function)
- **MySQL** database for subscriber management

### Radio Access Network
- **gNB** (5G Base Station) - RF Simulator
- **NR-UE** (5G User Equipment) - RF Simulator

### Logging Infrastructure
- **Secure Syslog Server** with TLS encryption
- **Log aggregation and analysis**
- **Event monitoring and alerting**

---

## Technical Stack

- **5G Framework:** OpenAirInterface (OAI)
- **Containerization:** Docker & Docker Compose
- **Environment:** WSL2 (Ubuntu)
- **Logging Protocol:** Syslog over TLS/DTLS
- **Network Simulation:** RF Simulator
- **Database:** MySQL 8.0

---

## Project Structure

```
WNS/
├── openairinterface5g/          # OAI 5G source code
│   └── ci-scripts/
│       └── yaml_files/
│           └── 5g_rfsimulator/  # Docker compose configurations
├── midterm_presentation.pptx     # Project presentation
├── 5G_SETUP_SUMMARY.md          # Setup documentation
├── proposal.pdf                  # Initial proposal
├── midterm.pdf                   # Midterm deliverable template
└── README.md                     # This file
```

---

## Prerequisites

- Windows 10/11 with WSL2 enabled
- Ubuntu 24.04 LTS in WSL
- Docker and Docker Compose
- Git
- At least 8GB RAM
- 50GB free disk space

---

## Installation & Setup

### 1. Install WSL and Docker
```bash
# Install WSL (PowerShell as Administrator)
wsl --install

# After restart, install Docker in WSL
sudo apt-get update
sudo apt-get install -y docker.io docker-compose
sudo service docker start
sudo usermod -aG docker $USER
```

### 2. Clone OAI Repository
```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
```

### 3. Pull Docker Images
```bash
docker pull mysql:8.0
docker pull oaisoftwarealliance/oai-amf:v2.1.10
docker pull oaisoftwarealliance/oai-smf:v2.1.10
docker pull oaisoftwarealliance/oai-upf:v2.1.10
docker pull oaisoftwarealliance/trf-gen-cn5g:focal
docker pull oaisoftwarealliance/oai-gnb:develop
docker pull oaisoftwarealliance/oai-nr-ue:develop
```

### 4. Configure Networks
```bash
# Create Docker networks
docker network create --driver=bridge --subnet=192.168.71.128/26 \
  --opt com.docker.network.bridge.name=rfsim5g-public \
  rfsim5g-oai-public-net

docker network create --driver=bridge --subnet=192.168.72.128/26 \
  --opt com.docker.network.bridge.name=rfsim5g-traffic \
  rfsim5g-oai-traffic-net
```

### 5. Deploy 5G Network
```bash
# Deploy core network
sudo docker-compose up -d mysql oai-amf oai-smf oai-upf oai-ext-dn

# Deploy gNB
sudo docker-compose up -d oai-gnb

# Deploy UE
sudo docker-compose up -d oai-nr-ue
```

---

## Verification

### Check Container Status
```bash
sudo docker ps
```

All containers should show "healthy" status.

### Verify gNB Connection
```bash
sudo docker logs rfsim5g-oai-amf | grep "Connected"
```

Expected: `Status: Connected | Global Id: 0x0E00 | gNB Name: gnb-rfsim`

### Verify UE Registration
```bash
sudo docker logs rfsim5g-oai-amf | grep "5GMM-REGISTERED"
```

Expected: UE with IMSI `208990100001100` in `5GMM-REGISTERED` state

### Test Connectivity
```bash
sudo docker exec rfsim5g-oai-nr-ue ping -I oaitun_ue1 -c 5 192.168.72.135
```

Expected: 0% packet loss

---

## Network Configuration

### Public Network (Control Plane)
- **Subnet:** 192.168.71.128/26
- **Purpose:** SCTP, NGAP, PFCP signaling
- **Components:**
  - MySQL: 192.168.71.131
  - AMF: 192.168.71.132
  - SMF: 192.168.71.133
  - UPF: 192.168.71.134
  - gNB: 192.168.71.140
  - UE: 192.168.71.150

### Traffic Network (User Plane)
- **Subnet:** 192.168.72.128/26
- **Purpose:** GTP-U tunneling, user data
- **Components:**
  - UPF: 192.168.72.134
  - External DN: 192.168.72.135

---

## Secure Syslog Integration

### Components
1. **TLS-enabled Syslog Server**
2. **Certificate Management**
3. **Log Forwarding Configuration**
4. **Event Parser and Formatter**

### Log Sources
- AMF events (registration, authentication)
- SMF events (session management)
- UPF events (data plane activities)
- gNB events (radio connection management)

### Security Features
- TLS/DTLS encryption
- Certificate-based authentication
- Secure log transmission
- Integrity verification

---

## Monitoring & Analysis

### Key Metrics
- UE registration success rate
- Session establishment time
- Handover performance
- Throughput and latency
- Error rates and failures

### Analysis Tools
- Wireshark for protocol analysis
- tcpdump for packet capture
- Custom log parsers
- Real-time dashboards

---

## Captured Protocols

The setup captures and analyzes:
- **NGAP** - NG Application Protocol
- **NAS** - Non-Access Stratum
- **PFCP** - Packet Forwarding Control Protocol
- **GTP-U** - GPRS Tunneling Protocol (User Plane)
- **SCTP** - Stream Control Transmission Protocol

---

## Troubleshooting

### Container not starting
```bash
sudo docker logs <container-name>
sudo docker-compose restart <service-name>
```

### Network issues
```bash
sudo docker network inspect rfsim5g-oai-public-net
sudo docker network inspect rfsim5g-oai-traffic-net
```

### UE not registering
```bash
# Check AMF logs
sudo docker logs rfsim5g-oai-amf

# Check gNB logs
sudo docker logs rfsim5g-oai-gnb

# Check UE logs
sudo docker logs rfsim5g-oai-nr-ue
```

---

## Results

### Successfully Achieved
✓ Full 5G SA network deployed  
✓ gNB connected to AMF (Status: Connected)  
✓ UE registered (State: 5GMM-REGISTERED)  
✓ IP address allocated to UE (12.1.1.2)  
✓ End-to-end connectivity verified (0% packet loss)  
✓ All protocols functioning correctly  

---

## Future Enhancements

- [ ] ML-based anomaly detection
- [ ] Interactive monitoring dashboard
- [ ] Multi-UE scenarios
- [ ] SIEM integration
- [ ] Cloud deployment
- [ ] Extended protocol coverage
- [ ] Performance optimization

---

## References

1. **3GPP Specifications**
   - TS 23.501: System Architecture for 5G
   - TS 38.300: NR Overall Description

2. **OpenAirInterface**
   - OAI GitLab: https://gitlab.eurecom.fr/oai/openairinterface5g
   - OAI Documentation: https://openairinterface.org/

3. **Syslog Specifications**
   - RFC 5424: The Syslog Protocol
   - RFC 5425: TLS Transport Mapping for Syslog

4. **Docker**
   - Docker Documentation: https://docs.docker.com/

---

## License

This is an academic project for educational purposes.

---

## Contact

**Rishabh Kumar (cs25resch04002)**  
Email: kumarrishabh73@gmail.com

---

## Acknowledgments

- OpenAirInterface Software Alliance
- WNS Institution
- 3GPP for 5G specifications

---

**Last Updated:** November 10, 2025
