---
name: qbuild
description: "Pre-build prompt for wayq: optionally runs /regress, asks whether you will build and test, recommends build type based on changes since last build, records the timestamp + build type when you confirm, then captures the build result (success/fail) from the user's next message."
---

# /qbuild — pre-build prompt

## Trigger

Invoke this skill when the user indicates they are about to build. Trigger phrases include:
- `/qbuild`
- "I'm going to build"
- "going to build"
- "about to build"
- "ready to build"
- "I'll build now"
- "building now"
- "time to build"
- "let me build"
- "gonna build"

## Behavior

**First**, before asking any questions, create a new backup slot at backup/yymmdd-hhmm/. For each file in current src/, search existing backup slots newest-to-oldest, then fall back to backup/baseline/src/, to find the last backed-up version of that file. Copy the file into the new slot's src/ subfolder only if it is not found in any prior slot or baseline (new file) or its content differs (modified file). After the copy loop completes, if zero files were copied into the slot's src/ subfolder, delete the slot folder entirely (rm -rf backup/yymmdd-hhmm/) and print "No files changed — backup slot skipped." Do not write deleteme.md or run the retention/recycling logic for that run. If at least one file was copied, proceed as normal: write backup/yymmdd-hhmm/deleteme.md listing any files present in the most recent prior slot's src/ that no longer exist in current src/. Keep up to 5 backup subfolders (sorted by name). When the count exceeds 5, before recycling: merge the oldest slot's files into baseline/src/ (copy each file from the oldest slot into baseline/src/, overwriting), then delete the oldest slot folder entirely. Never recycle or delete backup/baseline/.

When triggered, ask the user ONE question — no preamble, no summary:

> Should I run the regression check before you build?

**If yes (or any affirmative):**

Invoke `/regress` via the Skill tool with `skill="regress"`. After it completes, ask:

> Will you build and test now?

**If yes:** → go to **Build Type Analysis** below.

**If no:** Print exactly: `Not built yet.` Do NOT touch `last-build.txt`. Stop.

**If no (or any negative):**

Skip the regression check. Ask:

> Will you build and test now?

**If yes:** → go to **Build Type Analysis** below.

**If no:** Print exactly: `Not built yet.` Do NOT touch `last-build.txt`. Stop.

---

## Build Type Analysis

If /regress ran, use the diff it already computed. If /regress was skipped, compute the diff now: list dated backup subfolders sorted by name, take the newest. Diff it against the second newest if one exists, otherwise diff it against backup/baseline/src/. If no dated subfolders exist at all, skip build-type analysis and print `Recommend Quick Build — no prior backup to diff against.`

Apply these rules in order (first match wins):

1. **Full** — if any of these changed: `CMakeLists.txt`, any file under `cmake/`, any `.cmake` file, or any JUCE module source outside `src/`.
2. **Soft Rebuild** — if any new `.cpp` or `.h` file appears as an entirely new file in the backup diff, or if a `.h` file that is included by many other files changed (e.g. `PluginProcessor.h`, `PluginEditor.h`).
3. **Quick Build** — only existing `.cpp` or `.h` files under `src/` changed, no structural changes.

Print one line stating the recommendation and the reason in plain terms. Example:
- `Recommend Full — CMakeLists.txt changed.`
- `Recommend Soft Rebuild — PluginProcessor.h changed (included widely).`
- `Recommend Quick Build — only src/*.cpp files changed.`

Then run Bash `date +"%m/%d/%Y at %I:%M %p"` to get the current timestamp. Write `<timestamp> — <build type>` to `.claude/skills/qbuild/last-build.txt`, overwriting any previous content. Print exactly: `Build ready on <timestamp> — <build type>.`

Do NOT stop. Stay in the qbuild flow and wait for the user's next message — it is the build result.

---

## Build Result

Interpret the user's next message after "Build ready..." as the build outcome:

**Fail** — any of:
- User types `fail` (alone or in a sentence)
- User pastes an error message (compiler errors, linker errors, assertion failures, etc.)
- User directs you to `error.log` or another log file

On fail:
1. Rewrite `.claude/skills/qbuild/last-build.txt` to: `<timestamp> — <build type> — FAIL`
2. Immediately go into error-fixing mode — read `error.log` if directed, or parse the pasted error. Start addressing the problem. Do not ask the user what to do; act.

**Success** — anything else (a new task, a command, a question, bare "ok", silence followed by a new topic, etc.)

On success:
1. Rewrite `.claude/skills/qbuild/last-build.txt` to: `<timestamp> — <build type> — success`
2. Close the qbuild skill (stop treating this as a qbuild session).
3. Handle the user's message as a new, normal request.

---

## Hard rules

- Never block the build. This skill only asks and reports; the user decides.
- Never suggest closing any regression issue.
