#!/usr/bin/env python3
"""
Arduino Connector Textual-based UI
A modern terminal UI for the Arduino Connector API
"""
import sys
import requests
from datetime import datetime
from typing import Dict, List, Any, Optional

from textual import on
from textual.app import App, ComposeResult
from textual.containers import Container, Horizontal
from textual.reactive import reactive
from textual.widgets import (
    Button, DataTable, Footer, Header, Label, Static
)
from textual import events
from textual.widget import Widget

# API configuration
API_BASE_URL = "http://localhost:8765"

# Handle key events at the list view level
class ListViewBase(Widget):
    """Base class for list views"""
    
    def __init__(self, app_instance):
        super().__init__()
        self._app = app_instance
    
    def on_key(self, event: events.Key) -> None:
        """Handle key events"""
        if event.key == "q":
            self._app.exit()
        elif event.key == "r":
            if hasattr(self, 'on_refresh'):
                self.on_refresh()
            else:
                self._app.refresh_data()
        elif event.key == "enter":
            # When Enter key is pressed in list view, show details of selected Arduino
            if self._app.current_view == "list" and hasattr(self, 'on_show_details'):
                self.on_show_details()
            # When Enter key is pressed in detail view, go back to list
            elif self._app.current_view == "detail" and hasattr(self, 'on_back'):
                self.on_back()

class StatusBadge(Static):
    """A colored badge for showing status"""
    
    def __init__(self, status: str, **kwargs):
        super().__init__("", **kwargs)
        self.status = status
        
    def on_mount(self) -> None:
        self.update_status(self.status)
        
    def update_status(self, status: str) -> None:
        """Update the badge status"""
        self.status = status
        if status == "CONNECTED":
            self.add_class("connected")
            self.remove_class("disconnected")
            self.remove_class("disabled")
        elif status == "DISABLED":
            self.add_class("disabled")
            self.remove_class("connected")
            self.remove_class("disconnected")
        else:
            self.add_class("disconnected")
            self.remove_class("connected")
            self.remove_class("disabled")
        self.update(status)

class ArduinoListView(ListViewBase):
    """Shows a list of all connected Arduinos"""
    
    def __init__(self, app_instance, **kwargs):
        super().__init__(app_instance, **kwargs)
        self._app = app_instance
        
        # Create the Arduino table without columns (will add in on_mount)
        self.arduino_table = DataTable(id="arduino_table", zebra_stripes=True)
    
    def compose(self) -> ComposeResult:
        """Compose the user interface"""
        yield Header(show_clock=True)
        yield Label("Arduino LinuxCNC Connector V2.0", id="title", classes="heading")
        
        # Create styled table with border
        yield self.arduino_table
        
        # Add styled buttons
        with Horizontal(id="buttons_container", classes="button-container"):
            yield Button("Refresh", id="refresh", variant="primary")
            yield Button("Details", id="show_details", variant="success", disabled=True)
            yield Button("About", id="about", variant="primary")
            yield Button("Quit", id="quit", variant="error")
        
        yield Footer()
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        # Add columns to the table now that we have an app context
        self.arduino_table.add_column("Alias", width=20)
        self.arduino_table.add_column("Component Name", width=20)
        self.arduino_table.add_column("Device", width=25)
        self.arduino_table.add_column("Arduino Status", width=15)
        self.arduino_table.add_column("LinuxCNC", width=10)
        self.arduino_table.add_column("Features", width=20)
        
        # Set cursor_type to row for whole row selection
        self.arduino_table.cursor_type = "row"
        
        # Set up button event handlers
        refresh_button = self.query_one("#refresh")
        refresh_button.on_click = self.on_refresh
        
        details_button = self.query_one("#show_details")
        details_button.on_click = self.on_show_details
        
        about_button = self.query_one("#about")
        about_button.on_click = self.on_about
        
        quit_button = self.query_one("#quit")
        quit_button.on_click = self._app.exit
        
        # Set up table event handler
        self.arduino_table.on_row_highlighted = self.on_row_selected
        
        # Try to focus the table if possible
        try:
            # For newer Textual versions
            self.arduino_table.focus()
        except AttributeError:
            # Fallback for older versions
            pass
    
    def on_refresh(self) -> None:
        """Handle refresh button click"""
        self._app.refresh_data()
    
    def on_show_details(self) -> None:
        """Handle show details button click"""
        if self.arduino_table.cursor_row is not None:
            alias = self.arduino_table.get_cell_at((self.arduino_table.cursor_row, 0))
            self._app.switch_view("detail", alias)
    
    def on_about(self) -> None:
        """Handle about button click"""
        self._app.switch_view("about")
    
    def on_row_selected(self, cursor_row) -> None:
        """Enable/disable show details button based on row selection"""
        self.query_one("#show_details").disabled = (cursor_row is None)
    
    def update_data(self, arduinos: List[Dict[str, Any]]) -> None:
        """Update the Arduino data table"""
        # Clear the table
        self.arduino_table.clear()
        
        # Add the data rows with styled status cells
        for arduino in arduinos:
            # Alias
            alias = arduino.get("alias", "Unknown")
            
            # Component Name
            component_name = arduino.get("component_name", "")
            
            # Device
            device = arduino.get("device", "Unknown")
            
            # Arduino Status - determine status based on api_client_ui.py logic
            status = None
            status_fields = ["arduino_status", "state", "status", "connection_state", "connection"]
            for field in status_fields:
                if field in arduino:
                    status = arduino[field]
                    break
            
            if status is None:
                status = "UNKNOWN"
                
            # Check enabled state
            enabled = arduino.get("enabled")
            
            # Debug print for enabled status
            print(f"DEBUG - List view: Arduino {alias} enabled={enabled} (type: {type(enabled).__name__})")
            
            # Check multiple conditions that indicate disabled status (from api_client_ui.py)
            is_disabled = (
                enabled is False or 
                enabled == 'False' or 
                enabled == 'false' or 
                enabled == 0 or 
                enabled == '0' or 
                str(enabled).lower() == 'false' or
                "_DISABLED" in component_name
            )
            
            if is_disabled:
                status = "DISABLED"
                print(f"DEBUG - List view: Setting {alias} to DISABLED because enabled={enabled}")
            
            # Format status with color
            if status == "CONNECTED":
                status_str = f"[green]{status}[/]"
            elif status == "DISABLED":
                status_str = f"[yellow]{status}[/]"
            else:
                status_str = f"[red]{status}[/]"
            
            # LinuxCNC Status
            linuxcnc_status = arduino.get("linuxcnc_status", "N/A")
            if linuxcnc_status == "CONNECTED":
                linuxcnc_str = f"[green]{linuxcnc_status}[/]"
            elif linuxcnc_status == "ERROR":
                linuxcnc_str = f"[red]{linuxcnc_status}[/]"
            else:
                linuxcnc_str = f"{linuxcnc_status}"
            
            # Features
            features = arduino.get("features", [])
            if isinstance(features, list):
                features_str = ", ".join(features) if features else "None"
            else:
                features_str = str(features)
            
            # Add row to table
            self.arduino_table.add_row(
                alias, component_name, device, status_str, linuxcnc_str, features_str
            )
            
        # Enable details button only if rows exist
        self.query_one("#show_details").disabled = self.arduino_table.row_count == 0

class ArduinoDetailView(ListViewBase):
    """Shows detailed information for a single Arduino"""
    
    def __init__(self, app_instance: "APIClientApp", **kwargs):
        super().__init__(app_instance)
        self._app = app_instance
        self.current_alias = None
        self._update_timer = None
        
    @property
    def app(self) -> "APIClientApp":
        """Access to the app instance."""
        return self._app
    
    def compose(self) -> ComposeResult:
        """Compose the user interface"""
        yield Header(show_clock=True)
        yield Label("Arduino LinuxCNC Connector V2.0", id="title", classes="heading")
        
        # Container for Arduino details
        yield Container(id="details_container", classes="details-box")
        
        # Create styled table with border
        yield DataTable(id="pin_table", zebra_stripes=True, classes="table-border")
        
        # Add styled buttons (removed refresh button)
        with Horizontal(id="buttons_container", classes="button-container"):
            yield Button("Back", id="back", variant="default")
            yield Button("About", id="about", variant="primary")
            yield Button("Quit", id="quit", variant="error")
        
        yield Footer()
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        # Set up button event handlers
        back_button = self.query_one("#back")
        back_button.on_click = self.on_back
        
        about_button = self.query_one("#about")
        about_button.on_click = self.on_about
        
        quit_button = self.query_one("#quit")
        quit_button.on_click = self._app.exit
        
        # Set up pin table columns
        pin_table = self.query_one("#pin_table")
        pin_table.add_column("Pin", width=15)
        pin_table.add_column("Pin Type", width=10) 
        pin_table.add_column("HAL Pin Type", width=15)
        pin_table.add_column("HAL Pin Dir", width=15)
        pin_table.add_column("Pin ID", width=8)
        pin_table.add_column("Value", width=10)
    
    def on_show(self) -> None:
        """Called when the view becomes visible"""
        # Start automatic updates
        self.start_auto_updates()
    
    def on_hide(self) -> None:
        """Called when the view is hidden"""
        # Stop automatic updates
        self.stop_auto_updates()
    
    def start_auto_updates(self) -> None:
        """Start automatic pin updates"""
        # Setup a timer to refresh pin data every 1 second
        if self._update_timer is None:
            self._update_timer = self.app.set_interval(1.0, self.update_all_data)
    
    def stop_auto_updates(self) -> None:
        """Stop automatic pin updates"""
        if self._update_timer is not None:
            self.app.set_timer(self._update_timer, None)  # Cancel the timer by setting callback to None
            self._update_timer = None
    
    def update_all_data(self) -> None:
        """Update all Arduino data automatically"""
        if not self.current_alias:
            return
            
        try:
            # Get fresh Arduino details
            arduino_details = self._app.get_arduino_details(self.current_alias)
            if not arduino_details:
                return
                
            # Update details container
            self.update_details_container(arduino_details)
            
            # Update pin table
            self.update_pin_table(arduino_details.get("pins", []))
        except Exception as e:
            print(f"Error updating Arduino data: {e}")
    
    def update_details_container(self, arduino_details: Dict[str, Any]) -> None:
        """Update the details container with fresh data"""
        details_container = self.query_one("#details_container")
        details_container.remove_children()
        
        # Get status and format appropriately
        status = arduino_details.get("arduino_status", "Unknown")
        if status == "Unknown":  # Try alternate field names
            status = arduino_details.get("state", "Unknown")
        
        # Check enabled state and component_name (matching api_client_ui.py logic)
        enabled = arduino_details.get("enabled")
        component_name = arduino_details.get('component_name', '')
        
        # Print raw enabled value for debugging
        print(f"DEBUG - Raw enabled value: {enabled} (type: {type(enabled).__name__})")
        
        # Check multiple conditions that indicate disabled status (from api_client_ui.py)
        is_disabled = (
            enabled is False or 
            enabled == 'False' or 
            enabled == 'false' or 
            enabled == 0 or 
            enabled == '0' or 
            str(enabled).lower() == 'false' or
            "_DISABLED" in component_name
        )
        
        # Only override status if we're sure it's disabled
        if is_disabled:
            status = "DISABLED"
            print(f"DEBUG - Setting status to DISABLED because: enabled={enabled}")
        
        # Color status appropriately
        if status == "CONNECTED":
            status_str = f"[green]{status}[/]"
            status_style = "green"
        elif status == "DISABLED":
            status_str = f"[yellow]{status}[/]"
            status_style = "yellow"
        else:
            status_str = f"[red]{status}[/]"
            status_style = "red"
        
        # Format enabled status
        enabled_str = "No" if is_disabled else "Yes"
        enabled_color = "red" if is_disabled else "green"
        
        # Format features
        features = arduino_details.get("features", [])
        features_str = ", ".join(features) if features and isinstance(features, list) else "None"
        
        # Format uptimes (matching api_client_ui.py)
        arduino_uptime = arduino_details.get('arduino_reported_uptime', 'N/A')
        if arduino_uptime != 'N/A' and arduino_uptime is not None:
            try:
                if isinstance(arduino_uptime, str) and arduino_uptime.endswith('s'):
                    arduino_uptime = int(arduino_uptime[:-1])
                arduino_uptime = self._format_duration(arduino_uptime)
            except (ValueError, TypeError):
                pass
                
        connection_uptime = arduino_details.get('connection_uptime', 'N/A')
        if connection_uptime != 'N/A' and connection_uptime is not None:
            try:
                if isinstance(connection_uptime, str) and connection_uptime.endswith('s'):
                    connection_uptime = int(connection_uptime[:-1])
                connection_uptime = self._format_duration(connection_uptime)
            except (ValueError, TypeError):
                pass
        
        # LinuxCNC Status
        linuxcnc_status = arduino_details.get('linuxcnc_status', 'N/A')
        if linuxcnc_status == "CONNECTED":
            linuxcnc_str = f"[green]{linuxcnc_status}[/]"
        elif linuxcnc_status == "ERROR":
            linuxcnc_str = f"[red]{linuxcnc_status}[/]"
        else:
            linuxcnc_str = f"{linuxcnc_status}"
        
        # Add detailed information (matching api_client_ui.py layout)
        details_container.mount(
            Label(f"[bold]Component Name:[/] {component_name}"),
            Label(f"[bold]Device:[/] {arduino_details.get('device', 'N/A')}"),
            Label(f"[bold]Serial Port Available:[/] {'Yes' if arduino_details.get('serial_port_available', False) else 'No'}"),
            Label(f"[bold]Enabled:[/] [{enabled_color}]{enabled_str}[/]"),
            Label(f"[bold]Arduino Status:[/] [{status_style}]{status}[/]"),
            Label(f"[bold]LinuxCNC Status:[/] {linuxcnc_str}"),
            Label(f"[bold]Arduino Reported Uptime:[/] {arduino_uptime}"),
            Label(f"[bold]Connection to Arduino Uptime:[/] {connection_uptime}"),
        )
    
    def on_back(self) -> None:
        """Handle the back button click"""
        self.stop_auto_updates()
        self._app.switch_view("list")
    
    def on_about(self) -> None:
        """Handle about button click"""
        # Stop auto-updates first
        self.stop_auto_updates()
        self._app.switch_view("about")
    
    def load_details(self, alias: str) -> None:
        """Load the details for the given Arduino"""
        self.current_alias = alias
        
        # Get the Arduino details
        arduino_details = self._app.get_arduino_details(alias)
        if not arduino_details:
            return
        
        # Initial update of all data
        self.update_details_container(arduino_details)
        self.update_pin_table(arduino_details.get("pins", []))
        
        # Start automatic updates - timer will keep everything refreshed
        self.start_auto_updates()
    
    def update_pin_table(self, pins) -> None:
        """Update just the pin table with new data"""
        pin_table = self.query_one("#pin_table")
        if not pin_table:
            return
            
        # Store the current data to check for changes
        old_values = {}
        for i in range(pin_table.row_count):
            pin_name = pin_table.get_cell_at((i, 0))
            current_value = pin_table.get_cell_at((i, 5))
            old_values[pin_name] = current_value
        
        # Clear the table
        pin_table.clear()
        
        # Handle pins data which can be either a list or dictionary
        if isinstance(pins, dict):
            # If pins is a dictionary, process as key-value pairs
            for pin_name, pin_data in pins.items():
                self._add_pin_to_table(pin_table, pin_name, pin_data, old_values.get(pin_name))
        elif isinstance(pins, list):
            # If pins is a list, process each dictionary item
            for pin_item in pins:
                # Extract pin name from the dictionary
                pin_name = pin_item.get("pin_name", "Unknown")
                self._add_pin_to_table(pin_table, pin_name, pin_item, old_values.get(pin_name))
    
    def _add_pin_to_table(self, pin_table, pin_name, pin_data, old_value=None):
        """Helper method to add a pin to the table with consistent formatting"""
        # Format pin fields based on api_client_ui.py
        pin_type = pin_data.get("pin_type", "N/A")
        hal_pin_type = pin_data.get("hal_pin_type", "N/A")
        hal_pin_dir = pin_data.get("hal_pin_direction", "N/A")
        pin_id = pin_data.get("pin_id", "N/A")
        
        # Format value
        value = pin_data.get("current_value", pin_data.get("value", "N/A"))
        
        # Format digital pin values with color (matching api_client_ui.py)
        if hal_pin_type == "HAL_BIT" and value not in ('N/A', None):
            if value == 1 or value == "1" or value is True:
                value_str = f"[green]HIGH[/]"
            else:
                value_str = f"[red]LOW[/]"
        else:
            value_str = str(value)
            
        # Highlight the value if it has changed
        if old_value is not None and value_str != old_value:
            value_str = f"[bold][reverse]{value_str}[/reverse][/bold]"
        
        # Add the row
        pin_table.add_row(
            pin_name,
            pin_type,
            hal_pin_type,
            hal_pin_dir,
            str(pin_id),
            value_str
        )
        
    def _format_duration(self, seconds: int) -> str:
        """Format seconds into days, hours, minutes, seconds (matching api_client_ui.py)"""
        days, remainder = divmod(int(seconds), 86400)
        hours, remainder = divmod(remainder, 3600)
        minutes, seconds = divmod(remainder, 60)
        
        return f"{days}d {hours}h {minutes}m {seconds}s"

class AboutView(ListViewBase):
    """Shows information about the application"""
    
    def __init__(self, app_instance: "APIClientApp", **kwargs):
        super().__init__(app_instance)
        self._app = app_instance
        
    @property
    def app(self) -> "APIClientApp":
        """Access to the app instance."""
        return self._app
    
    def compose(self) -> ComposeResult:
        """Compose the user interface"""
        yield Header(show_clock=True)
        yield Label("About Arduino LinuxCNC Connector V2.0", id="title", classes="heading")
        
        # Container for About information
        with Container(id="about_container", classes="details-box"):
            yield Label("[bold underline]LinuxCNC_ArduinoConnector V2[/]", classes="about-title")
            yield Label("")
            yield Label("By Alexander Richter and Ken Thompson")
            yield Label("Copyright (c) 2023 Alexander Richter & Ken Thompson")
            yield Label("")
            yield Label("[bold]What's new?[/]")
            yield Label("With this new Version the configuration and communication between")
            yield Label("Arduino and the receiving Python script is completely reworked.")
            yield Label("")
            yield Label("[bold]For the User these are the Main new Features:[/]")
            yield Label("- all of the configuration is done in a single yaml script")
            yield Label("- support for multiple Microcontrollers simultaneously")
            yield Label("- support for Ethernet and Wifi connections (in the future)")
            yield Label("- more versatile communication protocol")
            yield Label("- REST API for monitoring and controlling Arduino connections")
            yield Label("")
            yield Label("[bold]Support the project:[/]")
            yield Label("Patreon: [link=https://www.patreon.com/theartoftinkering]https://www.patreon.com/theartoftinkering[/link]")
            yield Label("Website: [link=https://theartoftinkering.com]https://theartoftinkering.com[/link]")
            yield Label("YouTube: [link=https://youtube.com/@theartoftinkering]https://youtube.com/@theartoftinkering[/link]")
            yield Label("GitHub: [link=https://github.com/KennethThompson]https://github.com/KennethThompson[/link]")
            
        # Add styled buttons
        with Horizontal(id="buttons_container", classes="button-container"):
            yield Button("Back", id="back", variant="default")
            yield Button("Quit", id="quit", variant="error")
        
        yield Footer()
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        # Set up button event handlers
        back_button = self.query_one("#back")
        back_button.on_click = self.on_back
        
        quit_button = self.query_one("#quit")
        quit_button.on_click = self._app.exit
    
    def on_back(self) -> None:
        """Handle the back button click"""
        self._app.switch_view("list")

class APIClientApp(App):
    """Main application class for the API client"""
    
    # Define key bindings
    BINDINGS = [
        ("q", "quit", "Quit"),
        ("r", "refresh", "Refresh"),
        ("enter", "enter", "Select/Details"),
    ]
    
    # Define CSS for styling
    CSS = """
    Screen {
        background: #1f1d2e;
    }
    
    .heading {
        text-align: center;
        background: #2d3142;
        color: #f9f8f9;
        padding: 1;
        margin-bottom: 1;
        text-style: bold;
        width: 100%;
        border: wide #59546a;
    }
    
    .button-container {
        margin-top: 1;
        align: center middle;
        height: auto;
    }
    
    Button {
        margin: 1 2;
    }
    
    #back {
        background: #58546c;
    }
    
    #refresh {
        background: #3a78ab; 
    }
    
    #show_details {
        background: #39a870;
    }
    
    #about {
        background: #7a69b3;
    }
    
    #quit {
        background: #a84d39;
    }
    
    DataTable {
        width: 100%;
        height: 80%;
    }
    
    .table-border {
        border: wide #59546a;
        padding: 0 1;
    }
    
    #details_container, #about_container {
        width: 100%;
        height: auto;
        border: wide #59546a;
        padding: 1;
        margin-bottom: 1;
    }
    
    .details-box {
        background: #292537;
    }
    
    .about-title {
        text-align: center;
        margin: 1 0 2 0;
        text-style: bold;
        color: #e4c9ff;
    }
    """
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arduino_data = []
        self.current_view = "list"
        
        # Create views
        self.list_view = ArduinoListView(app_instance=self)
        self.detail_view = ArduinoDetailView(app_instance=self)
        self.about_view = AboutView(app_instance=self)
    
    def compose(self) -> ComposeResult:
        """Compose the application UI"""
        # Mount all views at startup
        yield self.list_view
        yield self.detail_view
        yield self.about_view
        
        # Initially hide the detail and about views
        self.detail_view.display = False
        self.about_view.display = False
    
    def on_mount(self) -> None:
        """Set up the application on mount"""
        # Set up timer for refreshing data
        self.set_interval(5, self.refresh_data)
        
        # Do initial data fetch
        self.refresh_data()
    
    def on_arduino_list_view_show_details_pressed(self, event: Button.Pressed) -> None:
        """Handle the show details button press"""
        # Get the selected row from the table
        table = self.list_view.query_one("#arduino_table")
        if table.cursor_row is not None:
            alias = table.get_cell_at((table.cursor_row, 0))
            self.switch_view("detail", alias)
    
    def switch_view(self, view_name: str, alias: str = None) -> None:
        """Switch between views"""
        if view_name == "list" and self.current_view != "list":
            # Stop auto-updates in detail view before hiding
            if self.current_view == "detail" and hasattr(self.detail_view, "stop_auto_updates"):
                self.detail_view.stop_auto_updates()
                
            # Switch to list view
            self.list_view.display = True
            self.detail_view.display = False
            self.about_view.display = False
            self.current_view = "list"
            # Update the list
            self.refresh_data()
        
        elif view_name == "detail" and self.current_view != "detail":
            # Switch to detail view
            self.list_view.display = False
            self.detail_view.display = True
            self.about_view.display = False
            self.current_view = "detail"
            
            # Load the Arduino details if an alias is provided
            if alias:
                self.detail_view.load_details(alias)
                
            # Start auto-updates in detail view
            if hasattr(self.detail_view, "start_auto_updates"):
                self.detail_view.start_auto_updates()
                
        elif view_name == "about" and self.current_view != "about":
            # Switch to about view
            self.list_view.display = False
            self.detail_view.display = False
            self.about_view.display = True
            self.current_view = "about"
    
    def refresh_data(self) -> None:
        """Refresh the data from the API"""
        try:
            # Get Arduino list
            response = requests.get(f"{API_BASE_URL}/arduinos", timeout=2)
            response.raise_for_status()
            arduinos = response.json()
            
            # Debug - print the first Arduino data to understand structure
            if arduinos and len(arduinos) > 0:
                print("\nDEBUG - First Arduino data:")
                for key, value in arduinos[0].items():
                    print(f"  {key}: {value} (type: {type(value).__name__})")
                    
                # Specifically check for enabled status fields
                enabled_fields = ["enabled", "is_enabled", "enable", "active", "is_active"]
                print("\nDEBUG - Checking for enabled status fields:")
                for field in enabled_fields:
                    if field in arduinos[0]:
                        print(f"  Found '{field}': {arduinos[0][field]} (type: {type(arduinos[0][field]).__name__})")
                
            self.arduino_data = arduinos
            
            # Update the current view
            if self.current_view == "list":
                self.list_view.update_data(arduinos)
            elif self.current_view == "detail" and self.detail_view.current_alias:
                self.detail_view.load_details(self.detail_view.current_alias)
                
        except requests.exceptions.RequestException as e:
            # Handle API connection errors
            if self.current_view == "list":
                # Clear the table and add an error row
                self.list_view.arduino_table.clear()
                self.list_view.arduino_table.add_row(
                    "[red]API Error[/]", 
                    f"[red]Cannot connect to API: {type(e).__name__}[/]",
                    f"Check if daemon is running at {API_BASE_URL}",
                    "", "", ""
                )
            # Disable buttons when API is unavailable
            if hasattr(self.list_view, "query_one"):
                try:
                    self.list_view.query_one("#show_details").disabled = True
                except:
                    pass
    
    def get_arduino_details(self, alias: str) -> Optional[Dict[str, Any]]:
        """Get detailed information about a specific Arduino"""
        try:
            response = requests.get(f"{API_BASE_URL}/arduinos/{alias}", timeout=2)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            # Show error in the details view
            details_container = self.detail_view.query_one("#details_container")
            details_container.remove_children()
            details_container.mount(
                Label(f"[red]Error: Cannot connect to API[/]"),
                Label(f"[red]{type(e).__name__}: {str(e)}[/]"),
                Label(f"Check if daemon is running at {API_BASE_URL}")
            )
            self.detail_view.query_one("#pin_table").clear()
            return None
    
    def exit(self) -> None:
        """Exit the application"""
        super().exit()
        
    def action_quit(self) -> None:
        """Action handler for quit key binding"""
        self.exit()
        
    def action_refresh(self) -> None:
        """Action handler for refresh key binding"""
        self.refresh_data()
        
    def action_enter(self) -> None:
        """Action handler for enter key binding"""
        if self.current_view == "list":
            table = self.list_view.query_one("#arduino_table")
            if table.cursor_row is not None:
                alias = table.get_cell_at((table.cursor_row, 0))
                self.switch_view("detail", alias)
        elif self.current_view == "detail" or self.current_view == "about":
            self.switch_view("list")

def main():
    app = APIClientApp()
    app.run()

if __name__ == "__main__":
    main() 