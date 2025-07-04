#!/usr/bin/env python3
"""
Launch script that starts the Arduino Connector daemon and UI.
This script first launches the daemon and then starts the UI after a 3-second delay.

Usage:
    python launch_daemon_with_ui.py [-p PROFILE_PATH]
"""

import subprocess
import sys
import time
import os
import argparse
import signal

# Get the absolute path to the project root directory
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)

def launch_daemon(profile_path=None):
    """Launch the Arduino Connector daemon"""
    daemon_script = os.path.join(project_root, "launch.py")
    
    if profile_path:
        cmd = [sys.executable, daemon_script, "-p", profile_path]
    else:
        cmd = [sys.executable, daemon_script]
    
    print(f"Starting Arduino Connector daemon...")
    process = subprocess.Popen(cmd)
    return process

def launch_ui():
    """Launch the Arduino Connector UI"""
    ui_script = os.path.join(script_dir, "api_client_ui.py")
    
    print(f"Starting Arduino Connector UI...")
    process = subprocess.Popen([sys.executable, ui_script])
    return process

def main():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Launch Arduino Connector daemon and UI")
    parser.add_argument("-p", "--profile", help="Path to Arduino profile YAML file")
    args = parser.parse_args()
    
    # Start the daemon
    daemon_process = launch_daemon(args.profile)
    
    # Set up signal handling to ensure child processes are terminated
    def signal_handler(sig, frame):
        print("Shutting down...")
        if daemon_process:
            daemon_process.terminate()
        if ui_process:
            ui_process.terminate()
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    # Wait for the daemon to initialize
    print("Waiting 3 seconds for daemon to initialize...")
    time.sleep(3)
    
    # Start the UI
    ui_process = launch_ui()
    
    # Keep the script running to maintain the child processes
    try:
        while True:
            time.sleep(1)
            
            # Check if either process has exited
            if daemon_process.poll() is not None:
                print("Daemon process has exited, shutting down...")
                if ui_process and ui_process.poll() is None:
                    ui_process.terminate()
                break
            
            if ui_process.poll() is not None:
                print("UI process has exited, daemon will continue running...")
                # Don't terminate the daemon, it can continue running without UI
                break
            
    except KeyboardInterrupt:
        print("Shutting down...")
        if daemon_process and daemon_process.poll() is None:
            daemon_process.terminate()
        if ui_process and ui_process.poll() is None:
            ui_process.terminate()

if __name__ == "__main__":
    main() 