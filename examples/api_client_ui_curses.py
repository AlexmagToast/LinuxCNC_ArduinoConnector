#!/usr/bin/env python3
import curses
import requests
import json
import time
import sys
import datetime
import os
import traceback
import locale

# Setup debugging
DEBUG_LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ui_debug.log")

# Add an argument to force test the API
FORCE_TEST_API = "--test-api" in sys.argv

def debug_log(message):
    """Write debug message to log file"""
    with open(DEBUG_LOG, "a") as f:
        f.write(f"{datetime.datetime.now()}: {message}\n")

# Test API directly if requested
def test_api_directly():
    debug_log("=== Testing API directly ===")
    try:
        # Get list of Arduinos
        arduino_list_url = f"{API_BASE_URL}/arduinos"
        debug_log(f"Requesting: {arduino_list_url}")
        response = requests.get(arduino_list_url)
        debug_log(f"Status code: {response.status_code}")
        
        if response.status_code != 200:
            debug_log(f"Error: {response.text}")
            return
        
        arduinos = response.json()
        debug_log(f"Found {len(arduinos)} Arduinos")
        
        # Get details for each Arduino
        for arduino in arduinos:
            alias = arduino['alias']
            detail_url = f"{API_BASE_URL}/arduinos/{alias}"
            debug_log(f"Requesting: {detail_url}")
            detail_response = requests.get(detail_url)
            debug_log(f"Status code: {detail_response.status_code}")
            
            if detail_response.status_code != 200:
                debug_log(f"Error for {alias}: {detail_response.text}")
                continue
            
            details = detail_response.json()
            pins = details.get('pins', [])
            enabled = details.get('enabled', True)
            debug_log(f"Arduino: {alias}, Status: {details.get('arduino_status')}, Enabled: {enabled}, Pins: {len(pins)}")
            
            # Log pin data
            for pin in pins:
                debug_log(f"  Pin: {pin.get('pin_name')}, ID: {pin.get('pin_id')}, Value: {pin.get('current_value')}")
    
    except Exception as e:
        debug_log(f"API test error: {str(e)}")
        debug_log(traceback.format_exc())

# Log environment information
debug_log("=== Starting UI with debug logging ===")
debug_log(f"Python version: {sys.version}")
debug_log(f"Terminal type: {os.environ.get('TERM', 'Unknown')}")
debug_log(f"Locale: {locale.getlocale()}")
for key, value in os.environ.items():
    if key.startswith('LC_') or key in ('LANG', 'TERM', 'COLORTERM', 'TERMINFO'):
        debug_log(f"Env: {key}={value}")

# Try to initialize locale
try:
    locale.setlocale(locale.LC_ALL, '')
    debug_log(f"Locale set to: {locale.getlocale()}")
except Exception as e:
    debug_log(f"Locale error: {str(e)}")

# Configuration
API_BASE_URL = "http://localhost:8765"

# Run API test if requested
if FORCE_TEST_API:
    test_api_directly()
    debug_log("API test complete")

def format_duration(seconds):
    """Convert seconds to days, hours, minutes, seconds format"""
    if seconds is None or seconds == "N/A" or seconds == "None":
        return "N/A"
    
    # Handle strings with 's' suffix like '0s'
    if isinstance(seconds, str):
        if seconds.endswith('s'):
            try:
                seconds = int(seconds[:-1])  # Remove the 's' and convert to int
            except ValueError:
                debug_log(f"Could not convert duration: {seconds}")
                return seconds  # Return original value if conversion fails
        else:
            try:
                seconds = int(seconds)  # Try direct conversion
            except ValueError:
                debug_log(f"Could not convert duration: {seconds}")
                return seconds  # Return original value if conversion fails
    
    seconds = int(seconds)
    days, remainder = divmod(seconds, 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, seconds = divmod(remainder, 60)
    
    return f"{days}d {hours}h {minutes}m {seconds}s"

def get_status():
    """Get overall daemon status"""
    try:
        response = requests.get(f"{API_BASE_URL}/status")
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        debug_log(f"API error in get_status: {str(e)}")
        return {"status": "ERROR", "message": "Cannot connect to API server", "uptime": 0, "arduino_count": 0}

def get_arduino_list():
    """Get list of configured Arduinos"""
    try:
        response = requests.get(f"{API_BASE_URL}/arduinos")
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        debug_log(f"API error in get_arduino_list: {str(e)}")
        return []

def get_arduino_details(alias):
    """Get detailed information about a specific Arduino"""
    try:
        response = requests.get(f"{API_BASE_URL}/arduinos/{alias}")
        debug_log(f"API response for {alias} details: status={response.status_code}")
        response.raise_for_status()
        details = response.json()
        debug_log(f"Got details for {alias}, pins count: {len(details.get('pins', []))}")
        return details
    except requests.exceptions.RequestException as e:
        debug_log(f"API error in get_arduino_details: {str(e)}")
        return None

def draw_header(stdscr, width):
    """Draw the application header"""
    try:
        header = "Arduino Connector V2.0"
        stdscr.attron(curses.color_pair(1))
        stdscr.addstr(0, 0, header + " " * (width - len(header)))
        stdscr.attroff(curses.color_pair(1))
    except curses.error as e:
        debug_log(f"Curses error in draw_header: {str(e)}")

def draw_status_list(stdscr, start_y, width, arduinos):
    """Draw the list of Arduino devices with their status"""
    try:
        # Header
        stdscr.addstr(start_y, 0, "Alias".ljust(20) + "| Component Name".ljust(20) + "| Device".ljust(25) + 
                    "| Arduino Status".ljust(15) + "| LinuxCNC".ljust(10) + "| Features")
        stdscr.addstr(start_y + 1, 0, "-" * min(width-1, 100))  # Limit line length
    except curses.error as e:
        debug_log(f"Curses error in draw_status_list (headers): {str(e)}")
    
    # Arduino list
    row = start_y + 2
    for arduino in arduinos:
        try:
            col = 0
            
            # Alias
            alias = arduino['alias']
            if len(alias) > 19:
                alias = alias[:16] + "..."
            stdscr.addstr(row, col, alias.ljust(20))
            col += 20
            
            # Component name
            comp_name = arduino.get('component_name', '')
            if len(comp_name) > 18:
                comp_name = comp_name[:15] + "..."
            stdscr.addstr(row, col, "| " + comp_name.ljust(18))
            col += 20
            
            # Device
            device = arduino.get('device', '')
            if len(device) > 22:
                device = device[:19] + "..."
            stdscr.addstr(row, col, "| " + device.ljust(23))
            col += 25
            
            # Status - Override to DISABLED if enabled is False
            status = arduino.get('arduino_status', 'UNKNOWN')
            
            # More robust check for disabled status
            enabled = arduino.get('enabled')
            component_name = arduino.get('component_name', '')
            
            # Check multiple conditions that indicate disabled status
            is_disabled = (
                enabled is False or 
                enabled == 'False' or 
                enabled == 'false' or 
                enabled == 0 or 
                enabled == '0' or 
                str(enabled).lower() == 'false' or
                enabled is None or  # Added to catch None values
                str(enabled).lower() == 'none' or  # Added to catch "None" string
                "_DISABLED" in component_name or  # Look for _DISABLED in component name
                component_name.endswith("_DISABLED")
            )
            
            if is_disabled:
                status = "DISABLED"
                
            stdscr.addstr(row, col, "| ")
            col += 2
            try:
                if status == "CONNECTED":
                    stdscr.attron(curses.color_pair(2))  # Green
                    stdscr.addstr(row, col, status.ljust(13))
                    stdscr.attroff(curses.color_pair(2))
                elif status == "DISABLED":
                    stdscr.attron(curses.color_pair(4))  # Yellow
                    stdscr.addstr(row, col, status.ljust(13))
                    stdscr.attroff(curses.color_pair(4))
                else:
                    stdscr.attron(curses.color_pair(3))  # Red
                    stdscr.addstr(row, col, status.ljust(13))
                    stdscr.attroff(curses.color_pair(3))
            except curses.error as e:
                debug_log(f"Curses error in color status: {str(e)}")
                # Fallback without color
                stdscr.addstr(row, col, status.ljust(13))
                
            col += 13
            
            # LinuxCNC Status
            linuxcnc_status = arduino.get('linuxcnc_status', 'N/A')
            stdscr.addstr(row, col, "| ")
            col += 2
            try:
                if linuxcnc_status == "CONNECTED":
                    stdscr.attron(curses.color_pair(2))  # Green
                    stdscr.addstr(row, col, linuxcnc_status.ljust(8))
                    stdscr.attroff(curses.color_pair(2))
                elif linuxcnc_status == "ERROR":
                    stdscr.attron(curses.color_pair(3))  # Red
                    stdscr.addstr(row, col, linuxcnc_status.ljust(8))
                    stdscr.attroff(curses.color_pair(3))
                else:
                    stdscr.addstr(row, col, linuxcnc_status.ljust(8))
            except curses.error as e:
                debug_log(f"Curses error in color linuxcnc status: {str(e)}")
                # Fallback without color
                stdscr.addstr(row, col, linuxcnc_status.ljust(8))
                
            col += 8
            
            # Features
            features = arduino.get('features', [])
            feature_str = ", ".join([f"{f}" for f in features])
            if len(feature_str) > width - col - 2:
                feature_str = feature_str[:width - col - 5] + "..."
            stdscr.addstr(row, col, "| " + feature_str)
            
            row += 1
        except curses.error as e:
            debug_log(f"Curses error in draw_status_list row {row}: {str(e)}")
            row += 1
    
    return row

def draw_arduino_details(stdscr, start_y, alias):
    """Draw detailed information for a specific Arduino"""
    details = get_arduino_details(alias)
    if not details:
        try:
            stdscr.addstr(start_y + 1, 2, f"Error: Could not get details for {alias}")
        except curses.error as e:
            debug_log(f"Curses error in draw_arduino_details (error message): {str(e)}")
        return start_y + 2
    
    # Debug info for pins
    debug_log(f"Drawing details for {alias}")
    debug_log(f"Pins data: {details.get('pins', [])}")
    
    # Title
    try:
        stdscr.addstr(start_y, 0, f"Details for {alias}")
    except curses.error as e:
        debug_log(f"Curses error in draw_arduino_details (title): {str(e)}")
        
    y = start_y + 2
    
    # Basic information
    try:
        stdscr.addstr(y, 0, f"Component Name: {details.get('component_name', 'N/A')}")
        y += 1
        stdscr.addstr(y, 0, f"Device: {details.get('device', 'N/A')}")
        y += 1
        stdscr.addstr(y, 0, f"Serial Port Available: {details.get('serial_port_available', False)}")
        y += 1
        enabled_status = "Yes" if details.get('enabled', False) else "No"
        stdscr.addstr(y, 0, f"Enabled: {enabled_status}")
        y += 1
    except curses.error as e:
        debug_log(f"Curses error in draw_arduino_details (basic info): {str(e)}")
        y += 4  # Skip these lines
    
    # Override status to DISABLED if not enabled
    status = details.get('arduino_status', 'UNKNOWN')
    debug_log(f"Raw enabled value: {details.get('enabled')} (type: {type(details.get('enabled'))})")
    debug_log(f"Raw status value: {status}")
    debug_log(f"Component name: {details.get('component_name')}")
    
    # More robust check for disabled status - any of these conditions should mark it as DISABLED
    enabled = details.get('enabled')
    component_name = details.get('component_name', '')
    
    # Check multiple conditions that indicate disabled status
    is_disabled = (
        enabled is False or 
        enabled == 'False' or 
        enabled == 'false' or 
        enabled == 0 or 
        enabled == '0' or 
        str(enabled).lower() == 'false' or
        enabled is None or  # Added to catch None values
        str(enabled).lower() == 'none' or  # Added to catch "None" string
        "_DISABLED" in component_name or  # Look for _DISABLED in component name
        component_name.endswith("_DISABLED")
    )
    
    if is_disabled:
        status = "DISABLED"
        debug_log(f"Setting status to DISABLED because one of the disabled conditions matched")
    
    # Arduino Status with color
    try:
        stdscr.addstr(y, 0, "Arduino Status: ")
        if status == "CONNECTED":
            stdscr.attron(curses.color_pair(2))  # Green
            stdscr.addstr(y, 16, status)
            stdscr.attroff(curses.color_pair(2))
        elif status == "DISABLED":
            stdscr.attron(curses.color_pair(4))  # Yellow
            stdscr.addstr(y, 16, status)
            stdscr.attroff(curses.color_pair(4))
        else:
            stdscr.attron(curses.color_pair(3))  # Red
            stdscr.addstr(y, 16, status)
            stdscr.attroff(curses.color_pair(3))
    except curses.error as e:
        debug_log(f"Curses error in draw_arduino_details (arduino status): {str(e)}")
        try:
            stdscr.addstr(y, 0, f"Arduino Status: {status}")
        except curses.error:
            pass  # Give up if even this fails
    y += 1
    
    # LinuxCNC Status with color
    linuxcnc_status = details.get('linuxcnc_status', 'N/A')
    try:
        stdscr.addstr(y, 0, "LinuxCNC Status: ")
        if linuxcnc_status == "CONNECTED":
            stdscr.attron(curses.color_pair(2))  # Green
            stdscr.addstr(y, 16, linuxcnc_status)
            stdscr.attroff(curses.color_pair(2))
        elif linuxcnc_status == "ERROR":
            stdscr.attron(curses.color_pair(3))  # Red
            stdscr.addstr(y, 16, linuxcnc_status)
            stdscr.attroff(curses.color_pair(3))
        else:
            stdscr.addstr(y, 16, linuxcnc_status)
    except curses.error as e:
        debug_log(f"Curses error in draw_arduino_details (linuxcnc status): {str(e)}")
        try:
            stdscr.addstr(y, 0, f"LinuxCNC Status: {linuxcnc_status}")
        except curses.error:
            pass  # Give up if even this fails
    y += 1
    
    # Uptime information
    try:
        arduino_uptime = details.get('arduino_reported_uptime', 'N/A')
        if arduino_uptime != 'N/A':
            arduino_uptime = format_duration(arduino_uptime)
        stdscr.addstr(y, 0, f"Arduino Reported Uptime: {arduino_uptime}")
        y += 1
        
        connection_uptime = details.get('connection_uptime', 'N/A')
        if connection_uptime != 'N/A':
            connection_uptime = format_duration(connection_uptime)
        stdscr.addstr(y, 0, f"Connection to Arduino Uptime: {connection_uptime}")
        y += 2
    except curses.error as e:
        debug_log(f"Curses error in draw_arduino_details (uptime): {str(e)}")
        y += 3  # Skip these lines
    
    # Pins table
    pins = details.get('pins', [])
    if pins:
        try:
            stdscr.addstr(y, 0, "Pins:")
            y += 1
            
            # Table header
            stdscr.addstr(y, 0, "Pin Name".ljust(15) + "| Pin Type".ljust(10) + 
                        "| HAL Pin Type".ljust(15) + "| HAL Pin Dir...".ljust(15) + 
                        "| Pin ID".ljust(8) + "| Current Value")
            y += 1
            stdscr.addstr(y, 0, "-" * 80)
            y += 1
        except curses.error as e:
            debug_log(f"Curses error in draw_arduino_details (pins header): {str(e)}")
            y += 3  # Skip these lines
        
        # Pin rows
        for pin in pins:
            try:
                pin_name = pin.get('pin_name', 'N/A')
                if len(pin_name) > 14:
                    pin_name = pin_name[:11] + "..."
                stdscr.addstr(y, 0, pin_name.ljust(15))
                
                pin_type = pin.get('pin_type', 'N/A')
                if len(pin_type) > 9:
                    pin_type = pin_type[:6] + "..."
                stdscr.addstr(y, 15, f"| {pin_type}".ljust(10))
                
                hal_pin_type = pin.get('hal_pin_type', 'N/A')
                if len(hal_pin_type) > 14:
                    hal_pin_type = hal_pin_type[:11] + "..."
                stdscr.addstr(y, 25, f"| {hal_pin_type}".ljust(15))
                
                hal_pin_dir = pin.get('hal_pin_direction', 'N/A')
                if len(hal_pin_dir) > 14:
                    hal_pin_dir = hal_pin_dir[:11] + "..."
                stdscr.addstr(y, 40, f"| {hal_pin_dir}".ljust(15))
                
                pin_id = pin.get('pin_id', 'N/A')
                stdscr.addstr(y, 55, f"| {pin_id}".ljust(8))
                
                # Current value with color for digital pins
                value = pin.get('current_value', 'N/A')
                debug_log(f"Drawing pin {pin_name} with value: {value} (type: {type(value)})")
                
                # Add special formatting for digital values
                if hal_pin_type == "HAL_BIT" and value not in ('N/A', None):
                    try:
                        # Convert value to integer if it's a string
                        if isinstance(value, str) and value.isdigit():
                            value = int(value)
                        
                        if value == 1 or value == "1" or value is True:
                            stdscr.addstr(y, 63, f"| ")
                            stdscr.attron(curses.color_pair(2))  # Green
                            stdscr.addstr(y, 65, f"HIGH")
                            stdscr.attroff(curses.color_pair(2))
                        else:
                            stdscr.addstr(y, 63, f"| ")
                            stdscr.attron(curses.color_pair(3))  # Red
                            stdscr.addstr(y, 65, f"LOW")
                            stdscr.attroff(curses.color_pair(3))
                    except curses.error as e:
                        debug_log(f"Color error for pin value: {str(e)}")
                        stdscr.addstr(y, 63, f"| {value}")
                else:
                    stdscr.addstr(y, 63, f"| {value}")
                
                y += 1
            except curses.error as e:
                debug_log(f"Curses error in draw_arduino_details (pin row): {str(e)}")
                y += 1  # Skip to next row
    
    return y

def draw_help(stdscr, start_y, width):
    """Draw help information at the bottom of the screen"""
    try:
        help_text = "X Exit  1 Select  2 Select  S/Enter Show Details"
        if width > len(help_text) + 2:
            stdscr.attron(curses.color_pair(4))
            stdscr.addstr(start_y, 0, help_text + " " * (width - len(help_text)))
            stdscr.attroff(curses.color_pair(4))
        else:
            # Screen too narrow, just show what fits
            stdscr.attron(curses.color_pair(4))
            stdscr.addstr(start_y, 0, help_text[:width-1])
            stdscr.attroff(curses.color_pair(4))
    except curses.error as e:
        debug_log(f"Curses error in draw_help: {str(e)}")

def main(stdscr):
    # Initialize colors
    try:
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)  # Header
        curses.init_pair(2, curses.COLOR_GREEN, curses.COLOR_BLACK)  # Connected
        curses.init_pair(3, curses.COLOR_RED, curses.COLOR_BLACK)    # Error
        curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLACK) # Help/Warning
    except Exception as e:
        debug_log(f"Color initialization error: {str(e)}")
    
    # Hide cursor
    try:
        curses.curs_set(0)
    except Exception as e:
        debug_log(f"Cursor set error: {str(e)}")
    
    # Enable keypad mode
    try:
        stdscr.keypad(True)
    except Exception as e:
        debug_log(f"Keypad mode error: {str(e)}")
    
    # Get screen dimensions
    try:
        height, width = stdscr.getmaxyx()
        debug_log(f"Terminal dimensions: {width}x{height}")
    except Exception as e:
        debug_log(f"Get dimensions error: {str(e)}")
        height, width = 24, 80  # Default fallback
    
    # State variables
    current_view = "list"  # "list" or "details"
    selected_index = 0
    selected_arduino = None
    refresh_interval = 1.0  # seconds
    last_refresh = 0
    
    # Main loop
    try:
        while True:
            # Check if it's time to refresh data
            current_time = time.time()
            if current_time - last_refresh >= refresh_interval:
                arduinos = get_arduino_list()
                if arduinos and selected_index >= len(arduinos):
                    selected_index = len(arduinos) - 1
                last_refresh = current_time
                
                # Clear the screen
                try:
                    stdscr.clear()
                except curses.error as e:
                    debug_log(f"Screen clear error: {str(e)}")
                
            # Get screen dimensions (might have changed)
            try:
                height, width = stdscr.getmaxyx()
            except Exception as e:
                debug_log(f"Get dimensions error in loop: {str(e)}")
            
            # Draw header
            draw_header(stdscr, width)
            
            # Draw content based on current view
            if current_view == "list":
                # Draw Arduino list
                start_y = 2
                try:
                    list_end_y = draw_status_list(stdscr, start_y, width, arduinos)
                
                    # Highlight selected row if there are any Arduinos
                    if arduinos:
                        try:
                            selected_row = start_y + 2 + selected_index
                            stdscr.attron(curses.A_REVERSE)
                            stdscr.addstr(selected_row, 0, " " * (min(width-1, 100)))
                            
                            # Redraw the row with highlight
                            arduino = arduinos[selected_index]
                            col = 0
                            
                            # Alias
                            alias = arduino['alias']
                            if len(alias) > 19:
                                alias = alias[:16] + "..."
                            stdscr.addstr(selected_row, col, alias.ljust(20))
                            col += 20
                            
                            # Component name
                            comp_name = arduino.get('component_name', '')
                            if len(comp_name) > 18:
                                comp_name = comp_name[:15] + "..."
                            stdscr.addstr(selected_row, col, "| " + comp_name.ljust(18))
                            col += 20
                            
                            # Device
                            device = arduino.get('device', '')
                            if len(device) > 22:
                                device = device[:19] + "..."
                            stdscr.addstr(selected_row, col, "| " + device.ljust(23))
                            col += 25
                            
                            # Status
                            status = arduino.get('arduino_status', 'UNKNOWN')
                            
                            # More robust check for disabled status
                            enabled = arduino.get('enabled')
                            component_name = arduino.get('component_name', '')
                            
                            # Check multiple conditions that indicate disabled status
                            is_disabled = (
                                enabled is False or 
                                enabled == 'False' or 
                                enabled == 'false' or 
                                enabled == 0 or 
                                enabled == '0' or 
                                str(enabled).lower() == 'false' or
                                enabled is None or  # Added to catch None values
                                str(enabled).lower() == 'none' or  # Added to catch "None" string
                                "_DISABLED" in component_name or  # Look for _DISABLED in component name
                                component_name.endswith("_DISABLED")
                            )
                            
                            if is_disabled:
                                status = "DISABLED"
                                
                            stdscr.addstr(selected_row, col, "| ")
                            col += 2
                            # Can't use color within reverse, so just use plain text
                            stdscr.addstr(selected_row, col, status.ljust(13))
                            col += 13
                            
                            # LinuxCNC Status
                            linuxcnc_status = arduino.get('linuxcnc_status', 'N/A')
                            stdscr.addstr(selected_row, col, "| " + linuxcnc_status.ljust(8))
                            col += 10
                            
                            # Features
                            features = arduino.get('features', [])
                            feature_str = ", ".join([f"{f}" for f in features])
                            if len(feature_str) > width - col - 2:
                                feature_str = feature_str[:width - col - 5] + "..."
                            stdscr.addstr(selected_row, col, "| " + feature_str)
                            
                            stdscr.attroff(curses.A_REVERSE)
                        except curses.error as e:
                            debug_log(f"Highlight row error: {str(e)}")
                        
                        # Store selected Arduino
                        selected_arduino = arduino['alias']
                except Exception as e:
                    debug_log(f"General list view error: {str(e)}")
            
            elif current_view == "details" and selected_arduino:
                # Draw details for selected Arduino
                try:
                    draw_arduino_details(stdscr, 2, selected_arduino)
                except Exception as e:
                    debug_log(f"General details view error: {str(e)}")
            
            # Draw help at the bottom
            try:
                draw_help(stdscr, height - 1, width)
            except Exception as e:
                debug_log(f"Help draw error: {str(e)}")
            
            # Refresh the screen
            try:
                stdscr.refresh()
            except curses.error as e:
                debug_log(f"Screen refresh error: {str(e)}")
            
            # Handle keyboard input
            try:
                stdscr.timeout(100)  # Wait 100ms for input, then continue
                key = stdscr.getch()
            except Exception as e:
                debug_log(f"Input handling error: {str(e)}")
                key = -1  # No key
            
            if key == ord('x') or key == ord('X') or key == ord('q') or key == ord('Q'):
                # Exit
                break
            elif key == ord('1') and current_view == "list" and arduinos:
                # Move selection up
                selected_index = (selected_index - 1) % len(arduinos)
            elif key == ord('2') and current_view == "list" and arduinos:
                # Move selection down
                selected_index = (selected_index + 1) % len(arduinos)
            elif (key == ord('s') or key == ord('S') or key == curses.KEY_ENTER or key == 10 or key == 13) and selected_arduino:
                # Toggle between list and details view
                # Note: 10 is ASCII for '\n' (ENTER key) and 13 is ASCII for '\r' (Carriage Return)
                current_view = "details" if current_view == "list" else "list"
                debug_log(f"Switching view to {current_view} for {selected_arduino}")
                if current_view == "details":
                    # Pre-load details to check for any issues
                    details = get_arduino_details(selected_arduino)
                    debug_log(f"Preloaded details has pins: {details.get('pins', []) if details else 'No details'}")
            elif key == curses.KEY_RESIZE:
                # Terminal was resized
                try:
                    stdscr.clear()
                    height, width = stdscr.getmaxyx()
                    debug_log(f"Terminal resized to: {width}x{height}")
                except Exception as e:
                    debug_log(f"Resize handling error: {str(e)}")
    
    except Exception as e:
        debug_log(f"Major exception in main loop: {str(e)}")
        debug_log(traceback.format_exc())

# Use a simple wrapper function to log any errors
def safe_main():
    try:
        curses.wrapper(main)
    except Exception as e:
        # Exit curses mode
        try:
            curses.endwin()
        except:
            pass
        # Log the error
        with open(DEBUG_LOG, "a") as f:
            f.write(f"{datetime.datetime.now()}: FATAL ERROR: {str(e)}\n")
            f.write(traceback.format_exc())
        # Print to console
        print(f"Error: {e}")
        print(f"See {DEBUG_LOG} for details")

if __name__ == "__main__":
    safe_main() 