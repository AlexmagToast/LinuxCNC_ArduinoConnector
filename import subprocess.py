import subprocess

def run_command(command):
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout.decode()

def flash_firmware(port, board_type, ino_file):
    # Initialize the ino project
    run_command(["ino", "init"])

    # Copy the downloaded .ino file to the src directory
    with open('src/sketch.ino', 'wb') as dest_file:
        with open(ino_file, 'rb') as src_file:
            dest_file.write(src_file.read())

    # Define the type of Arduino board
    # Example: board_type = "uno" for Arduino Uno
    run_command(["ino", "build", "-m", board_type])
    run_command(["ino", "upload", "-p", port])

# Example usage

destination = "LinuxCNC_ArduinoConnector.ino"
port = "COM6"  # Replace with your Arduino's serial port
board_type = "generic 8266"  # Replace with your Arduino board type (e.g., "uno", "mega2560", etc.)

# Flash the firmware to the Arduino
flash_firmware(port, board_type, destination)