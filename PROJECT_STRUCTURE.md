# WNS Term Project - Repository Structure

**Project**: Multi-Party Threshold TLS for 5G Secure Logging  
**Author**: Rishabh Kumar (cs25resch04002)  
**Date**: November 26, 2025

---

## 📁 Repository Organization

```
WNS/
├── README.md                           # Main project overview
├── Makefile                            # Build configuration
├── .gitignore                          # Git ignore rules
├── PROJECT_STRUCTURE.md               # This file
│
├── 📂 docs/                            # Documentation and reports
│   ├── presentations/                  # PowerPoint and LaTeX presentations
│   │   ├── final_presentation.pdf
│   │   ├── final_presentation.tex
│   │   ├── midterm_presentation.pdf
│   │   ├── midterm_presentation.tex
│   │   ├── midterm_presentation.pptx
│   │   ├── midterm_status.pdf
│   │   ├── midterm.pdf
│   │   ├── threshold_ecdsa_tls.pdf
│   │   ├── tls12_message_flow.pdf
│   │   └── Sample-MidTerm-Presentation-TermProject.pptx
│   │
│   ├── reports/                        # Project reports
│   │   ├── term_project_full.pdf       # ⭐ Final comprehensive report (32 pages)
│   │   ├── term_project_full.tex
│   │   ├── term_project_report.pdf
│   │   ├── term_project_report.tex
│   │   └── term_project_report_backup.tex
│   │
│   ├── proposals/                      # Project proposals and templates
│   │   ├── proposal.pdf
│   │   ├── proposal.txt
│   │   ├── threshold_ecdsa_tls_proposal.md
│   │   ├── Term Project Report_ Template.txt
│   │   ├── midterm_template.txt
│   │   └── Sample-*.txt
│   │
│   └── documentation/                  # Technical documentation
│       ├── 5G_SETUP_SUMMARY.md
│       ├── FULL_KEY_SPLITTING_COMPLETE.md
│       ├── GITHUB_SETUP.md
│       ├── IMPLEMENTATION_SUMMARY.txt
│       ├── MULTIPARTY_RSYSLOG_COMPLETE.md
│       ├── MULTIPARTY_RSYSLOG_QUICKREF.md
│       ├── MULTIPARTY_RSYSLOG_README.md
│       ├── MULTIPARTY_TLS_FLOW.md
│       ├── MULTIPARTY_TLS_VERIFICATION.md  # ⭐ Integration verification
│       ├── OPENSSL_TEST_RESULTS.md
│       ├── PROJECT_SUMMARY.md
│       ├── RSA_RECONSTRUCTION_NOTES.md
│       ├── SECURE_SYSLOG_SETUP.md
│       ├── SETUP.md
│       ├── SYSLOG_TLS_README.md
│       ├── TLS_MULTIPARTY_README.md
│       ├── threshold_tls_handshake_workflow.md
│       ├── UE_LOCATION_SERVICE.md
│       └── FINAL_IMPLEMENTATION_STATUS.txt
│
├── 📂 src/                             # Source code
│   ├── multiparty_tls/                 # Multi-party TLS implementation
│   │   ├── multiparty_key_generator.cpp    # ⭐ Main key generator (340 lines)
│   │   ├── multiparty_tls_simple.cpp
│   │   ├── multiparty_tls_rsyslog.cpp
│   │   ├── tls_multiparty.cpp
│   │   └── tls_multiparty.hpp
│   │
│   ├── shamir_secret_sharing/          # Shamir's Secret Sharing library
│   │   ├── shamir_secret_sharing.cpp
│   │   └── shamir_secret_sharing.hpp
│   │
│   ├── ue_location/                    # UE location tracking service
│   │   ├── ue_location_service.py
│   │   └── ue_location.json
│   │
│   └── tests/                          # Test files and data
│       ├── test_lagrange.cpp
│       ├── test_multiparty_tls_handshake.cpp
│       ├── test_openssl_rsa.cpp
│       ├── test_small_prime.cpp
│       ├── test_sss_minimal.cpp
│       ├── test_tls_multiparty.cpp
│       ├── test_message.txt
│       ├── test_pms.enc
│       ├── test_pms.txt
│       ├── test_pms_decrypted.txt
│       ├── decrypted_message.txt
│       └── encrypted_message.bin
│
├── 📂 scripts/                         # Automation scripts
│   ├── setup/                          # Setup and configuration scripts
│   │   ├── setup_multiparty_rsyslog.sh     # ⭐ Complete deployment automation
│   │   ├── setup_secure_syslog.sh
│   │   └── fix_amf2_rsyslog.sh
│   │
│   ├── capture/                        # Network capture and testing scripts
│   │   ├── capture_5g_traffic.sh
│   │   ├── capture_syslog_tls.sh
│   │   ├── capture_tls_simple.sh
│   │   ├── test_openssl_full.sh
│   │   ├── test_rsa_reconstruction.sh
│   │   └── run_multiparty_tls_test.sh
│   │
│   ├── deployment/                     # Deployment scripts
│   │   ├── demo_script.sh
│   │   └── build.sh
│   │
│   ├── create_midterm_ppt.py          # Presentation utilities
│   ├── extract_ppt.py
│   └── complete_sections.txt
│
├── 📂 certificates/                    # Certificates and keys
│   ├── certs/                          # General certificates
│   │   └── [CA and client certificates]
│   │
│   ├── syslog_certs/                   # Rsyslog TLS certificates
│   │   ├── ca-cert.pem                 # Certificate Authority
│   │   ├── ca-key.pem
│   │   ├── server-cert.pem             # Server certificate
│   │   ├── server-key.pem              # ⚠️ MULTI-PARTY THRESHOLD KEY
│   │   ├── server-key-public.pem
│   │   ├── server-key_party1_shares.dat    # Party 1: Judicial Authority
│   │   ├── server-key_party2_shares.dat    # Party 2: Law Enforcement
│   │   ├── server-key_party3_shares.dat    # Party 3: Network Security
│   │   ├── server-key_party4_shares.dat    # Party 4: Privacy Officer
│   │   ├── server-key_party5_shares.dat    # Party 5: Independent Auditor
│   │   ├── client-cert.pem             # Client certificate
│   │   └── client-key.pem
│   │
│   ├── rsa_cert.csr                    # Test certificates
│   ├── rsa_cert.pem
│   ├── rsa_private.pem
│   ├── rsa_public.pem
│   ├── test_server-key.pem
│   ├── test_server-key-public.pem
│   └── test_server-key_party*.dat      # Test party shares
│
├── 📂 config/                          # Configuration files
│   ├── rsyslog-server.conf            # Rsyslog server configuration
│   ├── tls-forward.conf               # TLS forwarding configuration
│   └── fix_rsyslog_commands.txt       # Rsyslog troubleshooting commands
│
├── 📂 artifacts/                       # Build artifacts
│   ├── latex_aux/                      # LaTeX auxiliary files
│   │   ├── *.aux
│   │   ├── *.log
│   │   ├── *.nav
│   │   ├── *.out
│   │   ├── *.snm
│   │   ├── *.toc
│   │   └── *.vrb
│   │
│   └── binaries/                       # Compiled binaries
│       ├── multiparty_key_generator    # ⭐ Multi-party key generator (58 KB)
│       ├── multiparty_tls_simple
│       ├── test_lagrange
│       ├── test_openssl_rsa
│       ├── test_small_prime
│       ├── test_sss_minimal
│       └── test_tls_multiparty
│
├── 📂 network_captures/                # Network traffic captures
│   └── [Wireshark .pcap files]
│
├── 📂 syslog_tls_captures/            # Syslog TLS captures
│   └── [TLS handshake captures]
│
├── 📂 openairinterface5g/             # 5G simulation environment
│   └── ci-scripts/yaml_files/5g_rfsimulator/
│       └── docker-compose.yml          # 5G network orchestration
│
├── 📂 openssl-multiparty/             # OpenSSL modifications (if any)
│   └── [Custom OpenSSL patches]
│
├── 📂 ppt_extract/                     # PowerPoint extraction utilities
│   └── [Extracted presentation content]
│
├── 📂 .venv/                           # Python virtual environment
│   └── [Python dependencies]
│
└── 🎥 WNS_20251112_03_01.mp4          # Demo video

```

---

## 🎯 Key Components

### 1. Multi-Party TLS Implementation
**Location**: `src/multiparty_tls/`

Core implementation of threshold cryptography for TLS:
- **multiparty_key_generator.cpp**: Main implementation (340 lines)
  - RSA-2048 key generation
  - (3,5)-threshold Shamir's Secret Sharing
  - Key reconstruction from parties
  - PEM format output

**Binary**: `artifacts/binaries/multiparty_key_generator` (58 KB)

### 2. Shamir's Secret Sharing Library
**Location**: `src/shamir_secret_sharing/`

Information-theoretic secret sharing:
- Finite field arithmetic GF(2^61-1)
- Lagrange interpolation
- Share generation and reconstruction

### 3. 5G Integration
**Location**: `openairinterface5g/`

OpenAirInterface 5G network:
- **Components**: MySQL, AMF (2 instances), SMF, UPF, gNB, 10 UEs
- **Networks**: Public (192.168.71.0/26), Traffic (192.168.72.0/26)
- **Status**: All containers healthy, UEs registered

**Integration Point**: AMF-2 container (rfsim5g-oai-amf-2)
- IP: 192.168.71.136
- Rsyslog client with TLS to host server
- Certificates in `/etc/rsyslog.d/syslog_certs/`

### 4. Secure Logging Infrastructure
**Components**:
- **Server**: Host WSL (172.31.130.37:6514)
  - Rsyslog with multi-party threshold TLS key
  - Configuration: `config/rsyslog-server.conf`
  - Logs: `/var/log/amf2/`

- **Client**: AMF-2 container
  - Rsyslog client forwarding via TLS
  - Protected by x509/name authentication

### 5. Documentation Suite
**Location**: `docs/documentation/`

Comprehensive technical documentation:
- **MULTIPARTY_TLS_VERIFICATION.md**: Integration verification report
- **MULTIPARTY_RSYSLOG_COMPLETE.md**: Complete rsyslog setup guide
- **MULTIPARTY_RSYSLOG_README.md**: User documentation
- **5G_SETUP_SUMMARY.md**: 5G network setup guide
- **SETUP.md**: General setup instructions

### 6. Automation Scripts
**Location**: `scripts/`

Production-ready automation:
- **setup/setup_multiparty_rsyslog.sh**: Complete deployment (11 KB)
  - Detects AMF-2 container IP
  - Generates multi-party keys
  - Creates certificates
  - Configures rsyslog server and client
  
- **capture/**: Network analysis scripts
- **deployment/**: Build and demo scripts

---

## 🔐 Security Model

### Threshold Cryptography
- **Scheme**: (3,5)-Shamir's Secret Sharing
- **Key Type**: RSA-2048 (2046 bits)
- **Chunks**: 34 × 61 bits = 2074 bits
- **Total Shares**: 170 (34 chunks × 5 parties)

### Party Distribution
1. **Party 1**: Judicial Authority (552 bytes)
2. **Party 2**: Law Enforcement (552 bytes)
3. **Party 3**: Network Security (552 bytes)
4. **Party 4**: Privacy Officer (552 bytes)
5. **Party 5**: Independent Auditor (552 bytes)

### Security Properties
✅ **Information-Theoretic Security**: < 3 shares reveal ZERO information  
✅ **Distributed Trust**: No single party can use the key  
✅ **Threshold Reconstruction**: Requires 3 of 5 parties  
✅ **TLS Compatibility**: Standard RFC 5425 compliance  

---

## 📊 Project Statistics

### Code Metrics
- **C++ Source Files**: 12 files (~2,000 lines)
- **Python Scripts**: 2 files (~500 lines)
- **Shell Scripts**: 13 files (~1,500 lines)
- **Documentation**: 20 markdown files (~100 KB)

### Deliverables
- **Final Report**: 32 pages (300 KB PDF)
- **Presentations**: 5 PDFs + 1 PowerPoint
- **Binaries**: 7 compiled executables
- **Certificates**: 15+ certificate files
- **Tests**: 10+ test programs

### Integration Status
✅ Multi-party key generator: **COMPLETE**  
✅ 5G simulation: **OPERATIONAL**  
✅ Rsyslog TLS server: **RUNNING**  
✅ AMF-2 integration: **VERIFIED**  
✅ Log forwarding: **WORKING**  

---

## 🚀 Quick Start

### 1. Build Multi-Party Key Generator
```bash
cd WNS
make
# Output: artifacts/binaries/multiparty_key_generator
```

### 2. Generate Threshold-Protected Keys
```bash
./artifacts/binaries/multiparty_key_generator server-key.pem 5 3
# Generates: server-key.pem + 5 party share files
```

### 3. Deploy to 5G Environment
```bash
cd scripts/setup
./setup_multiparty_rsyslog.sh
# Detects AMF-2, generates certs, configures rsyslog
```

### 4. Verify Integration
```bash
# Send test log from AMF-2
docker exec rfsim5g-oai-amf-2 logger -p local0.info "Test message"

# Check received logs
sudo tail /var/log/amf2/*.log
```

---

## 📖 Documentation Guide

### Getting Started
1. **README.md** - Project overview and quick start
2. **docs/documentation/SETUP.md** - Detailed setup instructions
3. **docs/documentation/5G_SETUP_SUMMARY.md** - 5G network configuration

### Implementation Details
1. **docs/documentation/MULTIPARTY_RSYSLOG_README.md** - User guide
2. **docs/documentation/MULTIPARTY_RSYSLOG_COMPLETE.md** - Technical reference
3. **docs/documentation/MULTIPARTY_TLS_FLOW.md** - Protocol flow

### Verification & Testing
1. **docs/documentation/MULTIPARTY_TLS_VERIFICATION.md** - Integration verification
2. **docs/documentation/OPENSSL_TEST_RESULTS.md** - OpenSSL tests
3. **docs/documentation/RSA_RECONSTRUCTION_NOTES.md** - Reconstruction tests

### Final Report
1. **docs/reports/term_project_full.pdf** - Comprehensive 32-page report

---

## 🔧 Build System

### Makefile Targets
```bash
make                    # Build all binaries
make clean             # Clean artifacts
make test              # Run test suite
make install           # Install to system
```

### Dependencies
- **C++ Compiler**: g++ with C++17 support
- **Libraries**: OpenSSL 3.0+, GMP (optional)
- **Python**: 3.8+ with dependencies in `.venv`
- **Docker**: For 5G simulation
- **LaTeX**: For building reports (pdflatex)

---

## 📝 Version Control

### Git Structure
- **Main Branch**: Production-ready code
- **Commits**: All major milestones documented
- **Tags**: Release versions

### Important Files for Git
```
.gitignore              # Excludes .venv, binaries, LaTeX aux
artifacts/              # Build outputs (ignored)
.venv/                  # Python environment (ignored)
```

---

## 🎓 Academic Context

**Course**: WNS (Wireless and Network Security)  
**Institution**: IIIT Hyderabad  
**Semester**: 2025  
**Project Type**: Term Project  

**Submission Includes**:
- ✅ Final Report (32 pages)
- ✅ Final Presentation (PDF)
- ✅ Source Code (GitHub)
- ✅ Demo Video
- ✅ Verification Results

---

## 📧 Contact

**Student**: Rishabh Kumar  
**Roll Number**: cs25resch04002  
**Email**: rishabh.kumar@research.iiit.ac.in  
**GitHub**: https://github.com/Rishabh0712/WNSTermProject  

---

## 📄 License

This project is submitted as part of academic coursework at IIIT Hyderabad.  
All rights reserved. Contact author for usage permissions.

---

**Last Updated**: November 26, 2025  
**Project Status**: ✅ **COMPLETE AND VERIFIED**
