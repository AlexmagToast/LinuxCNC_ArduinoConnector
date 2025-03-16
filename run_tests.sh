#!/bin/bash
# Run pytest tests

# Determine script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$SCRIPT_DIR"

# Create and activate a virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
else
    source venv/bin/activate
fi

# Run the tests
echo "Running tests..."
python -m pytest tests/ -v

# Generate coverage report (optional)
# python -m pytest tests/ --cov=linuxcnc_arduinoconnector --cov-report=html

# Deactivate virtual environment
deactivate 