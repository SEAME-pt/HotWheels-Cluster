#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define the repository path and program to run
REPO_PATH="/home/hotwheels/cluster"   # Change to your repository's path
PROGRAM="./my_project"              # Change to your program's name

echo "Starting script..."

# Navigate to the repository path
if [ -d "$REPO_PATH" ]; then
  echo "Navigating to repository at $REPO_PATH"
  cd "$REPO_PATH"
else
  echo "Error: Repository path $REPO_PATH does not exist."
  exit 1
fi

# Pull the latest changes from the repository
echo "Pulling latest changes from the repository..."
git pull
cd teste

# Run the program
if [ -x "$PROGRAM" ]; then
  echo "Running program $PROGRAM..."
  $PROGRAM
else
  echo "Error: Program $PROGRAM not found or is not executable."
  exit 1
fi

echo "Script completed successfully."
