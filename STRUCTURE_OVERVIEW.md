# WNS Term Project - Repository Structure Overview

```
WNS/ (Root Directory)
│
├── 📘 README.md ⭐ UPDATED - Main project overview with new structure
├── 📘 PROJECT_STRUCTURE.md ⭐ NEW - Detailed structure documentation
├── 📘 REORGANIZATION_SUMMARY.md ⭐ NEW - This reorganization summary
├── 📘 Makefile - Build configuration
├── 📘 .gitignore - Git ignore rules
│
├── 📂 docs/ ─────────────────────── Documentation & Reports (44 files)
│   │
│   ├── 📂 presentations/ (13 files)
│   │   ├── final_presentation.pdf
│   │   ├── midterm_presentation.pdf
│   │   └── [11 more presentation files]
│   │
│   ├── 📂 reports/ (6 files)
│   │   ├── term_project_full.pdf ⭐ (32 pages - Final Report)
│   │   └── [5 more report files]
│   │
│   ├── 📂 proposals/ (7 files)
│   │   ├── proposal.pdf
│   │   └── [6 more proposal files]
│   │
│   └── 📂 documentation/ (19 files)
│       ├── MULTIPARTY_TLS_VERIFICATION.md ⭐ (Integration verified)
│       ├── MULTIPARTY_RSYSLOG_COMPLETE.md
│       ├── 5G_SETUP_SUMMARY.md
│       └── [16 more documentation files]
│
├── 📂 src/ ──────────────────────── Source Code (24 files)
│   │
│   ├── 📂 multiparty_tls/ (5 files)
│   │   ├── multiparty_key_generator.cpp ⭐ (340 lines - Main implementation)
│   │   ├── tls_multiparty.cpp
│   │   ├── tls_multiparty.hpp
│   │   └── [2 more TLS files]
│   │
│   ├── 📂 shamir_secret_sharing/ (2 files)
│   │   ├── shamir_secret_sharing.cpp
│   │   └── shamir_secret_sharing.hpp
│   │
│   ├── 📂 ue_location/ (2 files)
│   │   ├── ue_location_service.py
│   │   └── ue_location.json
│   │
│   └── 📂 tests/ (11 files)
│       ├── test_lagrange.cpp
│       ├── test_multiparty_tls_handshake.cpp
│       └── [9 more test files]
│
├── 📂 scripts/ ──────────────────── Automation Scripts (15 files)
│   │
│   ├── 📂 setup/ (3 files)
│   │   ├── setup_multiparty_rsyslog.sh ⭐ (Complete deployment automation)
│   │   ├── setup_secure_syslog.sh
│   │   └── fix_amf2_rsyslog.sh
│   │
│   ├── 📂 capture/ (6 files)
│   │   ├── capture_5g_traffic.sh
│   │   ├── test_rsa_reconstruction.sh
│   │   └── [4 more capture scripts]
│   │
│   ├── 📂 deployment/ (2 files)
│   │   ├── demo_script.sh
│   │   └── build.sh
│   │
│   └── [3 utility Python scripts]
│       ├── create_midterm_ppt.py
│       └── extract_ppt.py
│
├── 📂 certificates/ ─────────────── Certificates & Keys (13 files)
│   │
│   ├── 📂 syslog_certs/ (Contains multi-party threshold TLS keys)
│   │   ├── ca-cert.pem (Certificate Authority)
│   │   ├── server-cert.pem (Server certificate)
│   │   ├── server-key.pem ⚠️ THRESHOLD-PROTECTED (3,5)-SSS
│   │   ├── server-key_party1_shares.dat (Judicial Authority)
│   │   ├── server-key_party2_shares.dat (Law Enforcement)
│   │   ├── server-key_party3_shares.dat (Network Security)
│   │   ├── server-key_party4_shares.dat (Privacy Officer)
│   │   ├── server-key_party5_shares.dat (Independent Auditor)
│   │   └── [client certificates]
│   │
│   └── [Test certificates and RSA keys]
│       ├── test_server-key.pem
│       ├── test_server-key_party*.dat (5 files)
│       └── [RSA test certs]
│
├── 📂 config/ ───────────────────── Configuration Files (3 files)
│   ├── rsyslog-server.conf
│   ├── tls-forward.conf
│   └── fix_rsyslog_commands.txt
│
├── 📂 artifacts/ ────────────────── Build Artifacts (50+ files)
│   │
│   ├── 📂 binaries/ (7 files)
│   │   ├── multiparty_key_generator ⭐ (58 KB - Main binary)
│   │   ├── test_lagrange
│   │   └── [5 more compiled binaries]
│   │
│   └── 📂 latex_aux/ (40+ files)
│       └── [All LaTeX auxiliary files: .aux, .log, .nav, .out, etc.]
│
├── 📂 network_captures/ ─────────── Network Traffic Captures
│   └── [Wireshark .pcap files]
│
├── 📂 syslog_tls_captures/ ──────── Syslog TLS Captures
│   └── [TLS handshake captures]
│
├── 📂 openairinterface5g/ ───────── 5G Simulation Environment
│   └── ci-scripts/yaml_files/5g_rfsimulator/
│       └── docker-compose.yml (5G network orchestration)
│
├── 📂 openssl-multiparty/ ───────── OpenSSL Modifications
│   └── [Custom patches if any]
│
├── 📂 ppt_extract/ ──────────────── PowerPoint Extraction Utilities
│   └── [Extracted content]
│
├── 📂 .venv/ ────────────────────── Python Virtual Environment
│   └── [Python dependencies]
│
└── 🎥 WNS_20251112_03_01.mp4 ────── Demo Video

```

---

## 📊 Statistics

### Files Organized
| Category | Count | Location |
|----------|-------|----------|
| **Documentation** | 19 | docs/documentation/ |
| **Presentations** | 13 | docs/presentations/ |
| **Reports** | 6 | docs/reports/ |
| **Proposals** | 7 | docs/proposals/ |
| **C++ Source** | 12 | src/ |
| **Python Scripts** | 5 | src/, scripts/ |
| **Shell Scripts** | 13 | scripts/ |
| **Certificates** | 20+ | certificates/ |
| **Binaries** | 7 | artifacts/binaries/ |
| **LaTeX Aux** | 40+ | artifacts/latex_aux/ |
| **Total** | **150+** | |

### Directory Summary
| Directory | Purpose | Files |
|-----------|---------|-------|
| **docs/** | All documentation | 44 |
| **src/** | Source code | 24 |
| **scripts/** | Automation | 15 |
| **certificates/** | Keys & certs | 13 |
| **config/** | Configuration | 3 |
| **artifacts/** | Build outputs | 50+ |

---

## 🎯 Key Improvements

### ✅ Organization
- Clear separation of documentation, code, and artifacts
- Logical grouping by purpose
- Easy navigation and discovery

### ✅ Professionalism
- Industry-standard structure
- Academic submission ready
- GitHub best practices followed

### ✅ Maintainability
- Easy to find and update files
- Clear dependency relationships
- Reduced root directory clutter

### ✅ Scalability
- Room for future additions
- Modular organization
- Easy to extend

---

## 🚀 Quick Access Guide

### For Users
```bash
# Read main overview
cat README.md

# View detailed structure
cat PROJECT_STRUCTURE.md

# Build the project
make

# Run key generator
./artifacts/binaries/multiparty_key_generator server-key.pem 5 3
```

### For Developers
```bash
# Browse source code
cd src/multiparty_tls/
cat multiparty_key_generator.cpp

# View documentation
cd docs/documentation/
cat MULTIPARTY_TLS_VERIFICATION.md

# Run setup scripts
cd scripts/setup/
./setup_multiparty_rsyslog.sh
```

### For Reviewers
```bash
# Read final report
xdg-open docs/reports/term_project_full.pdf

# View presentation
xdg-open docs/presentations/final_presentation.pdf

# Check verification
cat docs/documentation/MULTIPARTY_TLS_VERIFICATION.md
```

---

## 📌 Important Locations

### 🌟 Key Documents
- **Final Report**: `docs/reports/term_project_full.pdf` (32 pages)
- **Verification**: `docs/documentation/MULTIPARTY_TLS_VERIFICATION.md`
- **Structure Guide**: `PROJECT_STRUCTURE.md`

### 🔧 Key Source Files
- **Main Implementation**: `src/multiparty_tls/multiparty_key_generator.cpp`
- **SSS Library**: `src/shamir_secret_sharing/`
- **Main Binary**: `artifacts/binaries/multiparty_key_generator`

### 🔐 Key Certificates
- **Threshold Key**: `certificates/syslog_certs/server-key.pem`
- **Party Shares**: `certificates/syslog_certs/server-key_party*.dat` (5 files)
- **CA Certificate**: `certificates/syslog_certs/ca-cert.pem`

### 🛠️ Key Scripts
- **Complete Setup**: `scripts/setup/setup_multiparty_rsyslog.sh`
- **Build Script**: `scripts/deployment/build.sh`
- **Test Scripts**: `scripts/capture/`

---

**Repository Organization Date**: November 26, 2025  
**Total Files**: 150+  
**Directories Created**: 14  
**Status**: ✅ **COMPLETE**

*For detailed reorganization notes, see [REORGANIZATION_SUMMARY.md](REORGANIZATION_SUMMARY.md)*
