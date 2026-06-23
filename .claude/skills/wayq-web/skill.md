# /wayq-web — Load context for working on the wayq website

When Rich is about to ask you to make changes to the wayq website (landing page, beta form, thank-you pages, CSS, copy edits), this skill primes you with everything you need before he gives the next instruction.

## Invocation

User types `/wayq-web`. No arguments.

## What to do

1. Read `C:\Users\rich\.claude\projects\C--Users-rich\memory\project_wayq_web.md` for the full canonical-paths + workflows + current-palette context. Trust this as the entry point — it points to the other relevant memories (CSS sync, COPY sync, ADA, feedback pipeline).
2. Verify SSH alias `way` is reachable with a short ping:
   ```
   ssh -o BatchMode=yes -o ConnectTimeout=10 way "echo OK; date -u +%FT%TZ"
   ```
   If it fails, tell Rich SSH is down so he knows deploy won't work this session yet (HostGator drops occasionally).
3. Run `/sync-feedback` to surface any new beta-feedback submissions Rich may want to triage as part of this session. Report new + total counts.
4. Confirm ready in one short paragraph:
   - confirm SSH state
   - confirm feedback sync result
   - list the four pages you can edit
   - wait for Rich's task

Do NOT modify anything yet. This skill loads context only.

## JS change verification (feedback.html)

Any time feedback.html JavaScript is modified — timer, auto-save, submit handler, clear handler, or anything that touches `localStorage` — run this checklist before the scp goes out:

1. **Grep all handlers touching the affected variables.** For every key or variable the change touches (e.g. `STORE_KEY`, `TIMER_KEY`, `rhTypingTotal`), grep the full script block for every reference: event listeners, removeItem, setItem, getItem, value stamps. List them before editing.
2. **Trace the submit → return → resubmit path.** Confirm that after a successful form submission:
   - the draft (`STORE_KEY`) is NOT cleared — user returns to find their text and checkboxes intact
   - the timer (`TIMER_KEY`) is NOT cleared — accumulated time carries forward
   - only the explicit "Clear" button wipes the draft
3. **Trace the page-reload path.** Confirm that reloading without submitting restores draft and timer correctly.
4. **Confirm no unintended side effects** on the clear-button handler, the save-button handler, and the send-button handler.

If any step fails the trace, fix it before deploying. Do not scp until all four pass — and always show Rich the trace results and get explicit approval before running scp on feedback.html JS changes.

## Don'ts

- Don't read every file under `obsidian/wayq/www/` proactively — `project_wayq_web.md` already says what lives where. Read specific files when Rich's task names them.
- Don't run `convert.py` (html → md). That direction is destructive of Rich's vault edits. The convention runs `reverse.py` (md → html) for deploys.
- Don't bump versions, commit, push, or send mail.
- Don't propose a redesign. Edit-as-requested only.
