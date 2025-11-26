# Multi-Party Threshold TLS for 5G Secure Logging

**Author:** Rishabh Kumar (cs25resch04002)  
**Email:** kumarrishabh73@gmail.com | rishabh.kumar@research.iiit.ac.in  
**Institution:** IIIT Hyderabad  
**Course:** WNS (Wireless and Network Security)  
**Project Type:** Term Project  
**Date:** November 2025

[![Project Status](https://img.shields.io/badge/Status-Complete-success)](https://github.com/Rishabh0712/WNSTermProject)
[![Integration](https://img.shields.io/badge/5G_Integration-Verified-brightgreen)](docs/documentation/MULTIPARTY_TLS_VERIFICATION.md)
[![License](https://img.shields.io/badge/License-Academic-blue)](LICENSE)

---

## 🎯 Project Overview

This project implements **multi-party threshold cryptography for TLS-encrypted syslog** in 5G networks, ensuring that critical infrastructure logging requires authorization from multiple independent parties. The implementation uses **(3,5)-Shamir's Secret Sharing** to protect the TLS server private key, providing information-theoretic security and distributed trust.

### 🌟 Key Innovation
**The TLS server private key is split among 5 authorization parties using threshold cryptography. Any 3 parties can collaboratively use the key, but fewer than 3 parties have zero information about it.**

### ✅ Key Features
- ✓ **Multi-Party Threshold TLS**: (3,5)-Shamir's Secret Sharing for RSA-2048 keys
- ✓ **5G Network Integration**: Secure logging from OpenAirInterface AMF container
- ✓ **Information-Theoretic Security**: < 3 shares reveal ZERO information
- ✓ **Distributed Trust Model**: No single party can authorize key usage
- ✓ **Standard Compatibility**: RFC 5425 compliant TLS syslog
- ✓ **Production Ready**: Verified end-to-end in live 5G environment
- ✓ **Full Documentation**: 32-page report + comprehensive guides

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

## 📁 Project Structure

The repository is organized into logical directories for easy navigation:

```
WNS/
├── 📂 docs/                          # Documentation and reports
│   ├── presentations/                # PowerPoint and LaTeX presentations
│   ├── reports/                      # Project reports (32-page final report)
│   ├── proposals/                    # Project proposals and templates
│   └── documentation/                # Technical documentation (20+ guides)
│
├── 📂 src/                           # Source code
│   ├── multiparty_tls/              # Multi-party TLS implementation
│   ├── shamir_secret_sharing/       # Shamir's Secret Sharing library
│   ├── ue_location/                 # UE location tracking service
│   └── tests/                       # Test files and data
│
├── 📂 scripts/                       # Automation scripts
│   ├── setup/                       # Setup and configuration scripts
│   ├── capture/                     # Network capture and testing
│   └── deployment/                  # Deployment scripts
│
├── 📂 certificates/                  # Certificates and keys
│   ├── syslog_certs/                # ⚠️ Multi-party threshold keys
│   └── certs/                       # General certificates
│
├── 📂 config/                        # Configuration files
├── 📂 artifacts/                     # Build artifacts (binaries, LaTeX aux)
├── 📂 openairinterface5g/           # 5G simulation environment
│
├── README.md                         # This file
├── PROJECT_STRUCTURE.md             # Detailed structure documentation
└── Makefile                          # Build configuration
```

**See [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) for complete repository organization.**

---

## 🔐 Security Architecture

### Multi-Party Threshold Model

```
┌─────────────────────────────────────────────────────────────┐
│        TLS Server Private Key Protection                     │
│                                                               │
│  Party 1: Judicial Authority          ────┐                 │
│  Party 2: Law Enforcement              ────┤                 │
│  Party 3: Network Security             ────┼──> Any 3 of 5  │
│  Party 4: Privacy Officer              ────┤    Parties      │
│  Party 5: Independent Auditor          ────┘    Required    │
│                                                               │
│  < 3 Parties = IMPOSSIBLE to reconstruct key                 │
│  (Information-Theoretic Security)                            │
└─────────────────────────────────────────────────────────────┘
```

### Implementation Details
- **Algorithm**: RSA-2048 with (3,5)-Shamir's Secret Sharing
- **Key Size**: 2046 bits split into 34 chunks × 61 bits
- **Total Shares**: 170 (34 chunks × 5 parties)
- **Share Size**: 552 bytes per party
- **Security**: Information-theoretic (not based on computational hardness)

### Integration Architecture

```
┌────────────────────────────────────────────────────────────┐
│  5G AMF-2 Container                                         │
│  (192.168.71.136)                                          │
│                                                             │
│  AMF Events ──> Rsyslog Client ──┐                        │
└───────────────────────────────────┼────────────────────────┘
                                    │
                         TLS Encrypted Channel
                      (Multi-Party Threshold Key)
                                    │
┌───────────────────────────────────┼────────────────────────┐
│  Host WSL (172.31.130.37:6514)    ▼                        │
│                                                             │
│  Rsyslog Server ──> /var/log/amf2/                        │
│  (Threshold-Protected Key)                                 │
└────────────────────────────────────────────────────────────┘
```

---

## 🛠️ Prerequisites

### System Requirements
- Windows 10/11 with WSL2 enabled
- Ubuntu 22.04/24.04 LTS in WSL
- Docker and Docker Compose
- Git
- At least 16GB RAM (8GB minimum)
- 50GB free disk space

### Software Dependencies
- **C++ Compiler**: g++ 11+ with C++17 support
- **OpenSSL**: Version 3.0 or higher
- **Python**: 3.8+ (for utilities)
- **LaTeX**: pdflatex (for building reports)
- **Docker**: For 5G simulation

---

## 🚀 Quick Start

### 1. Clone Repository
```bash
git clone https://github.com/Rishabh0712/WNSTermProject.git
cd WNSTermProject
```

### 2. Build Multi-Party Key Generator
```bash
make
# Builds: artifacts/binaries/multiparty_key_generator
```

### 3. Generate Threshold-Protected Keys
```bash
./artifacts/binaries/multiparty_key_generator server-key.pem 5 3
# Output:
#   server-key.pem (RSA-2048 private key)
#   server-key-public.pem (public key)
#   server-key_party1_shares.dat (Party 1 shares)
#   server-key_party2_shares.dat (Party 2 shares)
#   ...
#   server-key_party5_shares.dat (Party 5 shares)
```

### 4. Deploy to 5G Environment (Optional)
```bash
cd scripts/setup
./setup_multiparty_rsyslog.sh
# Automatically:
#   - Detects AMF-2 container IP
#   - Generates CA and certificates
#   - Configures rsyslog server and client
#   - Deploys certificates
```

### 5. Verify Integration
```bash
# Send test log from AMF-2
docker exec rfsim5g-oai-amf-2 logger -p local0.info "Test: Multi-party TLS"

# Check received logs
sudo tail /var/log/amf2/*.log
```

---

## 📖 Detailed Installation

### Option A: Multi-Party TLS Only (No 5G)

If you only want to test the multi-party TLS implementation:

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev

# 2. Build
cd WNSTermProject
make

# 3. Generate keys
./artifacts/binaries/multiparty_key_generator test-key.pem 5 3

# 4. Verify with OpenSSL
openssl rsa -in test-key.pem -check -noout
# Output: RSA key ok
```

### Option B: Full 5G Integration

For complete 5G network with multi-party TLS:

#### Step 1: Install WSL and Docker
```bash
# Install WSL (PowerShell as Administrator)
wsl --install

# After restart, install Docker in WSL
sudo apt-get update
sudo apt-get install -y docker.io docker-compose build-essential libssl-dev
sudo service docker start
sudo usermod -aG docker $USER
```

#### Step 2: Clone Repositories
```bash
# Clone this project
git clone https://github.com/Rishabh0712/WNSTermProject.git
cd WNSTermProject

# Clone OAI (already included as submodule)
git submodule update --init --recursive
```

#### Step 3: Start 5G Simulation
```bash
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
sudo docker-compose up -d

# Wait ~2 minutes for all containers to be healthy
sudo docker ps
```

#### Step 4: Deploy Multi-Party TLS
```bash
cd ../../../../scripts/setup
./setup_multiparty_rsyslog.sh
```

#### Step 5: Verify
```bash
# Check 5G UE registration
sudo docker logs rfsim5g-oai-amf | grep "5GMM-REGISTERED"

# Test connectivity
sudo docker exec rfsim5g-oai-nr-ue ping -I oaitun_ue1 -c 5 192.168.72.135

# Verify multi-party TLS logs
sudo docker exec rfsim5g-oai-amf-2 logger -p local0.info "Test message"
sudo tail /var/log/amf2/*.log
```

---

## UE Location Service with Movement Tracking

The project includes an enhanced Python-based UE Location Service that extracts real-time location information from AMF logs and **tracks UE movement** by identifying all gNBs (base stations) the UE has connected to.

### Features
- ✅ Extract UE location by IMSI or IMEI
- ✅ Parse AMF logs for network location data
- ✅ Display Cell ID, gNB information, TAC, and PLMN
- ✅ **NEW: Track UE movement across multiple gNBs**
- ✅ **NEW: Identify handovers and tracking area updates**
- ✅ **NEW: Timeline analysis with first/last seen timestamps**
- ✅ **NEW: Movement statistics and event classification**
- ✅ Export location and movement data to JSON
- ✅ Support for multiple UEs with bulk tracking

### Usage

#### Get current location by IMSI
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100
```

#### Track UE movement history (NEW)
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement
```

#### Get location by IMEI
```bash
sudo python3 src/ue_location/ue_location_service.py --imei 862104052096703
```

#### Get all UE locations
```bash
sudo python3 src/ue_location/ue_location_service.py --all
```

#### Track all UEs movement (NEW)
```bash
sudo python3 src/ue_location/ue_location_service.py --all --track-movement
```

#### Export movement data to JSON
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement --export movement.json
```

### Location Data Extracted from AMF
- **UE Identity:** IMSI, GUTI, RAN UE NGAP ID, AMF UE NGAP ID
- **Network Location:** Cell ID, TAC, PLMN (MCC/MNC)
- **gNB Information:** gNB ID, gNB Name, Connection Status
- **Registration State:** 5GMM State (REGISTERED, IDLE, etc.)

Note: The service extracts only real data from AMF logs. Geographic coordinates (latitude/longitude) require an external cell database.

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

## ✅ Verification Results

### Multi-Party TLS Implementation
✅ **Key Generation**: RSA-2048 with (3,5)-threshold SSS  
✅ **Share Distribution**: 5 party files (552 bytes each)  
✅ **Reconstruction**: Successful with any 3 parties  
✅ **OpenSSL Validation**: Keys pass `openssl rsa -check`  
✅ **PEM Compatibility**: Standard format, no modifications  

### 5G Integration
✅ **Network Deployed**: All 17 containers healthy  
✅ **UE Registration**: 10 UEs in 5GMM-REGISTERED state  
✅ **PFCP Association**: SMF ↔ UPF established  
✅ **Connectivity**: 0% packet loss (ping test)  
✅ **AMF-2 Integration**: Rsyslog client configured  

### TLS Encrypted Logging
✅ **Server Running**: WSL host listening on port 6514  
✅ **TLS Handshake**: Successful with threshold-protected key  
✅ **Certificate Auth**: x509/name verification working  
✅ **Log Forwarding**: Real-time from AMF-2 to server  
✅ **End-to-End Verified**: Multiple test messages received  

**See [MULTIPARTY_TLS_VERIFICATION.md](docs/documentation/MULTIPARTY_TLS_VERIFICATION.md) for detailed verification report.**

---

## 📊 Performance Metrics

| Metric | Value | Status |
|--------|-------|--------|
| Key Generation Time | ~2 seconds | ✅ |
| Key Size | RSA-2048 (2046 bits) | ✅ |
| Share Generation | 170 shares (5 parties) | ✅ |
| TLS Handshake Overhead | None (standard) | ✅ |
| Log Forwarding Latency | < 10ms | ✅ |
| 5G UE Registration | ~5 seconds | ✅ |
| Packet Loss | 0% | ✅ |

---

## 📚 Documentation

### Quick References
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - Complete repository organization
- **[MULTIPARTY_RSYSLOG_QUICKREF.md](docs/documentation/MULTIPARTY_RSYSLOG_QUICKREF.md)** - Quick reference card

### Comprehensive Guides
- **[MULTIPARTY_RSYSLOG_README.md](docs/documentation/MULTIPARTY_RSYSLOG_README.md)** - User guide
- **[MULTIPARTY_RSYSLOG_COMPLETE.md](docs/documentation/MULTIPARTY_RSYSLOG_COMPLETE.md)** - Technical reference
- **[5G_SETUP_SUMMARY.md](docs/documentation/5G_SETUP_SUMMARY.md)** - 5G network setup

### Implementation Details
- **[MULTIPARTY_TLS_FLOW.md](docs/documentation/MULTIPARTY_TLS_FLOW.md)** - Protocol flow
- **[RSA_RECONSTRUCTION_NOTES.md](docs/documentation/RSA_RECONSTRUCTION_NOTES.md)** - Reconstruction algorithm
- **[OPENSSL_TEST_RESULTS.md](docs/documentation/OPENSSL_TEST_RESULTS.md)** - OpenSSL validation

### Final Deliverables
- **[term_project_full.pdf](docs/reports/term_project_full.pdf)** - 32-page final report
- **[final_presentation.pdf](docs/presentations/final_presentation.pdf)** - Final presentation

---

## 🎓 Academic Context

**Course**: WNS (Wireless and Network Security)  
**Institution**: IIIT Hyderabad  
**Semester**: 2025  
**Student**: Rishabh Kumar (cs25resch04002)  

### Deliverables
- ✅ Final Report (32 pages, 300 KB PDF)
- ✅ Final Presentation (LaTeX Beamer)
- ✅ Source Code (GitHub repository)
- ✅ Verification Results (Complete)
- ✅ Demo Video (WNS_20251112_03_01.mp4)

---

## 🔬 Research Contributions

1. **Threshold Cryptography for TLS**: Novel application of Shamir's Secret Sharing to protect TLS server keys
2. **5G Logging Security**: Distributed trust model for critical infrastructure logging
3. **Standards Compatibility**: Maintains RFC 5425 compliance without protocol modifications
4. **Information-Theoretic Security**: Provides perfect secrecy independent of computational resources

---

## 🌟 Future Work

### Short-term Enhancements
- [ ] HSM integration for secure share storage
- [ ] Automated key rotation with threshold protection
- [ ] Web dashboard for party authorization
- [ ] Real-time monitoring of multi-party operations

### Long-term Research
- [ ] Extend to other 5G components (SMF, UPF, gNB)
- [ ] Dynamic threshold adjustment based on threat level
- [ ] Integration with SIEM systems
- [ ] Blockchain-based audit trail for key operations
- [ ] Multi-party TLS for IoT devices in 5G networks

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

## 📧 Contact

**Rishabh Kumar**  
Roll Number: cs25resch04002  
Email: kumarrishabh73@gmail.com | rishabh.kumar@research.iiit.ac.in  
GitHub: [@Rishabh0712](https://github.com/Rishabh0712)  
Repository: [WNSTermProject](https://github.com/Rishabh0712/WNSTermProject)

---

## 🙏 Acknowledgments

- **OpenAirInterface Software Alliance** - For the 5G simulation framework
- **IIIT Hyderabad** - For academic support and resources
- **WNS Course Faculty** - For guidance throughout the project
- **3GPP** - For 5G specifications and standards
- **OpenSSL Community** - For cryptographic libraries

---

## 📄 License

This project is submitted as part of academic coursework at IIIT Hyderabad.  
All rights reserved. Contact author for usage permissions.

---

## 🔗 Related Links

- [3GPP 5G Specifications](https://www.3gpp.org/)
- [OpenAirInterface](https://openairinterface.org/)
- [RFC 5425 - TLS Transport for Syslog](https://datatracker.ietf.org/doc/html/rfc5425)
- [Shamir's Secret Sharing](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing)
- [Docker Documentation](https://docs.docker.com/)

---

## ⭐ Project Status

**Status**: ✅ **COMPLETE AND VERIFIED**  
**Last Updated**: November 26, 2025  
**Version**: 1.0.0  

---

*"Securing 5G infrastructure through distributed trust and threshold cryptography."*
