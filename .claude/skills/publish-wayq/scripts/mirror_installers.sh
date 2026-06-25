#!/usr/bin/env bash
# Mirror production WayQ installers from way.net down to local Dropbox.
# Remote source: way:~/public_html/wayq/installers/
# Local target:  /e/Dropbox/www/way/public_html/wayq/installers/
# Production folder ONLY. Safe to re-run; idempotent.

set -euo pipefail

REMOTE_HOST="way"
REMOTE_PATH="~/public_html/wayq/installers/"
LOCAL_TARGET="/e/Dropbox/www/way/public_html/wayq/installers/"

trap 'echo "ERROR: mirror_installers.sh aborted at line $LINENO" >&2; exit 1' ERR

mkdir -p "$LOCAL_TARGET"

if command -v rsync &>/dev/null; then
    echo "Using rsync..."
    rsync -avz --delete --out-format="%o %n" \
        "${REMOTE_HOST}:${REMOTE_PATH}" \
        "${LOCAL_TARGET}"
    echo "Mirror complete."
else
    echo "rsync not found — falling back to ssh+scp+rm..."

    remote_files=$(ssh "${REMOTE_HOST}" \
        "find public_html/wayq/installers/ -maxdepth 1 -type f -printf '%f\n'" \
        | sort)

    local_files=$(find "${LOCAL_TARGET}" -maxdepth 1 -type f -printf '%f\n' 2>/dev/null \
        | sort || true)

    # Download every remote file (scp overwrites — always correct).
    while IFS= read -r fname; do
        [[ -z "$fname" ]] && continue
        echo "  sync: $fname"
        scp -q "${REMOTE_HOST}:public_html/wayq/installers/${fname}" \
            "${LOCAL_TARGET}${fname}" \
            || { echo "  WARNING: scp failed for $fname" >&2; continue; }
    done <<< "$remote_files"

    # Delete local-only files (keep the mirror exact).
    while IFS= read -r fname; do
        [[ -z "$fname" ]] && continue
        if ! grep -qxF "$fname" <<< "$remote_files"; then
            echo "  delete: $fname"
            rm "${LOCAL_TARGET}${fname}"
        fi
    done <<< "$local_files"

    echo "Mirror complete."
fi
