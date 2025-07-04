#!/bin/bash

# ref: https://askubuntu.com/a/30157/8698
if ! [ $(id -u) = 0 ]; then
   echo "The script need to be run as root." >&2
   exit 1
fi

if [ $SUDO_USER ]; then
    real_user=$SUDO_USER
else
    real_user=$(whoami)
fi
#set -x #echo on

# Create a virtual environment
VENV_PATH="/opt/arduino-connector-venv"
echo "Creating Python virtual environment at $VENV_PATH"
python3 -m venv $VENV_PATH

# Install the package into the virtual environment
echo "Installing package into virtual environment"
$VENV_PATH/bin/pip install .

# Create executable script
sudo chmod +x launch.py
sudo tee /usr/bin/arduino-connector > /dev/null << EOF
#!/bin/bash
$VENV_PATH/bin/python $(pwd)/launch.py "\$@"
EOF
sudo chmod +x /usr/bin/arduino-connector

echo "Installation complete!"
echo "To test: execute 'hal_run' and then 'loadusr arduino-connector'"
#set +x #echo off

# Commands that you don't want running as root would be invoked
# with: sudo -u $real_user
# So they will be run as the user who invoked the sudo command
# Keep in mind if the user is using a root shell (they're logged in as root),
# then $real_user is actually root
# sudo -u $real_user non-root-command

# Commands that need to be ran with root would be invoked without sudo
# root-command