import pytest
#import os
from pathlib import Path

from linuxcnc_arduinoconnector.ArduinoConnector import AnalogConfigElement, ArduinoYamlParser, ConfigPinTypes, DigitalConfigElement, HalPinDirection, PinConfigElement


@pytest.fixture
def base_path() -> Path:
    """Get the current folder of the test"""
    return Path(__file__).parent

def test_config_defaults(base_path: Path, monkeypatch: pytest.MonkeyPatch):
    monkeypatch.chdir(base_path / "data")
    a = ArduinoYamlParser.parseYaml(path='defaults.yaml')
    assert len(a) == 1
    arduino = a[0]
    assert len(arduino.io_map) > 0
    # Analog Input Pins Test (Defaults)
    assert ConfigPinTypes.ANALOG_INPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.ANALOG_INPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.ANALOG_INPUTS]:
        assert apin.pinSmoothing == AnalogConfigElement.PIN_SMOOTHING.value[1]
        assert apin.pinMaxVal == AnalogConfigElement.PIN_MAX_VALUE.value[1]
        assert apin.pinMinVal == AnalogConfigElement.PIN_MIN_VALUE.value[1]
        assert apin.halPinDirection == HalPinDirection.HAL_IN
        assert apin.pinInitialState == PinConfigElement.PIN_INITIAL_STATE.value[1]
        assert apin.pinConnectState == PinConfigElement.PIN_CONNECTED_STATE.value[1]
        assert apin.pinDisconnectState == PinConfigElement.PIN_DISCONNECTED_STATE.value[1]

    # Anlog Output Pins Tests (Defaults)
    assert ConfigPinTypes.ANALOG_OUTPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.ANALOG_OUTPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.ANALOG_OUTPUTS]:
        assert apin.pinSmoothing == AnalogConfigElement.PIN_SMOOTHING.value[1]
        assert apin.pinMaxVal == AnalogConfigElement.PIN_MAX_VALUE.value[1]
        assert apin.pinMinVal == AnalogConfigElement.PIN_MIN_VALUE.value[1]
        assert apin.halPinDirection == HalPinDirection.HAL_OUT
        assert apin.pinInitialState == PinConfigElement.PIN_INITIAL_STATE.value[1]
        assert apin.pinConnectState == PinConfigElement.PIN_CONNECTED_STATE.value[1]
        assert apin.pinDisconnectState == PinConfigElement.PIN_DISCONNECTED_STATE.value[1]

    # Digital Input Pins Tests (Defaults)
    assert ConfigPinTypes.DIGITAL_INPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.DIGITAL_INPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.DIGITAL_INPUTS]:
        assert apin.halPinDirection == HalPinDirection.HAL_IN
        assert apin.pinDebounce == DigitalConfigElement.PIN_DEBOUNCE.value[1]
        assert apin.pinInitialState == PinConfigElement.PIN_INITIAL_STATE.value[1]
        assert apin.pinConnectState == PinConfigElement.PIN_CONNECTED_STATE.value[1]
        assert apin.pinDisconnectState == PinConfigElement.PIN_DISCONNECTED_STATE.value[1]
    
    # Digital Output Pins Tests (Defaults)
    assert ConfigPinTypes.DIGITAL_OUTPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.DIGITAL_OUTPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.DIGITAL_OUTPUTS]:
        assert apin.halPinDirection == HalPinDirection.HAL_OUT
        assert apin.pinInitialState == PinConfigElement.PIN_INITIAL_STATE.value[1]
        assert apin.pinConnectState == PinConfigElement.PIN_CONNECTED_STATE.value[1]
        assert apin.pinDisconnectState == PinConfigElement.PIN_DISCONNECTED_STATE.value[1]
        
    # PWM Output Pins Tests (Defaults)
    assert ConfigPinTypes.PWM_OUTPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.PWM_OUTPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.PWM_OUTPUTS]:
        assert apin.halPinDirection == HalPinDirection.HAL_OUT
        assert apin.pinInitialState == PinConfigElement.PIN_INITIAL_STATE.value[1]
        assert apin.pinConnectState == PinConfigElement.PIN_CONNECTED_STATE.value[1]
        assert apin.pinDisconnectState == PinConfigElement.PIN_DISCONNECTED_STATE.value[1]

def test_config_not_found():
    with pytest.raises(FileNotFoundError) as excinfo:  
        a = ArduinoYamlParser.parseYaml(path='missing.yaml')
    assert str(excinfo.value) == "Error. missing.yaml not found."  
        
def test_config_convert(base_path: Path, monkeypatch: pytest.MonkeyPatch):
    monkeypatch.chdir(base_path / "data")
    a = ArduinoYamlParser.parseYaml(path='defaults.yaml')
    pass