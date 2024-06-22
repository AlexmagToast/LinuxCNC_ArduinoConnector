# arduino_connector/__init__.py

# Import key classes and functions to make them accessible at the package level
from .YamlParser import ArduinoYamlParser
from .ConfigModels import ArduinoSettings
from .ArduinoComms import SerialConnection

# Package metadata
__version__ = "0.1.0"
__author__ = "Alexander Richter, Ken Thompson"

__all__ = [
    "ArduinoYamlParser",
    "ArduinoSettings",
    "SerialConnection"
]