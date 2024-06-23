import curses
from datetime import timedelta, datetime
import logging
import traceback

from linuxcnc_arduinoconnector.Utils import format_elapsed_time

# File: /home/ken/git-repos/LinuxCNC_ArduinoConnector/linuxcnc_arduinoconnector/Console.py

def display_arduino_statuses(stdscr, arduino_connections, scroll_offset, selected_index):
    try:
        # Initialize colors
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)    # Red background
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)  # Green background
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW) # Yellow background
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)  # Highlight background

        curses.curs_set(0)

        stdscr.clear()

        height, width = stdscr.getmaxyx()
        max_widths = {
            "alias": 17,
            "component_name": 22,
            "device": 15,
            "status": 13,
            "linuxcnc_status": 10,  # Width for LinuxCNC status
            "features": width - (17 + 22 + 15 + 13 + 10 + 9)  # Remaining width for features, minus 9 for separators
        }

        # Check if the console is too small height-wise to show the column names and at least one row
        if height < 5:  # 1 row for the title, 1 row for the column names, 1 row for the separator, and at least 1 data row
            stdscr.addstr(0, 0, "Error: Terminal height is too short to display the statuses.")
            stdscr.refresh()
            return

        def truncate(text, max_length):
            return text if len(text) <= max_length else text[:max_length-3] + '...'

        row = 0
        stdscr.addstr(row, 0, "Arduino Connection Statuses:")
        row += 1
        stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | {:<{status}} | {:<{linuxcnc_status}} | {}".format(
            "Alias", "Component Name", "Device", "Arduino Status", "LinuxCNC", "Features",
            **max_widths))
        row += 1
        stdscr.addstr(row, 0, "-" * width)
        row += 1

        # Separate enabled and disabled connections
        enabled_connections = [conn for conn in arduino_connections if conn.settings.enabled]
        disabled_connections = [conn for conn in arduino_connections if not conn.settings.enabled]

        all_connections = enabled_connections + disabled_connections

        # Calculate the number of rows available for displaying connections
        available_rows = height - row - 1  # Subtracting the current row and one for the truncated message

        # Check if we need to truncate the list
        if len(all_connections) > available_rows:
            all_connections = all_connections[:available_rows - 1]  # Leave space for the truncated message
            truncated = True
        else:
            truncated = False

        for index, connection in enumerate(all_connections):
            alias = truncate(connection.settings.alias, max_widths["alias"])
            component_name = truncate(connection.settings.component_name, max_widths["component_name"])
            device = connection.settings.dev
            features = truncate(", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.settings.io_map.items()]), max_widths["features"])

            if connection.settings.enabled:
                status = truncate(connection.serialConn.connectionState, max_widths["status"])
            else:
                status = "DISABLED"

            # Determine the LinuxCNC status
            if not connection.settings.enabled:
                linuxcnc_status = "N/A"
                linuxcnc_color_pair = curses.color_pair(0)  # Default background
            else:
                if connection.settings.hal_emulation:
                    linuxcnc_status = "N/A"
                    linuxcnc_color_pair = curses.color_pair(0)  # Default background
                else:
                    linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"
                    linuxcnc_color_pair = curses.color_pair(2) if linuxcnc_status == "OK" else curses.color_pair(1)

            # Determine the color pair for the status
            if status == "DISCONNECTED":
                color_pair = curses.color_pair(1)  # Red background
            elif status == "CONNECTED":
                color_pair = curses.color_pair(2)  # Green background
            elif status == "DISABLED":
                color_pair = curses.color_pair(0)  # Default background
            else:
                color_pair = curses.color_pair(3)  # Yellow background

            # Handle scrolling for device
            if len(device) > max_widths["device"]:
                if scroll_offset < len(device):
                    device_display = device[scroll_offset:scroll_offset + max_widths["device"]]
                    if scroll_offset + max_widths["device"] < len(device):
                        device_display = device_display[:-2] + '..'
                    else:
                        device_display = device_display + ' ' * (max_widths["device"] - len(device_display))
                else:
                    scroll_position = scroll_offset - len(device)
                    device_display = device[scroll_position:scroll_position + max_widths["device"]]
                    if scroll_position + max_widths["device"] < len(device):
                        device_display = device_display[:-2] + '..'
                    else:
                        device_display = device_display + ' ' * (max_widths["device"] - len(device_display))
            else:
                device_display = device.ljust(max_widths["device"])

            # Highlight the selected row
            if index == selected_index:
                stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | ".format(
                    alias, component_name, device_display,
                    **max_widths), curses.color_pair(4))
                stdscr.addstr("{:<{status}}".format(status, **max_widths), curses.color_pair(4) | color_pair)
                stdscr.addstr(" | {:<{linuxcnc_status}} | ".format(linuxcnc_status, **max_widths), curses.color_pair(4) | linuxcnc_color_pair)
                stdscr.addstr("{}".format(features, **max_widths), curses.color_pair(4))
            else:
                stdscr.addstr(row, 0, "{:<{alias}} | {:<{component_name}} | {:<{device}} | ".format(
                    alias, component_name, device_display,
                    **max_widths))
                stdscr.addstr("{:<{status}}".format(status, **max_widths), color_pair)
                stdscr.addstr(" | {:<{linuxcnc_status}} | ".format(linuxcnc_status, **max_widths), linuxcnc_color_pair)
                stdscr.addstr("{}".format(features, **max_widths))
            row += 1

        if truncated:
            stdscr.addstr(row, 0, "List truncated. Not all connections are shown.")

        stdscr.refresh()
    except Exception as e:
        just_the_string = traceback.format_exc()
        stdscr.clear()
        stdscr.addstr(0, 0, f"Unable to show console UI due to an exception. Details: {str(just_the_string)}")
        stdscr.refresh()

def display_connection_details(stdscr, connection):
    stdscr.clear()
    height, width = stdscr.getmaxyx()

    # Initialize colors
    curses.start_color()
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)    # Red background
    curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)  # Green background
    curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW) # Yellow background
    curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)  # Highlight background

    alias_display = connection.settings.alias
    if not connection.settings.enabled:
        alias_display += " [DISABLED]"

    component_name = connection.settings.component_name
    device = connection.settings.dev
    is_serial_port_available = connection.serialDeviceAvailable if connection.settings.enabled else "N/A"
    status = connection.serialConn.connectionState if connection.settings.enabled else "DISABLED"
    
    # Determine arduinoReportedUptime value
    if connection.settings.enabled and status == "CONNECTED":
        arduino_reported_uptime = connection.serialConn.arduinoReportedUptime
        if arduino_reported_uptime:
            elapsed_uptime = timedelta(minutes=arduino_reported_uptime)
            uptime_str = format_elapsed_time(elapsed_uptime)
        else:
            uptime_str = "N/A"
    else:
        uptime_str = "N/A"

    # Determine connLastFormed value
    if connection.settings.enabled and status == "CONNECTED":
        conn_last_formed = connection.serialConn.connLastFormed
        if conn_last_formed:
            elapsed_time = datetime.now() - conn_last_formed
            elapsed_str = format_elapsed_time(elapsed_time)
        else:
            elapsed_str = "N/A"
    else:
        elapsed_str = "N/A"

    # Determine the LinuxCNC status
    if not connection.settings.enabled:
        linuxcnc_status = "N/A"
    else:
        linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"

    # Determine the color pair for the LinuxCNC status
    if linuxcnc_status == "ERROR":
        linuxcnc_color_pair = curses.color_pair(1)  # Red background
    elif linuxcnc_status == "OK":
        linuxcnc_color_pair = curses.color_pair(2)  # Green background
    else:
        linuxcnc_color_pair = curses.color_pair(0)  # Default background

    row = 0
    stdscr.addstr(row, 0, f"Details for {alias_display}")
    row += 2  # Add a blank line

    stdscr.addstr(row, 0, f"Component Name: {component_name}")
    row += 1
    stdscr.addstr(row, 0, f"Device: {device}")
    row += 1
    stdscr.addstr(row, 0, f"Serial Port Available: {is_serial_port_available}")
    row += 1

    # Determine the color pair for the status
    if status == "DISCONNECTED":
        color_pair = curses.color_pair(1)  # Red background
    elif status == "CONNECTED":
        color_pair = curses.color_pair(2)  # Green background
    elif status == "DISABLED":
        color_pair = curses.color_pair(0)  # Default background
    else:
        color_pair = curses.color_pair(3)  # Yellow background

    stdscr.addstr(row, 0, "Arduino Status: ")
    stdscr.addstr(f"{status}", color_pair)
    row += 1
    stdscr.addstr(row, 0, "LinuxCNC Status: ")
    stdscr.addstr(f"{linuxcnc_status}", linuxcnc_color_pair)
    row += 1
    stdscr.addstr(row, 0, f"Arduino Reported Uptime: {uptime_str}")
    row += 1
    stdscr.addstr(row, 0, f"Connection to Arduino Uptime: {elapsed_str}")
    row += 2  # Add a blank line

    stdscr.addstr(row, 0, "Pins:")
    row += 1

    pin_headers = ["Pin Name", "Pin Type", "HAL Pin Type", "HAL Pin Direction", "Pin ID", "Current Value"]
    pin_column_widths = [15, 10, 15, 15, 10, 15]

    total_width = sum(pin_column_widths) + len(pin_headers) - 1
    available_width = width - 1
    truncate_length = lambda text, max_length: text if len(text) <= max_length else text[:max_length-3] + '...'

    if total_width > available_width:
        scaling_factor = available_width / total_width
        pin_column_widths = [int(w * scaling_factor) for w in pin_column_widths]

    stdscr.addstr(row, 0, "{:<{width0}} | {:<{width1}} | {:<{width2}} | {:<{width3}} | {:<{width4}} | {:<{width5}}".format(
        *[truncate_length(header, pin_column_widths[i]) for i, header in enumerate(pin_headers)],
        width0=pin_column_widths[0],
        width1=pin_column_widths[1],
        width2=pin_column_widths[2],
        width3=pin_column_widths[3],
        width4=pin_column_widths[4],
        width5=pin_column_widths[5]
    ))
    row += 1
    stdscr.addstr(row, 0, "-" * width)
    row += 1

    for feature, pins in connection.settings.io_map.items():
        for pin in pins:
            if not pin.pinEnabled:
                current_value = "DISABLED"
            else:
                current_value = str(getattr(pin, 'halPinCurrentValue', 'N/A'))  # Example way to get current value, adjust as needed
            stdscr.addstr(row, 0, "{:<{width0}} | {:<{width1}} | {:<{width2}} | {:<{width3}} | {:<{width4}} | {:<{width5}}".format(
                truncate_length(pin.pinName, pin_column_widths[0]),
                truncate_length(str(pin.pinType), pin_column_widths[1]),
                truncate_length(str(pin.halPinType), pin_column_widths[2]),
                truncate_length(str(pin.halPinDirection), pin_column_widths[3]),
                truncate_length(str(pin.pinID), pin_column_widths[4]),
                truncate_length(current_value, pin_column_widths[5]),
                width0=pin_column_widths[0],
                width1=pin_column_widths[1],
                width2=pin_column_widths[2],
                width3=pin_column_widths[3],
                width4=pin_column_widths[4],
                width5=pin_column_widths[5]
            ))
            row += 1

    stdscr.refresh()