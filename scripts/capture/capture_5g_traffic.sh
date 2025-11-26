#!/bin/bash

# Script: Capture and Analyze 5G Network Traffic with Tshark
# Purpose: Capture packets from 5G simulation and analyze with tshark
# Author: Rishabh Kumar (cs25resch04002)
# Date: November 12, 2025

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Configuration
CAPTURE_DURATION=20
OUTPUT_DIR="./network_captures"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
PCAP_FILE="${OUTPUT_DIR}/5g_traffic_${TIMESTAMP}.pcap"

echo "=========================================="
echo "5G Network Traffic Capture & Analysis"
echo "Using Tshark"
echo "=========================================="
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check if tools are installed
if ! command -v tcpdump &> /dev/null; then
    echo -e "${RED}Error: tcpdump not installed${NC}"
    exit 1
fi

if ! command -v tshark &> /dev/null; then
    echo -e "${RED}Error: tshark not installed${NC}"
    exit 1
fi

# Check Docker containers
echo -e "${BLUE}[Step 1] Checking 5G Container Status${NC}"
echo "=========================================="
RUNNING_CONTAINERS=$(docker ps | grep rfsim5g | wc -l)
echo -e "  Running containers: ${YELLOW}$RUNNING_CONTAINERS${NC}"

if [ $RUNNING_CONTAINERS -eq 0 ]; then
    echo -e "${RED}No 5G containers running!${NC}"
    echo "Please start containers with: docker-compose up -d"
    exit 1
fi

# List key containers
docker ps --format "table {{.Names}}\t{{.Status}}" | grep -E "mysql|amf|smf|upf|gnb|nr-ue" | grep -v "amf-2" | head -7
echo ""

# Detect network interfaces
echo -e "${BLUE}[Step 2] Network Configuration${NC}"
echo "=========================================="

# Get Docker bridge interfaces
INTERFACES=$(ip link show | grep -E 'docker0|br-' | awk -F: '{print $2}' | tr -d ' ')
echo "Available Docker interfaces:"
ip link show | grep -E 'docker0|br-' | awk '{print "  " $2}'
echo ""

# Use the 5G public network bridge
BRIDGE_INTERFACE=$(docker network inspect rfsim5g-oai-public-net -f '{{.Id}}' 2>/dev/null | cut -c1-12)
if [ -n "$BRIDGE_INTERFACE" ]; then
    CAPTURE_INTERFACE="br-${BRIDGE_INTERFACE}"
    echo -e "  Capture Interface: ${YELLOW}$CAPTURE_INTERFACE${NC}"
    echo "  Network: rfsim5g-oai-public-net (192.168.71.0/26)"
else
    CAPTURE_INTERFACE="any"
    echo -e "  Capture Interface: ${YELLOW}$CAPTURE_INTERFACE${NC} (all interfaces)"
fi
echo ""

# Start capture
echo -e "${BLUE}[Step 3] Starting Packet Capture${NC}"
echo "=========================================="
echo -e "${YELLOW}Capturing 5G network traffic for $CAPTURE_DURATION seconds...${NC}"
echo "Protocols: SCTP (NGAP), GTP-U, HTTP, MySQL, etc."
echo ""
echo "Started at: $(date)"

# Run tcpdump
timeout $CAPTURE_DURATION sudo tcpdump -i "$CAPTURE_INTERFACE" \
    -w "$PCAP_FILE" \
    -s 0 \
    'not port 22' \
    2>&1 | tee "${OUTPUT_DIR}/capture_log_${TIMESTAMP}.txt" &

TCPDUMP_PID=$!

# Progress bar
for ((i=1; i<=CAPTURE_DURATION; i++)); do
    PERCENT=$((i * 100 / CAPTURE_DURATION))
    BAR=$(printf "%${i}s" | tr ' ' '=')
    echo -ne "  Progress: [${BAR}>$(printf "%$((CAPTURE_DURATION - i))s")] ${PERCENT}% \r"
    sleep 1
done
echo ""

wait $TCPDUMP_PID 2>/dev/null
echo "Stopped at: $(date)"
echo ""

# Check if file was created
if [ ! -f "$PCAP_FILE" ]; then
    echo -e "${RED}Error: Capture file not created${NC}"
    exit 1
fi

FILE_SIZE=$(stat -f%z "$PCAP_FILE" 2>/dev/null || stat -c%s "$PCAP_FILE" 2>/dev/null)
echo -e "  File size: ${YELLOW}$(numfmt --to=iec-i --suffix=B $FILE_SIZE 2>/dev/null || echo $FILE_SIZE bytes)${NC}"
echo ""

# Analyze with tshark
echo -e "${BLUE}[Step 4] Analyzing with Tshark${NC}"
echo "=========================================="

# Get packet count
PACKET_COUNT=$(sudo tshark -r "$PCAP_FILE" 2>/dev/null | wc -l)
echo -e "  Total packets captured: ${YELLOW}$PACKET_COUNT${NC}"
echo ""

if [ $PACKET_COUNT -eq 0 ]; then
    echo -e "${YELLOW}⚠ No packets captured${NC}"
    echo "Try running with 'any' interface or check container networking"
    exit 1
fi

# Protocol hierarchy
echo -e "${YELLOW}Protocol Hierarchy:${NC}"
echo "----------------------------------------"
sudo tshark -r "$PCAP_FILE" -q -z io,phs 2>/dev/null | head -30
echo ""

# Protocol statistics
echo -e "${YELLOW}Protocol Distribution:${NC}"
echo "----------------------------------------"
sudo tshark -r "$PCAP_FILE" -q -z prot,colinfo,protocol,protocol 2>/dev/null | sort -k2 -rn | head -15
echo ""

# Top talkers
echo -e "${YELLOW}Top IP Conversations:${NC}"
echo "----------------------------------------"
sudo tshark -r "$PCAP_FILE" -q -z conv,ip 2>/dev/null | tail -n +6 | head -15
echo ""

# Look for 5G specific protocols
echo -e "${YELLOW}5G Protocol Analysis:${NC}"
echo "----------------------------------------"

# NGAP (3GPP 5G signaling)
NGAP_COUNT=$(sudo tshark -r "$PCAP_FILE" -Y "ngap" 2>/dev/null | wc -l)
echo -e "  NGAP (5G Signaling): ${YELLOW}$NGAP_COUNT packets${NC}"

# GTP-U (User plane)
GTPU_COUNT=$(sudo tshark -r "$PCAP_FILE" -Y "gtp" 2>/dev/null | wc -l)
echo -e "  GTP-U (User Plane): ${YELLOW}$GTPU_COUNT packets${NC}"

# SCTP (Transport for NGAP)
SCTP_COUNT=$(sudo tshark -r "$PCAP_FILE" -Y "sctp" 2>/dev/null | wc -l)
echo -e "  SCTP (Transport): ${YELLOW}$SCTP_COUNT packets${NC}"

# HTTP/REST API
HTTP_COUNT=$(sudo tshark -r "$PCAP_FILE" -Y "http" 2>/dev/null | wc -l)
echo -e "  HTTP (REST APIs): ${YELLOW}$HTTP_COUNT packets${NC}"

# MySQL
MYSQL_COUNT=$(sudo tshark -r "$PCAP_FILE" -Y "mysql" 2>/dev/null | wc -l)
echo -e "  MySQL: ${YELLOW}$MYSQL_COUNT packets${NC}"
echo ""

# Sample packets
if [ $NGAP_COUNT -gt 0 ]; then
    echo -e "${YELLOW}Sample NGAP Messages:${NC}"
    echo "----------------------------------------"
    sudo tshark -r "$PCAP_FILE" -Y "ngap" -T fields \
        -e frame.number -e ip.src -e ip.dst -e ngap.procedureCode \
        -E header=y -E separator='|' 2>/dev/null | column -t -s '|' | head -10
    echo ""
fi

if [ $GTPU_COUNT -gt 0 ]; then
    echo -e "${YELLOW}Sample GTP-U Tunnels:${NC}"
    echo "----------------------------------------"
    sudo tshark -r "$PCAP_FILE" -Y "gtp" -T fields \
        -e frame.number -e ip.src -e ip.dst -e gtp.teid \
        -E header=y -E separator='|' 2>/dev/null | column -t -s '|' | head -10
    echo ""
fi

# Generate detailed report
echo -e "${BLUE}[Step 5] Generating Analysis Report${NC}"
echo "=========================================="
REPORT_FILE="${OUTPUT_DIR}/tshark_analysis_${TIMESTAMP}.txt"

{
    echo "=========================================="
    echo "5G Network Traffic Analysis Report"
    echo "Using Tshark"
    echo "=========================================="
    echo ""
    echo "Capture Details:"
    echo "  Date: $(date)"
    echo "  Interface: $CAPTURE_INTERFACE"
    echo "  Duration: $CAPTURE_DURATION seconds"
    echo "  Total Packets: $PACKET_COUNT"
    echo "  File: $PCAP_FILE"
    echo "  Size: $(numfmt --to=iec-i --suffix=B $FILE_SIZE 2>/dev/null || echo $FILE_SIZE bytes)"
    echo ""
    echo "5G Protocol Counts:"
    echo "  NGAP: $NGAP_COUNT"
    echo "  GTP-U: $GTPU_COUNT"
    echo "  SCTP: $SCTP_COUNT"
    echo "  HTTP: $HTTP_COUNT"
    echo "  MySQL: $MYSQL_COUNT"
    echo ""
    echo "=========================================="
    echo "Full Protocol Hierarchy"
    echo "=========================================="
    sudo tshark -r "$PCAP_FILE" -q -z io,phs 2>/dev/null
    echo ""
    echo "=========================================="
    echo "All Conversations"
    echo "=========================================="
    sudo tshark -r "$PCAP_FILE" -q -z conv,ip 2>/dev/null
    echo ""
    echo "=========================================="
    echo "First 50 Packets Detail"
    echo "=========================================="
    sudo tshark -r "$PCAP_FILE" -V -c 50 2>/dev/null
} > "$REPORT_FILE"

echo -e "  Report saved: ${YELLOW}$REPORT_FILE${NC}"
echo ""

# Summary
echo -e "${GREEN}=========================================="
echo "Capture & Analysis Complete!"
echo "==========================================${NC}"
echo ""
echo "Generated files:"
echo "  📦 PCAP: $PCAP_FILE"
echo "  📊 Report: $REPORT_FILE"
echo ""
echo "Analysis commands:"
echo ""
echo "1. View protocol hierarchy:"
echo "   sudo tshark -r $PCAP_FILE -q -z io,phs"
echo ""
echo "2. Filter 5G NGAP messages:"
echo "   sudo tshark -r $PCAP_FILE -Y ngap"
echo ""
echo "3. Filter GTP-U traffic:"
echo "   sudo tshark -r $PCAP_FILE -Y gtp"
echo ""
echo "4. Export to JSON:"
echo "   sudo tshark -r $PCAP_FILE -T json > traffic.json"
echo ""
echo "5. Open in Wireshark:"
echo "   wireshark $PCAP_FILE"
echo ""
echo "=========================================="
