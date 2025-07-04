import pytest
from unittest.mock import MagicMock, patch
import sys
import os

# Add parent directory to path so we can import our modules
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linuxcnc_arduinoconnector.api import (
    app, set_arduino_connections, format_uptime, 
    get_daemon_status, get_arduinos, get_arduino_details
)
from fastapi.testclient import TestClient

@pytest.fixture
def client():
    """Create a test client for the API"""
    return TestClient(app)

@pytest.fixture
def mock_arduino_connections():
    """Create and set up mock Arduino connections"""
    # Create mock Arduino connection
    mock_arduino = MagicMock()
    mock_arduino.settings.alias = "Arduino Mega2560"
    mock_arduino.settings.component_name = "arduinomega2560"
    mock_arduino.settings.dev = "/dev/cu.usbmodem"
    mock_arduino.settings.enabled = True
    mock_arduino.serialConn.connectionState = "CONNECTED"
    mock_arduino.serialDeviceAvailable = True
    
    # Mock feature with pins
    mock_feature = MagicMock()
    mock_feature.featureName = "DIGITAL_INPUTS"
    
    # Mock pins
    mock_pin = MagicMock()
    mock_pin.pinName = "din.03"
    mock_pin.pinType = "dout"
    mock_pin.halPinType = "HAL_BIT"
    mock_pin.halPinDirection = "HAL_OUT"
    mock_pin.pinID = 3
    mock_pin.halPinConnection.Get.return_value = 0
    
    # Connect mocks
    mock_feature.pinList = [mock_pin]
    mock_arduino.settings.io_map.keys.return_value = [mock_feature]
    
    # Set the mock Arduino for testing
    set_arduino_connections([mock_arduino])
    
    return [mock_arduino]

def test_format_uptime():
    """Test the format_uptime function"""
    assert format_uptime(0) == "0s"
    assert format_uptime(30) == "30s"
    assert format_uptime(90) == "1m 30s"
    assert format_uptime(3600) == "1h 0m 0s"
    assert format_uptime(86400) == "1d 0h 0m 0s"
    assert format_uptime(90061) == "1d 1h 1m 1s"

def test_get_status(client, mock_arduino_connections):
    """Test the /status endpoint"""
    response = client.get("/status")
    assert response.status_code == 200
    data = response.json()
    assert data["status"] == "RUNNING"
    assert data["arduino_count"] == 1

def test_get_arduinos(client, mock_arduino_connections):
    """Test the /arduinos endpoint"""
    response = client.get("/arduinos")
    assert response.status_code == 200
    data = response.json()
    assert len(data) == 1
    assert data[0]["alias"] == "Arduino Mega2560"
    assert data[0]["device"] == "/dev/cu.usbmodem"
    assert data[0]["arduino_status"] == "CONNECTED"
    assert data[0]["enabled"] is True
    assert data[0]["features"] == ["DIGITAL_INPUTS"]

def test_get_arduino_details(client, mock_arduino_connections):
    """Test the /arduinos/{alias} endpoint"""
    response = client.get("/arduinos/Arduino%20Mega2560")
    assert response.status_code == 200
    data = response.json()
    assert data["component_name"] == "arduinomega2560"
    assert data["device"] == "/dev/cu.usbmodem"
    assert data["arduino_status"] == "CONNECTED"
    assert len(data["pins"]) == 1
    assert data["pins"][0]["pin_name"] == "din.03"
    assert data["pins"][0]["pin_id"] == 3
    assert data["pins"][0]["current_value"] == 0

def test_get_nonexistent_arduino(client, mock_arduino_connections):
    """Test the /arduinos/{alias} endpoint with a nonexistent Arduino"""
    response = client.get("/arduinos/NonExistentArduino")
    assert response.status_code == 404 