# UE Location Service Documentation

## Overview

The UE Location Service is a Python application that extracts and tracks User Equipment (UE) location information from 5G AMF (Access and Mobility Management Function) logs. It can query location data by IMSI or IMEI and provides detailed network and geographic location information.

---

## Features

- ✅ **Location by IMSI** - Query UE location using IMSI (International Mobile Subscriber Identity)
- ✅ **Location by IMEI** - Query UE location using IMEI (International Mobile Equipment Identity)
- ✅ **All UEs Tracking** - Get location information for all registered UEs
- ✅ **Network Details** - Cell ID, Tracking Area Code, PLMN information
- ✅ **gNB Information** - Base station details and status
- ✅ **Geographic Location** - Approximate latitude/longitude coordinates
- ✅ **JSON Export** - Export location data to JSON format
- ✅ **Real-time Parsing** - Parses live AMF Docker container logs

---

## Installation

### Prerequisites

```bash
# Ensure you have Python 3.8+ installed
python3 --version

# Ensure Docker is running and AMF container is active
wsl sudo docker ps | grep amf
```

### No Additional Dependencies Required

The service uses only Python standard library modules:
- `re` - Regular expressions
- `json` - JSON handling
- `argparse` - Command-line parsing
- `subprocess` - Docker log access
- `datetime` - Timestamp handling
- `typing` - Type hints

---

## Usage

### Basic Commands

#### 1. Get Location by IMSI

```bash
# In WSL/Linux
wsl python3 ue_location_service.py --imsi 208990100001100

# In Windows (from WNS directory)
C:/Users/kumarrishabh/Documents/WNS/.venv/Scripts/python.exe ue_location_service.py --imsi 208990100001100
```

**Output Example:**
```
======================================================================
UE LOCATION INFORMATION
======================================================================

📱 UE Identity:
  IMSI: 208990100001100
  GUTI: 2089901004140563820
  RAN UE NGAP ID: 0x01
  AMF UE NGAP ID: 0x01

📡 Network Location:
  PLMN: MCC=208, MNC=99
  Cell ID: 0000e014e
  Tracking Area Code (TAC): 00 00 01

🗼 gNB Information:
  gNB ID: 0x0E00
  gNB Name: gnb-rfsim
  Status: Connected

🌍 Geographic Location:
  Latitude: 48.8606
  Longitude: 2.3376
  Area: Paris West
  Accuracy: N/A

📊 State: 5GMM-REGISTERED
🕐 Last Updated: 2025-11-10T17:02:24.013
======================================================================
```

#### 2. Get Location by IMEI

```bash
wsl python3 ue_location_service.py --imei 862104052096703
```

The service will map IMEI to IMSI and retrieve location information.

#### 3. Get All UE Locations

```bash
wsl python3 ue_location_service.py --all
```

This displays location information for all registered UEs in the network.

#### 4. Export to JSON

```bash
# Export single UE location
wsl python3 ue_location_service.py --imsi 208990100001100 --export ue_location.json

# Export all UE locations
wsl python3 ue_location_service.py --all --export all_ues_location.json
```

#### 5. Specify Custom AMF Container

```bash
wsl python3 ue_location_service.py --imsi 208990100001100 --container my-amf-container
```

---

## Command-Line Arguments

| Argument | Type | Description |
|----------|------|-------------|
| `--imsi` | string | IMSI of the UE (15 digits) |
| `--imei` | string | IMEI of the UE (15 digits) |
| `--all` | flag | Get locations for all registered UEs |
| `--export` | string | Export results to JSON file (specify filename) |
| `--container` | string | AMF container name (default: `rfsim5g-oai-amf`) |

---

## Output Format

### Console Output

The service displays information in a formatted, human-readable layout with sections:
- **UE Identity** - IMSI, IMEI, GUTI, NGAP IDs
- **Network Location** - PLMN, Cell ID, TAC
- **gNB Information** - Base station details
- **Geographic Location** - Coordinates and area
- **State** - Registration state
- **Timestamp** - Last update time

### JSON Output

```json
{
  "ue_identity": {
    "imsi": "208990100001100",
    "imei": "862104052096703",
    "guti": "2089901004140563820",
    "ran_ue_ngap_id": "0x01",
    "amf_ue_ngap_id": "0x01"
  },
  "network_location": {
    "plmn": {
      "mcc": "208",
      "mnc": "99"
    },
    "cell_id": "0000e014e",
    "tac": "00 00 01",
    "tracking_area": "00 00 01"
  },
  "gnb_info": {
    "gnb_id": "0x0E00",
    "gnb_name": "gnb-rfsim",
    "status": "Connected"
  },
  "geographic_location": {
    "lat": 48.8606,
    "lon": 2.3376,
    "area": "Paris West",
    "accuracy": "N/A",
    "cell_id": "0000e014e",
    "gnb_name": "gnb-rfsim"
  },
  "state": "5GMM-REGISTERED",
  "last_updated": "2025-11-10T17:02:24.013"
}
```

---

## How It Works

### 1. Log Extraction
The service executes `sudo docker logs <amf-container>` to fetch AMF logs in real-time.

### 2. Pattern Matching
Uses regular expressions to parse:
- **UE Information Tables** - Extract IMSI, GUTI, Cell ID, state
- **NG Setup Requests** - Extract gNB information and TAC
- **Initial UE Messages** - Extract detailed location data
- **Registration Events** - Track UE state changes

### 3. Data Correlation
Correlates data from multiple log sections to build complete location profile.

### 4. Location Calculation
Maps Cell ID to approximate geographic coordinates (in production, this would query a cell location database).

---

## IMEI to IMSI Mapping

The service includes a mock IMEI-to-IMSI mapping for demonstration:

| IMEI | IMSI |
|------|------|
| 862104052096703 | 208990100001100 |
| 862104052096704 | 208990100001101 |
| 862104052096705 | 208990100001102 |

In a production system, this mapping would be retrieved from:
- HSS (Home Subscriber Server)
- UDM (Unified Data Management)
- Network database

---

## Integration Examples

### Python Script Integration

```python
from ue_location_service import UELocationService

# Initialize service
service = UELocationService()

# Get location by IMSI
location = service.get_ue_location_by_imsi('208990100001100')
if location:
    print(f"UE is in cell: {location['network_location']['cell_id']}")
    print(f"Coordinates: {location['geographic_location']['lat']}, {location['geographic_location']['lon']}")

# Get all UE locations
all_ues = service.get_all_ue_locations()
print(f"Total registered UEs: {len(all_ues)}")
```

### REST API Wrapper

You could wrap this service in a Flask/FastAPI application:

```python
from flask import Flask, jsonify
from ue_location_service import UELocationService

app = Flask(__name__)
service = UELocationService()

@app.route('/location/imsi/<imsi>')
def get_location_by_imsi(imsi):
    location = service.get_ue_location_by_imsi(imsi)
    if location:
        return jsonify(location)
    return jsonify({'error': 'UE not found'}), 404

@app.route('/location/imei/<imei>')
def get_location_by_imei(imei):
    location = service.get_ue_location_by_imei(imei)
    if location:
        return jsonify(location)
    return jsonify({'error': 'UE not found'}), 404

@app.route('/locations/all')
def get_all_locations():
    locations = service.get_all_ue_locations()
    return jsonify({'count': len(locations), 'ues': locations})

if __name__ == '__main__':
    app.run(port=5000)
```

---

## Testing

### Test with Current UE

The 5G setup has a registered UE with:
- **IMSI:** 208990100001100
- **Cell ID:** 0000e014e
- **State:** 5GMM-REGISTERED

Test command:
```bash
wsl python3 ue_location_service.py --imsi 208990100001100
```

### Test All UEs

```bash
wsl python3 ue_location_service.py --all
```

### Test Export

```bash
wsl python3 ue_location_service.py --imsi 208990100001100 --export test_location.json
cat test_location.json
```

---

## Troubleshooting

### Permission Denied

If you get "Permission denied" when accessing Docker:

```bash
# Add sudo
wsl sudo python3 ue_location_service.py --imsi 208990100001100

# Or add user to docker group (already done in setup)
wsl sudo usermod -aG docker $USER
```

### Container Not Found

If AMF container is not running:

```bash
# Check container status
wsl sudo docker ps | grep amf

# Start containers
cd openairinterface5g/ci-scripts/yaml_files/5g_rfsimulator
wsl sudo docker-compose up -d
```

### No UE Found

If no UE information is returned:
1. Verify UE is registered: `wsl sudo docker logs rfsim5g-oai-amf | grep "5GMM-REGISTERED"`
2. Check correct IMSI: `wsl sudo docker logs rfsim5g-oai-amf | grep "IMSI"`
3. Ensure containers are healthy: `wsl sudo docker ps`

---

## Extending the Service

### Add Real Geographic Database

Replace the mock `cell_locations` dictionary with a real database:

```python
import sqlite3

def calculate_approximate_location(self, cell_id: str, gnb_info: Dict) -> Dict:
    conn = sqlite3.connect('cell_locations.db')
    cursor = conn.cursor()
    cursor.execute('SELECT lat, lon, area FROM cells WHERE cell_id = ?', (cell_id,))
    result = cursor.fetchone()
    
    if result:
        return {
            'lat': result[0],
            'lon': result[1],
            'area': result[2],
            'cell_id': cell_id
        }
    return default_location
```

### Add IMEI Database Integration

Connect to HSS/UDM:

```python
def get_ue_location_by_imei(self, imei: str) -> Optional[Dict]:
    # Query HSS database
    imsi = query_hss_database(imei)
    if imsi:
        return self.get_ue_location_by_imsi(imsi)
    return None
```

### Add Historical Tracking

Store location history in a database for tracking UE movement over time.

---

## Use Cases

1. **Network Monitoring** - Track UE distribution across cells
2. **Emergency Services** - Locate UE for E911 calls
3. **Network Optimization** - Analyze cell load and coverage
4. **Troubleshooting** - Debug connection issues by location
5. **Analytics** - Study mobility patterns and handover performance
6. **Security** - Detect anomalous location changes

---

## Performance

- **Query Time:** < 1 second for single UE
- **Log Parsing:** Efficient regex-based parsing
- **Memory Usage:** Minimal (logs processed in streaming fashion)
- **Scalability:** Handles logs from multiple UEs simultaneously

---

## Security Considerations

- Requires `sudo` access to Docker logs
- Contains sensitive subscriber information (IMSI, IMEI)
- Should be run in secure environment
- Consider encryption for exported JSON files
- Add authentication for REST API deployment

---

## Future Enhancements

- [ ] Real-time location updates using log streaming
- [ ] WebSocket support for live tracking
- [ ] Map visualization integration
- [ ] Cell triangulation for improved accuracy
- [ ] Historical location tracking database
- [ ] Integration with external GIS systems
- [ ] Support for multiple AMF instances
- [ ] Location-based alerting system

---

## License

This is an academic project for educational purposes.

**Author:** Rishabh Kumar (cs25resch04002)  
**Email:** kumarrishabh73@gmail.com

---

## References

- 3GPP TS 23.501 - System Architecture for 5G
- 3GPP TS 23.502 - Procedures for 5G System
- 3GPP TS 38.413 - NG Application Protocol (NGAP)
- OpenAirInterface Documentation
