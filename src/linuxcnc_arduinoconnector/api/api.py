from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import List, Dict, Any, Optional
import uvicorn
import threading
import time
import logging
from datetime import datetime
from enum import Enum

# Import necessary modules from your codebase
from linuxcnc_arduinoconnector.interfaces.ArduinoComms import ArduinoConnection, ThreadStatus, ConnectionState
from linuxcnc_arduinoconnector.config.Config import DEFAULT_API_PORT, DEFAULT_API_BIND_ADDRESS

# Create FastAPI app
app = FastAPI(title="Arduino Connector API", 
              description="API for managing Arduino connections in LinuxCNC")

# Global reference to our Arduino connections
arduino_connections = []

class DaemonStatus(str, Enum):
    RUNNING = "RUNNING"
    STOPPED = "STOPPED"
    ERROR = "ERROR"

class PinInfo(BaseModel):
    pin_name: str
    pin_type: str
    hal_pin_type: str
    hal_pin_direction: str
    pin_id: str
    current_value: Any

class ArduinoDetailedInfo(BaseModel):
    component_name: str
    device: str
    serial_port_available: bool
    arduino_status: str
    linuxcnc_status: str
    arduino_reported_uptime: str
    connection_uptime: str
    pins: List[PinInfo]

class ArduinoBasicInfo(BaseModel):
    alias: str
    component_name: str
    device: str
    arduino_status: str
    enabled: bool
    features: List[str]

class DaemonStatusResponse(BaseModel):
    status: DaemonStatus
    message: str
    uptime: str
    arduino_count: int

# Track API server start time
start_time = time.time()

def format_uptime(seconds):
    """Format seconds into days, hours, minutes, seconds."""
    days, remainder = divmod(seconds, 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, seconds = divmod(remainder, 60)
    
    if days > 0:
        return f"{int(days)}d {int(hours)}h {int(minutes)}m {int(seconds)}s"
    elif hours > 0:
        return f"{int(hours)}h {int(minutes)}m {int(seconds)}s"
    elif minutes > 0:
        return f"{int(minutes)}m {int(seconds)}s"
    else:
        return f"{int(seconds)}s"

@app.get("/status", response_model=DaemonStatusResponse)
async def get_daemon_status():
    """Get overall daemon status"""
    uptime = time.time() - start_time
    
    return {
        "status": DaemonStatus.RUNNING,  # Stubbed for now as requested
        "message": "Arduino connector daemon is running",
        "uptime": format_uptime(uptime),
        "arduino_count": len(arduino_connections)
    }

@app.get("/arduinos", response_model=List[ArduinoBasicInfo])
async def get_arduinos():
    """Get list of all configured Arduinos and their basic status"""
    result = []
    
    for arduino in arduino_connections:
        # Get features as strings
        features = []
        for feature in arduino.settings.io_map.keys():
            features.append(feature.featureName)
            
        info = {
            "alias": arduino.settings.alias,
            "component_name": arduino.settings.component_name,
            "device": arduino.settings.dev,
            "arduino_status": str(arduino.serialConn.connectionState),
            "enabled": arduino.settings.enabled,
            "features": features
        }
        result.append(info)
        
    return result

@app.get("/arduinos/{alias}", response_model=ArduinoDetailedInfo)
async def get_arduino_details(alias: str):
    """Get detailed information about a specific Arduino"""
    try:
        # Find the Arduino with the given alias
        arduino = next((a for a in arduino_connections if a.settings.alias == alias), None)
        
        if not arduino:
            raise HTTPException(status_code=404, detail=f"Arduino with alias '{alias}' not found")
        
        # Log detailed information for debugging
        logging.info(f"Processing details for Arduino: {alias}")
        logging.info(f"Arduino object: {arduino}")
        logging.info(f"SerialConn state: {getattr(arduino.serialConn, 'connectionState', 'Unknown')}")
        
        # Get all pins
        pins = []
        try:
            if hasattr(arduino.settings, 'io_map'):
                logging.info(f"IO Map keys: {list(arduino.settings.io_map.keys())}")
                for feature in arduino.settings.io_map.keys():
                    logging.info(f"Processing feature: {feature.featureName if hasattr(feature, 'featureName') else 'Unknown'}")
                    if hasattr(feature, 'pinList'):
                        for pin in feature.pinList:
                            # Get current value if available
                            current_value = None
                            if hasattr(pin, "halPinConnection") and pin.halPinConnection:
                                try:
                                    current_value = pin.halPinConnection.Get()
                                except Exception as e:
                                    logging.error(f"Error getting pin value: {str(e)}")
                                    # Fall back to stored value if HAL pin access fails
                                    current_value = pin.currentValue if hasattr(pin, 'currentValue') else "N/A"
                            else:
                                # Use stored value if no HAL connection
                                current_value = pin.currentValue if hasattr(pin, 'currentValue') else "N/A"
                            
                            pin_info = {
                                "pin_name": pin.pinName if hasattr(pin, 'pinName') else "Unknown",
                                "pin_type": pin.pinType if hasattr(pin, 'pinType') else "Unknown",
                                "hal_pin_type": str(pin.halPinType) if hasattr(pin, 'halPinType') else "Unknown",
                                "hal_pin_direction": str(pin.halPinDirection) if hasattr(pin, 'halPinDirection') else "Unknown",
                                "pin_id": str(pin.pinID) if hasattr(pin, 'pinID') else "Unknown",
                                "current_value": current_value if current_value is not None else "N/A"
                            }
                            pins.append(pin_info)
                    else:
                        logging.warning(f"Feature has no pinList attribute: {feature}")
            else:
                logging.warning(f"Arduino has no io_map attribute: {arduino.settings}")
        except Exception as e:
            logging.error(f"Error processing pins for Arduino {alias}: {str(e)}")
            import traceback
            logging.error(traceback.format_exc())
            pins = []  # Default to empty list if we can't get pins
        
        # Calculate uptime with careful error handling
        arduino_uptime = "N/A"
        try:
            if hasattr(arduino.serialConn, 'arduinoReportedUptime') and arduino.serialConn.arduinoReportedUptime > 0:
                arduino_uptime = format_uptime(arduino.serialConn.arduinoReportedUptime / 1000)  # Convert from ms to seconds
        except Exception as e:
            logging.error(f"Error calculating arduino uptime: {str(e)}")
        
        connection_uptime = "N/A"
        try:
            if hasattr(arduino.serialConn, 'connLastFormed') and arduino.serialConn.connLastFormed is not None:
                connection_seconds = (time.time() - arduino.serialConn.connLastFormed.timestamp())
                connection_uptime = format_uptime(connection_seconds)
        except Exception as e:
            logging.error(f"Error calculating connection uptime: {str(e)}")
        
        # Determine LinuxCNC status - this is a stub
        linuxcnc_status = "DISCONNECTED"  # More appropriate default
        
        # Build the response with safe access to all properties
        try:
            return {
                "component_name": arduino.settings.component_name if hasattr(arduino.settings, 'component_name') else "Unknown",
                "device": arduino.settings.dev if hasattr(arduino.settings, 'dev') else "Unknown",
                "serial_port_available": getattr(arduino, 'serialDeviceAvailable', False),
                "arduino_status": str(getattr(arduino.serialConn, 'connectionState', 'UNKNOWN')),
                "linuxcnc_status": linuxcnc_status,
                "arduino_reported_uptime": arduino_uptime,
                "connection_uptime": connection_uptime,
                "pins": pins
            }
        except Exception as e:
            logging.error(f"Error building response object: {str(e)}")
            import traceback
            logging.error(traceback.format_exc())
            raise
    except Exception as e:
        # Log the error for debugging
        logging.error(f"Error in get_arduino_details for {alias}: {str(e)}")
        import traceback
        logging.error(traceback.format_exc())
        # Return a 500 error to the client
        raise HTTPException(status_code=500, detail=f"Internal server error: {str(e)}")

def set_arduino_connections(connections):
    """Set the Arduino connections for the API to use"""
    global arduino_connections
    arduino_connections = connections

def start_api_server():
    """Start the API server"""
    logging.info(f"Starting API server on {DEFAULT_API_BIND_ADDRESS}:{DEFAULT_API_PORT}")
    try:
        uvicorn.run(app, host=DEFAULT_API_BIND_ADDRESS, port=DEFAULT_API_PORT)
    except Exception as e:
        logging.error(f"Failed to start API server: {str(e)}")

def run_api_server_in_thread():
    """Run the API server in a background thread"""
    api_thread = threading.Thread(target=start_api_server, daemon=True)
    api_thread.start()
    return api_thread 