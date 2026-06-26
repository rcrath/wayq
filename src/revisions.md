# WayQ revision history

## 0.1.1-alpha (2026-06-25)

First alpha to actually publish. No audio or plugin changes since the 0.1.0-alpha build — re-cut to get a clean release all the way through CI.

- The 0.1.0 tag's build failed only on a missing macOS signing certificate secret; that is now set on the release repo.
- CLAP support now fetches via a full clone so the pinned clap-juce-extensions commit is always reachable on CI runners.
- Local build and the backup slots moved out of Dropbox into the sandbox, so Dropbox can't grab files mid-build.
- Added a pre-build backup + regression-check pass; backups live outside the repo and Dropbox.

## 0.1.0-alpha (2026-06-25)

First release. Two-mode filter: Pass (HP / Bell / LP) and Baxandall tilt (bass/treble see-saw with a phantom mid, yin/yang pivots) plus a bandpass. VST3 + CLAP, AU on macOS. Single response graph with draggable HP/LP/bell handles and SVG yin-yang tilt handles, drag tooltips, and a top panel with a 128-slot preset/bank manager, undo/redo, and bypass. Filter stage order fixed to match RhythmEcho; treble-tilt direction corrected so the outer edge follows the handle. Removed the carried-over unused row widgets and the dead route switch. Added the GitHub Actions release pipeline (all platforms to way.net installers), the publish-wayq skill, install helpers, a GPLv3 install agreement, and the way.net/wayq landing page.
