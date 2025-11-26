#!/bin/bash

# Script: Simple TLS Packet Capture and Analysis
# Purpose: Capture TLS handshake for secure syslog without requiring tshark
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 12, 2025

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
CAPTURE_DURATION=30
SYSLOG_TLS_PORT=6514
AMF2_CONTAINER="rfsim5g-oai-amf-2"
OUTPUT_DIR="./syslog_tls_captures"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
PCAP_FILE="${OUTPUT_DIR}/syslog_tls_${TIMESTAMP}.pcap"

echo "=========================================="
echo "TLS Syslog Packet Capture"
echo "=========================================="
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if tcpdump is installed
if ! command -v tcpdump &> /dev/null; then
    echo -e "${RED}Error: tcpdump is not installed${NC}"
    exit 1
fi

# Check AMF-2 status
echo -e "${BLUE}[Step 1] Checking AMF-2 Container${NC}"
echo "=========================================="
if docker ps | grep -q "$AMF2_CONTAINER"; then
    echo -e "${GREEN}✓ AMF-2 is running${NC}"
    AMF2_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$AMF2_CONTAINER")
    echo -e "  AMF-2 IP: ${YELLOW}$AMF2_IP${NC}"
else
    echo -e "${YELLOW}⚠ AMF-2 is not running${NC}"
    echo "Capturing traffic on port $SYSLOG_TLS_PORT from any source..."
fi
echo ""

# Detect network interface
echo -e "${BLUE}[Step 2] Network Configuration${NC}"
echo "=========================================="
DOCKER_INTERFACE=$(ip route | grep "192.168.71.0/26" | awk '{print $3}' | head -1)
if [ -z "$DOCKER_INTERFACE" ]; then
    DOCKER_INTERFACE="any"
fi
echo -e "  Interface: ${YELLOW}$DOCKER_INTERFACE${NC}"
echo -e "  Port: ${YELLOW}$SYSLOG_TLS_PORT${NC}"
echo -e "  Output: ${YELLOW}$PCAP_FILE${NC}"
echo ""

# Start capture
echo -e "${BLUE}[Step 3] Starting Packet Capture${NC}"
echo "=========================================="
echo -e "${YELLOW}Capturing for $CAPTURE_DURATION seconds...${NC}"
echo "Capture started at: $(date)"
echo ""

# Run tcpdump with timeout
timeout $CAPTURE_DURATION sudo tcpdump -i "$DOCKER_INTERFACE" \
    -w "$PCAP_FILE" \
    "tcp port $SYSLOG_TLS_PORT" \
    -v 2>&1 | tee "${OUTPUT_DIR}/capture_output_${TIMESTAMP}.txt" &

TCPDUMP_PID=$!

# Wait for capture to complete
for ((i=1; i<=CAPTURE_DURATION; i++)); do
    echo -ne "  Progress: [$i/$CAPTURE_DURATION] \r"
    sleep 1
done
echo ""

wait $TCPDUMP_PID 2>/dev/null
echo "Capture stopped at: $(date)"
echo ""

# Basic analysis with tcpdump
echo -e "${BLUE}[Step 4] Analyzing Captured Packets${NC}"
echo "=========================================="

if [ ! -f "$PCAP_FILE" ]; then
    echo -e "${RED}Error: Capture file not created${NC}"
    exit 1
fi

PACKET_COUNT=$(sudo tcpdump -r "$PCAP_FILE" 2>&1 | grep -v "reading from" | wc -l)
echo -e "  Total packets: ${YELLOW}$PACKET_COUNT${NC}"
echo ""

if [ $PACKET_COUNT -eq 0 ]; then
    echo -e "${YELLOW}⚠ No packets captured on port $SYSLOG_TLS_PORT${NC}"
    echo ""
    echo "This could mean:"
    echo "  • Secure syslog is not configured yet"
    echo "  • No log messages were generated during capture"
    echo "  • Different port is being used"
else
    echo -e "${GREEN}✓ Packets captured successfully!${NC}"
    echo ""
    
    # Show packet summary
    echo -e "${YELLOW}Packet Summary:${NC}"
    echo "----------------------------------------"
    sudo tcpdump -r "$PCAP_FILE" -nn -q 2>&1 | head -20
    echo ""
    
    # Count unique connections
    echo -e "${YELLOW}Connection Statistics:${NC}"
    echo "----------------------------------------"
    CONNECTIONS=$(sudo tcpdump -r "$PCAP_FILE" -nn 2>&1 | grep -oP '\d+\.\d+\.\d+\.\d+\.\d+ > \d+\.\d+\.\d+\.\d+\.\d+' | sort -u | wc -l)
    echo -e "  Unique connections: ${YELLOW}$CONNECTIONS${NC}"
    
    # Look for TLS handshake indicators
    echo ""
    echo -e "${YELLOW}TLS Indicators:${NC}"
    echo "----------------------------------------"
    
    # Check for SYN packets (connection establishment)
    SYN_COUNT=$(sudo tcpdump -r "$PCAP_FILE" 'tcp[tcpflags] & tcp-syn != 0' 2>&1 | grep -v "reading from" | wc -l)
    echo -e "  TCP SYN packets: ${YELLOW}$SYN_COUNT${NC}"
    
    # Check for data packets (likely TLS)
    DATA_COUNT=$(sudo tcpdump -r "$PCAP_FILE" 'tcp[((tcp[12:1] & 0xf0) >> 2):4] = 0x16030000:0xFFFF0000' 2>&1 | grep -v "reading from" | wc -l)
    if [ $DATA_COUNT -gt 0 ]; then
        echo -e "  TLS handshake packets: ${GREEN}$DATA_COUNT${NC}"
    else
        echo -e "  TLS handshake packets: ${YELLOW}0 (may be encrypted)${NC}"
    fi
    
    # Show packet sizes (TLS handshakes typically have specific sizes)
    echo ""
    echo -e "${YELLOW}Packet Size Distribution:${NC}"
    echo "----------------------------------------"
    sudo tcpdump -r "$PCAP_FILE" -nn 2>&1 | grep "length" | awk '{print $NF}' | sort -n | uniq -c | tail -10
fi
echo ""

# Generate report
echo -e "${BLUE}[Step 5] Generating Report${NC}"
echo "=========================================="
REPORT_FILE="${OUTPUT_DIR}/analysis_${TIMESTAMP}.txt"

{
    echo "=========================================="
    echo "TLS Syslog Capture Analysis Report"
    echo "=========================================="
    echo ""
    echo "Capture Details:"
    echo "  Date: $(date)"
    echo "  Interface: $DOCKER_INTERFACE"
    echo "  Port: $SYSLOG_TLS_PORT"
    echo "  Duration: $CAPTURE_DURATION seconds"
    echo "  Total Packets: $PACKET_COUNT"
    echo ""
    echo "Files:"
    echo "  PCAP: $PCAP_FILE"
    echo ""
    
    if [ $PACKET_COUNT -gt 0 ]; then
        echo "=========================================="
        echo "Detailed Packet List"
        echo "=========================================="
        sudo tcpdump -r "$PCAP_FILE" -nn -tttt -v 2>&1 | head -50
        echo ""
        echo "=========================================="
        echo "Full Packet Hex Dump (First 5 packets)"
        echo "=========================================="
        sudo tcpdump -r "$PCAP_FILE" -nn -X -c 5 2>&1
    fi
} > "$REPORT_FILE"

echo -e "  Report: ${YELLOW}$REPORT_FILE${NC}"
echo ""

# Summary
echo -e "${GREEN}=========================================="
echo "Capture Complete!"
echo "==========================================${NC}"
echo ""
echo "Generated files:"
echo "  📦 PCAP: $PCAP_FILE"
echo "  📊 Report: $REPORT_FILE"
echo ""

if [ $PACKET_COUNT -gt 0 ]; then
    echo "Analysis options:"
    echo ""
    echo "1. View with tcpdump:"
    echo "   sudo tcpdump -r $PCAP_FILE -nn -v"
    echo ""
    echo "2. View hex dump:"
    echo "   sudo tcpdump -r $PCAP_FILE -XX"
    echo ""
    echo "3. Filter by IP:"
    echo "   sudo tcpdump -r $PCAP_FILE host $AMF2_IP"
    echo ""
    echo "4. Count packets by type:"
    echo "   sudo tcpdump -r $PCAP_FILE -nn | awk '{print \$5}' | sort | uniq -c"
else
    echo -e "${YELLOW}Next steps:${NC}"
    echo "  1. Configure secure syslog in AMF-2"
    echo "  2. Trigger log generation (register UE, send data)"
    echo "  3. Run this script again"
fi
echo "=========================================="
