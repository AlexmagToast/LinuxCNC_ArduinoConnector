import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path
import sys
import os

def setup_logger(
    logger_name: str,
    log_file_path: Path = None,
    max_bytes: int = 1000000,
    backup_count: int = 3,
    log_level: int = logging.DEBUG,
    log_format: str = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
) -> logging.Logger:
    """
    Set up a logger with either a rolling file handler or a console handler.
    
    :param logger_name: Name of the logger
    :param log_file_path: Path object pointing to the log file. If None, console logging is used
    :param max_bytes: Maximum size of each log file in bytes
    :param backup_count: Number of backup files to keep
    :param log_level: Logging level
    :param log_format: Format string for log messages
    :return: Configured logger object
    """
    # Create a logger
    logger = logging.getLogger(logger_name)
    logger.setLevel(log_level)

    # Create a formatting for the logs
    formatter = logging.Formatter(log_format)

    if log_file_path:
        # Ensure the directory exists
        #log_file_path.parent.mkdir(parents=True, exist_ok=True)
        #if not os.path.exists(log_file_path):
            
        
        # Create a rotating file handler
        handler = RotatingFileHandler(
            str(log_file_path),
            maxBytes=max_bytes,
            backupCount=backup_count
        )
    else:
        # Create a console handler
        handler = logging.StreamHandler(sys.stdout)

    handler.setFormatter(formatter)

    # Remove any existing handlers to avoid duplicate logging
    for existing_handler in logger.handlers[:]:
        logger.removeHandler(existing_handler)

    # Add the handler to the logger
    logger.addHandler(handler)

    return logger
