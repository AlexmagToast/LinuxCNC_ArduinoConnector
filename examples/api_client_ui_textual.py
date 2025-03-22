#!/usr/bin/env python3
"""
Arduino Connector Textual-based UI
A modern terminal UI for the Arduino Connector API
"""
import sys
import requests
import atexit
import signal
from datetime import datetime
from typing import Dict, List, Any, Optional
import asyncio
import time

from textual import on
from textual.app import App, ComposeResult
from textual.containers import Container, Horizontal
from textual.reactive import reactive
from textual.widgets import (
    Button, DataTable, Footer, Header, Label, Static, LoadingIndicator
)
from textual import events
from textual.widget import Widget
from textual.screen import Screen
from textual.message import Message
from textual.timer import Timer

# Monkey patch to fix timer cleanup issue on exit
original_stop_all = Timer._stop_all
async def safe_stop_all(cls, timers):
    """Safe version of _stop_all that handles Timer objects in _interval"""
    try:
        return await original_stop_all(timers)
    except TypeError:
        print("Caught Timer cleanup error, exiting safely")
        return None

Timer._stop_all = classmethod(safe_stop_all)

# API configuration
API_BASE_URL = "http://localhost:8765"

# API Health Check Constants
HEALTH_CHECK_INTERVAL = 5.0  # Seconds between health checks
HEALTH_CHECK_TIMEOUT = 2.0   # Seconds to wait for a health check response

class ApiStatusOverlay(Container):
    """An overlay that shows when the API is down"""
    
    def __init__(self, app_instance):
        super().__init__(id="api_status_overlay")
        self._app = app_instance
        self._countdown = HEALTH_CHECK_INTERVAL
        self._timer_id = None
        
    def compose(self) -> ComposeResult:
        """Compose the overlay"""
        yield Container(
            Label("API Connection Error", id="api_error_title", classes="error-title"),
            Label("The Arduino Connector API is not responding", id="api_error_message"),
            Label("", id="retry_countdown"),
            LoadingIndicator(id="retry_loading"),
            Button("Try Again Now", id="retry_now", variant="primary"),
            Label("Press 'q' to quit the application", id="quit_hint", classes="quit-hint"),
            id="api_error_container",
            classes="error-container"
        )
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        retry_button = self.query_one("#retry_now")
        retry_button.on_click = self.retry_now
        
        # Initialize UI elements
        self.query_one("#retry_loading").styles.display = "none"
        
        # Start countdown
        self.start_countdown()
    
    def start_countdown(self) -> None:
        """Start the countdown timer for automatic retry"""
        self._countdown = int(HEALTH_CHECK_INTERVAL)
        self.update_countdown_display()
        
        # Cancel existing timer if any
        if self._timer_id is not None:
            try:
                self._app.set_timer(self._timer_id, None)
            except Exception as e:
                print(f"Error cancelling timer: {e}")
            self._timer_id = None
        
        # Use a simple callback for countdown
        def countdown_callback():
            self._countdown -= 1
            self.update_countdown_display()
            
            if self._countdown <= 0:
                # Time to retry
                if self._timer_id is not None:
                    try:
                        self._app.set_timer(self._timer_id, None)
                    except Exception:
                        pass
                    self._timer_id = None
                
                # Execute health check
                self._app.check_api_health()
            else:
                # Schedule next countdown tick
                self._timer_id = self._app.set_timer(1.0, countdown_callback)
        
        # Start the countdown
        self._timer_id = self._app.set_timer(1.0, countdown_callback)
    
    def update_countdown_display(self) -> None:
        """Update the countdown label"""
        countdown_label = self.query_one("#retry_countdown")
        countdown_label.update(f"Automatically retrying in {self._countdown} seconds...")
    
    def retry_now(self) -> None:
        """Handle manual retry button click"""
        # Cancel the countdown timer
        if self._timer_id is not None:
            try:
                self._app.set_timer(self._timer_id, None)
            except Exception:
                pass
            self._timer_id = None
        
        # Show loading indicator
        self.query_one("#retry_loading").styles.display = "block"
        self.query_one("#retry_countdown").styles.display = "none"
        self.query_one("#retry_now").disabled = True
        
        # Execute health check after a small delay
        def execute_health_check():
            try:
                self._app.check_api_health()
            except Exception as e:
                print(f"Error during health check: {e}")
                self.query_one("#retry_now").disabled = False
                self.query_one("#retry_loading").styles.display = "none"
                self.query_one("#retry_countdown").styles.display = "block"
                # Restart countdown
                self.start_countdown()
                
        self._app.set_timer(0.5, execute_health_check)

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
        elif event.key == "b" or event.key == "escape":
            # Use 'b' and escape as shortcuts to go back from details view
            if self._app.current_view == "detail" and hasattr(self, 'on_back'):
                print("DEBUG - Back key pressed")
                self.on_back()
            # Also handle escape to go back from about view
            elif self._app.current_view == "about" and hasattr(self, 'on_back'):
                print("DEBUG - Back key pressed from about")
                self.on_back()
        elif event.key == "enter":
            # Skip enter handling if API is unavailable
            if not getattr(self._app, 'api_available', True):
                return
                
            # When Enter key is pressed in list view, show details of selected Arduino
            if self._app.current_view == "list" and hasattr(self, 'on_show_details'):
                self.on_show_details()
            # Remove the enter key handling for going back to list view
            # This allows the enter key to be used exclusively for pin toggling in detail view

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
        
        # Set up table event handler
        self.arduino_table.on_row_highlighted = self.on_row_selected
        
        # Try to focus the table if possible
        try:
            # For newer Textual versions
            self.arduino_table.focus()
        except AttributeError:
            # Fallback for older versions
            pass
    
    @on(Button.Pressed, "#refresh")
    def on_refresh_button_pressed(self, event: Button.Pressed) -> None:
        """Handle refresh button press"""
        print("DEBUG - Refresh button pressed")
        self.on_refresh()
        
    @on(Button.Pressed, "#show_details")
    def on_details_button_pressed(self, event: Button.Pressed) -> None:
        """Handle details button press"""
        print("DEBUG - Details button pressed")
        self.on_show_details()
        
    @on(Button.Pressed, "#about")
    def on_about_button_pressed(self, event: Button.Pressed) -> None:
        """Handle about button press"""
        print("DEBUG - About button pressed")
        self.on_about()
        
    @on(Button.Pressed, "#quit")
    def on_quit_button_pressed(self, event: Button.Pressed) -> None:
        """Handle quit button press"""
        print("DEBUG - Quit button pressed")
        self._app.exit()
    
    def on_refresh(self) -> None:
        """Handle refresh button click"""
        self._app.refresh_data()
    
    def on_show_details(self) -> None:
        """Handle show details button click"""
        # Check if table exists and has rows before trying to access data
        if not hasattr(self, 'arduino_table') or self.arduino_table is None:
            print("DEBUG - Cannot show details: arduino_table doesn't exist")
            return
            
        if self.arduino_table.cursor_row is None:
            print("DEBUG - Cannot show details: No row selected")
            return
            
        # Check if table has any rows
        if self.arduino_table.row_count == 0:
            print("DEBUG - Cannot show details: Table has no rows")
            return
            
        try:
            # Get the selected Arduino's alias
            alias = self.arduino_table.get_cell_at((self.arduino_table.cursor_row, 0))
            if not alias or alias == "Unknown":
                print(f"DEBUG - Cannot show details: Invalid alias '{alias}'")
                return
                
            print(f"DEBUG - Showing details for Arduino: {alias}")
            self._app.switch_view("detail", alias)
        except Exception as e:
            print(f"DEBUG - Error showing details: {e}")
            import traceback
            traceback.print_exc()
            # Don't try to switch views if we can't get the alias
            
    def on_about(self) -> None:
        """Handle about button click"""
        self._app.switch_view("about")
    
    def on_row_selected(self, cursor_row) -> None:
        """Enable/disable show details button based on row selection"""
        # Enable details button only when a row is selected AND table has rows
        should_enable = cursor_row is not None and self.arduino_table.row_count > 0
        details_button = self.query_one("#show_details")
        
        if details_button.disabled == should_enable:  # Only update if needed
            print(f"DEBUG - Setting details button enabled: {should_enable}")
            details_button.disabled = not should_enable
    
    def update_data(self, arduinos: List[Dict[str, Any]]) -> None:
        """Update the Arduino data table"""
        print(f"DEBUG - ArduinoListView.update_data called with {len(arduinos)} arduinos")
        
        # Clear the table
        self.arduino_table.clear()
        print(f"DEBUG - Cleared table, now has {self.arduino_table.row_count} rows")
        
        # Add the data rows with styled status cells
        for arduino in arduinos:
            # Alias
            alias = arduino.get("alias", "Unknown")
            print(f"DEBUG - Processing Arduino: {alias}")
            
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
                    print(f"DEBUG - Found status in field '{field}': {status}")
                    break
            
            if status is None:
                status = "UNKNOWN"
                print(f"DEBUG - No status field found, using default: {status}")
                
            # Check enabled state
            enabled = arduino.get("enabled")
            
            # Debug print for enabled status
            print(f"DEBUG - Arduino {alias} enabled={enabled} (type: {type(enabled).__name__})")
            
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
                print(f"DEBUG - Setting {alias} to DISABLED because enabled={enabled}")
            
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
            try:
                self.arduino_table.add_row(
                    alias, component_name, device, status_str, linuxcnc_str, features_str
                )
                print(f"DEBUG - Added row for {alias}, table now has {self.arduino_table.row_count} rows")
            except Exception as e:
                print(f"DEBUG - Error adding row for {alias}: {e}")
                import traceback
                traceback.print_exc()
            
        # Enable details button only if rows exist
        has_rows = self.arduino_table.row_count > 0
        print(f"DEBUG - Table has {self.arduino_table.row_count} rows, setting details button to {'enabled' if has_rows else 'disabled'}")
        self.query_one("#show_details").disabled = not has_rows

class ArduinoDetailView(ListViewBase):
    """Shows detailed information for a single Arduino"""
    
    def __init__(self, app_instance: "APIClientApp", **kwargs):
        super().__init__(app_instance)
        self._app = app_instance
        self.current_alias = None
        self._update_timer_id = None
        self._selection_cooldown = 1.0  # Seconds to delay updates after selection changes
        self._last_user_interaction = 0  # Track when user last interacted with UI
        self.hal_emulation = False  # Track if HAL emulation is enabled
        self.pin_data = {}  # Store pin data for access in key handlers
        
    @property
    def app(self) -> "APIClientApp":
        """Access to the app instance."""
        return self._app
    
    def compose(self) -> ComposeResult:
        """Compose the user interface"""
        yield Header(show_clock=True)
        yield Label("Arduino LinuxCNC Connector V2.0", id="title", classes="heading")
        
        # Add a prominent back button at the top
        with Horizontal(id="back_button_container", classes="back-button-container"):
            yield Button("« Back to List", id="back", variant="default")
        
        # Container for Arduino details
        yield Container(id="details_container", classes="details-box")
        
        # Create styled table with border
        yield DataTable(id="pin_table", zebra_stripes=True, classes="table-border")
        
        # Add styled buttons (removed refresh button)
        with Horizontal(id="buttons_container", classes="button-container"):
            yield Button("Back to List", id="back_bottom", variant="default")
            yield Button("About", id="about", variant="primary")
            yield Button("Quit", id="quit", variant="error")
        
        yield Footer()
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        # Set up pin table columns
        pin_table = self.query_one("#pin_table")
        pin_table.add_column("Pin", width=15)
        pin_table.add_column("Pin Type", width=10) 
        pin_table.add_column("HAL Pin Type", width=15)
        pin_table.add_column("HAL Pin Dir", width=15)
        pin_table.add_column("Pin ID", width=8)
        pin_table.add_column("Value", width=10)
        
        # Set cursor_type to row for whole row selection
        pin_table.cursor_type = "row"
        
    def on_key(self, event: events.Key) -> None:
        """Handle key events with active period extension"""
        # Update the interaction timestamp to prevent updates while user is active
        self._last_user_interaction = time.time()
        
        # Handle pin value toggling in HAL emulation mode
        if event.key == "enter" and self.hal_emulation:
            pin_table = self.query_one("#pin_table")
            if pin_table and pin_table.cursor_row is not None:
                try:
                    # Get pin name and type from selected row
                    pin_name = pin_table.get_cell_at((pin_table.cursor_row, 0))
                    pin_type = pin_table.get_cell_at((pin_table.cursor_row, 1))
                    
                    # Only toggle digital input pins (din)
                    if pin_type == "din" and pin_name in self.pin_data:
                        # Get current value
                        pin_data = self.pin_data[pin_name]
                        current_value = pin_data.get("current_value", pin_data.get("value", 0))
                        
                        # Convert to boolean/int if needed
                        if isinstance(current_value, str):
                            if current_value.lower() in ("true", "1", "high"):
                                current_value = 1
                            else:
                                current_value = 0
                        elif isinstance(current_value, bool):
                            current_value = 1 if current_value else 0
                        
                        # Toggle value (0->1, 1->0)
                        new_value = 0 if current_value else 1
                        print(f"DEBUG - Toggling pin {pin_name} from {current_value} to {new_value}")
                        
                        # Update pin data
                        pin_data["current_value"] = new_value
                        pin_data["value"] = new_value
                        
                        # Update table display
                        value_str = "[green]HIGH[/]" if new_value else "[red]LOW[/]"
                        value_str = f"[bold][reverse]{value_str}[/reverse][/bold]"
                        pin_table.update_cell_at((pin_table.cursor_row, 5), value_str)
                        
                        # Make API call to update the pin value if available
                        try:
                            if self.app.api_available and self.current_alias:
                                requests.post(
                                    f"{API_BASE_URL}/arduinos/{self.current_alias}/pins/{pin_name}/value",
                                    json={"value": new_value},
                                    timeout=1.0
                                )
                                print(f"DEBUG - Sent pin value update to API: {pin_name}={new_value}")
                        except Exception as e:
                            print(f"DEBUG - Failed to send pin update to API: {e}")
                        
                        # Don't call parent handler to avoid default Enter behavior
                        return
                        
                except Exception as e:
                    print(f"DEBUG - Error toggling pin value: {e}")
        
        # Call parent handler for normal key processing
        super().on_key(event)

    @on(Button.Pressed, "#back")
    def on_back_button_pressed(self, event: Button.Pressed) -> None:
        """Handle back button press"""
        print("DEBUG - Back button pressed")
        self.on_back()
        
    @on(Button.Pressed, "#back_bottom")
    def on_back_bottom_button_pressed(self, event: Button.Pressed) -> None:
        """Handle back bottom button press"""
        print("DEBUG - Back bottom button pressed")
        self.on_back()
    
    @on(Button.Pressed, "#about")
    def on_about_button_pressed(self, event: Button.Pressed) -> None:
        """Handle about button press"""
        print("DEBUG - About button pressed")
        self.on_about()
    
    @on(Button.Pressed, "#quit")
    def on_quit_button_pressed(self, event: Button.Pressed) -> None:
        """Handle quit button press"""
        print("DEBUG - Quit button pressed")
        self._app.exit()
    
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
        if self._update_timer_id is None:
            self._update_timer_id = self.app.set_timer(1.0, self.perform_update)
    
    def perform_update(self) -> None:
        """Execute a single update and schedule the next one"""
        try:
            # Check if we should skip update due to recent user interaction
            current_time = time.time()
            if (current_time - self._last_user_interaction) < self._selection_cooldown:
                print(f"DEBUG - User interaction cooldown active, skipping update")
                # Schedule next update
                self._update_timer_id = self.app.set_timer(1.0, self.perform_update)
                return
                
            # Update the data
            self.update_all_data()
            
            # Schedule the next update
            self._update_timer_id = self.app.set_timer(1.0, self.perform_update)
        except Exception as e:
            print(f"Error in perform_update: {e}")
            # Try to schedule another update anyway
            try:
                self._update_timer_id = self.app.set_timer(1.0, self.perform_update)
            except Exception:
                pass
    
    def stop_auto_updates(self) -> None:
        """Stop automatic pin updates"""
        if self._update_timer_id is not None:
            try:
                self.app.set_timer(self._update_timer_id, None)
            except Exception as e:
                print(f"Error cancelling timer: {e}")
            self._update_timer_id = None
    
    def update_all_data(self) -> None:
        """Update all Arduino data automatically"""
        if not self.current_alias:
            return
        
        # Skip update if API is unavailable
        if not self.app.api_available:
            return
            
        try:
            # Get fresh Arduino details
            arduino_details = self._app.get_arduino_details(self.current_alias)
            if not arduino_details:
                return
                
            # Update details container
            self.update_details_container(arduino_details)
            
            # Update pin table with in-place cell updates to preserve selection
            self.update_pin_table(arduino_details.get("pins", []))
        except Exception as e:
            print(f"Error updating Arduino data: {e}")
    
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
        
        # Store HAL emulation state
        self.hal_emulation = arduino_details.get("hal_emulation", False)
        print(f"DEBUG - HAL emulation mode: {self.hal_emulation}")
        
        # Initial update of all data
        self.update_details_container(arduino_details)
        self.update_pin_table(arduino_details.get("pins", []))
        
        # Start automatic updates - timer will keep everything refreshed
        self.start_auto_updates()
        
    def update_pin_table(self, pins) -> None:
        """Update the pin table with in-place updates to preserve selection"""
        pin_table = self.query_one("#pin_table")
        if not pin_table:
            return
            
        # Process the pins data into a name-indexed dict for easier lookup
        pin_data_by_name = {}
        if isinstance(pins, dict):
            # Convert dict to name-indexed format
            for pin_name, pin_data in pins.items():
                pin_data_by_name[pin_name] = pin_data.copy() if isinstance(pin_data, dict) else {"value": pin_data}
                pin_data_by_name[pin_name]["pin_name"] = pin_name
        elif isinstance(pins, list):
            # Convert list to name-indexed format
            for pin_item in pins:
                pin_name = pin_item.get("pin_name", "Unknown")
                pin_data_by_name[pin_name] = pin_item
        
        # Store pin data for access in key handlers
        self.pin_data = pin_data_by_name
        
        # Check if this is the first update (empty table)
        if pin_table.row_count == 0:
            # First time - build the table from scratch
            self._build_initial_pin_table(pin_table, pin_data_by_name)
            return
            
        # Store all current displayed pin names
        current_pins = []
        for i in range(pin_table.row_count):
            try:
                current_pins.append(pin_table.get_cell_at((i, 0)))
            except Exception:
                pass
                
        # Find pins to remove, update, or add
        pins_to_remove = [pin for pin in current_pins if pin not in pin_data_by_name]
        pins_to_update = [pin for pin in current_pins if pin in pin_data_by_name]
        pins_to_add = [pin for pin in pin_data_by_name if pin not in current_pins]
        
        # Remember cursor position
        cursor_row = pin_table.cursor_row
        cursor_col = pin_table.cursor_column
        
        # Update existing pins in-place - this preserves selection
        for pin_name in pins_to_update:
            try:
                # Find the row for this pin
                row_idx = current_pins.index(pin_name)
                
                # Get pin data
                pin_data = pin_data_by_name[pin_name]
                
                # Only update the value column (column 5)
                # Get current and new values
                current_value = pin_table.get_cell_at((row_idx, 5))
                
                # Format the new value
                value = pin_data.get("current_value", pin_data.get("value", "N/A"))
                hal_pin_type = pin_data.get("hal_pin_type", "N/A")
                
                # Format digital pin values with color
                if hal_pin_type == "HAL_BIT" and value not in ('N/A', None):
                    if value == 1 or value == "1" or value is True:
                        value_str = f"[green]HIGH[/]"
                    else:
                        value_str = f"[red]LOW[/]"
                else:
                    value_str = str(value)
                
                # Highlight changed values
                if current_value != value_str:
                    value_str = f"[bold][reverse]{value_str}[/reverse][/bold]"
                    
                # Update just this cell
                pin_table.update_cell_at((row_idx, 5), value_str)
            except Exception as e:
                print(f"DEBUG - Error updating pin {pin_name}: {e}")
        
        # There are table changes (adds/removes) - do a full rebuild if needed
        if pins_to_add or pins_to_remove:
            print(f"DEBUG - Table structure changed! {len(pins_to_add)} pins added, {len(pins_to_remove)} removed")
            self._rebuild_pin_table(pin_table, pin_data_by_name, cursor_row, cursor_col)
    
    def _build_initial_pin_table(self, pin_table, pin_data_by_name):
        """Build the initial pin table from scratch"""
        print("DEBUG - Building initial pin table")
        # Add all pins to the table
        for pin_name, pin_data in pin_data_by_name.items():
            # Format fields
            pin_type = pin_data.get("pin_type", "N/A")
            hal_pin_type = pin_data.get("hal_pin_type", "N/A")
            hal_pin_dir = pin_data.get("hal_pin_direction", "N/A")
            pin_id = pin_data.get("pin_id", "N/A")
            
            # Format value
            value = pin_data.get("current_value", pin_data.get("value", "N/A"))
            
            # Format digital pin values with color
            if hal_pin_type == "HAL_BIT" and value not in ('N/A', None):
                if value == 1 or value == "1" or value is True:
                    value_str = f"[green]HIGH[/]"
                else:
                    value_str = f"[red]LOW[/]"
            else:
                value_str = str(value)
            
            # Add the row
            pin_table.add_row(
                pin_name,
                pin_type,
                hal_pin_type,
                hal_pin_dir,
                str(pin_id),
                value_str
            )
    
    def _rebuild_pin_table(self, pin_table, pin_data_by_name, old_cursor_row=None, old_cursor_col=None):
        """Rebuild the entire pin table when structure changes"""
        # Remember which pin was selected
        selected_pin_name = None
        if old_cursor_row is not None and old_cursor_row < pin_table.row_count:
            try:
                selected_pin_name = pin_table.get_cell_at((old_cursor_row, 0))
                print(f"DEBUG - Remembering selected pin: {selected_pin_name}")
            except Exception:
                pass
        
        # Remember old values for highlighting
        old_values = {}
        for i in range(pin_table.row_count):
            try:
                pin_name = pin_table.get_cell_at((i, 0))
                current_value = pin_table.get_cell_at((i, 5))
                old_values[pin_name] = current_value
            except Exception:
                pass
        
        # Clear the table
        pin_table.clear()
        
        # Add all pins in order
        for pin_name, pin_data in pin_data_by_name.items():
            self._add_pin_to_table(pin_table, pin_name, pin_data, old_values.get(pin_name))
            
        # Restore selection if possible
        if selected_pin_name in pin_data_by_name:
            # Find the row index of the selected pin
            new_pin_names = list(pin_data_by_name.keys())
            try:
                new_row = new_pin_names.index(selected_pin_name)
                pin_table.cursor_row = new_row
                pin_table.cursor_column = old_cursor_col if old_cursor_col is not None else 0
                print(f"DEBUG - Restored selection to {selected_pin_name} at row {new_row}")
            except ValueError:
                print(f"DEBUG - Could not find {selected_pin_name} in new data")
        elif pin_table.row_count > 0 and old_cursor_row is not None:
            # Try to select a row at approximately the same position
            new_row = min(old_cursor_row, pin_table.row_count - 1)
            pin_table.cursor_row = new_row
            pin_table.cursor_column = old_cursor_col if old_cursor_col is not None else 0
            print(f"DEBUG - Selected row {new_row} based on previous position")

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

    def update_details_container(self, arduino_details: Dict[str, Any]) -> None:
        """Update the details container with fresh data"""
        details_container = self.query_one("#details_container")
        details_container.remove_children()
        
        # Debug print the full arduino_details for diagnosis
        print("\nDEBUG - Full arduino_details:")
        for key, value in arduino_details.items():
            if key != "pins":  # Don't print the pins array
                print(f"  {key}: {value} (type: {type(value).__name__})")
        
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
        enabled_str = "NO" if is_disabled else "YES"
        enabled_color = "red" if is_disabled else "green"
        
        # Format features
        features = arduino_details.get("features", [])
        features_str = ", ".join(features) if features and isinstance(features, list) else "None"
        
        # Get Arduino uptime - first check for ut value (directly from Arduino heartbeat)
        ut_minutes = arduino_details.get('ut')
        print(f"DEBUG - Looking for 'ut' value in API response: {ut_minutes}")
        
        if ut_minutes is not None:
            print(f"DEBUG - Using direct 'ut' value: {ut_minutes} minutes (type: {type(ut_minutes).__name__})")
            try:
                # "ut" is already in minutes, so use it directly
                arduino_uptime = self._format_duration_from_minutes(ut_minutes)
                print(f"DEBUG - Formatted 'ut' to: {arduino_uptime}")
            except (ValueError, TypeError) as e:
                print(f"DEBUG - Error formatting 'ut' value: {e}")
                arduino_uptime = f"{ut_minutes}m"
        else:
            # Fall back to the formatted arduino_reported_uptime from the API
            raw_uptime = arduino_details.get('arduino_reported_uptime', 'N/A')
            print(f"DEBUG - Raw arduino_reported_uptime: {raw_uptime}")
            
            # Process the uptime to remove seconds if it's in the format "Xd Yh Zm Ws"
            if raw_uptime != 'N/A' and raw_uptime is not None:
                try:
                    # Check if it's already in a time format with "s" at the end
                    if isinstance(raw_uptime, str) and any(unit in raw_uptime for unit in ['d', 'h', 'm', 's']):
                        print(f"DEBUG - Reformatting time string: {raw_uptime}")
                        # Parse out just the days, hours, and minutes - ignore seconds
                        parts = raw_uptime.split()
                        days = hours = minutes = 0
                        
                        for part in parts:
                            if part.endswith('d'):
                                days = int(part[:-1])
                            elif part.endswith('h'):
                                hours = int(part[:-1])
                            elif part.endswith('m'):
                                minutes = int(part[:-1])
                        
                        # Create a new format without seconds
                        arduino_uptime = f"{days}d {hours}h {minutes}m"
                        print(f"DEBUG - Reformatted to: {arduino_uptime}")
                    elif raw_uptime.endswith('s'):
                        # If it's just seconds, convert to minutes
                        secs = int(raw_uptime[:-1])
                        minutes = secs // 60
                        arduino_uptime = f"0d 0h {minutes}m"
                        print(f"DEBUG - Converted seconds to minutes: {arduino_uptime}")
                    else:
                        # If it's not a recognized format, pass it through
                        arduino_uptime = raw_uptime
                except Exception as e:
                    print(f"DEBUG - Error reformatting uptime: {e}")
                    arduino_uptime = raw_uptime
            else:
                arduino_uptime = raw_uptime
        
        print(f"DEBUG - Final uptime to display: {arduino_uptime}")
            
        # Get connection uptime with seconds included
        connection_uptime = arduino_details.get('connection_uptime', 'N/A')
        print(f"DEBUG - Connection uptime received from API: {connection_uptime}")
        
        # Make sure seconds are preserved in connection uptime
        if connection_uptime != 'N/A':
            # Check if uptime only shows minutes without seconds
            if connection_uptime.endswith('m') and not any(unit in connection_uptime for unit in ['s']):
                # Add "0s" to ensure seconds are displayed
                connection_uptime = f"{connection_uptime} 0s"
                print(f"DEBUG - Added seconds to connection uptime: {connection_uptime}")
        
        # LinuxCNC Status
        linuxcnc_status = arduino_details.get('linuxcnc_status', 'N/A')
        
        # Check if HAL emulation is enabled
        hal_emulation = arduino_details.get('hal_emulation', False)
        
        if hal_emulation:
            # Show special emulation status with yellow background and black text
            linuxcnc_str = f"[black on yellow]EMULATION_ENABLED[/]"
        elif linuxcnc_status == "CONNECTED":
            linuxcnc_str = f"[green]{linuxcnc_status}[/]"
        elif linuxcnc_status == "ERROR":
            linuxcnc_str = f"[red]{linuxcnc_status}[/]"
        else:
            linuxcnc_str = f"{linuxcnc_status}"
        
        # Add detailed information (matching api_client_ui.py layout)
        details_container.mount(
            Label(f"[bold]Component Name:[/] {component_name}"),
            Label(f"[bold]Device:[/] {arduino_details.get('device', 'N/A')}"),
            Label(f"[bold]Serial Port Available:[/] {'[green]YES[/]' if arduino_details.get('serial_port_available', False) else '[red]NO[/]'}"),
            Label(f"[bold]Enabled:[/] [{enabled_color}]{enabled_str}[/]"),
            Label(f"[bold]Arduino Status:[/] [{status_style}]{status}[/]"),
            Label(f"[bold]LinuxCNC Status:[/] {linuxcnc_str}"),
            Label(f"[bold]Arduino Reported Uptime:[/] {arduino_uptime}"),
            Label(f"[bold]Connection to Arduino Uptime:[/] {connection_uptime}"),
        )

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
            # Use simple format for links - no markup
            yield Label("Patreon: https://www.patreon.com/theartoftinkering")
            yield Label("Website: https://theartoftinkering.com")
            yield Label("YouTube: https://youtube.com/@theartoftinkering")
            yield Label("GitHub: https://github.com/KennethThompson")
            
        # Add styled buttons
        with Horizontal(id="buttons_container", classes="button-container"):
            yield Button("Back", id="back", variant="default")
            yield Button("Quit", id="quit", variant="error")
        
        yield Footer()
    
    def on_mount(self) -> None:
        """Set up event handlers"""
        pass
        
    @on(Button.Pressed, "#back")
    def on_back_button_pressed(self, event: Button.Pressed) -> None:
        """Handle back button press"""
        print("DEBUG - About: Back button pressed")
        self.on_back()
    
    @on(Button.Pressed, "#quit")
    def on_quit_button_pressed(self, event: Button.Pressed) -> None:
        """Handle quit button press"""
        print("DEBUG - About: Quit button pressed")
        self._app.exit()
    
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
        ("escape", "back", "Back"),
        ("b", "back", "Back"),
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
    
    .back-button-container {
        margin: 1 0;
        align: left middle;
        height: auto;
        background: #252235;
        padding: 1;
        border-bottom: wide #59546a;
    }
    
    #back {
        background: #58546c;
        margin-left: 2;
    }
    
    #back_bottom {
        background: #58546c;
    }
    
    Button {
        margin: 1 2;
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
    
    /* API Status Overlay Styling */
    #api_status_overlay {
        width: 100%;
        height: 100%;
        background: rgba(0, 0, 0, 0.7);
        align: center middle;
        layer: above;  /* Use Textual's layer property instead of z-index */
    }
    
    #api_error_container {
        background: #2d2138;
        width: 60%;
        height: auto;
        padding: 2;
        border: round #ec505e;
        align: center middle;
    }
    
    .error-container {
        layout: vertical;
        align: center middle;
    }
    
    .error-title {
        text-align: center;
        text-style: bold;
        color: #ec505e;
        margin-bottom: 1;
    }
    
    #api_error_message {
        text-align: center;
        margin-bottom: 1;
    }
    
    #retry_countdown {
        text-align: center;
        margin-bottom: 1;
        color: #a3a2a6;
    }
    
    #retry_loading {
        margin: 1 0;
    }
    
    #retry_now {
        margin-top: 1;
        background: #3a78ab;
    }
    
    .quit-hint {
        margin-top: 1;
        text-align: center;
        color: #a3a2a6;
        text-style: italic;
    }
    """
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arduino_data = []
        self.current_view = "list"
        self.api_available = True
        self._health_check_timer_id = None
        
        # Create views
        self.list_view = ArduinoListView(app_instance=self)
        self.detail_view = ArduinoDetailView(app_instance=self)
        self.about_view = AboutView(app_instance=self)
        self.api_status_overlay = ApiStatusOverlay(app_instance=self)
    
    def compose(self) -> ComposeResult:
        """Compose the application UI"""
        # Mount all views at startup
        yield self.list_view
        yield self.detail_view
        yield self.about_view
        
        # Initially hide the detail and about views
        self.detail_view.display = False
        self.about_view.display = False
        
        # Add the API status overlay (initially hidden)
        yield self.api_status_overlay
        self.api_status_overlay.display = False
    
    def on_mount(self) -> None:
        """Set up the application on mount"""
        print("DEBUG - Application mounting")
        
        # Do initial health check
        self.check_api_health()
        
        # Add explicit initial data refresh with delay
        def initial_refresh():
            print("DEBUG - Performing initial data refresh")
            self.refresh_data()
            
        # Schedule the refresh with a short delay to ensure UI is ready
        self.set_timer(0.5, initial_refresh)
    
    def check_api_health(self) -> None:
        """Check if the API is available"""
        print("DEBUG - Starting API health check")
        # Cancel any existing health check timer
        if self._health_check_timer_id is not None:
            try:
                self.set_timer(self._health_check_timer_id, None)
            except Exception:
                pass
            self._health_check_timer_id = None
            
        try:
            # Try to reach the health endpoint
            print("DEBUG - Sending request to /health endpoint")
            response = requests.get(f"{API_BASE_URL}/health", timeout=HEALTH_CHECK_TIMEOUT)
            print(f"DEBUG - Health check response: status={response.status_code}")
            
            # Check if response is successful (status code 200)
            if response.status_code == 200:
                # API is available
                if not self.api_available:
                    print("DEBUG - API state changed: unavailable -> available")
                    # Update state
                    self.api_available = True
                    
                    # Hide the overlay
                    self.hide_api_status_overlay()
                    
                    # Refresh data based on current view
                    if self.current_view == "list":
                        print("DEBUG - Health check triggering data refresh for list view")
                        self.refresh_data()
                    elif self.current_view == "detail":
                        # If we are in detail view, refresh the details
                        if self.detail_view.current_alias:
                            print(f"DEBUG - Health check triggering detail refresh for {self.detail_view.current_alias}")
                            self.detail_view.load_details(self.detail_view.current_alias)
                else:
                    print("DEBUG - API remains available") 
                
                # Schedule next health check
                def schedule_next_check():
                    self.check_api_health()
                
                print(f"DEBUG - Scheduling next health check in {HEALTH_CHECK_INTERVAL} seconds")    
                self._health_check_timer_id = self.set_timer(HEALTH_CHECK_INTERVAL, schedule_next_check)
            else:
                # API responded but with an error status
                print(f"DEBUG - API health check failed with status code: {response.status_code}")
                self.handle_api_unavailable()
                
        except requests.exceptions.RequestException as e:
            # API is unavailable or not responding
            print(f"DEBUG - API health check failed with request error: {str(e)}")
            self.handle_api_unavailable()
        except Exception as e:
            # Catch any other errors that might occur
            print(f"DEBUG - Unexpected error in health check: {str(e)}")
            import traceback
            traceback.print_exc()
            self.handle_api_unavailable()
    
    def handle_api_unavailable(self) -> None:
        """Handle case when API is unavailable"""
        # If API was previously available, log state change
        if self.api_available:
            print("API is now unavailable")
            
        # Update state
        self.api_available = False
        
        # Show API status overlay
        self.show_api_status_overlay()
    
    def show_api_status_overlay(self) -> None:
        """Show the API status overlay"""
        # Set loading indicator to hidden and countdown to visible
        try:
            if self.api_status_overlay.display is False:
                self.api_status_overlay.display = True
                
            loading = self.api_status_overlay.query_one("#retry_loading")
            if loading:
                loading.styles.display = "none"
                
            countdown = self.api_status_overlay.query_one("#retry_countdown")
            if countdown:
                countdown.styles.display = "block"
                
            retry_button = self.api_status_overlay.query_one("#retry_now")
            if retry_button:
                retry_button.disabled = False
                
            # Start the countdown
            self.api_status_overlay.start_countdown()
        except Exception as e:
            print(f"Error showing API status overlay: {e}")
    
    def hide_api_status_overlay(self) -> None:
        """Hide the API status overlay"""
        # Cancel any running countdown timer in the overlay
        if hasattr(self.api_status_overlay, "_timer_id") and self.api_status_overlay._timer_id is not None:
            try:
                self.set_timer(self.api_status_overlay._timer_id, None)
            except Exception as e:
                print(f"Error cancelling timer: {e}")
            self.api_status_overlay._timer_id = None
            
        # Hide the overlay
        self.api_status_overlay.display = False
        
        # If we're in detail view, restart auto-updates
        if self.current_view == "detail" and hasattr(self.detail_view, "start_auto_updates"):
            self.detail_view.start_auto_updates()
    
    def on_arduino_list_view_show_details_pressed(self, event: Button.Pressed) -> None:
        """Handle the show details button press"""
        # Get the selected row from the table
        table = self.list_view.query_one("#arduino_table")
        if table.cursor_row is not None:
            alias = table.get_cell_at((table.cursor_row, 0))
            self.switch_view("detail", alias)
    
    def switch_view(self, view_name: str, alias: str = None) -> None:
        """Switch between views"""
        # Don't switch views if API is unavailable
        if not self.api_available and view_name != "about":
            return
            
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
        # Skip refresh if API is unavailable
        if not self.api_available:
            print("DEBUG - Skipping refresh because API marked as unavailable")
            return
            
        print(f"DEBUG - Starting data refresh, current_view={self.current_view}")
        try:
            # Get Arduino list
            print("DEBUG - Sending request to /arduinos endpoint")
            response = requests.get(f"{API_BASE_URL}/arduinos", timeout=2)
            print(f"DEBUG - Got response: status={response.status_code}")
            response.raise_for_status()
            
            # Check response content
            raw_content = response.text
            print(f"DEBUG - Response content length: {len(raw_content)} chars")
            print(f"DEBUG - Response starts with: {raw_content[:100]}")
            
            # Parse JSON
            try:
                arduinos = response.json()
                
                # Handle case where API returns null or non-list
                if arduinos is None:
                    print("DEBUG - API returned None instead of a list")
                    arduinos = []
                elif not isinstance(arduinos, list):
                    print(f"DEBUG - API returned {type(arduinos).__name__} instead of a list")
                    if isinstance(arduinos, dict):
                        # If it's a single Arduino as a dict, convert to a list
                        arduinos = [arduinos]
                    else:
                        arduinos = []
                
                print(f"DEBUG - Parsed JSON successfully, got {len(arduinos)} Arduino(s)")
            except Exception as e:
                print(f"DEBUG - JSON parse error: {e}")
                print(f"DEBUG - Raw content: {raw_content}")
                raise
            
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
            else:
                print("DEBUG - No Arduino data returned from API")
                
            self.arduino_data = arduinos
            
            # Update the current view
            if self.current_view == "list":
                print("DEBUG - Updating list view with data")
                try:
                    self.list_view.update_data(arduinos)
                    print(f"DEBUG - List view updated, rows count: {self.list_view.arduino_table.row_count}")
                except Exception as e:
                    print(f"DEBUG - Error updating list view: {e}")
                    import traceback
                    traceback.print_exc()
            elif self.current_view == "detail" and self.detail_view.current_alias:
                print("DEBUG - Updating detail view for alias:", self.detail_view.current_alias)
                self.detail_view.load_details(self.detail_view.current_alias)
            else:
                print(f"DEBUG - No view update needed, current_view={self.current_view}")
                
        except requests.exceptions.RequestException as e:
            print(f"DEBUG - API request error: {e}")
            # Treat any API error as potential unavailability
            self.check_api_health()
        except Exception as e:
            print(f"DEBUG - Unexpected error during refresh: {e}")
            import traceback
            traceback.print_exc()
    
    def get_arduino_details(self, alias: str) -> Optional[Dict[str, Any]]:
        """Get detailed information about a specific Arduino"""
        # Skip if API is unavailable
        if not self.api_available:
            return None
            
        try:
            response = requests.get(f"{API_BASE_URL}/arduinos/{alias}", timeout=2)
            response.raise_for_status()
            
            # Log raw response for debugging
            raw_text = response.text
            print(f"DEBUG - Raw API response for arduino details: {raw_text[:500]}...")
            
            # Check if response contains 'ut' field
            if '"ut":' in raw_text:
                print("DEBUG - Found 'ut' field in raw response")
                
            resp_data = response.json()
            print(f"DEBUG - Parsed response keys: {list(resp_data.keys())}")
            return resp_data
        except requests.exceptions.RequestException as e:
            # Treat any API error as potential unavailability
            self.check_api_health()
            return None
    
    def exit(self) -> None:
        """Exit the application"""
        # First disable all automatic updates
        try:
            # Turn off all automatic processing
            if self.current_view == "detail":
                self.detail_view.stop_auto_updates()
                
            # Cancel API status overlay timer
            if hasattr(self, 'api_status_overlay') and hasattr(self.api_status_overlay, '_timer_id') and self.api_status_overlay._timer_id is not None:
                try:
                    self.set_timer(self.api_status_overlay._timer_id, None)
                    self.api_status_overlay._timer_id = None
                except Exception as e:
                    print(f"Error cancelling overlay timer: {e}")
                    
            # Cancel health check timer
            if self._health_check_timer_id is not None:
                try:
                    self.set_timer(self._health_check_timer_id, None)
                    self._health_check_timer_id = None
                except Exception as e:
                    print(f"Error cancelling health check timer: {e}")
        except Exception as e:
            # Catch any errors during timer cleanup
            print(f"Error during timer cleanup: {e}")
            
        # Exit without any further timer operations
        try:
            # Call the parent exit method
            super().exit()
        except TypeError:
            # If we get a TypeError about Timer operations, force exit
            import sys
            print("Forcing exit due to Timer cleanup error")
            sys.exit(0)
        
    def action_quit(self) -> None:
        """Action handler for quit key binding"""
        self.exit()
        
    def action_refresh(self) -> None:
        """Action handler for refresh key binding"""
        self.refresh_data()
        
    def action_enter(self) -> None:
        """Action handler for enter key binding"""
        try:
            if self.current_view == "list":
                table = self.list_view.query_one("#arduino_table")
                if table and table.cursor_row is not None and table.row_count > 0:
                    try:
                        alias = table.get_cell_at((table.cursor_row, 0))
                        self.switch_view("detail", alias)
                    except Exception as e:
                        print(f"Error getting cell data: {e}")
            elif self.current_view == "detail" or self.current_view == "about":
                self.switch_view("list")
        except Exception as e:
            print(f"Error in action_enter: {e}")

    def action_back(self) -> None:
        """Action handler for back key bindings"""
        if self.current_view == "detail" or self.current_view == "about":
            print("DEBUG - Back action triggered")
            self.switch_view("list")

    def _format_duration(self, seconds: int) -> str:
        """Format seconds into days, hours, minutes (no seconds)"""
        days, remainder = divmod(int(seconds), 86400)
        hours, remainder = divmod(remainder, 3600)
        minutes, _ = divmod(remainder, 60)  # Ignore seconds
        
        return f"{days}d {hours}h {minutes}m"

    def _format_duration_from_minutes(self, minutes: float) -> str:
        """Format minutes into days, hours, minutes"""
        total_minutes = int(minutes)
        days, remainder = divmod(total_minutes, 1440)  # 1440 = minutes in a day
        hours, minutes = divmod(remainder, 60)
        
        return f"{days}d {hours}h {minutes}m"

def force_exit():
    """Force exit the application when all else fails"""
    print("Forcing exit...")
    sys.exit(0)

# Register the force exit function with atexit
atexit.register(force_exit)

# Register a signal handler for SIGINT (Ctrl+C)
def signal_handler(sig, frame):
    """Handle SIGINT signal"""
    print("SIGINT received, exiting...")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def main():
    app = APIClientApp()
    app.run()

if __name__ == "__main__":
    main() 