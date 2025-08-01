#!/bin/bash

# Shell script wrapper for fix_manifest_paths.py
# Usage: ./fix_manifest_paths.sh [--backup] [--dry-run] [--verbose]

echo "ESP-IDF Manifest Path Fixer"
echo "==========================="
echo

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    if ! command -v python &> /dev/null; then
        echo "ERROR: Python is not installed or not in PATH"
        echo "Please install Python and try again"
        exit 1
    else
        PYTHON_CMD="python"
    fi
else
    PYTHON_CMD="python3"
fi

# Check if the script file exists
if [ ! -f "fix_manifest_paths.py" ]; then
    echo "ERROR: fix_manifest_paths.py not found in current directory"
    echo "Please run this script from the project root directory"
    exit 1
fi

# Make the Python script executable
chmod +x fix_manifest_paths.py

# Run the Python script with all arguments
$PYTHON_CMD fix_manifest_paths.py "$@"

# Check if the script ran successfully
if [ $? -ne 0 ]; then
    echo
    echo "ERROR: The script encountered an error"
    exit 1
fi

echo
echo "Script completed successfully!"
echo
echo "Next steps:"
echo "1. Run 'idf.py reconfigure' to verify the fix"
echo "2. Test your build with 'idf.py build'"
echo 