#!/bin/bash
# WayQ Linux installer
# Run from the folder containing linux/WayQ.vst3 and linux/WayQ.clap

DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_VST3="$DIR/linux/WayQ.vst3"
SRC_CLAP="$DIR/linux/WayQ.clap"
EULA_FILE="$DIR/linux/EULA.md"

if [ ! -d "$SRC_VST3" ]; then
    echo "Error: Cannot find linux/WayQ.vst3 next to this installer."
    exit 1
fi

if [ ! -f "$EULA_FILE" ]; then
    echo "Error: Cannot find EULA.md next to this installer."
    exit 1
fi

# Display license (EULA) and require acceptance
echo "=========================================="
echo "  WayQ License Agreement"
echo "=========================================="
echo ""
cat "$EULA_FILE"
echo ""
echo "=========================================="
echo ""
read -p "Do you accept these terms? (y/n): " ACCEPT
if [ "$ACCEPT" != "y" ] && [ "$ACCEPT" != "Y" ]; then
    echo "Installation cancelled."
    exit 0
fi
echo ""

# VST3 → ~/.vst3
VST3_DEST="$HOME/.vst3"
mkdir -p "$VST3_DEST"
rm -rf "$VST3_DEST/WayQ.vst3"
cp -R "$SRC_VST3" "$VST3_DEST/"
echo "Installed $VST3_DEST/WayQ.vst3"

# CLAP → ~/.clap (if present)
if [ -e "$SRC_CLAP" ]; then
    CLAP_DEST="$HOME/.clap"
    mkdir -p "$CLAP_DEST"
    rm -rf "$CLAP_DEST/WayQ.clap"
    cp -R "$SRC_CLAP" "$CLAP_DEST/"
    echo "Installed $CLAP_DEST/WayQ.clap"
fi

echo ""
echo "Restart your DAW to see WayQ."
