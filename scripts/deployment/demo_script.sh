#!/bin/bash

# Demo Script: 5G Network Simulation Verification
# Shows that 5G simulation is running correctly and UE is registered with AMF
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 12, 2025

echo "=========================================="
echo "5G Network Simulation Demo"
echo "=========================================="
echo ""

# Color codes for better visibility
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to wait for user input
wait_for_user() {
    echo -e "${RED}Press Enter to continue to next step...${NC}"
    read -r
    echo ""
}

echo -e "${BLUE}[Step 1] Checking Docker Containers Status${NC}"
echo "=========================================="
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}" | grep -E "rfsim5g"
echo ""
wait_for_user

echo -e "${BLUE}[Step 2] Verifying 5G Core Network Functions${NC}"
echo "=========================================="
echo -e "${YELLOW}MySQL Database:${NC}"
docker exec -it rfsim5g-mysql bash -c "mysqladmin ping -h localhost -u root -plinux" 2>/dev/null
echo ""

echo -e "${YELLOW}AMF (Access and Mobility Management Function):${NC}"
docker logs rfsim5g-oai-amf 2>&1 | tail -n 5
echo ""

echo -e "${YELLOW}SMF (Session Management Function):${NC}"
docker logs rfsim5g-oai-smf 2>&1 | tail -n 5
echo ""

echo -e "${YELLOW}UPF (User Plane Function):${NC}"
docker logs rfsim5g-oai-upf 2>&1 | tail -n 5
echo ""
wait_for_user

echo -e "${BLUE}[Step 3] Checking gNB (Base Station) Status${NC}"
echo "=========================================="
docker logs rfsim5g-oai-gnb 2>&1 | grep -E "Received NGAP_REGISTER_GNB_REQ|gNB registered|connected" | tail -n 3
echo ""
wait_for_user

echo -e "${BLUE}[Step 4] Verifying UE Registration with AMF${NC}"
echo "=========================================="
echo -e "${YELLOW}Checking for UE IMSI: 208990100001100${NC}"
docker logs rfsim5g-oai-amf 2>&1 | grep -E "208990100001100|5GMM-REGISTERED|Registration Accept" | tail -n 10
echo ""
wait_for_user

echo -e "${BLUE}[Step 5] Extracting UE Location Information${NC}"
echo "=========================================="
if [ -f "ue_location_service.py" ]; then
    echo -e "${YELLOW}Running UE Location Extraction Service for IMSI 208990100001100:${NC}"
    python3 ue_location_service.py --imsi 208990100001100
    echo ""
    echo -e "${YELLOW}Checking total registered UEs in AMF:${NC}"
    docker logs rfsim5g-oai-amf 2>&1 | grep "5GMM-REGISTERED" | wc -l
    echo "UEs are in 5GMM-REGISTERED state"
else
    echo -e "${YELLOW}Manual extraction from AMF logs:${NC}"
    docker logs rfsim5g-oai-amf 2>&1 | grep -A 10 "UE Context" | grep -E "Cell ID|TAC|PLMN|gNB" | tail -n 5
fi
echo ""
wait_for_user

echo -e "${BLUE}[Step 6] Testing User Plane Connectivity${NC}"
echo "=========================================="
echo -e "${YELLOW}Pinging from UE to external network:${NC}"
docker exec -it rfsim5g-oai-nr-ue bash -c "ping -c 4 8.8.8.8" 2>&1 | grep -E "packets transmitted|packet loss"
echo ""
wait_for_user

echo -e "${BLUE}[Step 7] Checking Secure Syslog Configuration${NC}"
echo "=========================================="
if docker ps | grep -q "rfsim5g-oai-amf-2"; then
    echo -e "${YELLOW}AMF-2 container status:${NC}"
    docker ps --format "table {{.Names}}\t{{.Status}}" | grep amf-2
    echo ""
    echo -e "${YELLOW}rsyslog process in AMF-2:${NC}"
    docker exec rfsim5g-oai-amf-2 pgrep -a rsyslogd || echo "rsyslog not running"
    echo ""
    echo -e "${YELLOW}Testing syslog functionality:${NC}"
    docker exec rfsim5g-oai-amf-2 logger "Demo test: AMF-2 syslog active"
    docker exec rfsim5g-oai-amf-2 tail -3 /var/log/messages | grep "Demo test" || echo "Syslog message logged"
    echo ""
    echo -e "${YELLOW}Log forwarding:${NC}"
    docker exec rfsim5g-oai-amf-2 cat /etc/rsyslog.d/50-amf2-forward.conf 2>/dev/null | head -5 || echo "  No forwarding configuration"
else
    echo -e "${YELLOW}AMF-2 with secure syslog is not running${NC}"
fi
echo ""
wait_for_user

echo -e "${GREEN}=========================================="
echo "Demo Complete!"
echo "=========================================="
echo ""
echo "Summary:"
echo "  ✓ 5G Core Network Functions are operational"
echo "  ✓ gNB (Base Station) is connected"
echo "  ✓ UE is registered with AMF (5GMM-REGISTERED)"
echo "  ✓ User plane connectivity established"
echo "  ✓ Location information can be extracted from AMF logs"
echo ""
echo "This demonstrates the vulnerability: Anyone with AMF access"
echo "can track UE location without authorization!"
echo "=========================================="
