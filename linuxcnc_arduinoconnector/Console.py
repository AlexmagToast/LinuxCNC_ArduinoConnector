import curses
from datetime import timedelta, datetime
import logging
import traceback
import time
from linuxcnc_arduinoconnector.Utils import format_elapsed_time

class UIElement:
    def __init__(self, stdscr):
        self.stdscr = stdscr

    def draw(self):
        raise NotImplementedError("Subclasses should implement this!")

class TextElement(UIElement):
    def __init__(self, stdscr, text, row, col, color_pair=0):
        super().__init__(stdscr)
        self.text = text
        self.row = row
        self.col = col
        self.color_pair = color_pair

    def draw(self):
        height, width = self.stdscr.getmaxyx()
        self.stdscr.addstr(self.row, 0, ' ' * width, curses.color_pair(self.color_pair))
        self.stdscr.addstr(self.row, self.col, self.text, curses.color_pair(self.color_pair))
        
        
class TableElement(UIElement):
    def __init__(self, stdscr, headers, data, start_row, start_col, max_widths, selected_index=None, scroll_offset=0):
        super().__init__(stdscr)
        self.headers = headers
        self.data = data
        self.start_row = start_row
        self.start_col = start_col
        self.max_widths = max_widths
        self.selected_index = selected_index
        self.scroll_offset = scroll_offset

    def draw(self):
        row = self.start_row
        col = self.start_col
        self.stdscr.addstr(row, col, self.format_row(self.headers))
        row += 1
        self.stdscr.addstr(row, col, "-" * sum(self.max_widths.values()))
        row += 1

        for index, row_data in enumerate(self.data):
            status = row_data[3]  # Assuming the status is in the 4th column
            if status == "CONNECTED":
                status_color_pair = curses.color_pair(2)  # Green background
            elif status in ["DISCONNECTED", "CONNECTING"]:
                status_color_pair = curses.color_pair(3)  # Yellow background
            elif status == "ERROR":
                status_color_pair = curses.color_pair(1)  # Red background
            else:
                status_color_pair = curses.color_pair(0)  # Default color

            formatted_row = []
            for key, item in zip(self.max_widths.keys(), row_data):
                if key == 'device':
                    # Apply scrolling to the device value
                    item_str = str(item)
                    if len(item_str) > self.max_widths[key]:
                        start = self.scroll_offset % len(item_str)
                        scrolled = item_str[start:] + item_str[:start]
                        formatted_item = scrolled[:self.max_widths[key]-2] + '..'
                    else:
                        formatted_item = item_str.ljust(self.max_widths[key])
                else:
                    formatted_item = str(item)[:self.max_widths[key]].ljust(self.max_widths[key])
                formatted_row.append(formatted_item)

            formatted_row_str = " | ".join(formatted_row)
            
            if index == self.selected_index:
                self.stdscr.addstr(row, col, formatted_row_str, curses.color_pair(4))
            else:
                # Draw the row without color
                self.stdscr.addstr(row, col, formatted_row_str)
                # Overwrite the status part with the colored status
                status_col_start = sum(self.max_widths[key] + 3 for key in list(self.max_widths.keys())[:3])  # Calculate the start position of the status column
                self.stdscr.addstr(row, col + status_col_start, f"{status:<{self.max_widths['status']}}", status_color_pair)
            row += 1

    def format_row(self, row_data):
        return " | ".join(str(item).ljust(self.max_widths[key]) for key, item in zip(self.max_widths.keys(), row_data))
class StatusBar(UIElement):
    def __init__(self, stdscr, text):
        super().__init__(stdscr)
        self.text = text

    def draw(self):
        height, width = self.stdscr.getmaxyx()
        parts = self.text.split(' ')
        col = 0
        for part in parts:
            if part in ['^C', '↑', '↓', '⏎']:
                self.stdscr.addstr(height - 1, col, part, curses.color_pair(6))
            else:
                self.stdscr.addstr(height - 1, col, part)
            col += len(part) + 1

class ConsoleUI:
    def __init__(self, stdscr):
        self.stdscr = stdscr
        self.elements = []

    def add_element(self, element):
        self.elements.append(element)

    def draw(self):
        self.stdscr.clear()
        for element in self.elements:
            element.draw()
        self.stdscr.refresh()

    
def display_arduino_statuses(stdscr, arduino_connections, scroll_offset, selected_index):
    try:
        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW)
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(5, curses.COLOR_WHITE, curses.COLOR_BLUE)
        curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.curs_set(0)

        height, width = stdscr.getmaxyx()
        max_widths = {
            "alias": 17,
            "component_name": 22,
            "device": 15,
            "status": 13,
            "linuxcnc_status": 10,
            "features": width - (17 + 22 + 15 + 13 + 10 + 9)
        }

        if height < 5:
            stdscr.addstr(0, 0, "Error: Terminal height is too short to display the statuses.")
            stdscr.refresh()
            return

        ui = ConsoleUI(stdscr)
        ui.add_element(TextElement(stdscr, "Arduino Connector V2.0", 0, 0, 5))

        headers = ["Alias", "Component Name", "Device", "Arduino Status", "LinuxCNC", "Features"]
        data = []

        enabled_connections = [conn for conn in arduino_connections if conn.settings.enabled]
        disabled_connections = [conn for conn in arduino_connections if not conn.settings.enabled]
        all_connections = enabled_connections + disabled_connections

        available_rows = height - 5
        if len(all_connections) > available_rows:
            all_connections = all_connections[:available_rows - 1]
            truncated = True
        else:
            truncated = False

        for connection in all_connections:
            alias = connection.settings.alias[:max_widths["alias"]]
            component_name = connection.settings.component_name[:max_widths["component_name"]]
            device = connection.settings.dev#[:max_widths["device"]]
            features = ", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.settings.io_map.items()])[:max_widths["features"]]

            if connection.settings.enabled:
                status = connection.serialConn.connectionState[:max_widths["status"]]
            else:
                status = "DISABLED"

            if not connection.settings.enabled:
                linuxcnc_status = "N/A"
            else:
                if connection.settings.hal_emulation:
                    linuxcnc_status = "N/A"
                else:
                    linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"

            data.append([alias, component_name, device, status, linuxcnc_status, features])

        ui.add_element(TableElement(stdscr, headers, data, 2, 0, max_widths, selected_index, scroll_offset))

        if truncated:
            ui.add_element(TextElement(stdscr, "List truncated. Not all connections are shown.", height - 2, 0))

        ui.add_element(StatusBar(stdscr, "^C Exit  ↑ Select  ↓ Select  ⏎ Show Details"))

        ui.draw()
    except Exception as e:
        just_the_string = traceback.format_exc()
        stdscr.clear()
        stdscr.addstr(0, 0, f"Unable to show console UI due to an exception. Details: {str(just_the_string)}")
        stdscr.refresh()


def display_connection_details(stdscr, connection):
    try:
        stdscr.clear()
        height, width = stdscr.getmaxyx()

        curses.start_color()
        curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_RED)
        curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_GREEN)
        curses.init_pair(3, curses.COLOR_BLACK, curses.COLOR_YELLOW)
        curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_WHITE)

        alias_display = connection.settings.alias
        if not connection.settings.enabled:
            alias_display += " [DISABLED]"

        component_name = connection.settings.component_name
        device = connection.settings.dev
        is_serial_port_available = connection.serialDeviceAvailable if connection.settings.enabled else "N/A"
        status = connection.serialConn.connectionState if connection.settings.enabled else "DISABLED"

        if connection.settings.enabled and status == "CONNECTED":
            arduino_reported_uptime = connection.serialConn.arduinoReportedUptime
            if arduino_reported_uptime:
                elapsed_uptime = timedelta(minutes=arduino_reported_uptime)
                uptime_str = format_elapsed_time(elapsed_uptime)
            else:
                uptime_str = "N/A"
        else:
            uptime_str = "N/A"

        if connection.settings.enabled and status == "CONNECTED":
            conn_last_formed = connection.serialConn.connLastFormed
            if conn_last_formed:
                elapsed_time = datetime.now() - conn_last_formed
                elapsed_str = format_elapsed_time(elapsed_time)
            else:
                elapsed_str = "N/A"
        else:
            elapsed_str = "N/A"

        if not connection.settings.enabled:
            linuxcnc_status = "N/A"
        else:
            linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"

        linuxcnc_color_pair = curses.color_pair(2) if linuxcnc_status == "OK" else curses.color_pair(1)

        num_lines = 12 + len(connection.settings.io_map) * 2
        if height < num_lines:
            stdscr.addstr(0, 0, "Error: Terminal height is too short to display the connection details.")
            stdscr.refresh()
            return

        row = 0
        stdscr.addstr(row, 0, f"Details for {alias_display}")
        row += 2

        stdscr.addstr(row, 0, f"Component Name: {component_name}")
        row += 1
        stdscr.addstr(row, 0, f"Device: {device}")
        row += 1
        stdscr.addstr(row, 0, f"Serial Port Available: {is_serial_port_available}")
        row += 1

        color_pair = curses.color_pair(2) if status == "CONNECTED" else curses.color_pair(1) if status == "DISCONNECTED" else curses.color_pair(0)
        stdscr.addstr(row, 0, "Arduino Status: ")
        stdscr.addstr(f"{status}", color_pair)
        row += 1
        stdscr.addstr(row, 0, "LinuxCNC Status: ")
        stdscr.addstr(f"{linuxcnc_status}", linuxcnc_color_pair)
        row += 1
        stdscr.addstr(row, 0, f"Arduino Reported Uptime: {uptime_str}")
        row += 1
        stdscr.addstr(row, 0, f"Connection to Arduino Uptime: {elapsed_str}")
        row += 2

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
                current_value = "DISABLED" if not pin.pinEnabled else str(getattr(pin, 'halPinCurrentValue', 'N/A'))
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
    except Exception as e:
        just_the_string = traceback.format_exc()
        stdscr.clear()
        stdscr.addstr(0, 0, f"Unable to show console UI due to an exception. Details: {str(just_the_string)}")
        stdscr.refresh()