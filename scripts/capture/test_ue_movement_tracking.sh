#!/bin/bash
# Test script for UE Movement Tracking Service
# Author: Rishabh Kumar (cs25resch04002)

echo "======================================"
echo "UE Movement Tracking - Test Script"
echo "======================================"
echo ""

# Test IMSI
TEST_IMSI="208990100001100"

echo "Test 1: Current UE Location"
echo "------------------------------"
sudo python3 src/ue_location/ue_location_service.py --imsi $TEST_IMSI
echo ""

echo "Test 2: UE Movement History"
echo "------------------------------"
sudo python3 src/ue_location/ue_location_service.py --imsi $TEST_IMSI --track-movement
echo ""

echo "Test 3: Export Movement Data to JSON"
echo "------------------------------"
sudo python3 src/ue_location/ue_location_service.py --imsi $TEST_IMSI --track-movement --export ue_movement_history.json
echo "✓ Movement data exported to ue_movement_history.json"
echo ""

echo "Test 4: Track All UEs Movement"
echo "------------------------------"
sudo python3 src/ue_location/ue_location_service.py --all --track-movement
echo ""

echo "======================================"
echo "All tests completed!"
echo "======================================"
