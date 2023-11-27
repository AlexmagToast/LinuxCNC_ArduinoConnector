import pytest
#import os
from pathlib import Path

from LinuxCNC_ArduinoConnector.ArduinoConnector import ArduinoYamlParser

@pytest.fixture
def base_path() -> Path:
    """Get the current folder of the test"""
    return Path(__file__).parent

def test_something(base_path: Path, monkeypatch: pytest.MonkeyPatch):
    #monkeypatch.chdir(base_path / "data")
    a = ArduinoYamlParser.parseYaml(path='new_config.yaml')
    assert True == True