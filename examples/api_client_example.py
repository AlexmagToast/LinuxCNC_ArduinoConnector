#!/usr/bin/env python3
"""
Example client for the Arduino Connector API.
This script demonstrates how to interact with the API to monitor Arduino connections.

Usage:
    python api_client_example.py
"""

import requests
import json
import time
import sys

# Configuration
API_BASE_URL = "http://localhost:8765"

def print_status():
    """Print the overall daemon status"""
    try:
        response = requests.get(f"{API_BASE_URL}/status")
        response.raise_for_status()
        data = response.json()
        
        print("===== Arduino Connector Daemon Status =====")
        print(f"Status: {data['status']}")
        print(f"Message: {data['message']}")
        print(f"Uptime: {data['uptime']}")
        print(f"Arduino Count: {data['arduino_count']}")
        print()
        
        return data['arduino_count']
    except requests.exceptions.RequestException as e:
        print(f"Error fetching status: {e}")
        return 0

def list_arduinos():
    """List all configured Arduinos"""
    try:
        response = requests.get(f"{API_BASE_URL}/arduinos")
        response.raise_for_status()
        arduinos = response.json()
        
        print("===== Configured Arduinos =====")
        for i, arduino in enumerate(arduinos, 1):
            print(f"{i}. {arduino['alias']} ({arduino['device']})")
            print(f"   Status: {arduino['arduino_status']}")
            print(f"   Enabled: {'Yes' if arduino['enabled'] else 'No'}")
            print(f"   Features: {', '.join(arduino['features'])}")
            print()
        
        return [arduino["alias"] for arduino in arduinos]
    except requests.exceptions.RequestException as e:
        print(f"Error fetching Arduino list: {e}")
        return []

def print_arduino_details(alias):
    """Print detailed information about a specific Arduino"""
    try:
        response = requests.get(f"{API_BASE_URL}/arduinos/{alias}")
        print(f"DEBUG: Response status code: {response.status_code}")
        response.raise_for_status()
        details = response.json()
        
        print(f"===== Details for {alias} =====")
        print(f"Component Name: {details['component_name']}")
        print(f"Device: {details['device']}")
        print(f"Serial Port Available: {'Yes' if details['serial_port_available'] else 'No'}")
        print(f"Arduino Status: {details['arduino_status']}")
        print(f"LinuxCNC Status: {details['linuxcnc_status']}")
        print(f"Arduino Reported Uptime: {details['arduino_reported_uptime']}")
        print(f"Connection Uptime: {details['connection_uptime']}")
        
        print("\nPins:")
        print(f"{'Pin Name':<15} | {'Pin Type':<10} | {'HAL Pin Type':<15} | {'HAL Pin Dir':<15} | {'Pin ID':<7} | {'Current Value'}")
        print("-" * 80)
        for pin in details['pins']:
            print(f"{pin['pin_name']:<15} | {pin['pin_type']:<10} | {pin['hal_pin_type']:<15} | {pin['hal_pin_direction']:<15} | {pin['pin_id']:<7} | {pin['current_value']}")
    except requests.exceptions.RequestException as e:
        print(f"Error fetching Arduino details: {e}")

def main():
    """Main function that demonstrates the API client"""
    try:
        arduino_count = print_status()
        
        if arduino_count == 0:
            print("No Arduinos configured or API server not running.")
            return
        
        aliases = list_arduinos()
        
        if not aliases:
            return
        
        # If there are Arduinos, show details for the first one
        if aliases:
            print_arduino_details(aliases[0])
            
        # If there's more than one Arduino, let the user select which one to display
        if len(aliases) > 1:
            print("\nSelect an Arduino to view details (or 'q' to quit):")
            for i, alias in enumerate(aliases, 1):
                print(f"{i}. {alias}")
            
            while True:
                choice = input("Enter selection: ")
                if choice.lower() == 'q':
                    break
                
                try:
                    index = int(choice) - 1
                    if 0 <= index < len(aliases):
                        print("\n")
                        print_arduino_details(aliases[index])
                    else:
                        print("Invalid selection. Please try again.")
                except ValueError:
                    print("Invalid input. Please enter a number or 'q' to quit.")
                
                print("\nSelect another Arduino (or 'q' to quit):")
        
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Unexpected error: {e}")

if __name__ == "__main__":
    main() 