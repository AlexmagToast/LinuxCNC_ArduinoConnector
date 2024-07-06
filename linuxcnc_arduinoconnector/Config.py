# Defaults
DEFAULT_PROFILE_NAME = 'default_profile.yaml'

# The following will override any other settings such as from the linuxcnc profile.
# Use these overrides for debugging purposes, such as some mysterious startup crash.
DEFAULT_LOGGING_ENABLED = True
DEFAULT_LOGGING_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
DEFAULT_LAUNCH_CONSOLE_UI = False
DEFAULT_LOG_FILE_NAME = 'arduino_connector.log'
DEFAULT_LOG_FILE_PATH = '/home/ken/git-repos/LinuxCNC_ArduinoConnector'

DEFAULT_REMOTE_DEBUG_ENABLED = True
DEFAULT_REMOTE_DEBUG_PORT = 5678
DEFAULT_REMOTE_DEBUG_BIND_ADDRESS = '0.0.0.0'
DEFAULT_REMOTE_DEBUG_WAIT_ON_CONNECT = True

DEFAULT_LAUNCH_CONSOLE_UI = True