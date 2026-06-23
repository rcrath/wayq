# /sync-feedback — Pull wayq beta feedback JSONL → vault

Manually-invoked sync. Pulls `/home1/rcrath/wayq-data/feedback.jsonl` from way.net and writes one markdown note per submission into `E:\Dropbox\obsidian\wayq\feedback\`. Idempotent: existing notes are never overwritten so Rich's annotations (status field, inline notes) are preserved across runs. Regenerates the `00_feedback.md` MOC each run.

## Invocation

User types `/sync-feedback`. No arguments.

## What to do

1. Run `python .claude/skills/sync-feedback/sync.py` via the Bash tool.
2. Parse the script's stdout. Report to the user in 1–2 lines:
   - submissions seen on server
   - new entries created
   - existing skipped (annotation-preserving idempotency)
3. If any output went to stderr (lines starting with `WARN:` or any non-zero exit), surface it. Common failure modes:
   - SSH unreachable (HostGator dropping connections — wait + retry)
   - `feedback.jsonl` not found (no submissions yet, or someone moved it)
   - Vault folder unwritable (Dropbox sync conflict — pause Dropbox)

## Don'ts

- Don't manually edit existing notes during sync — the script handles it.
- Don't truncate or move the JSONL on the server. It's append-only and is the canonical store.
- Don't delete vault notes whose `ts` no longer appears on the server. Server is canonical for the JSON record, but the vault note may carry annotations Rich added — never destroy that work.
- Don't bump versions, commit, or push.
