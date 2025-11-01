#!/usr/bin/env bash
set -euo pipefail

# Arguments from GitHub Actions matrix
OS="${1:-$(uname -s | tr '[:upper:]' '[:lower:]')}"
ARCH="${2:-$(uname -m)}"

RELEASE_TAG="v20251101"
URL="https://github.com/pspdev/pspdev/releases/download/${RELEASE_TAG}/pspdev-${OS}-${ARCH}.tar.gz"
FILE=$(basename "$URL")

# Download and extract PSPDEV toolchain
if [ ! -f "$FILE" ]; then
    echo "Downloading $URL..."
    wget "$URL"
fi

if [ ! -d "pspdev" ]; then
    echo "Extracting $FILE..."
    tar -xzf "$FILE"
fi

# Setup environment variables
PSPDEV_PATH="$PWD/pspdev"
export PSPDEV="$PSPDEV_PATH"
export PATH="$PSPDEV_PATH/bin:$PATH"

# For GitHub Actions, write to GITHUB_ENV and GITHUB_PATH
if [ -n "${GITHUB_ENV:-}" ]; then
    echo "PSPDEV=$PSPDEV_PATH" >> "$GITHUB_ENV"
    echo "$PSPDEV_PATH/bin" >> "$GITHUB_PATH"
fi

# macOS-specific paths (if brew is available)
if command -v brew >/dev/null 2>&1; then
    GNU_SED_PATH="$(brew --prefix gnu-sed)/libexec/gnubin"
    LIBTOOL_PATH="$(brew --prefix libtool)/libexec/gnubin"
    export PATH="$GNU_SED_PATH:$LIBTOOL_PATH:$PATH"

    if [ -n "${GITHUB_PATH:-}" ]; then
        echo "$GNU_SED_PATH" >> "$GITHUB_PATH"
        echo "$LIBTOOL_PATH" >> "$GITHUB_PATH"
    fi

    PKG_CONFIG_PATH="$(brew --prefix libarchive)/lib/pkgconfig:$(brew --prefix openssl)/lib/pkgconfig"
    export PKG_CONFIG_PATH

    if [ -n "${GITHUB_ENV:-}" ]; then
        echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" >> "$GITHUB_ENV"
    fi
fi

echo "PSPDEV setup complete: $PSPDEV_PATH"
echo "PSP toolchain ready to use."