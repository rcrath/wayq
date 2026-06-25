---
name: from-rhythmecho
description: Track RhythmEcho issues tagged `2wayq` and analyze how each could be ported into WayQ (the two share the output-EQ module). For each tagged RhythmEcho issue, produces a plan-only port analysis and opens/updates a PUBLIC WayQ tracking issue, sanitized to the shared EQ only. Trigger when the user types `/from-rhythmecho` or asks any variant of "show me the rhythmecho issues for wayq" / "what's flagged from rhythmecho".
---

# /from-rhythmecho — RhythmEcho → WayQ port tracker

WayQ and RhythmEcho share the output-EQ code (the sealed band: `OutputFilter` / `BP` / `HpLp` / `Baxandall`, plus the `OutFilterRow` GUI), so a fix or feature done in RhythmEcho is often portable to WayQ. This skill watches RhythmEcho for issues the user has flagged for WayQ, works out how each could land, and files a PUBLIC WayQ tracking issue — sanitized to the shared EQ only.

## Repos and paths

- **Source — `rcrath/rhythmecho`** (PRIVATE, proprietary): read-only. Issues carrying the `2wayq` label are the input. Reads work via the user's local gh login (account rcrath owns the private repo). The owner applies that label by hand; this skill never creates or edits it.
- **Target — `rcrath/WayQ`** (PUBLIC): where port-tracking issues are created/updated.
- **Code:** local RhythmEcho checkout `E:/Dropbox/audio/git/rhythmecho/src`; WayQ `E:/Dropbox/audio/git/wayq/src`.
- **Staging dir:** `E:/Dropbox/audio/vst/wayq/claude/from-rhythmecho-<YYYY-MM-DD>/`.

## HARD RULES

- **Privacy (the central rule):** RhythmEcho is PRIVATE and proprietary; WayQ is PUBLIC. A WayQ tracking issue may contain ONLY the shared output-EQ change. NEVER copy RhythmEcho-only internals into a public issue — no delay/feedback routing, no delta/polarity math, no proprietary preset or UX specifics. Sanitize every analysis down to the EQ component (which is already public in WayQ) before it goes into a WayQ issue.
- **Read-only on RhythmEcho:** never post, comment, create an issue, or write anything to `rcrath/rhythmecho`, or to its code on disk. Read it only.
- **Plan-only:** no code is written, no version bumped, no build run, in either repo. The skill analyzes and files tracking issues; porting happens later, by the user, after approval.
- **First-run gate:** on any run, present what will be posted and WAIT for the user to say "go" before creating or updating any WayQ issue. Never auto-post.
- **No label creation on RhythmEcho.** If the `2wayq` label or the repo isn't reachable, say so and stop.
- **Pairing marker:** every WayQ tracking issue body ends with `<!-- rhythmecho-of: rhythmecho#<N> -->` so future runs find the pairing and never create duplicates. Never strip or alter it.

## Phase 1 — fetch flagged RhythmEcho issues

```bash
gh issue list --repo rcrath/rhythmecho --label 2wayq --state open \
  --json number,title,body,url,updatedAt
```

- If the repo or label isn't reachable (private-access failure, label missing), tell the user and stop.
- If zero issues, say "No RhythmEcho issues are tagged `2wayq`." and stop.

## Phase 2 — pair against existing WayQ tracking issues

Build the pairing index once:

```bash
gh issue list --repo rcrath/WayQ --state all --limit 500 \
  --json number,title,body,state
```

In code, extract `rhythmecho#<N>` from each body via `<!-- rhythmecho-of: rhythmecho#(\d+) -->` and index by N.

- **No match for rhythmecho#N** → NEW: create a tracking issue (Phase 4).
- **Match** → EXISTING: only refresh if the RhythmEcho issue's `updatedAt` is newer than the last analysis (compare to a `<!-- rhythmecho-synced: <ISO8601> -->` marker in the WayQ issue body). Otherwise skip.

## Phase 3 — port analysis (plan-only, parallel)

Create the staging dir. For each RhythmEcho issue to process (new + stale-existing), spawn one plan-only `Agent` (`subagent_type: "general-purpose"`, model `sonnet`, opus for cross-cutting/ambiguous ones), in parallel (single message, multiple Agent calls). Each agent prompt is self-contained and:

1. Reads the RhythmEcho issue title/body (passed inline) and the local RhythmEcho source at `E:/Dropbox/audio/git/rhythmecho/src` to see how RhythmEcho frames or solves it.
2. Reads the matching WayQ code at `E:/Dropbox/audio/git/wayq/src`, focused on the shared EQ band and the `OutFilterRow` GUI.
3. Classifies the port path as exactly one of:
   - **DIRECT PORT** — lives in the shared EQ band; drops into WayQ with little or no adaptation.
   - **ADAPT** — needs WayQ-specific rework (GUI/look-and-feel differences, parameter-ID constraints, WayQ signal-flow differences).
   - **SKIP** — RhythmEcho-only; not applicable to WayQ (explain why, without disclosing proprietary internals).
4. Gives a recommendation, file/line pointers in both repos, a rough effort tier, and risks — especially anything that could change WayQ's sound or break preset compatibility (APVTS parameter IDs are immutable).
5. Writes the analysis to `<STAGING_DIR>/rhythmecho-<N>.md`, **sanitized to the shared EQ only**, and returns it.

Plan-only: agents read freely but write nothing to either codebase and post nothing to RhythmEcho.

## Phase 4 — create/update PUBLIC WayQ tracking issues (gated)

Present the planned issues to the user first. After the user says "go":

- **NEW** — create in `rcrath/WayQ`:
  - Title: `[RE#<N>] <rhythmecho title>`  (sanitize the title too — drop any proprietary terms)
  - Body: the sanitized port analysis, then a blank line, then the two markers on their own lines:
    ```
    <!-- rhythmecho-synced: <ISO8601 of the RhythmEcho issue updatedAt> -->
    <!-- rhythmecho-of: rhythmecho#<N> -->
    ```
  - Label `from-rhythmecho` (create it in `rcrath/WayQ` first if missing).
  - Command: `gh issue create --repo rcrath/WayQ --title "<title>" --body-file <tmpfile> --label from-rhythmecho`
  - Capture and report the new issue URL.
- **EXISTING + stale** — post a comment with the refreshed sanitized analysis and update the `rhythmecho-synced` marker (edit the body). One comment per run.

## Phase 5 — report and hand off

Print a table: RE# / title / recommendation (DIRECT PORT / ADAPT / SKIP) / WayQ tracking issue link. Point the user at the local staging dir for full analyses. The user approves and implements the ones they want, later.

## Access note

Interactive runs rely on the user's local gh login (which already reads private `rcrath/rhythmecho`) and the local RhythmEcho checkout for code. For headless/CI runs, a fine-grained PAT with read access to `rcrath/rhythmecho` issues + contents would be required.

## Don'ts

- Don't write to RhythmEcho in any way. Read-only.
- Don't leak RhythmEcho internals into a public WayQ issue — EQ component only.
- Don't change code, bump versions, or build.
- Don't create duplicate tracking issues — always pair via the `rhythmecho-of` marker first.
- Don't post anything before the user says "go".
