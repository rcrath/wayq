#!/bin/bash
# Runs after PKG install — strips quarantine from installed plugins
xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/WayQ.vst3" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/CLAP/WayQ.clap" 2>/dev/null || true
xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/WayQ.component" 2>/dev/null || true
exit 0
