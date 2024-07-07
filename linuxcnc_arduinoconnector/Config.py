# Defaults
DEFAULT_PROFILE_NAME = 'default_profile.yaml'
DEFAULT_LINUXCNC_PROFILE_INI_HEADER = 'ARDUINO_CONNECTOR'
DEFAULT_LINUXCNC_PROFILE_INI_YAML_PATH_KEY = 'YAML_PATH'
DEFAULT_LINUXCNC_PROFILE_INI_REMOTE_DEBUG_KEY = 'REMOTE_DEBUG'
DEFAULT_LINUXCNC_PROFILE_INI_WAIT_ON_REMOTE_DEBUG_CONNECT_KEY = 'WAIT_ON_REMOTE_DEBUG_CONNECT'

# The following will override any other settings such as from the linuxcnc profile.
# Use these overrides for debugging purposes, such as some mysterious startup crash.
DEFAULT_LOGGING_ENABLED = True
DEFAULT_LOGGING_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'

DEFAULT_LOG_FILE_NAME = 'arduino_connector.log'
DEFAULT_LOG_FILE_PATH = '/home/ken/git-repos/LinuxCNC_ArduinoConnector'

DEFAULT_REMOTE_DEBUG_ENABLED = True
DEFAULT_REMOTE_DEBUG_PORT = 5678
DEFAULT_REMOTE_DEBUG_BIND_ADDRESS = '0.0.0.0'
DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT = True
