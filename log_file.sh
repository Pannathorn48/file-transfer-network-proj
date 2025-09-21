#!/bin/bash

# A script to log the content of all .c, .h, and Makefile files in a directory tree.
# It also echoes the file location and name before printing the contents.

# Check if a starting directory is provided, otherwise use the current directory.
START_DIR=${1:-.}

# Use 'find' to locate regular files with .c, .h, or Makefile extensions.
# The -o operator acts as a logical OR.
# The -exec option runs a command on each found file.
# The `sh -c` part is used to execute multiple commands (echo and cat) for each file.
find "$START_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "Makefile" \) -exec sh -c 'echo "--- File: {} ---" && cat "$0"' {} \;

# To run this script, save it and make it executable:
# chmod +x log_files_simple.sh
# ./log_files_simple.sh
#
# To specify a different directory:
# ./log_files_simple.sh /path/to/directory
