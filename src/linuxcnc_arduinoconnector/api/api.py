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

class ApiHealth(BaseModel):
    """Schema for health check response"""
    status: str
    timestamp: str

class PinInfo(BaseModel):
    pin_name: str
    pin_type: str
    hal_pin_type: str
    hal_pin_direction: str
    pin_id: str
    current_value: Any

class FeatureStatus(BaseModel):
    """Schema for feature status information"""
    name: str
    ready: bool
    config_complete: bool = False
    config_sync_error: bool = False

class ArduinoDetailedInfo(BaseModel):
    component_name: str
    device: str
    serial_port_available: bool
    arduino_status: str
    linuxcnc_status: str
    arduino_reported_uptime: str
    connection_uptime: str
    hal_emulation: bool = False
    enabled: bool = False  # Add enabled flag for UI
    features: List[FeatureStatus] = []  # Add feature status information
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

class PinValueUpdate(BaseModel):
    """Schema for pin value update request"""
    value: Any

# Track API server start time
start_time = time.time()

def format_uptime(seconds):
    """Format seconds into days, hours, minutes (no seconds)."""
    days, remainder = divmod(seconds, 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, _ = divmod(remainder, 60)  # Ignore seconds
    
    if days > 0:
        return f"{int(days)}d {int(hours)}h {int(minutes)}m"
    elif hours > 0:
        return f"{int(hours)}h {int(minutes)}m"
    else:
        return f"{int(minutes)}m"

def format_connection_uptime(seconds):
    """Format seconds into days, hours, minutes, seconds for connection uptime."""
    days, remainder = divmod(seconds, 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, secs = divmod(remainder, 60)
    
    # Always include seconds
    if days > 0:
        return f"{int(days)}d {int(hours)}h {int(minutes)}m {int(secs)}s"
    elif hours > 0:
        return f"{int(hours)}h {int(minutes)}m {int(secs)}s"
    elif minutes > 0:
        return f"{int(minutes)}m {int(secs)}s"
    else:
        return f"{int(secs)}s"

@app.get("/health", response_model=ApiHealth)
async def health_check():
    """Simple health check endpoint to verify API is running"""
    return {
        "status": "UP",
        "timestamp": datetime.now().isoformat()
    }

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
        
        # Collect feature status information
        features_status = []
        try:
            if hasattr(arduino.settings, 'io_map'):
                for feature in arduino.settings.io_map.keys():
                    # Use feature lock when accessing feature properties
                    with feature.with_lock(timeout=2) as acquired:
                        if not acquired:
                            logging.warning(f"Could not acquire lock for feature {feature.featureName if hasattr(feature, 'featureName') else 'Unknown'}")
                            continue  # Skip this feature if we can't get the lock
                            
                        if hasattr(feature, 'featureName'):
                            # Access feature properties under lock protection
                            is_ready = feature.FeatureReady() if hasattr(feature, 'FeatureReady') else False
                            config_complete = feature.ConfigComplete() if hasattr(feature, 'ConfigComplete') else False
                            config_sync_error = feature.ConfigSyncError() if hasattr(feature, 'ConfigSyncError') else False
                            
                            # Add feature status to the list
                            features_status.append({
                                "name": feature.featureName,
                                "ready": is_ready,
                                "config_complete": config_complete,
                                "config_sync_error": config_sync_error
                            })
                            logging.info(f"Feature {feature.featureName}: ready={is_ready}, config_complete={config_complete}, config_sync_error={config_sync_error}")
        except Exception as e:
            logging.error(f"Error processing features for Arduino {alias}: {str(e)}")
            import traceback
            logging.error(traceback.format_exc())
        
        # Get all pins
        pins = []
        try:
            if hasattr(arduino.settings, 'io_map'):
                logging.info(f"IO Map keys: {list(arduino.settings.io_map.keys())}")
                for feature in arduino.settings.io_map.keys():
                    # Use feature lock when accessing pins
                    with feature.with_lock(timeout=2) as acquired:
                        if not acquired:
                            logging.warning(f"Could not acquire lock for feature {feature.featureName if hasattr(feature, 'featureName') else 'Unknown'} pins")
                            continue  # Skip this feature's pins if we can't get the lock
                            
                        logging.info(f"Processing feature: {feature.featureName if hasattr(feature, 'featureName') else 'Unknown'}")
                        # Get feature name for associating with pins
                        feature_name = feature.featureName if hasattr(feature, 'featureName') else "Unknown"
                        feature_ready = feature.FeatureReady() if hasattr(feature, 'FeatureReady') else False
                        
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
                                        current_value = pin.arduinoPinCurrentValue if hasattr(pin, 'arduinoPinCurrentValue') else "N/A"
                                else:
                                    # Use stored value if no HAL connection
                                    current_value = pin.arduinoPinCurrentValue if hasattr(pin, 'arduinoPinCurrentValue') else "N/A"
                                
                                pin_info = {
                                    "pin_name": pin.pinName if hasattr(pin, 'pinName') else "Unknown",
                                    "pin_type": pin.pinType if hasattr(pin, 'pinType') else "Unknown",
                                    "hal_pin_type": str(pin.halPinType) if hasattr(pin, 'halPinType') else "Unknown",
                                    "hal_pin_direction": str(pin.halPinDirection) if hasattr(pin, 'halPinDirection') else "Unknown",
                                    "pin_id": str(pin.pinID) if hasattr(pin, 'pinID') else "Unknown",
                                    "current_value": current_value if current_value is not None else "N/A",
                                    "feature_name": feature_name,  # Include feature name with each pin
                                    "feature_ready": feature_ready  # Include feature readiness state
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
        ut_minutes = None  # Initialize ut_minutes
        try:
            if hasattr(arduino.serialConn, 'arduinoReportedUptime') and arduino.serialConn.arduinoReportedUptime > 0:
                # Log the raw uptime value for debugging
                logging.info(f"Raw arduinoReportedUptime: {arduino.serialConn.arduinoReportedUptime}")
                
                # If ut_minutes wasn't found, use the existing uptime value
                if ut_minutes is None:
                    # Check whether the value is likely to be milliseconds or minutes
                    # If the value is very small (less than 1000), it's likely in minutes already
                    if arduino.serialConn.arduinoReportedUptime < 1000:
                        # Assume this is minutes already
                        ut_minutes = int(arduino.serialConn.arduinoReportedUptime)
                        arduino_uptime = format_uptime(ut_minutes * 60)  # Convert to seconds
                        logging.info(f"Treating small value as minutes directly: {ut_minutes} -> {arduino_uptime}")
                    else:
                        # For backward compatibility, convert the existing uptime value
                        # Note: arduinoReportedUptime is in milliseconds, convert to seconds
                        arduino_uptime = format_uptime(arduino.serialConn.arduinoReportedUptime / 1000)
                        logging.info(f"Using default uptime calculation: {arduino_uptime}")
        except Exception as e:
            logging.error(f"Error calculating arduino uptime: {str(e)}")
        
        connection_uptime = "N/A"
        try:
            if hasattr(arduino.serialConn, 'connLastFormed') and arduino.serialConn.connLastFormed is not None:
                connection_seconds = (time.time() - arduino.serialConn.connLastFormed.timestamp())
                logging.info(f"Raw connection seconds: {connection_seconds}")
                connection_uptime = format_connection_uptime(connection_seconds)
                logging.info(f"Formatted connection uptime: {connection_uptime}")
        except Exception as e:
            logging.error(f"Error calculating connection uptime: {str(e)}")
        
        # Determine LinuxCNC status
        linuxcnc_status = "DISCONNECTED"  # Default status
        
        # Get the HAL emulation flag from settings
        hal_emulation = getattr(arduino.settings, 'hal_emulation', False)
        logging.info(f"HAL emulation for {alias}: {hal_emulation}")
        
        # Get enabled state (needed by UI)
        enabled = getattr(arduino.settings, 'enabled', False)
        
        # Build the response with safe access to all properties
        try:
            response_data = {
                "component_name": arduino.settings.component_name if hasattr(arduino.settings, 'component_name') else "Unknown",
                "device": arduino.settings.dev if hasattr(arduino.settings, 'dev') else "Unknown",
                "serial_port_available": getattr(arduino, 'serialDeviceAvailable', False),
                "arduino_status": str(getattr(arduino.serialConn, 'connectionState', 'UNKNOWN')),
                "linuxcnc_status": linuxcnc_status,
                "arduino_reported_uptime": arduino_uptime,
                "connection_uptime": connection_uptime,
                "hal_emulation": hal_emulation,
                "enabled": enabled,
                "features": features_status,
                "pins": pins
            }
            
            # Add ut value if it was found
            if ut_minutes is not None:
                response_data["ut"] = ut_minutes
                logging.info(f"Adding ut={ut_minutes} to response")
            
            return response_data
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

@app.post("/arduinos/{alias}/pins/{pin_name}/value")
async def update_pin_value(alias: str, pin_name: str, update: PinValueUpdate):
    """Update the value of a pin in HAL emulation mode"""
    try:
        logging.info(f"Updating pin value for {alias}/{pin_name} to {update.value}")
        # Find the Arduino with the given alias
        arduino = next((a for a in arduino_connections if a.settings.alias == alias), None)
        
        if not arduino:
            raise HTTPException(status_code=404, detail=f"Arduino with alias '{alias}' not found")
        
        # Check if HAL emulation is enabled
        hal_emulation = getattr(arduino.settings, 'hal_emulation', False)
        if not hal_emulation:
            raise HTTPException(status_code=400, detail="HAL emulation is not enabled for this Arduino")
        
        # First, find which feature has the pin (without locking)
        target_feature = None
        for feature in arduino.settings.io_map.keys():
            if hasattr(feature, 'pinList'):
                for pin in feature.pinList:
                    if getattr(pin, 'pinName', '') == pin_name:
                        target_feature = feature
                        break
                if target_feature:
                    break
                    
        if not target_feature:
            raise HTTPException(status_code=404, detail=f"Pin '{pin_name}' not found on Arduino '{alias}'")
            
        # Now we have the feature, acquire its lock and update the pin
        with target_feature.with_lock(timeout=5) as acquired:
            if not acquired:
                logging.error(f"Could not acquire lock for feature {target_feature.featureName} to update pin {pin_name}")
                raise HTTPException(status_code=503, detail="Could not acquire lock to update pin, try again later")
                
            # Check feature readiness
            if not target_feature.FeatureReady():
                raise HTTPException(status_code=400, detail="Feature is not ready for updating")
                
            # Find the pin now that we have the lock
            pin_found = False
            for pin in target_feature.pinList:
                if getattr(pin, 'pinName', '') == pin_name:
                    # Update the pin value under lock protection
                    logging.info(f"Updating pin {pin_name} value to {update.value} in HAL emulation mode")
                    
                    # Update both HAL pin value and Arduino pin value
                    pin.halPinCurrentValue = update.value
                    target_feature.SetPinChangePending(True)
                    # If there's a HAL pin connection, update it too
                    if hasattr(pin, "halPinConnection") and pin.halPinConnection:
                        try:
                            pin.halPinConnection.Set(update.value)
                            logging.info(f"Updated HAL pin connection value for {pin_name}")
                        except Exception as e:
                            logging.error(f"Error updating HAL pin connection: {str(e)}")
                            # Continue anyway since we're in emulation mode
                    
                    pin_found = True
                    break
                    
            if not pin_found:
                # Unlikely to happen since we already found the pin before, but good to check
                raise HTTPException(status_code=404, detail=f"Pin '{pin_name}' not found on Arduino '{alias}'")
        
        return {"status": "success", "message": f"Pin {pin_name} value updated to {update.value}"}
    
    except HTTPException:
        # Re-raise HTTP exceptions
        raise
    except Exception as e:
        # Log the error for debugging
        logging.error(f"Error updating pin value for {alias}/{pin_name}: {str(e)}")
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