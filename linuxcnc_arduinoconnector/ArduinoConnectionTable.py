# FOR FUTURE DEV - DO NOT USE FOR NOW!!

from enum import Enum
from dataclasses import dataclass
from typing import Dict, List

from typing import Callable, List
import threading
import random
import time
class ConnectionState(str, Enum):
    CONNECTED = "CONNECTED"
    DISCONNECTED = "DISCONNECTED"
    CONNECTING = "CONNECTING"
    ERROR = "ERROR"

@dataclass
class ArduinoSettings:
    alias: str
    component_name: str
    dev: str
    enabled: bool = True
    hal_emulation: bool = False

class SerialConnection:
    def __init__(self, state: ConnectionState):
        self.connectionState = state

class Feature:
    def __init__(self, name: str):
        self.featureName = name

class ArduinoConnection:
    def __init__(self, settings: ArduinoSettings, serial_conn: SerialConnection, io_map: Dict[Feature, List], linuxcnc_error: bool = False):
        self.settings = settings
        self.serialConn = serial_conn
        self.io_map = io_map
        self.linuxcnc_error = linuxcnc_error
        self._observers: List[Callable[['ArduinoConnection'], None]] = []

    def subscribe(self, callback: Callable[['ArduinoConnection'], None]):
        self._observers.append(callback)

    def unsubscribe(self, callback: Callable[['ArduinoConnection'], None]):
        self._observers.remove(callback)

    def notify_observers(self):
        for callback in self._observers:
            callback(self)

    def update_property(self, property_name: str, value):
        if property_name == 'serialConn':
            self.serialConn = value
        elif property_name == 'io_map':
            self.io_map = value
        else:
            setattr(self, property_name, value)
        self.notify_observers()
# Create dummy data
dummy_connections = [
    ArduinoConnection(
        settings=ArduinoSettings(alias="Arduino1", component_name="comp1", dev="/dev/ttyUSB0"),
        serial_conn=SerialConnection(ConnectionState.CONNECTED),
        io_map={
            Feature("DigitalInputs"): [1, 2, 3],
            Feature("DigitalOutputs"): [4, 5]
        }
    ),
    ArduinoConnection(
        settings=ArduinoSettings(alias="Arduino2", component_name="comp2", dev="/dev/ttyUSB1"),
        serial_conn=SerialConnection(ConnectionState.DISCONNECTED),
        io_map={
            Feature("AnalogInputs"): [1, 2],
            Feature("AnalogOutputs"): [3]
        }
    ),
    ArduinoConnection(
        settings=ArduinoSettings(alias="Arduino3", component_name="comp3", dev="/dev/ttyACM0", enabled=False),
        serial_conn=SerialConnection(ConnectionState.DISCONNECTED),
        io_map={
            Feature("DigitalInputs"): [1, 2],
            Feature("AnalogOutputs"): [3, 4, 5]
        }
    ),
    ArduinoConnection(
        settings=ArduinoSettings(alias="Arduino4", component_name="comp4", dev="/dev/ttyACM1"),
        serial_conn=SerialConnection(ConnectionState.ERROR),
        io_map={
            Feature("DigitalInputs"): [1],
            Feature("DigitalOutputs"): [2],
            Feature("AnalogInputs"): [3],
            Feature("AnalogOutputs"): [4]
        },
        linuxcnc_error=True
    ),
    ArduinoConnection(
        settings=ArduinoSettings(alias="Arduino5", component_name="comp5", dev="/dev/ttyUSB2", hal_emulation=True),
        serial_conn=SerialConnection(ConnectionState.CONNECTING),
        io_map={
            Feature("DigitalInputs"): [1, 2, 3, 4, 5],
            Feature("DigitalOutputs"): [6, 7, 8, 9, 10]
        }
    )
]

from textual.app import App, ComposeResult
from textual.widgets import DataTable, Static
from textual.containers import Container, VerticalScroll
from textual.reactive import reactive

class ConnectionDetails(Static):
    def __init__(self):
        super().__init__()
        self.styles.width = "100%"
        self.styles.height = "auto"
        self.styles.border = ("heavy", "white")
        self.styles.padding = (1, 1)

    def update_details(self, connection):
        if connection is None:
            self.update("No connection selected")
            return

        details = f"""Alias: {connection.settings.alias}
Component Name: {connection.settings.component_name}
Device: {connection.settings.dev}
Enabled: {connection.settings.enabled}
HAL Emulation: {connection.settings.hal_emulation}
Status: {connection.serialConn.connectionState}
LinuxCNC Status: {"OK" if not connection.linuxcnc_error else "ERROR"}

Features:
"""
        for feature, pins in connection.io_map.items():
            details += f"  {feature.featureName}: {len(pins)} pins\n"
            for pin in pins:
                details += f"    - Pin {pin}\n"

        self.update(details)

class ArduinoConnectionTable(App):
    selected_row = reactive(-1)
    ui_ready = False
    def __init__(self, arduino_connections, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.arduino_connections = arduino_connections
        for connection in self.arduino_connections:
            connection.subscribe(self.on_arduino_updated)
        
        # Start the random update thread
        #self.update_thread = threading.Thread(target=self.random_update_thread, daemon=True)
        #self.update_thread.start()

    def on_data_table_row_selected(self, event: DataTable.RowSelected) -> None:
        self.selected_row = int(event.row_key.value)
        self.update_connection_details()

    def on_arduino_updated(self, connection: ArduinoConnection):
        def update():
            index = self.arduino_connections.index(connection)
            self.update_table_row(index)
            if index == self.selected_row:
                self.update_connection_details()
        self.call_from_thread(update)
        
    def compose(self) -> ComposeResult:
        yield Container(
            DataTable(id="connection_table"),
            VerticalScroll(
                ConnectionDetails(),
                id="details_scroll"
            )
        )

    def update_table_row(self, index: int) -> None:
        if not self.ui_ready:
            return
        table = self.query_one("#connection_table")
        if str(index) not in table.rows:
            return  # Row doesn't exist, so we can't update it
        connection = self.arduino_connections[index]
        
        alias = connection.settings.alias
        component_name = connection.settings.component_name
        device = connection.settings.dev
        status = str(connection.serialConn.connectionState)
        linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"
        features = ", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.io_map.items()])

        new_row = (alias, component_name, device, status, linuxcnc_status, features)
        
        try:
            table.remove_row(str(index))
            table.add_row(*new_row, key=str(index))
        except Exception as e:
            print(f"Error updating row {index}: {e}")
    def update_connection_details(self) -> None:
        if not self.ui_ready:
            return
        details = self.query_one("#details_scroll ConnectionDetails")
        if 0 <= self.selected_row < len(self.arduino_connections):
            selected_connection = self.arduino_connections[self.selected_row]
            details.update_details(selected_connection)
        else:
            details.update_details(None)

    def on_mount(self) -> None:
        table = self.query_one("#connection_table")
        table.add_columns("Alias", "Component Name", "Device", "Status", "LinuxCNC Status", "Features")
        table.cursor_type = "row"

        for index, connection in enumerate(self.arduino_connections):
            alias = connection.settings.alias
            component_name = connection.settings.component_name
            device = connection.settings.dev
            status = str(connection.serialConn.connectionState)
            linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"
            features = ", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.io_map.items()])

            table.add_row(alias, component_name, device, status, linuxcnc_status, features, key=str(index))

        # Automatically select the first row if there are any connections
        if self.arduino_connections:
            self.selected_row = 0
            table.move_cursor(row=0)
            self.update_connection_details()

        self.ui_ready = True
        
        # Start the random update thread after UI is ready
        self.update_thread = threading.Thread(target=self.random_update_thread, daemon=True)
        self.update_thread.start()
    def on_mount(self) -> None:
        table = self.query_one("#connection_table")
        table.add_columns("Alias", "Component Name", "Device", "Status", "LinuxCNC Status", "Features")
        table.cursor_type = "row"

        for index, connection in enumerate(self.arduino_connections):
            alias = connection.settings.alias
            component_name = connection.settings.component_name
            device = connection.settings.dev
            status = str(connection.serialConn.connectionState)
            linuxcnc_status = "OK" if not connection.linuxcnc_error else "ERROR"
            features = ", ".join([f"{k.featureName} [{len(v)}]" for k, v in connection.io_map.items()])

            table.add_row(alias, component_name, device, status, linuxcnc_status, features, key=str(index))

        # Automatically select the first row if there are any connections
        if self.arduino_connections:
            self.selected_row = 0
            table.move_cursor(row=0)
            self.update_connection_details()

        self.ui_ready = True
        
        # Start the random update thread after UI is ready
        self.update_thread = threading.Thread(target=self.random_update_thread, daemon=True)
        self.update_thread.start()
    def random_update_thread(self):
        while not self.ui_ready:
            time.sleep(0.1)  # Wait until UI is ready

        while True:
            try:
                # Sleep for a random time between 2 and 5 seconds
                time.sleep(random.uniform(2, 5))
                
                # Randomly select an Arduino connection
                connection = random.choice(self.arduino_connections)
                
                # Randomly choose a property to update
                update_type = random.choice(['status', 'linuxcnc_error', 'feature'])
                
                if update_type == 'status':
                    new_status = random.choice(list(ConnectionState))
                    connection.update_property('serialConn', SerialConnection(new_status))
                elif update_type == 'linuxcnc_error':
                    new_error_state = random.choice([True, False])
                    connection.update_property('linuxcnc_error', new_error_state)
                elif update_type == 'feature':
                    feature = random.choice(list(connection.io_map.keys()))
                    new_pin_count = random.randint(1, 10)
                    connection.update_property('io_map', {**connection.io_map, feature: list(range(1, new_pin_count + 1))})
            except Exception as e:
                print(f"Error in random update thread: {e}")

def run_arduino_connection_table(arduino_connections):
    app = ArduinoConnectionTable(arduino_connections)
    app.run()

if __name__ == "__main__":
    run_arduino_connection_table(dummy_connections)