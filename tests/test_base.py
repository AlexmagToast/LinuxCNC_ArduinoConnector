import pytest
#import os
from pathlib import Path

from linuxcnc_arduinoconnector.ArduinoConnector import AnalogConfigElement, ArduinoYamlParser, ConfigPinTypes


@pytest.fixture
def base_path() -> Path:
    """Get the current folder of the test"""
    return Path(__file__).parent

def test_analog_inputs_defaults(base_path: Path, monkeypatch: pytest.MonkeyPatch):
    monkeypatch.chdir(base_path / "data")
    a = ArduinoYamlParser.parseYaml(path='defaults.yaml')
    assert len(a) == 1
    arduino = a[0]
    assert len(arduino.io_map) > 0
    assert ConfigPinTypes.ANALOG_INPUTS in arduino.io_map
    assert len(arduino.io_map[ConfigPinTypes.ANALOG_INPUTS]) == 8
    for apin in arduino.io_map[ConfigPinTypes.ANALOG_INPUTS]:
        assert apin.pinSmoothing == AnalogConfigElement.PIN_SMOOTHING.value[1]
        assert apin.pinMaxVal == AnalogConfigElement.PIN_MAX_VALUE.value[1]
        assert apin.pinMinVal == AnalogConfigElement.PIN_MIN_VALUE.value[1]
    assert True == True

def test_config_not_found():
    with pytest.raises(FileNotFoundError) as excinfo:  
        a = ArduinoYamlParser.parseYaml(path='missing.yaml')
    assert str(excinfo.value) == "Error. missing.yaml not found."  
        
