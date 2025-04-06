import datetime
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path
import sys
import os

from linuxcnc_arduinoconnector.config.Config import DEFAULT_LOG_TO_CONSOLE

# Global logger instance
global_logger = None

def get_logger():
    """
    Get the global logger instance. If it hasn't been initialized,
    returns a basic console logger.
    """
    global global_logger
    if global_logger is None:
        # Create a basic console logger as fallback
        logger = logging.getLogger("linuxcnc_arduinoconnector")
        logger.setLevel(logging.INFO)
        handler = logging.StreamHandler(sys.stdout)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        global_logger = logger
    return global_logger

def init_logger(
    log_file_path: Path = None,
    max_bytes: int = 1000000,
    backup_count: int = 3,
    log_level: int = logging.DEBUG,
    log_format: str = '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    log_to_console: bool = DEFAULT_LOG_TO_CONSOLE
):
    """
    Initialize the global logger. This should be called early in your application.
    
    :param log_file_path: Path object pointing to the log file. If None, console logging is used
    :param max_bytes: Maximum size of each log file in bytes
    :param backup_count: Number of backup files to keep
    :param log_level: Logging level
    :param log_format: Format string for log messages
    :param log_to_console: If True, log to console even if log_file_path is specified
    :return: The global logger instance
    """
    global global_logger
    global_logger = setup_logger(
        logger_name="linuxcnc_arduinoconnector",
        log_file_path=log_file_path,
        max_bytes=max_bytes,
        backup_count=backup_count,
        log_level=log_level,
        log_format=log_format,
        log_to_console=log_to_console
    )
    return global_logger

def reconfigure_logger(
    log_file_path: Path = None,
    max_bytes: int = 1000000,
    backup_count: int = 3,
    log_level: int = logging.DEBUG,
    log_format: str = '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    log_to_console: bool = DEFAULT_LOG_TO_CONSOLE
):
    """
    Reconfigure the existing global logger without replacing it.
    This ensures that all references to the logger throughout the code
    will use the updated configuration.
    
    :param log_file_path: Path object pointing to the log file. If None, console logging is used
    :param max_bytes: Maximum size of each log file in bytes
    :param backup_count: Number of backup files to keep
    :param log_level: Logging level
    :param log_format: Format string for log messages
    :param log_to_console: If True, log to console even if log_file_path is specified
    :return: The reconfigured global logger instance
    """
    global global_logger
    
    # If no global logger exists yet, just initialize it
    if global_logger is None:
        return init_logger(
            log_file_path=log_file_path,
            max_bytes=max_bytes,
            backup_count=backup_count,
            log_level=log_level,
            log_format=log_format,
            log_to_console=log_to_console
        )
    
    # Get the existing logger
    logger = global_logger
    
    # Update log level
    print(f'Arduino Connector: Reconfiguring logger with log_level: {log_level}')
    if 'debug' in log_level.lower():
        log_level = logging.DEBUG
    elif 'info' in log_level.lower():
        log_level = logging.INFO
    elif 'warning' in log_level.lower():
        log_level = logging.WARNING
    elif 'error' in log_level.lower():
        log_level = logging.ERROR
    elif 'all' in log_level.lower():
        log_level = logging.DEBUG
    else:
        log_level = logging.NOTSET
    logger.setLevel(log_level)
    
    # Create formatter
    formatter = logging.Formatter(log_format)
    
    # Remove existing handlers
    for existing_handler in logger.handlers[:]:
        logger.removeHandler(existing_handler)
    
    # Add new handlers
    if log_file_path:
        print(f'Arduino Connector: Reconfiguring logger with file: {log_file_path}')
        print(f'Arduino Connector: max_bytes: {max_bytes}')
        print(f'Arduino Connector: backup_count: {backup_count}')   
        print(f'Arduino Connector: log_level: {log_level}')
        print(f'Arduino Connector: log_format: {log_format}')
        
        # Create log directory and file
        filename = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S') + '.log'
        log_file_path = Path(log_file_path).joinpath(filename)
        os.makedirs(log_file_path.parent, exist_ok=True)
        
        # Create rotating file handler
        file_handler = RotatingFileHandler(
            str(log_file_path),
            maxBytes=max_bytes,
            backupCount=backup_count
        )
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
    
    # Add console handler if requested or if no file handler
    if log_to_console or log_file_path is None:
        # Use console handler
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)
    
    # Log that reconfiguration happened
    logger.info("Logger reconfigured with updated settings")
    
    return logger

def setup_logger(
    logger_name: str,
    log_file_path: Path = None,
    max_bytes: int = 1000000,
    backup_count: int = 3,
    log_level: int = logging.DEBUG,
    log_format: str = '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    log_to_console: bool = DEFAULT_LOG_TO_CONSOLE
) -> logging.Logger:
    """
    Set up a logger with either a rolling file handler or a console handler.
    
    :param logger_name: Name of the logger
    :param log_file_path: Path object pointing to the log file. If None, console logging is used
    :param max_bytes: Maximum size of each log file in bytes
    :param backup_count: Number of backup files to keep
    :param log_level: Logging level
    :param log_format: Format string for log messages
    :param log_to_console: If True, log to console even if log_file_path is specified
    :return: Configured logger object
    """
    # Create a logger
    logger = logging.getLogger(logger_name)
    if 'debug' in logger_name.lower():
        log_level = logging.DEBUG
    elif 'info' in logger_name.lower():
        log_level = logging.INFO
    else:
        log_level = logging.NOTSET
    logger.setLevel(log_level)

    # Create a formatting for the logs
    formatter = logging.Formatter(log_format)

    # Remove any existing handlers to avoid duplicate logging
    for existing_handler in logger.handlers[:]:
        logger.removeHandler(existing_handler)

    if log_file_path:
        print(f'Arduino Connector: Creating log file: {log_file_path}')
        print(f'Arduino Connector: max_bytes: {max_bytes}')
        print(f'Arduino Connector: backup_count: {backup_count}')   
        print(f'Arduino Connector: log_level: {log_level}')
        print(f'Arduino Connector: log_format: {log_format}')
        
        filename = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S') + '.log'
        log_file_path = Path(log_file_path).joinpath(filename)
        os.makedirs(log_file_path.parent, exist_ok=True)
        
        # Create a rotating file handler
        file_handler = RotatingFileHandler(
            str(log_file_path),
            maxBytes=max_bytes,
            backupCount=backup_count
        )
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
    
    # Add console handler if requested or if no file handler
    if log_to_console or log_file_path is None:
        # Create a console handler
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)

    return logger
