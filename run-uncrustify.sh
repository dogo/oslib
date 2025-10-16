#!/bin/bash

# Script to run uncrustify while respecting .uncrustifyignore

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UNCRUSTIFY_CFG="${SCRIPT_DIR}/uncrustify.cfg"
IGNORE_FILE="${SCRIPT_DIR}/.uncrustifyignore"

# Find all .c and .h files in src/
files=$(find "${SCRIPT_DIR}/src" -type f \( -name "*.c" -o -name "*.h" \))

# Read ignore patterns
ignored_patterns=()
if [ -f "$IGNORE_FILE" ]; then
    while IFS= read -r line; do
        # Skip empty lines and comments
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        ignored_patterns+=("$line")
    done < "$IGNORE_FILE"
fi

# Filter files
filtered_files=()
for file in $files; do
    should_ignore=false

    for pattern in "${ignored_patterns[@]}"; do
        # Remove trailing slashes and convert to relative path
        rel_path="${file#"${SCRIPT_DIR}"/}"

        # Check if file matches pattern
        if [[ "$rel_path" == $pattern* ]] || [[ "$rel_path" == *"$pattern"* ]]; then
            should_ignore=true
            break
        fi
    done

    if [ "$should_ignore" = false ]; then
        filtered_files+=("$file")
    fi
done

# Run uncrustify on filtered files
if [ ${#filtered_files[@]} -gt 0 ]; then
    echo "Running uncrustify on ${#filtered_files[@]} files..."
    uncrustify -c "$UNCRUSTIFY_CFG" --no-backup --replace "${filtered_files[@]}"
    echo "Done!"
else
    echo "No files to process."
fi
