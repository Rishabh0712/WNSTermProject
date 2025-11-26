# 5G RF Simulator Setup Summary

## Setup Completed Successfully! ✓

Date: November 10, 2025

---

## What Was Accomplished

### 1. Environment Setup
- ✓ WSL (Windows Subsystem for Linux) installed with Ubuntu
- ✓ Docker installed and configured in WSL
- ✓ User added to docker group
- ✓ All necessary dependencies installed (tcpdump, git, etc.)

### 2. Docker Images Downloaded
All OpenAirInterface 5G components successfully pulled:
- ✓ MySQL 8.0
- ✓ OAI AMF (Access and Mobility Management Function) v2.1.10
- ✓ OAI SMF (Session Management Function) v2.1.10
- ✓ OAI UPF (User Plane Function) v2.1.10
- ✓ OAI gNB (5G Base Station) - develop branch
- ✓ OAI NR-UE (5G User Equipment) - develop branch
- ✓ Traffic Generator CN5G - focal

### 3. Network Configuration
Two Docker networks created for 5G simulation:
- **Public Network (rfsim5g-oai-public-net)**: 192.168.71.128/26
  - Used for: Core signaling (SCTP, PFCP, NGAP)
- **Traffic Network (rfsim5g-oai-traffic-net)**: 192.168.72.128/26
  - Used for: User-plane traffic (GTP-U)

### 4. 5G Core Network Deployed
All core network functions successfully deployed and healthy:
- ✓ MySQL Database (rfsim5g-mysql)
- ✓ AMF at 192.168.71.132
- ✓ SMF at 192.168.71.133
- ✓ UPF at 192.168.71.134 (public) and 192.168.72.134 (traffic)
- ✓ External DN at 192.168.72.135

### 5. Radio Access Network (RAN) Deployed
- ✓ gNB (Base Station) deployed at 192.168.71.140
- ✓ gNB successfully connected to AMF
- ✓ PLMN: MCC=208, MNC=99
- ✓ Global gNB ID: 0x0E00
- ✓ gNB Name: gnb-rfsim

### 6. User Equipment (UE) Deployed and Registered
- ✓ NR-UE deployed at 192.168.71.150
- ✓ UE successfully registered with 5G network
- ✓ IMSI: 208990100001100
- ✓ 5GMM State: **5GMM-REGISTERED**
- ✓ UE IP Address assigned: **12.1.1.2** (on oaitun_ue1 interface)
- ✓ RAN UE NGAP ID: 0x01
- ✓ AMF UE NGAP ID: 0x01

### 7. End-to-End Connectivity Verified
- ✓ Ping test from UE to Data Network (DN) successful
- ✓ Destination: 192.168.72.135
- ✓ Result: 5 packets transmitted, 5 received, **0% packet loss**
- ✓ Average RTT: 22.364 ms

---

## Network Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    5G Core Network                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  MySQL   │  │   AMF    │  │   SMF    │  │   UPF    │       │
│  │  :3306   │  │ :38412   │  │  :8805   │  │  :2152   │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
│       │             │              │              │             │
└───────┼─────────────┼──────────────┼──────────────┼─────────────┘
        │             │              │              │
        │         [Public Network - 192.168.71.0/26]│
        │             │              │              │
        │             │              │         [Traffic Network]
        │             │              │         [192.168.72.0/26]
        │             │              │              │
        │             │              │              │
    ┌───┴─────────────┴──────────────┴──────────────┴────┐
    │                                                      │
    │  ┌──────────┐        ┌──────────┐     ┌─────────┐ │
    │  │   gNB    │◄──────►│  NR-UE   │────►│   DN    │ │
    │  │ (Base    │  NGAP  │ (Phone)  │ GTP │ (Dest)  │ │
    │  │ Station) │        │12.1.1.2  │     │.72.135  │ │
    │  └──────────┘        └──────────┘     └─────────┘ │
    │                                                      │
    └──────────────────────────────────────────────────────┘
```

---

## Key Protocols Captured
The following 5G protocols are active and can be analyzed:
- **NGAP**: NG Application Protocol (AMF ↔ gNB)
- **PFCP**: Packet Forwarding Control Protocol (SMF ↔ UPF)
- **GTP-U**: GPRS Tunneling Protocol - User Plane (encapsulated data)
- **SCTP**: Stream Control Transmission Protocol (signaling transport)
- **NAS**: Non-Access Stratum (UE ↔ AMF)

---

## Container Status
All 7 containers running and healthy:
1. rfsim5g-mysql - Database
2. rfsim5g-oai-amf - Access and Mobility Management
3. rfsim5g-oai-smf - Session Management
4. rfsim5g-oai-upf - User Plane Function
5. rfsim5g-oai-ext-dn - External Data Network
6. rfsim5g-oai-gnb - 5G Base Station
7. rfsim5g-oai-nr-ue - User Equipment

---

## Useful Commands

### Check container status:
```bash
wsl sudo docker ps
```

### View logs:
```bash
wsl sudo docker logs rfsim5g-oai-amf
wsl sudo docker logs rfsim5g-oai-gnb
wsl sudo docker logs rfsim5g-oai-nr-ue
```

### Access UE container:
```bash
wsl sudo docker exec -it rfsim5g-oai-nr-ue bash
```

### Test connectivity from UE:
```bash
wsl sudo docker exec rfsim5g-oai-nr-ue ping -I oaitun_ue1 -c 5 192.168.72.135
```

### Stop all containers:
```bash
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
wsl sudo docker-compose down
```

### Restart the setup:
```bash
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
wsl sudo docker-compose up -d
```

---

## Files and Locations

### Project Directory:
`C:\Users\kumarrishabh\Documents\WNS\`

### OAI Source Code:
`C:\Users\kumarrishabh\Documents\WNS\openairinterface5g\`

### Docker Compose Files:
`C:\Users\kumarrishabh\Documents\WNS\openairinterface5g\ci-scripts\yaml_files\5g_rfsimulator\`

### Configuration Files:
- `docker-compose.yaml` (modified to use external networks)
- `docker-compose.yaml.bak` (original backup)
- `mini_nonrf_config.yaml` (5G core configuration)

---

## Next Steps for Secure Syslog Integration

To integrate secure syslog with this 5G setup:

1. **Install and configure rsyslog-gnutls** in the core network containers
2. **Generate TLS certificates** for secure communication
3. **Configure log forwarding** from:
   - AMF logs → Syslog server
   - SMF logs → Syslog server
   - UPF logs → Syslog server
   - gNB logs → Syslog server
4. **Set up centralized syslog server** with TLS support
5. **Parse and format 5G events** for structured logging
6. **Create monitoring dashboard** for real-time visibility

---

## Troubleshooting

### If containers fail to start:
```bash
# Check logs
wsl sudo docker logs <container-name>

# Restart specific container
wsl sudo docker-compose restart <service-name>
```

### If networking issues occur:
```bash
# Check networks
wsl sudo docker network ls
wsl sudo docker network inspect rfsim5g-oai-public-net

# Recreate networks if needed
wsl sudo docker network rm rfsim5g-oai-public-net
wsl sudo docker network create --driver=bridge --subnet=192.168.71.128/26 rfsim5g-oai-public-net
```

### If Docker service is not running:
```bash
wsl sudo service docker start
wsl sudo service docker status
```

---

## Success Criteria Met ✓

✓ Full 5G Standalone (SA) network deployed
✓ gNB successfully connected to AMF  
✓ UE registered with 5GMM-REGISTERED state
✓ IP address allocated to UE (12.1.1.2)
✓ End-to-end connectivity verified (0% packet loss)
✓ All containers healthy and operational
✓ Ready for secure syslog integration

---

## References
- OAI GitLab: https://gitlab.eurecom.fr/oai/openairinterface5g
- Setup Guide: https://gist.github.com/emsaumay/b395f091487c8a7d1b03a592e7e580f2
- 3GPP 5G Specifications: TS 23.501, TS 38.300
- Docker Documentation: https://docs.docker.com/

---

**Setup completed successfully on November 10, 2025**
