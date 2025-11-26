# UE Movement Tracking Service

**Author**: Rishabh Kumar (cs25resch04002)  
**Date**: November 26, 2025  
**Version**: 2.0

---

## Overview

The enhanced UE Location Service now includes **movement tracking** capabilities that identify all gNBs (base stations) a UE has connected to over time by analyzing AMF logs. This provides insights into UE mobility patterns, handovers, and network behavior.

---

## Key Features

### ✅ Movement Tracking
- **All gNB Connections**: Identifies every gNB the UE has connected to
- **Timeline Analysis**: Timestamps for first and last connection to each gNB
- **Event Classification**: Categorizes events (registration, handover, TAU, etc.)
- **Statistics**: Total events, unique gNBs, event types

### ✅ Event Detection
The service detects the following movement-related events from AMF logs:
- **Registration**: Initial UE registration events
- **Handover**: Inter-gNB handovers
- **Tracking Area Update (TAU)**: Location updates
- **Initial UE Message**: New connections
- **UE Context Release**: Disconnection events
- **Path Switch**: Path switching events

### ✅ Data Extraction
For each gNB connection, the service extracts:
- gNB ID (Global gNB Identifier)
- gNB Name
- Cell ID
- Tracking Area Code (TAC)
- First seen timestamp
- Last seen timestamp
- Event count
- Event types

---

## Usage

### Basic Commands

#### 1. Get Current Location (Existing)
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100
```

#### 2. Track Movement History (NEW)
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement
```

#### 3. Export Movement Data
```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement --export movement.json
```

#### 4. Track All UEs Movement
```bash
sudo python3 src/ue_location/ue_location_service.py --all --track-movement
```

#### 5. Track by IMEI (with movement)
```bash
sudo python3 src/ue_location/ue_location_service.py --imei 862104052096703 --track-movement
```

---

## Command-Line Options

| Option | Description |
|--------|-------------|
| `--imsi <IMSI>` | Track specific UE by IMSI |
| `--imei <IMEI>` | Track specific UE by IMEI |
| `--all` | Track all registered UEs |
| `--track-movement` | **NEW**: Enable movement tracking |
| `--export <file>` | Export data to JSON file |
| `--container <name>` | Specify AMF container name (default: rfsim5g-oai-amf) |

---

## Output Format

### Movement Tracking Display

```
================================================================================
UE MOVEMENT TRACKING
================================================================================

📱 IMSI: 208990100001100

📊 Movement Statistics:
  Total Events: 45
  Unique gNB Connections: 3
  First Event: 2025-11-26T10:15:32.123456
  Last Event: 2025-11-26T16:45:18.789012
  Event Types: registration, handover, tau, initial_ue_message

🗼 gNB Connections (3 unique):
--------------------------------------------------------------------------------

  [1] gNB Connection:
      gNB ID: 0x0E00
      gNB Name: gnb-rfsim
      Cell ID: 0000e000
      TAC: 0x0001
      First Seen: 2025-11-26T10:15:32.123456
      Last Seen: 2025-11-26T12:30:45.678901
      Event Count: 15
      Event Types: registration, initial_ue_message

  [2] gNB Connection:
      gNB ID: 0x0E01
      gNB Name: gnb-rfsim-2
      Cell ID: 0000e001
      TAC: 0x0002
      First Seen: 2025-11-26T12:35:12.345678
      Last Seen: 2025-11-26T15:20:33.456789
      Event Count: 20
      Event Types: handover, tau

  [3] gNB Connection:
      gNB ID: 0x0E02
      gNB Name: gnb-rfsim-3
      Cell ID: 0000e002
      TAC: 0x0003
      First Seen: 2025-11-26T15:25:45.567890
      Last Seen: 2025-11-26T16:45:18.789012
      Event Count: 10
      Event Types: handover, path_switch

📍 Detailed Movement Events (45 total):

  Showing first 10 and last 5 events (total: 45)

  [1] 2025-11-26T10:15:32.123456
      Event Type: registration
      gNB ID: 0x0E00
      gNB Name: gnb-rfsim
      Cell ID: 0000e000
      TAC: 0x0001
      RAN UE NGAP ID: 1

  [2] 2025-11-26T10:18:45.234567
      Event Type: initial_ue_message
      gNB ID: 0x0E00
      Cell ID: 0000e000
      ...

================================================================================
```

---

## JSON Export Format

### Movement Data Structure

```json
{
  "imsi": "208990100001100",
  "total_events": 45,
  "unique_gnbs": 3,
  "first_event": "2025-11-26T10:15:32.123456",
  "last_event": "2025-11-26T16:45:18.789012",
  "event_types": ["registration", "handover", "tau", "initial_ue_message"],
  
  "gnb_connections": [
    {
      "gnb_id": "0x0E00",
      "gnb_name": "gnb-rfsim",
      "cell_id": "0000e000",
      "tac": "0x0001",
      "first_seen": "2025-11-26T10:15:32.123456",
      "last_seen": "2025-11-26T12:30:45.678901",
      "event_count": 15,
      "event_types": ["registration", "initial_ue_message"]
    },
    {
      "gnb_id": "0x0E01",
      "gnb_name": "gnb-rfsim-2",
      "cell_id": "0000e001",
      "tac": "0x0002",
      "first_seen": "2025-11-26T12:35:12.345678",
      "last_seen": "2025-11-26T15:20:33.456789",
      "event_count": 20,
      "event_types": ["handover", "tau"]
    }
  ],
  
  "movement_events": [
    {
      "timestamp": "2025-11-26T10:15:32.123456",
      "event_type": "registration",
      "imsi": "208990100001100",
      "gnb_id": "0x0E00",
      "gnb_name": "gnb-rfsim",
      "cell_id": "0000e000",
      "tac": "0x0001",
      "ran_ue_ngap_id": "1"
    },
    ...
  ]
}
```

---

## Use Cases

### 1. Network Coverage Analysis
Track which gNBs UEs connect to most frequently to identify coverage gaps.

```bash
sudo python3 src/ue_location/ue_location_service.py --all --track-movement --export coverage_analysis.json
```

### 2. Handover Performance
Analyze UE handovers between gNBs to evaluate handover success rates.

```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement | grep -i handover
```

### 3. User Mobility Patterns
Study how UEs move through the network for capacity planning.

```bash
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement
```

### 4. Tracking Area Optimization
Identify tracking area updates to optimize TAC configuration.

```bash
sudo python3 src/ue_location/ue_location_service.py --all --track-movement --export tau_analysis.json
```

---

## Implementation Details

### Key Functions

#### 1. `parse_ue_movement_events(logs, imsi)`
- Parses AMF logs line by line
- Identifies movement-related events using regex patterns
- Extracts gNB information from event context
- Returns list of movement events with timestamps

#### 2. `get_unique_gnb_connections(movement_events)`
- Deduplicates gNB connections
- Calculates first/last seen timestamps
- Counts events per gNB
- Returns unique gNB connection list

#### 3. `track_ue_movement(imsi)`
- Main movement tracking function
- Calls parsing and deduplication functions
- Calculates movement statistics
- Returns comprehensive movement data

#### 4. `display_movement_history(movement_data)`
- Formats and displays movement data
- Shows statistics, gNB connections, and events
- Handles large datasets intelligently (first 10 + last 5)

---

## Event Patterns

### Detected Patterns

The service uses the following regex patterns to detect events:

```python
patterns = {
    'registration': r'.*IMSI.*{imsi}.*(?:Registration|REGISTERED)',
    'handover': r'.*IMSI.*{imsi}.*(?:Handover|HO)',
    'tau': r'.*IMSI.*{imsi}.*(?:Tracking Area Update|TAU)',
    'initial_ue_message': r'.*IMSI.*{imsi}.*Initial UE Message',
    'ue_context_release': r'.*IMSI.*{imsi}.*UE Context Release',
    'path_switch': r'.*IMSI.*{imsi}.*Path Switch',
    'ran_ue_ngap': r'.*{imsi}.*RAN UE NGAP ID',
}
```

---

## Testing

### Run Test Script

```bash
cd /mnt/c/Users/kumarrishabh/Documents/WNS
chmod +x scripts/capture/test_ue_movement_tracking.sh
./scripts/capture/test_ue_movement_tracking.sh
```

### Manual Testing

```bash
# Test with known IMSI
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement

# Verify JSON export
sudo python3 src/ue_location/ue_location_service.py --imsi 208990100001100 --track-movement --export test_movement.json
cat test_movement.json | jq .

# Test with multiple UEs
sudo python3 src/ue_location/ue_location_service.py --all --track-movement
```

---

## Performance Considerations

### Log Size
- Large AMF log files may take longer to parse
- Consider filtering logs by time range if needed
- Use `--export` to save results for offline analysis

### Memory Usage
- Movement events are stored in memory during parsing
- For very long-running systems, consider log rotation
- Export and clear data periodically

---

## Limitations

1. **Log-Based Tracking**: Only events present in AMF logs are tracked
2. **Timestamp Accuracy**: Depends on AMF log timestamp format
3. **Log Retention**: Historical data limited by log retention policy
4. **Single AMF**: Tracks UEs connected to one AMF container only

---

## Future Enhancements

- [ ] Real-time streaming of movement events
- [ ] Geographic mapping with cell database
- [ ] Predictive movement analysis using ML
- [ ] Multi-AMF tracking support
- [ ] Visualization dashboard
- [ ] Performance metrics (handover latency, etc.)
- [ ] Alert system for abnormal movement patterns

---

## Troubleshooting

### No Movement Events Found

**Problem**: `No movement events found in logs`

**Solutions**:
1. Verify UE is registered: `sudo docker logs rfsim5g-oai-amf | grep "5GMM-REGISTERED"`
2. Check IMSI is correct
3. Ensure AMF logs contain movement events
4. Try with `--all` flag to see all UEs

### Incomplete gNB Information

**Problem**: Some gNB fields show `N/A`

**Causes**:
- Log format variations
- Missing context in logs
- Event type doesn't include all fields

**Solutions**:
- This is expected for some event types
- Check detailed movement events for more info
- Combine data from multiple events

### Performance Issues

**Problem**: Slow parsing for large logs

**Solutions**:
```bash
# Filter logs by time
sudo docker logs rfsim5g-oai-amf --since 1h | grep IMSI

# Use specific container
python3 ue_location_service.py --imsi <IMSI> --container rfsim5g-oai-amf-2
```

---

## Example Output

See `scripts/capture/test_ue_movement_tracking.sh` for example usage and expected output.

---

## Related Files

- **Source**: `src/ue_location/ue_location_service.py`
- **Test Script**: `scripts/capture/test_ue_movement_tracking.sh`
- **Documentation**: This file

---

## Version History

### Version 2.0 (November 26, 2025)
- ✅ Added movement tracking capability
- ✅ Event detection for multiple event types
- ✅ gNB connection history
- ✅ Movement statistics
- ✅ Enhanced JSON export with movement data
- ✅ Support for tracking all UEs

### Version 1.0 (November 10, 2025)
- Initial release with location extraction
- IMSI/IMEI-based queries
- Basic gNB information
- JSON export

---

**Status**: ✅ **Implemented and Tested**  
**Last Updated**: November 26, 2025
