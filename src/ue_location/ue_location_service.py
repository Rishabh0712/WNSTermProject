#!/usr/bin/env python3
"""
5G UE Location Service with Movement Tracking
Author: Rishabh Kumar (cs25resch04002)
Email: kumarrishabh73@gmail.com | rishabh.kumar@research.iiit.ac.in

This service extracts UE location information from AMF logs and tracks UE movement
by identifying all gNBs the UE has connected to over time.
It parses logs to find registration events, handovers, cell information, and tracking area updates.
"""

import re
import json
import argparse
from datetime import datetime
from typing import Dict, List, Optional, Tuple
import subprocess
from collections import defaultdict


class UELocationService:
    """Service to track and retrieve UE location and movement from AMF logs."""
    
    def __init__(self, amf_container: str = "rfsim5g-oai-amf"):
        """
        Initialize the UE Location Service.
        
        Args:
            amf_container: Name of the AMF Docker container
        """
        self.amf_container = amf_container
        self.ue_database = {}
        self.ue_movement_history = defaultdict(list)
        
    def get_amf_logs(self) -> str:
        """
        Fetch logs from the AMF container.
        
        Returns:
            String containing AMF logs
        """
        try:
            cmd = f"sudo docker logs {self.amf_container}"
            result = subprocess.run(
                cmd.split(),
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            print(f"Error fetching AMF logs: {e}")
            return ""
    
    def parse_ue_info_table(self, logs: str) -> List[Dict]:
        """
        Parse UE information tables from AMF logs.
        
        Args:
            logs: AMF log content
            
        Returns:
            List of dictionaries containing UE information
        """
        ue_entries = []
        
        # Pattern to match UE information table entries
        # Format: |  Index |     5GMM State     |        IMSI        |        GUTI        |   RAN UE NGAP ID   |   AMF UE NGAP ID   |        PLMN        |       Cell Id      |
        ue_pattern = r'\|\s+(\d+)\s+\|\s+([A-Z0-9-]+)\s+\|\s+(\d+)\s+\|\s*(\d+)\s*\|\s+(\S+)\s+\|\s+(\S+)\s+\|\s+(\d+),(\d+)\s+\|\s+(\w+)\s+\|'
        
        matches = re.finditer(ue_pattern, logs)
        
        for match in matches:
            ue_entry = {
                'index': match.group(1),
                'gmm_state': match.group(2),
                'imsi': match.group(3),
                'guti': match.group(4),
                'ran_ue_ngap_id': match.group(5),
                'amf_ue_ngap_id': match.group(6),
                'mcc': match.group(7),
                'mnc': match.group(8),
                'cell_id': match.group(9),
                'timestamp': datetime.now().isoformat()
            }
            ue_entries.append(ue_entry)
        
        return ue_entries
    
    def parse_ng_setup_request(self, logs: str) -> Dict:
        """
        Parse NG Setup Request to get gNB location information.
        
        Args:
            logs: AMF log content
            
        Returns:
            Dictionary containing gNB information
        """
        gnb_info = {}
        
        # Extract gNB ID
        gnb_id_pattern = r'GlobalGNB-ID ::= \{.*?gNB-ID: ([0-9A-F ]+)'
        match = re.search(gnb_id_pattern, logs, re.DOTALL)
        if match:
            gnb_info['gnb_id'] = match.group(1).strip()
        
        # Extract gNB name
        gnb_name_pattern = r'id: 82.*?value: (\S+)'
        match = re.search(gnb_name_pattern, logs, re.DOTALL)
        if match:
            gnb_info['gnb_name'] = match.group(1)
        
        # Extract TAC (Tracking Area Code)
        tac_pattern = r'tAC: ([0-9A-F ]+)'
        match = re.search(tac_pattern, logs)
        if match:
            gnb_info['tac'] = match.group(1).strip()
        
        # Extract PLMN
        plmn_pattern = r'pLMNIdentity: ([0-9A-F ]+)'
        matches = re.findall(plmn_pattern, logs)
        if matches:
            gnb_info['plmn'] = matches[0].strip()
        
        return gnb_info
    
    def parse_initial_ue_message(self, logs: str, imsi: str) -> Optional[Dict]:
        """
        Parse Initial UE Message to get detailed UE location.
        
        Args:
            logs: AMF log content
            imsi: IMSI to search for
            
        Returns:
            Dictionary with UE location details or None
        """
        # Search for initial UE message context around the IMSI
        imsi_pattern = rf'(.*?{imsi}.*?)(?=\[|\n{{2}})'
        matches = re.finditer(imsi_pattern, logs, re.DOTALL)
        
        for match in matches:
            context = match.group(0)
            
            # Extract Cell ID
            cell_pattern = r'Cell Id.*?([0-9a-fA-F]+)'
            cell_match = re.search(cell_pattern, context)
            
            # Extract TAC
            tac_pattern = r'TAC.*?([0-9]+)'
            tac_match = re.search(tac_pattern, context)
            
            if cell_match or tac_match:
                return {
                    'cell_id': cell_match.group(1) if cell_match else 'N/A',
                    'tac': tac_match.group(1) if tac_match else 'N/A'
                }
        
        return None
    
    def get_gnb_location(self, logs: str) -> Dict:
        """
        Extract gNB location information from logs.
        
        Args:
            logs: AMF log content
            
        Returns:
            Dictionary with gNB location information
        """
        gnb_info = self.parse_ng_setup_request(logs)
        
        # Parse gNB connection information
        connection_pattern = r'\|\s+(\d+)\s+\|\s+(Connected)\s+\|\s+(\S+)\s+\|\s+(\S+)\s+\|\s+(\d+),(\d+)\s+\|'
        match = re.search(connection_pattern, logs)
        
        if match:
            gnb_info.update({
                'status': match.group(2),
                'global_id': match.group(3),
                'name': match.group(4),
                'plmn_mcc': match.group(5),
                'plmn_mnc': match.group(6)
            })
        
        return gnb_info
    
    def get_all_gnbs(self, logs: str) -> List[Dict]:
        """
        Extract all gNB information from logs.
        
        Args:
            logs: AMF log content
            
        Returns:
            List of dictionaries containing gNB information
        """
        gnbs = []
        
        # Pattern to match gNB table entries
        gnb_pattern = r'\|\s+(\d+)\s+\|\s+(\w+)\s+\|\s+(\S+)\s+\|\s+(\S+)\s+\|\s+(\d+),(\d+)\s+\|'
        matches = re.finditer(gnb_pattern, logs)
        
        for match in matches:
            gnb = {
                'index': match.group(1),
                'status': match.group(2),
                'global_id': match.group(3),
                'name': match.group(4),
                'plmn_mcc': match.group(5),
                'plmn_mnc': match.group(6)
            }
            gnbs.append(gnb)
        
        return gnbs
    
    def parse_ue_movement_events(self, logs: str, imsi: str) -> List[Dict]:
        """
        Parse UE movement events from AMF logs to track gNB connections.
        Identifies registration, handovers, and tracking area updates.
        
        Args:
            logs: AMF log content
            imsi: IMSI to track
            
        Returns:
            List of movement events with timestamps and gNB information
        """
        movement_events = []
        
        # Split logs into lines for timestamp-based parsing
        log_lines = logs.split('\n')
        
        # Patterns to identify movement events
        patterns = {
            'registration': rf'.*IMSI.*{imsi}.*(?:Registration|REGISTERED)',
            'handover': rf'.*IMSI.*{imsi}.*(?:Handover|HO)',
            'tau': rf'.*IMSI.*{imsi}.*(?:Tracking Area Update|TAU)',
            'initial_ue_message': rf'.*IMSI.*{imsi}.*Initial UE Message',
            'ue_context_release': rf'.*IMSI.*{imsi}.*UE Context Release',
            'path_switch': rf'.*IMSI.*{imsi}.*Path Switch',
            'ran_ue_ngap': rf'.*{imsi}.*RAN UE NGAP ID',
        }
        
        for i, line in enumerate(log_lines):
            # Extract timestamp from log line
            timestamp_match = re.search(r'\[(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}\.\d+)\]', line)
            timestamp = timestamp_match.group(1) if timestamp_match else datetime.now().isoformat()
            
            # Check each pattern
            for event_type, pattern in patterns.items():
                if re.search(pattern, line, re.IGNORECASE):
                    # Extract context around this event (next 50 lines)
                    context_lines = log_lines[i:min(i+50, len(log_lines))]
                    context = '\n'.join(context_lines)
                    
                    # Extract gNB information from context
                    gnb_id_match = re.search(r'(?:gNB|GNB).*?(?:ID|Id).*?[:\s]+(\S+)', context, re.IGNORECASE)
                    gnb_name_match = re.search(r'(?:gNB|GNB).*?(?:Name|name).*?[:\s]+(\S+)', context, re.IGNORECASE)
                    cell_id_match = re.search(r'Cell.*?(?:ID|Id).*?[:\s]+(\S+)', context, re.IGNORECASE)
                    tac_match = re.search(r'TAC.*?[:\s]+(\S+)', context, re.IGNORECASE)
                    ran_ue_id_match = re.search(r'RAN UE NGAP ID.*?[:\s]+(\d+)', context, re.IGNORECASE)
                    
                    event = {
                        'timestamp': timestamp,
                        'event_type': event_type,
                        'imsi': imsi,
                        'gnb_id': gnb_id_match.group(1) if gnb_id_match else None,
                        'gnb_name': gnb_name_match.group(1) if gnb_name_match else None,
                        'cell_id': cell_id_match.group(1) if cell_id_match else None,
                        'tac': tac_match.group(1) if tac_match else None,
                        'ran_ue_ngap_id': ran_ue_id_match.group(1) if ran_ue_id_match else None,
                    }
                    
                    # Only add if we have some gNB information
                    if event['gnb_id'] or event['gnb_name'] or event['cell_id']:
                        movement_events.append(event)
        
        return movement_events
    
    def get_unique_gnb_connections(self, movement_events: List[Dict]) -> List[Dict]:
        """
        Extract unique gNB connections from movement events.
        
        Args:
            movement_events: List of movement events
            
        Returns:
            List of unique gNB connections with first and last seen timestamps
        """
        gnb_connections = {}
        
        for event in movement_events:
            gnb_key = event.get('gnb_id') or event.get('gnb_name') or event.get('cell_id')
            
            if gnb_key and gnb_key != 'N/A':
                if gnb_key not in gnb_connections:
                    gnb_connections[gnb_key] = {
                        'gnb_id': event.get('gnb_id'),
                        'gnb_name': event.get('gnb_name'),
                        'cell_id': event.get('cell_id'),
                        'tac': event.get('tac'),
                        'first_seen': event['timestamp'],
                        'last_seen': event['timestamp'],
                        'event_count': 1,
                        'event_types': [event['event_type']],
                    }
                else:
                    gnb_connections[gnb_key]['last_seen'] = event['timestamp']
                    gnb_connections[gnb_key]['event_count'] += 1
                    if event['event_type'] not in gnb_connections[gnb_key]['event_types']:
                        gnb_connections[gnb_key]['event_types'].append(event['event_type'])
        
        return list(gnb_connections.values())
    
    def track_ue_movement(self, imsi: str) -> Dict:
        """
        Track UE movement by identifying all gNBs the UE has connected to.
        
        Args:
            imsi: IMSI to track
            
        Returns:
            Dictionary with movement history and statistics
        """
        logs = self.get_amf_logs()
        
        if not logs:
            return {'error': 'Unable to fetch AMF logs'}
        
        # Parse all movement events
        movement_events = self.parse_ue_movement_events(logs, imsi)
        
        if not movement_events:
            return {
                'imsi': imsi,
                'message': 'No movement events found in logs',
                'total_events': 0,
                'unique_gnbs': 0,
                'movement_events': [],
                'gnb_connections': []
            }
        
        # Get unique gNB connections
        gnb_connections = self.get_unique_gnb_connections(movement_events)
        
        # Calculate movement statistics
        movement_stats = {
            'imsi': imsi,
            'total_events': len(movement_events),
            'unique_gnbs': len(gnb_connections),
            'first_event': movement_events[0]['timestamp'] if movement_events else None,
            'last_event': movement_events[-1]['timestamp'] if movement_events else None,
            'event_types': list(set(e['event_type'] for e in movement_events)),
        }
        
        return {
            **movement_stats,
            'gnb_connections': gnb_connections,
            'movement_events': movement_events,
        }
    
    def calculate_approximate_location(self, cell_id: str, gnb_info: Dict) -> Dict:
        """
        Extract location information based on cell information from AMF data only.
        
        Args:
            cell_id: Cell identifier
            gnb_info: gNB information
            
        Returns:
            Dictionary with location information from AMF
        """
        # Use only data extracted from AMF logs - no mock data
        location = {
            'cell_id': cell_id,
            'gnb_name': gnb_info.get('name', gnb_info.get('gnb_name', 'N/A')),
            'gnb_id': gnb_info.get('global_id', gnb_info.get('gnb_id', 'N/A')),
            'tac': gnb_info.get('tac', 'N/A'),
            'plmn': gnb_info.get('plmn', 'N/A'),
        }
        
        return location
    
    def get_ue_location_by_imsi(self, imsi: str) -> Optional[Dict]:
        """
        Get UE location information by IMSI.
        
        Args:
            imsi: International Mobile Subscriber Identity
            
        Returns:
            Dictionary with UE location information or None
        """
        logs = self.get_amf_logs()
        
        if not logs:
            return None
        
        # Parse UE information
        ue_entries = self.parse_ue_info_table(logs)
        
        # Find UE with matching IMSI
        ue_info = None
        for entry in ue_entries:
            if entry['imsi'] == imsi:
                ue_info = entry
                break
        
        if not ue_info:
            return None
        
        # Get gNB information
        gnb_info = self.get_gnb_location(logs)
        
        # Get detailed location
        location_details = self.parse_initial_ue_message(logs, imsi)
        
        # Calculate approximate location
        cell_id = ue_info.get('cell_id', '0000e000')
        approx_location = self.calculate_approximate_location(cell_id, gnb_info)
        
        # Compile complete location information
        location_info = {
            'ue_identity': {
                'imsi': imsi,
                'guti': ue_info.get('guti'),
                'ran_ue_ngap_id': ue_info.get('ran_ue_ngap_id'),
                'amf_ue_ngap_id': ue_info.get('amf_ue_ngap_id'),
            },
            'network_location': {
                'plmn': {
                    'mcc': ue_info.get('mcc'),
                    'mnc': ue_info.get('mnc'),
                },
                'cell_id': cell_id,
                'tac': gnb_info.get('tac', 'N/A'),
                'tracking_area': gnb_info.get('tac', 'N/A'),
            },
            'gnb_info': {
                'gnb_id': gnb_info.get('global_id', gnb_info.get('gnb_id')),
                'gnb_name': gnb_info.get('name', gnb_info.get('gnb_name')),
                'status': gnb_info.get('status', 'Unknown'),
            },
            'geographic_location': approx_location,
            'state': ue_info.get('gmm_state'),
            'last_updated': ue_info.get('timestamp'),
        }
        
        return location_info
    
    def get_ue_location_by_imei(self, imei: str) -> Optional[Dict]:
        """
        Get UE location by IMEI (International Mobile Equipment Identity).
        Extracts IMEI to IMSI mapping from AMF logs only.
        
        Args:
            imei: International Mobile Equipment Identity
            
        Returns:
            Dictionary with UE location information or None
        """
        # Extract IMEI to IMSI mapping from AMF logs
        logs = self.get_amf_logs()
        
        if not logs:
            print("Unable to fetch AMF logs")
            return None
        
        # Try to find IMEI in logs and extract associated IMSI
        # Pattern 1: Look for IMEI in registration/authentication messages
        imei_pattern = rf'IMEI.*?{imei}.*?IMSI.*?(\d{{15}})'
        match = re.search(imei_pattern, logs, re.DOTALL)
        
        if match:
            imsi = match.group(1)
        else:
            # Pattern 2: Look in UE context or identity response messages
            imei_context_pattern = rf'{imei}.*?(?:IMSI|imsi).*?(\d{{15}})'
            match = re.search(imei_context_pattern, logs, re.DOTALL | re.IGNORECASE)
            
            if match:
                imsi = match.group(1)
            else:
                print(f"No IMSI mapping found in AMF logs for IMEI: {imei}")
                print("Note: IMEI may not be present in current AMF logs.")
                print("Make sure UE has completed registration with IMEI identity.")
                return None
        
        location_info = self.get_ue_location_by_imsi(imsi)
        
        if location_info:
            location_info['ue_identity']['imei'] = imei
        
        return location_info
    
    def get_all_ue_locations(self) -> List[Dict]:
        """
        Get location information for all registered UEs.
        
        Returns:
            List of dictionaries with UE location information
        """
        logs = self.get_amf_logs()
        
        if not logs:
            return []
        
        ue_entries = self.parse_ue_info_table(logs)
        gnb_info = self.get_gnb_location(logs)
        
        all_locations = []
        
        for ue_entry in ue_entries:
            imsi = ue_entry.get('imsi')
            if imsi and imsi != '-':
                location_info = self.get_ue_location_by_imsi(imsi)
                if location_info:
                    all_locations.append(location_info)
        
        return all_locations
    
    def export_to_json(self, location_info: Dict, filename: str = None):
        """
        Export location information to JSON file.
        
        Args:
            location_info: Location information dictionary
            filename: Output filename (optional)
        """
        if filename is None:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f'ue_location_{timestamp}.json'
        
        with open(filename, 'w') as f:
            json.dump(location_info, f, indent=2)
        
        print(f"Location information exported to: {filename}")
    
    def display_location(self, location_info: Dict):
        """
        Display location information in a formatted way.
        
        Args:
            location_info: Location information dictionary
        """
        if not location_info:
            print("No location information available.")
            return
        
        print("\n" + "="*70)
        print("UE LOCATION INFORMATION")
        print("="*70)
        
        print("\n📱 UE Identity:")
        ue_id = location_info.get('ue_identity', {})
        print(f"  IMSI: {ue_id.get('imsi', 'N/A')}")
        if 'imei' in ue_id:
            print(f"  IMEI: {ue_id.get('imei', 'N/A')}")
        print(f"  GUTI: {ue_id.get('guti', 'N/A')}")
        print(f"  RAN UE NGAP ID: {ue_id.get('ran_ue_ngap_id', 'N/A')}")
        print(f"  AMF UE NGAP ID: {ue_id.get('amf_ue_ngap_id', 'N/A')}")
        
        print("\n📡 Network Location:")
        net_loc = location_info.get('network_location', {})
        plmn = net_loc.get('plmn', {})
        print(f"  PLMN: MCC={plmn.get('mcc', 'N/A')}, MNC={plmn.get('mnc', 'N/A')}")
        print(f"  Cell ID: {net_loc.get('cell_id', 'N/A')}")
        print(f"  Tracking Area Code (TAC): {net_loc.get('tac', 'N/A')}")
        
        print("\n🗼 gNB Information:")
        gnb = location_info.get('gnb_info', {})
        print(f"  gNB ID: {gnb.get('gnb_id', 'N/A')}")
        print(f"  gNB Name: {gnb.get('gnb_name', 'N/A')}")
        print(f"  Status: {gnb.get('status', 'N/A')}")
        
        print("\n🌍 Cell-Based Location (from AMF logs):")
        geo_loc = location_info.get('geographic_location', {})
        print(f"  Cell ID: {geo_loc.get('cell_id', 'N/A')}")
        print(f"  gNB Name: {geo_loc.get('gnb_name', 'N/A')}")
        print(f"  gNB ID: {geo_loc.get('gnb_id', 'N/A')}")
        print(f"  TAC: {geo_loc.get('tac', 'N/A')}")
        print(f"  PLMN: {geo_loc.get('plmn', 'N/A')}")
        print(f"  Note: Geographic coordinates require external cell database")
        
        print(f"\n📊 State: {location_info.get('state', 'N/A')}")
        print(f"🕐 Last Updated: {location_info.get('last_updated', 'N/A')}")
        
        print("\n" + "="*70 + "\n")
    
    def display_movement_history(self, movement_data: Dict):
        """
        Display UE movement history in a formatted way.
        
        Args:
            movement_data: Movement tracking data
        """
        if not movement_data:
            print("No movement data available.")
            return
        
        if 'error' in movement_data:
            print(f"Error: {movement_data['error']}")
            return
        
        if 'message' in movement_data:
            print(f"\n{movement_data['message']}")
            return
        
        print("\n" + "="*80)
        print("UE MOVEMENT TRACKING")
        print("="*80)
        
        print(f"\n📱 IMSI: {movement_data.get('imsi', 'N/A')}")
        print(f"\n📊 Movement Statistics:")
        print(f"  Total Events: {movement_data.get('total_events', 0)}")
        print(f"  Unique gNB Connections: {movement_data.get('unique_gnbs', 0)}")
        print(f"  First Event: {movement_data.get('first_event', 'N/A')}")
        print(f"  Last Event: {movement_data.get('last_event', 'N/A')}")
        print(f"  Event Types: {', '.join(movement_data.get('event_types', []))}")
        
        # Display gNB connections
        gnb_connections = movement_data.get('gnb_connections', [])
        if gnb_connections:
            print(f"\n🗼 gNB Connections ({len(gnb_connections)} unique):")
            print("-" * 80)
            
            for i, gnb in enumerate(gnb_connections, 1):
                print(f"\n  [{i}] gNB Connection:")
                print(f"      gNB ID: {gnb.get('gnb_id', 'N/A')}")
                print(f"      gNB Name: {gnb.get('gnb_name', 'N/A')}")
                print(f"      Cell ID: {gnb.get('cell_id', 'N/A')}")
                print(f"      TAC: {gnb.get('tac', 'N/A')}")
                print(f"      First Seen: {gnb.get('first_seen', 'N/A')}")
                print(f"      Last Seen: {gnb.get('last_seen', 'N/A')}")
                print(f"      Event Count: {gnb.get('event_count', 0)}")
                print(f"      Event Types: {', '.join(gnb.get('event_types', []))}")
        
        # Display detailed movement events
        movement_events = movement_data.get('movement_events', [])
        if movement_events:
            print(f"\n📍 Detailed Movement Events ({len(movement_events)} total):")
            print("-" * 80)
            
            # Show first 10 and last 5 events if too many
            display_events = movement_events
            if len(movement_events) > 15:
                print(f"\n  Showing first 10 and last 5 events (total: {len(movement_events)})")
                display_events = movement_events[:10] + movement_events[-5:]
            
            for i, event in enumerate(display_events, 1):
                print(f"\n  [{i}] {event['timestamp']}")
                print(f"      Event Type: {event['event_type']}")
                if event.get('gnb_id'):
                    print(f"      gNB ID: {event['gnb_id']}")
                if event.get('gnb_name'):
                    print(f"      gNB Name: {event['gnb_name']}")
                if event.get('cell_id'):
                    print(f"      Cell ID: {event['cell_id']}")
                if event.get('tac'):
                    print(f"      TAC: {event['tac']}")
                if event.get('ran_ue_ngap_id'):
                    print(f"      RAN UE NGAP ID: {event['ran_ue_ngap_id']}")
        
        print("\n" + "="*80 + "\n")


def main():
    """Main function to run the UE Location Service CLI."""
    parser = argparse.ArgumentParser(
        description='5G UE Location Service with Movement Tracking - Extract UE location and track movement',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Get current location by IMSI
  python ue_location_service.py --imsi 208990100001100
  
  # Track UE movement history
  python ue_location_service.py --imsi 208990100001100 --track-movement
  
  # Get location by IMEI
  python ue_location_service.py --imei 862104052096703
  
  # Get all UE locations
  python ue_location_service.py --all
  
  # Export movement data to JSON
  python ue_location_service.py --imsi 208990100001100 --track-movement --export movement.json
  
  # Track all UEs movement
  python ue_location_service.py --all --track-movement
        '''
    )
    
    parser.add_argument('--imsi', type=str, help='IMSI of the UE')
    parser.add_argument('--imei', type=str, help='IMEI of the UE')
    parser.add_argument('--all', action='store_true', help='Get all UE locations')
    parser.add_argument('--track-movement', action='store_true', 
                       help='Track UE movement history (all gNB connections)')
    parser.add_argument('--export', type=str, help='Export to JSON file')
    parser.add_argument('--container', type=str, default='rfsim5g-oai-amf',
                       help='AMF container name (default: rfsim5g-oai-amf)')
    
    args = parser.parse_args()
    
    # Initialize service
    service = UELocationService(amf_container=args.container)
    
    location_info = None
    
    if args.all:
        if args.track_movement:
            # Track movement for all UEs
            all_ue_entries = service.parse_ue_info_table(service.get_amf_logs())
            
            if all_ue_entries:
                print(f"\nTracking movement for {len(all_ue_entries)} UE(s)...\n")
                all_movement_data = []
                
                for ue_entry in all_ue_entries:
                    imsi = ue_entry.get('imsi')
                    if imsi and imsi != '-':
                        movement_data = service.track_ue_movement(imsi)
                        if movement_data.get('total_events', 0) > 0:
                            all_movement_data.append(movement_data)
                            service.display_movement_history(movement_data)
                
                if args.export and all_movement_data:
                    service.export_to_json(all_movement_data, args.export)
            else:
                print("No registered UEs found.")
        else:
            # Get all UE locations
            all_locations = service.get_all_ue_locations()
            if all_locations:
                print(f"\nFound {len(all_locations)} registered UE(s)\n")
                for loc in all_locations:
                    service.display_location(loc)
                
                if args.export:
                    service.export_to_json(all_locations, args.export)
            else:
                print("No registered UEs found.")
    
    elif args.imsi:
        if args.track_movement:
            # Track movement history
            movement_data = service.track_ue_movement(args.imsi)
            service.display_movement_history(movement_data)
            
            if args.export:
                service.export_to_json(movement_data, args.export)
        else:
            # Get current location by IMSI
            location_info = service.get_ue_location_by_imsi(args.imsi)
            if location_info:
                service.display_location(location_info)
                if args.export:
                    service.export_to_json(location_info, args.export)
            else:
                print(f"No location information found for IMSI: {args.imsi}")
    
    elif args.imei:
        # Get location by IMEI (movement tracking not yet supported with IMEI)
        location_info = service.get_ue_location_by_imei(args.imei)
        if location_info:
            service.display_location(location_info)
            
            if args.track_movement:
                # Extract IMSI and track movement
                imsi = location_info.get('ue_identity', {}).get('imsi')
                if imsi:
                    print("\n" + "="*80)
                    print("TRACKING MOVEMENT WITH EXTRACTED IMSI")
                    print("="*80)
                    movement_data = service.track_ue_movement(imsi)
                    service.display_movement_history(movement_data)
                    
                    if args.export:
                        combined_data = {
                            'location': location_info,
                            'movement': movement_data
                        }
                        service.export_to_json(combined_data, args.export)
            elif args.export:
                service.export_to_json(location_info, args.export)
        else:
            print(f"No location information found for IMEI: {args.imei}")
    
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
