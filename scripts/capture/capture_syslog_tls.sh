#!/bin/bash

# Script: TLS Handshake Capture for Secure Syslog (AMF-2)
# Purpose: Capture and analyze TLS handshake between host and AMF-2 syslog server
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 12, 2025

# Color codes for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
CAPTURE_DURATION=30  # seconds
SYSLOG_TLS_PORT=6514  # RFC 5425 standard port for TLS syslog
AMF2_CONTAINER="rfsim5g-oai-amf-2"
OUTPUT_DIR="./syslog_tls_captures"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
PCAP_FILE="${OUTPUT_DIR}/syslog_tls_${TIMESTAMP}.pcap"
LOG_FILE="${OUTPUT_DIR}/capture_log_${TIMESTAMP}.txt"

echo "=========================================="
echo "TLS Syslog Handshake Capture Tool"
echo "=========================================="
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if tcpdump is installed
if ! command -v tcpdump &> /dev/null; then
    echo -e "${RED}Error: tcpdump is not installed${NC}"
    echo "Install with: sudo apt-get install tcpdump"
    exit 1
fi

# Check if AMF-2 container is running
echo -e "${BLUE}[Step 1] Checking AMF-2 Container Status${NC}"
echo "=========================================="
if docker ps | grep -q "$AMF2_CONTAINER"; then
    echo -e "${GREEN}✓ AMF-2 container is running${NC}"
    AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$AMF2_CONTAINER")
    echo -e "  AMF-2 IP Address: ${YELLOW}$AMF2_IP${NC}"
else
    echo -e "${RED}✗ AMF-2 container is not running${NC}"
    echo "Please start AMF-2 with secure syslog configuration first."
    exit 1
fi
echo ""

# Get network interface for capture
echo -e "${BLUE}[Step 2] Detecting Network Interface${NC}"
echo "=========================================="
# Find the docker network interface
DOCKER_INTERFACE=$(ip route | grep "192.168.71.0/26" | awk '{print $3}' | head -1)
if [ -z "$DOCKER_INTERFACE" ]; then
    # Fallback to docker0 or detect automatically
    DOCKER_INTERFACE=$(docker network inspect rfsim5g-oai-public-net -f '{{range .IPAM.Config}}{{.Gateway}}{{end}}' 2>/dev/null | xargs -I {} ip route | grep {} | awk '{print $3}' | head -1)
fi

if [ -z "$DOCKER_INTERFACE" ]; then
    DOCKER_INTERFACE="docker0"
fi

echo -e "  Capture Interface: ${YELLOW}$DOCKER_INTERFACE${NC}"
echo -e "  Target Port: ${YELLOW}$SYSLOG_TLS_PORT${NC}"
echo ""

# Display capture filter
echo -e "${BLUE}[Step 3] Capture Configuration${NC}"
echo "=========================================="
echo -e "  Output File: ${YELLOW}$PCAP_FILE${NC}"
echo -e "  Duration: ${YELLOW}$CAPTURE_DURATION seconds${NC}"
echo -e "  Filter: TCP port $SYSLOG_TLS_PORT (TLS Syslog)"
echo -e "  Log File: ${YELLOW}$LOG_FILE${NC}"
echo ""

# Start capture
echo -e "${BLUE}[Step 4] Starting Packet Capture${NC}"
echo "=========================================="
echo -e "${YELLOW}Capturing TLS handshake packets...${NC}"
echo "Started at: $(date)" | tee "$LOG_FILE"
echo ""

# Run tcpdump in background
sudo tcpdump -i "$DOCKER_INTERFACE" -w "$PCAP_FILE" \
    "tcp port $SYSLOG_TLS_PORT" \
    -v -nn 2>&1 | tee -a "$LOG_FILE" &

TCPDUMP_PID=$!
echo -e "  tcpdump PID: ${YELLOW}$TCPDUMP_PID${NC}"
echo ""

# Monitor for specified duration
echo -e "${YELLOW}Monitoring for $CAPTURE_DURATION seconds...${NC}"
echo "Press Ctrl+C to stop early"
echo ""

# Progress indicator
for ((i=1; i<=CAPTURE_DURATION; i++)); do
    echo -ne "  Progress: [$i/$CAPTURE_DURATION seconds] \r"
    sleep 1
done
echo ""

# Stop capture
echo -e "${BLUE}[Step 5] Stopping Capture${NC}"
echo "=========================================="
sudo kill -SIGTERM $TCPDUMP_PID 2>/dev/null
wait $TCPDUMP_PID 2>/dev/null
echo "Stopped at: $(date)" | tee -a "$LOG_FILE"
echo ""

# Analyze captured data
echo -e "${BLUE}[Step 6] Analyzing Captured Packets${NC}"
echo "=========================================="

if [ ! -f "$PCAP_FILE" ]; then
    echo -e "${RED}Error: Capture file not created${NC}"
    exit 1
fi

PACKET_COUNT=$(sudo tcpdump -r "$PCAP_FILE" 2>/dev/null | wc -l)
echo -e "  Total packets captured: ${YELLOW}$PACKET_COUNT${NC}"

if [ $PACKET_COUNT -eq 0 ]; then
    echo -e "${YELLOW}Warning: No packets captured on port $SYSLOG_TLS_PORT${NC}"
    echo "Possible reasons:"
    echo "  1. AMF-2 is not sending syslog messages"
    echo "  2. TLS syslog is not configured"
    echo "  3. Wrong network interface or port"
else
    echo ""
    echo -e "${GREEN}✓ Packets captured successfully${NC}"
    
    # Extract TLS handshake details
    echo ""
    echo -e "${YELLOW}TLS Handshake Analysis:${NC}"
    echo "----------------------------------------"
    
    # Show Client Hello, Server Hello, Certificate exchanges
    sudo tshark -r "$PCAP_FILE" -Y "tls.handshake.type" -T fields \
        -e frame.number -e ip.src -e ip.dst -e tls.handshake.type \
        -E header=y -E separator='|' 2>/dev/null | column -t -s '|' | head -20
    
    echo ""
    echo -e "${YELLOW}TLS Version and Cipher Suites:${NC}"
    echo "----------------------------------------"
    sudo tshark -r "$PCAP_FILE" -Y "tls.handshake.version" -T fields \
        -e tls.handshake.version -e tls.handshake.ciphersuite \
        2>/dev/null | head -10
fi
echo ""

# Generate detailed report
echo -e "${BLUE}[Step 7] Generating Analysis Report${NC}"
echo "=========================================="
REPORT_FILE="${OUTPUT_DIR}/tls_analysis_${TIMESTAMP}.txt"

{
    echo "=========================================="
    echo "TLS Syslog Handshake Analysis Report"
    echo "=========================================="
    echo ""
    echo "Capture Details:"
    echo "  Date: $(date)"
    echo "  Interface: $DOCKER_INTERFACE"
    echo "  AMF-2 IP: $AMF2_IP"
    echo "  Port: $SYSLOG_TLS_PORT"
    echo "  Duration: $CAPTURE_DURATION seconds"
    echo "  Total Packets: $PACKET_COUNT"
    echo ""
    echo "Files Generated:"
    echo "  PCAP: $PCAP_FILE"
    echo "  Log: $LOG_FILE"
    echo ""
    
    if [ $PACKET_COUNT -gt 0 ]; then
        echo "=========================================="
        echo "TLS Handshake Details"
        echo "=========================================="
        sudo tshark -r "$PCAP_FILE" -Y "tls.handshake" -V 2>/dev/null | head -100
        echo ""
        echo "=========================================="
        echo "Connection Statistics"
        echo "=========================================="
        sudo tshark -r "$PCAP_FILE" -q -z conv,tcp 2>/dev/null
    fi
} > "$REPORT_FILE"

echo -e "  Report saved: ${YELLOW}$REPORT_FILE${NC}"
echo ""

# Check for actual TLS handshake
echo -e "${BLUE}[Step 8] TLS Handshake Verification${NC}"
echo "=========================================="

TLS_CLIENT_HELLO=$(sudo tshark -r "$PCAP_FILE" -Y "tls.handshake.type == 1" 2>/dev/null | wc -l)
TLS_SERVER_HELLO=$(sudo tshark -r "$PCAP_FILE" -Y "tls.handshake.type == 2" 2>/dev/null | wc -l)
TLS_CERTIFICATE=$(sudo tshark -r "$PCAP_FILE" -Y "tls.handshake.type == 11" 2>/dev/null | wc -l)

echo "TLS Handshake Messages Detected:"
echo "  Client Hello: $TLS_CLIENT_HELLO"
echo "  Server Hello: $TLS_SERVER_HELLO"
echo "  Certificate: $TLS_CERTIFICATE"
echo ""

if [ $TLS_CLIENT_HELLO -gt 0 ] && [ $TLS_SERVER_HELLO -gt 0 ]; then
    echo -e "${GREEN}✓ Complete TLS handshake captured!${NC}"
else
    echo -e "${YELLOW}⚠ Incomplete or no TLS handshake detected${NC}"
    echo ""
    echo "Troubleshooting steps:"
    echo "  1. Verify AMF-2 is configured with rsyslog-gnutls"
    echo "  2. Check if syslog server is listening on port $SYSLOG_TLS_PORT"
    echo "  3. Trigger AMF-2 to send log messages"
    echo "  4. Verify TLS certificates are properly configured"
fi
echo ""

# Summary
echo -e "${GREEN}=========================================="
echo "Capture Complete!"
echo "==========================================${NC}"
echo ""
echo "Output files:"
echo "  📦 PCAP File: $PCAP_FILE"
echo "  📄 Log File: $LOG_FILE"
echo "  📊 Report: $REPORT_FILE"
echo ""
echo "View with Wireshark:"
echo "  wireshark $PCAP_FILE"
echo ""
echo "Analyze with tshark:"
echo "  tshark -r $PCAP_FILE -Y tls.handshake"
echo ""
echo "Filter for specific handshake types:"
echo "  tshark -r $PCAP_FILE -Y 'tls.handshake.type == 1'  # Client Hello"
echo "  tshark -r $PCAP_FILE -Y 'tls.handshake.type == 2'  # Server Hello"
echo "  tshark -r $PCAP_FILE -Y 'tls.handshake.type == 11' # Certificate"
echo "=========================================="
