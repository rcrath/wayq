---
name: wayq-regress
description: "Check all GitHub wayq issues tagged regress_meta against changes since the last successful build. Spawns parallel haiku agents — one per issue — and reports Clear or Risk in plain audio terms. Run standalone via /wayq-regress or automatically from /qbuild."
---

# /wayq-regress — regression issue check against last successful build

## Trigger

- `/wayq-regress`
- Called by `/qbuild`

## Behavior

**Backup location:** the backup root is `C:\Users\rich\sandbox\backups\wayq\` (outside the repo). Every `backup/…` (and bare `baseline/…`) path below means that root — e.g. `backup/baseline/src/` = `C:\Users\rich\sandbox\backups\wayq\baseline\src\`.

1. List dated subfolders of `backup/` (exclude `baseline/` and `_recycled/`) sorted by name. If none exist, print "No prior build to diff against — skipping regression check." and stop.

2. Take the newest dated subfolder (NEWEST). Use the second newest (SECOND_NEWEST) as the comparison base if it exists; otherwise use `backup/baseline/src/`.

3. Compute the diff: `diff -r backup/NEWEST/src <comparison-base>`

4. Fetch ALL issues in `rcrath/wayq` with the `regress_meta` label (open and closed):
   ```
   gh issue list --repo rcrath/wayq --label regress_meta --state all --limit 100 --json number,title,body
   ```

5. If zero issues are returned: say "No regress_meta issues found." and stop.

6. For each issue, spawn a parallel haiku Agent (model: "haiku") — all in ONE message as multiple Agent tool calls. Each agent receives the issue number, title, body, and the full diff. Each agent returns exactly one of:
   - `Clear` — the diff does not touch code related to this regression
   - `Risk: <plain audio explanation of the connection>`

7. Collect all results. Print the full report to the user, prefixed with the build baseline used (e.g. "Diffing against build from 06/11/2026 at 04:20 PM"):

   **If any Risk items exist**, list them first under a `**RISKS:**` header, then all Clear items below a `---` separator:

   ```
   **RISKS:**

   #N "title" — Risk: <explanation>
   #M "title" — Risk: <explanation>

   ---

   #P "title" — Clear
   #Q "title" — Clear
   ```

   **If all Clear**, print one line:

   ```
   All clear — #N, #M, #P all Clear.
   ```

8. Never omit the report. Always print it before returning to the caller (e.g. /qbuild).

## Hard rules

- Label is `regress_meta`. Never search for `regress_check`.
- Diff baseline is the second most recent dated slot, or backup/baseline/src/ if only one dated slot exists. Never diff against baseline/ or _recycled/ as NEWEST. Backup retention is managed by /qbuild.
- Use model name `"haiku"` (not a versioned ID) for all sub-agents.
- loking specifically for diffs that break. do not do a general code review. just code that breaks things.  
- Explain every Risk in plain audio terms — name the behavior (e.g. "delay time jumps on preset load"), not the code.
- Never suggest closing any issue.
- Never block the build — report only.
