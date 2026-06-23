---
name: wayq-issues
description: "Pull open issues from rcrath/wayq (private — all technical iteration lives here), filter by milestone via a numbered menu, then spawn a background orchestrator that triages each issue to an appropriately-modeled sub-agent for PLAN-ONLY analysis. Plans are written locally AND posted as comments on the originating private issue. The user responds on the issue and re-runs the skill iteratively. Sub-agents never write code; they produce plans you approve before any implementation. For any wayq issue carrying the `mirror-public` label, a stub mirror is created (or kept in sync) in the public rcrath/wayq-issues repo with title/body/whitelisted-labels/milestone only — no technical chatter leaks public. Trigger when the user types `/wayq-issues` or asks any variant of 'show me the open issues' / 'pull the wayq issues' / 'work on milestone X issues'."
license: MIT
version: "1.1.0"
user_invocable: true
---

# /wayq-issues — Milestone-filtered issue triage with plan-first spawn, iterative via GitHub

**Dual-repo architecture:**
- **`rcrath/wayq`** — PRIVATE. The code repo, also the issue tracker for all technical work. All issue iteration, planning, comments, and decisions happen here. This is the source of truth for the skill.
- **`rcrath/wayq-issues`** — PUBLIC. A thin stub mirror. Only issues in `wayq` carrying the `mirror-public` label get a stub mirror created here, containing title + body + whitelisted labels (those prefixed `public:`) + milestone + assignees. No claude-plan comments, no code, no decisions. When the wayq issue closes, the public mirror auto-closes with a generic resolution message.

Pulls open issues from `rcrath/wayq`, filters by milestone, and spawns a background orchestrator that fans out plan-only sub-agents (one per issue). Plans are written locally AND posted as comments on the originating private issue. The user responds to open questions on the issue; on the next `/wayq-issues` run, sub-agents read the full comment history and produce revised plans.

**No sub-agent ever edits code.** They analyze and propose. The user approves plans, and only then are coding agents spawned (via `/bga`-style flow).

## HARD RULE — ask before touching code

Planning phases (milestone selection, fetch, classify, mirror sync, orchestrator, plan writing, posting issue comments) auto-proceed. Read freely, write planning docs freely, post issue comments freely — these are reversible and don't touch the codebase.

After Phase 1, briefly confirm the chosen milestone if the menu was used. After Phase 2, report the classification summary as the run proceeds — don't block.

After Phase 4 (plans ready), STOP. Ask which issues to implement. **Never spawn an implementation agent (Phase 5) without an explicit per-issue yes.** Code edits are the only gate.

**Comment marker convention:** every Claude-posted issue comment MUST start with the literal HTML-comment marker `<!-- claude-plan -->` on its first line, followed by a blank line, then the plan. This marker is how future runs identify their own comments to compute reply state.

## Phase 1 — Milestone menu

1. Fetch all open milestones:

   ```bash
   gh api repos/rcrath/wayq/milestones?state=open --jq '.[] | "\(.number)\t\(.title)\t\(.open_issues)"'
   ```

2. Offer an "all open issues, no milestone filter" option as a fallback.

3. Display as a numbered text menu. Use `AskUserQuestion` for 2–4 milestones; for more, render as plain text and wait for the user's reply.

4. On selection, proceed to Phase 2 with the chosen milestone title (or "all").

## Phase 2 — Fetch issues + classify by comment-reply state

1. Fetch issues:

   ```bash
   gh issue list --repo rcrath/wayq \
     --milestone "<chosen-title>" \
     --state open \
     --limit 50 \
     --json number,title,body,labels,milestone,createdAt
   ```

   (Drop `--milestone` if user picked "all".)

2. For EACH fetched issue, fetch its comments:

   ```bash
   gh api repos/rcrath/wayq/issues/<N>/comments \
     --jq '[.[] | {id, user: .user.login, created_at, body}]'
   ```

   Classify each issue into one of six states by inspecting the comment list:

   - **fresh** — no comment whose body starts with `<!-- claude-plan -->` exists. Sub-agent will produce an initial plan. **Proceeds to orchestrator.**
   - **user-replied** — at least one Claude-marker comment exists AND the most recent comment in the issue does NOT start with `<!-- claude-plan -->` (i.e. the user has posted after Claude's last plan). Sub-agent will read the history and produce a revised plan. **Proceeds to orchestrator.**
   - **awaiting-clarification** — last comment is a Claude-marker comment whose verdict line contains `**NEEDS CLARIFICATION`. **Skip this run** — don't re-pester before the user replies on GitHub.
   - **ready-to-implement** — last comment is a Claude-marker comment whose verdict line contains `**PLAN READY`. **Skip planning this run** — but surface separately in Phase 4 so the user can choose to spawn a coding agent (Phase 5).
   - **implemented** — last comment is a Claude-marker comment whose verdict line contains `**IMPLEMENTED`. **Skip this run** — surface separately in Phase 4 with a recommendation to close the GitHub issue (or that the user verify with a build and close it).
   - **closed-recommendation** — last comment is a Claude-marker comment whose verdict line contains `**WONT FIX` or `**DUPLICATE OF`. **Skip this run** — surface separately in Phase 4 with a recommendation to close.

   Verdict parsing: examine the body of the last Claude-marker comment. The chosen verdict is always written as `**KEYWORD` (markdown bold prefix) in the `## Verdict` section. Match the first of the keywords above that appears in the body — order matters because `**IMPLEMENTED` should win over `**PLAN READY` if both happen to appear (e.g. a Revision N implementation note that quotes the original verdict).

## Cleanup pass — issues you closed since the last touch

After classifying (step 2 above) and BEFORE posting the run summary, scan for closed issues that were previously on Claude's pending list.

**Fetch closed issues in scope:**

```bash
gh issue list --repo rcrath/wayq \
  --milestone "<chosen-title>" \
  --state closed \
  --limit 50 \
  --json number,title,closedAt
```

(Drop `--milestone` if user picked "all". The `--limit 50` cap keeps this fast — only recent closures matter.)

For each closed issue, fetch its comments (same `gh api` call as step 2). Check whether the **most recent claude-marker comment** (`<!-- claude-plan -->` on line 1) carries a pending-class verdict — any of:

- `**NEEDS CLARIFICATION`
- `**PLAN READY`
- `**IMPLEMENTED`
- `**WONT FIX`
- `**DUPLICATE OF #M`

Skip closed issues with NO claude-marker comments — Claude never looked at them, nothing to clear.

**Two output shapes:**

- **Nothing to clear:** stay silent. Proceed directly to step 3 (the regular Phase 2 summary).
- **One or more cleared:** post a separate message — only the cleanup result, NOT mixed with the open-issues summary.

  One item:
  > Clearing #N "title" — was on the pending list as \<verdict\>. You closed it; removing from this run's scope.

  Multiple items:
  > Clearing N closed-with-pending-verdict issues from this run's scope:
  > | # | title | prior verdict |
  > |---|-------|--------------|
  > | … | … | … |

  Then ask: "Say go / proceed / go ahead when ready and I'll post the open-issues summary."

  Wait for a go-variant ("go", "proceed", "go ahead", "ok", "yes", or similar) before continuing to step 3 or proceeding to mirror sync / orchestrator.

3. Report a brief summary to the user, e.g.:

   > "5 open issues in milestone `v0.9.8-beta`: #37, #38, #40, #43, #44.
   >
   > Re-planning (2):
   >   #43 user-replied
   >   #44 user-replied
   >
   > Skipped — awaiting your response on GitHub (1):
   >   #38 NEEDS CLARIFICATION
   >
   > Skipped — ready to implement (1):
   >   #37 PLAN READY — say the word and I'll spawn a coding agent
   >
   > Skipped — already implemented (1):
   >   #40 IMPLEMENTED — close the GitHub issue when you've built/verified
   >
   > Launching orchestrator on the 2 re-planning items."

   Then **stop and ask the user**: "Ready to run the mirror sync and start planning these N issues? (yes / no / pick specific issue numbers)"

   Wait for the user's answer before proceeding. If the user says no or picks specific issues, scope the rest of the run accordingly. Do not proceed to mirror sync or orchestrator until the user explicitly confirms.

## Public Mirror Transfer — sync new and updated issues to rcrath/wayq-issues

Runs after fetch+classify, before orchestrator dispatch. Only touches wayq issues carrying the `mirror-public` label.

### Conventions

- **Trigger label** (on the wayq issue): `mirror-public`. Without it, no mirror is created or touched.
- **Label whitelist** (which labels propagate from wayq → public mirror): only labels whose name starts with `public:`. Everything else (including `mirror-public` itself, `bug`, `enhancement`, internal labels) stays private. Default behavior is "do not share."
- **Pairing marker**: each public-mirror issue body ends with an HTML-comment line: `<!-- mirror-of: wayq#<N> -->`. The skill uses this to find existing mirrors.
- **Body SHA marker**: each public-mirror issue body also contains `<!-- private-body-sha: <hex> -->` on the line immediately before the pairing marker. This is a SHA-256 of the private wayq body at the time of last sync, used for drift detection.
- **Closing message** (auto-posted on the public mirror when its wayq counterpart closes): `Resolved. See release notes for the version this lands in.`

### Simplification sub-agent

When creating or updating a public mirror body, the raw private issue body is NEVER written verbatim. Instead, spawn a haiku sub-agent with this prompt:

---
You are a simplification agent for end-user-facing release notes.

Input:
  Title: <issue title>
  Body: <private wayq issue body>

Output a SHORT block in exactly this format — no preamble, no explanation:

**Problem:** <one or two sentences: what the user experiences, in GUI/manual terms only>
**Solution:** <one or two sentences: what changed, in GUI/manual terms only>

Do not mention internal code names, implementation details, or the private repository.

---

Model: haiku. Short I/O — no codebase reading needed.

If the sub-agent spawn fails, fall back to a direct API call: `POST /v1/messages` with haiku and the same prompt. Whichever path succeeds is the only one kept in the skill — remove the unused branch after the first successful run.

### Sweep — iterate over EVERY wayq issue with the `mirror-public` label, regardless of state

```bash
gh issue list --repo rcrath/wayq --label mirror-public --state all --limit 100 \
  --json number,title,body,state,labels,milestone,assignees,closedAt
```

For each issue:

1. **Find or create the mirror in `rcrath/wayq-issues`.** Build a lookup index by listing all public issues once and parsing their bodies for the marker:
   ```bash
   gh issue list --repo rcrath/wayq-issues --state all --limit 500 \
     --json number,title,body,state,labels,milestone,closedAt
   ```
   Then in code, iterate the result and extract `wayq#<N>` from each body via `<!-- mirror-of: wayq#(\d+) -->`. Index by that N. This is **strictly more reliable than `gh search issues`** — the search command's qualifier parsing is brittle (it tends to wrap multi-token queries weirdly, producing "Invalid search query" or false-empty results), AND search-index propagation has a ~30 minute delay, so freshly-created mirrors won't appear via search even though they're listable via the issues list. The list-and-filter approach has neither problem.
   - **No match in the index for wayq#<N>** → CREATE the mirror (skip if the wayq issue is already closed; no point creating a stub for a resolved issue).
   - **Match** → reconcile drift (next step).

2. **Mirror creation (when missing AND wayq issue is OPEN):**
   - Compose body: spawn the simplification sub-agent (see above) with the private issue title and body. Compute SHA-256 (hex) of the private body. The public mirror body becomes:
     ```
     <simplified output from sub-agent>

     <!-- private-body-sha: <hex> -->
     <!-- mirror-of: wayq#<N> -->
     ```
   - Whitelist labels: keep only those starting with `public:`. If the public repo doesn't have one of these labels yet, create it first (`gh label create "<name>" --repo rcrath/wayq-issues`).
   - Milestone: look up by title in the public repo. If absent, create it (copy title + description from the wayq-side milestone).
   - Assignees: copy them (will fail silently for users without public-repo access; that's fine).
   - Command:
     ```bash
     gh issue create --repo rcrath/wayq-issues \
       --title "<title>" --body-file <tmpfile> \
       --milestone "<title>" --label "<csv>" --assignee "<csv>"
     ```
   - Capture the new mirror URL and print it to the user as part of the run summary.

3. **Mirror reconciliation (when match exists):**
   - If wayq title differs from mirror title → `gh issue edit <mirror-N> --repo rcrath/wayq-issues --title "<new>"`.
   - **Body drift (SHA-based):** Extract the `<!-- private-body-sha: <hex> -->` line from the current public mirror body. Compute SHA-256 of the current private wayq body. If the SHA differs (or no SHA line exists) → re-run the simplification sub-agent on the new private body → update the public mirror body with the new simplified text + updated SHA comment + pairing marker. If the SHA matches → skip body update.
   - If whitelisted labels differ → add/remove on the mirror to match.
   - If milestone differs → update.

4. **Auto-close orphaned mirrors:**
   - If the wayq issue is CLOSED and the mirror is still OPEN:
     ```bash
     gh issue comment <mirror-N> --repo rcrath/wayq-issues --body "Resolved. See release notes for the version this lands in."
     gh issue close <mirror-N> --repo rcrath/wayq-issues
     ```
   - The closing comment is posted FIRST, then the close.

5. **Report what happened.** A tight summary line per affected mirror:
   - `created mirror wayq-issues#<M> for wayq#<N>`
   - `synced mirror wayq-issues#<M> (title/body/labels updated)`
   - `closed mirror wayq-issues#<M> for resolved wayq#<N>`
   - (no output if nothing changed for that issue)

### Hard rules for mirror sync

- NEVER copy claude-plan comments, NEVER copy any private discussion to the public mirror.
- NEVER mention the private repo by name in any public mirror artifact (title, body, comment). The mirror should read as a standalone public issue.
- NEVER strip the `<!-- mirror-of: wayq#<N> -->` marker from a public mirror body — it's how future runs find the pairing.
- Whitelist is strict: only `public:` prefixed labels propagate. Other labels — `mirror-public`, `claude-plan`, `bug`, `enhancement`, anything else — do NOT go to the public mirror.

## Phase 3 — Create staging dir + spawn orchestrator

1. Staging path: `claude/<milestone-slug-or-all>-<YYYY-MM-DD>/`

   Slug: lowercase the milestone title, replace whitespace with `-`, strip special chars. For "all", use `all`.

2. `mkdir -p` the staging dir.

3. Write two files into the staging dir:
   - `issues.json` — only the `fresh` + `user-replied` items the orchestrator will plan. Each entry includes `number`, `title`, `body`, `labels`, `milestone`, `createdAt`, `_state`, and `_comments` (full history).
   - `skipped.json` — every skipped item (`awaiting-clarification`, `ready-to-implement`, `implemented`, `closed-recommendation`) with `number`, `title`, `_state`, plus the parsed verdict, the GitHub URL, and the complexity tier from the plan if extractable. The orchestrator does NOT read this file — it's for Phase 4's unified report.

4. SPAWN ONE orchestrator agent via the `Agent` tool:
   - `subagent_type: "general-purpose"`
   - `model: "sonnet"` (opus only if >10 issues or unusually complex bodies)
   - `run_in_background: true`
   - `description: "wayq-issues orchestrator (milestone: <chosen>)"`
   - `prompt:` use the ORCHESTRATOR PROMPT TEMPLATE below.

## ORCHESTRATOR PROMPT TEMPLATE

```
You are the orchestrator for a plan-only issue-triage run on the wayq VST3 plugin codebase.

You will SPAWN sub-agents DIRECTLY via the `Agent` tool with `subagent_type: "general-purpose"`. That is the ONLY delegation mechanism here. Do not invoke any other skill — including any worker-CLI or checklist-spawning skill — to delegate this work. The `Agent` tool is sufficient and is the entire mechanism.

Repo (code, where any future implementation would land):
  . (the wayq repo root)

Issues repo:
  rcrath/wayq

Staging dir for this run:
  <STAGING_DIR>

Issue manifest (read this first):
  <STAGING_DIR>/issues.json — contains: milestone-filtered open issues that are NOT in awaiting-response state, plus per-issue comment history and state classification (fresh / user-replied). Issues that were in awaiting-response state were already filtered out by the calling Claude.

YOUR JOB:
1. Read issues.json and ~/.claude/projects/E--Dropbox-audio-git-wayq/memory/MEMORY.md.
2. For EACH issue in issues.json, decide:
   a. Kind of work (bug fix / small feature / refactor / docs / architectural / triage-only).
   b. Smallest model that can produce a thorough plan:
      - haiku: typo fixes, doc tweaks, one-line bugs, label/title cleanup
      - sonnet (default): focused implementations, contained bug fixes, single-file refactors
      - opus: cross-cutting refactors, architectural decisions, ambiguous bug reports needing deep investigation
3. SPAWN ONE sub-agent per issue in PARALLEL via the `Agent` tool (single message, multiple Agent tool calls):
   - subagent_type: "general-purpose"
   - model: <your per-issue choice>
   - run_in_background: false (you wait synchronously for each, in parallel)
   - description: "Plan for issue #<N>: <short>"
   - prompt: use the SUB-AGENT PROMPT TEMPLATE below. INCLUDE the issue's full comment history inline in the prompt (so the sub-agent doesn't have to re-fetch).
4. Wait for all sub-agents to complete.
5. Verify each sub-agent wrote plan-<N>.md to <STAGING_DIR>/ AND posted a comment to its GitHub issue.
6. Write <STAGING_DIR>/SUMMARY.md listing each issue with: number, title, state (fresh / user-replied), model used, verdict, one-paragraph plan summary, link to local plan file, link to GitHub issue URL.
7. Return a short status (≤200 words): plans completed, any sub-agent that failed, path to SUMMARY.md.

CRITICAL CONSTRAINTS:
- NO sub-agent edits, writes, or modifies any code file. Plan-only.
- NO commits, no builds, no git mutations on the source repo.
- Sub-agents may READ from git/wayq/ freely.
- Sub-agents MUST be able to Write to <STAGING_DIR>/ AND post one comment per issue via `gh issue comment`.
- Use the `Agent` tool directly for all delegation; do not route through any other skill.

SUB-AGENT PROMPT TEMPLATE:
"""
Issue #<N> from rcrath/wayq. Milestone: <milestone>.

State: <fresh | user-replied>

Title: <title>

Body:
<body>

Labels: <labels>

Comment history (oldest first, most recent last). Each entry: user, created_at, body. Claude-posted comments start with `<!-- claude-plan -->` on line 1:
<comment-history-json>

Repository: ./ (the wayq repo root) — explore freely, read any source file, but do NOT edit, write, or modify anything.

Do not invoke any worker-CLI or checklist-spawning skill from inside this sub-agent. Read the codebase, write the plan, post the comment, return DONE.

TASK:
1. Read source code as needed to understand the issue.
2. Read the comment history above before drafting your plan, regardless of state. ANY comments — whether claude-marker or user-authored, whether pre- or post-marker — contain context that informs your plan. The user may have posted answers, clarifications, or context directly on the issue at any point, including before the first Claude plan was ever posted.
3. If state is "user-replied":
   - Your prior Claude-marker comment(s) contain the plan(s) you produced last time; the user's subsequent comment(s) are their answers to your open questions.
   - Produce a REVISED plan that incorporates the user's answers. DO NOT ask the same questions again. If the user introduced new questions, those are now the open items.
4. If state is "fresh":
   - Produce an initial plan. IF the issue has any pre-marker user comments (the user wrote on the issue before any Claude plan existed), treat those as context/answers and incorporate them — do NOT ask questions the user has already answered there.
4. Write your plan to: <STAGING_DIR>/plan-<N>.md
   - The file MUST start with the marker line on its very first line:
     <!-- claude-plan -->
   - Followed by a blank line, then the plan body.
5. Post the SAME plan as a comment on the issue:
     gh issue comment <N> --repo rcrath/wayq --body-file <STAGING_DIR>/plan-<N>.md

Plan body format (after the marker line + blank line):

  # Issue #<N>: <title>
  _Revision <K> — count existing claude-plan comments + 1_

  ## Reading
  Files read; what they revealed.

  ## Risks / open questions
  Blockers; questions for the user.

  ## Proposed approach
  Steps, file-by-file, with line numbers.

  ## Complexity
  haiku / sonnet / opus.

  ## Verdict

  Write the chosen verdict as a single bolded line, exactly one of:

  - `**PLAN READY**` — followed by a one-sentence rationale
  - `**NEEDS CLARIFICATION**` — followed by the specific questions
  - `**IMPLEMENTED**` — followed by a note (e.g. "pending build verification"; reserved for revisions after the fix has been applied)
  - `**WONT FIX**` — followed by reasoning
  - `**DUPLICATE OF #<M>**` — followed by reasoning

  Do NOT bury the verdict in prose — it must appear as a bolded prefix.

STANDING RULES:
- DO NOT edit, write, or modify any code file in the wayq repo.
- DO NOT bump any version string.
- DO NOT run prompt_version.ps1 or any build script.
- DO NOT commit, push, or run any git mutating command beyond `gh issue comment` for posting the plan.
- Use the `Agent` tool directly; do not route delegation through any other skill.
- Code-reading is fine. Writing plan-<N>.md to <STAGING_DIR>/ is fine. Posting ONE comment per run via `gh issue comment` is fine. Anything else: don't.

Reply with just "DONE" once both the local file AND the GitHub comment are posted.
"""

Wait for ALL sub-agents to complete before writing SUMMARY.md.
```

## Phase 4 — When the orchestrator returns

1. Push a `PushNotification`: `"wayq-issues: N plans ready (R revised, F fresh). See <SUMMARY.md>."`
2. Read `SUMMARY.md`.
3. Report to user a unified table covering BOTH the issues the orchestrator just re-planned AND the issues that were skipped earlier (with their reason). The skipped issues remain part of the milestone's open work — they need user-side action, not orchestrator work. Suggested layout:

   **Re-planned this run** (from orchestrator output):
   #, title, prior state (fresh / user-replied), verdict, one-paragraph plan summary, GitHub URL.

   **Skipped — awaiting your reply on GitHub** (`awaiting-clarification`):
   #, title, link to the open question.

   **Skipped — ready to implement** (`ready-to-implement`):
   #, title, complexity tier from the plan, GitHub URL.

   **Skipped — already implemented** (`implemented`):
   #, title, recommendation: close the GitHub issue once you've verified with a build (or I can close it with `gh issue close`).

   **Skipped — recommended closure** (`closed-recommendation`):
   #, title, verdict (WONT FIX or DUPLICATE), recommendation: close on GitHub.

4. For any `NEEDS CLARIFICATION` (from re-plan output or `awaiting-clarification` skip), tell the user to respond on the issue on GitHub. Re-run `/wayq-issues` when ready to iterate.
5. For `PLAN READY` items (whether freshly planned or carried over from `ready-to-implement` skip), offer the standard choices:
   - Approve all for implementation
   - Approve specific issues (by number)
   - Read the full plan for a specific issue
   - Pass / skip implementation entirely
6. For `implemented` items, offer to close the GitHub issue.
6. **Wait for the user's selection.**

## Phase 5 — Implementation (only on user approval, one issue at a time)

For each user-approved issue:

1. Re-read its plan file.
2. SPAWN ONE coding agent via the `Agent` tool:
   - subagent_type: "general-purpose"
   - model: the complexity tier the plan recommended
   - run_in_background: true
   - description: "Code issue #<N>: <short>"
   - prompt: a coding prompt referencing the plan file + standard `/bga`-style standing rules (no version bumps, no commits, flag invented names).
3. When the coding agent returns, push notification + report to user as with /bga.

## Hard constraints (apply through every phase)

- NO sub-agent or orchestrator agent edits the wayq codebase during planning. Plans only.
- All delegation goes through the `Agent` tool directly. Do not route through any other skill, even ones whose descriptions sound like "delegate work to background agents".
- Comment-marker convention: every Claude-posted issue comment MUST start with `<!-- claude-plan -->` on line 1. Future runs use this to detect their own comments and compute reply state.
- Even at implementation time, agents follow standard standing rules: no version bumps, no commits, no build offers, flag invented names.
- Don't auto-approve. Plans always pass through user review (on the issue, or locally) before coding.

## Don'ts

- Don't spawn sub-agents from the calling Claude directly — the orchestrator does that. The calling Claude only spawns the orchestrator (one agent).
- Don't try to render all plans inline if there are many issues; point the user at the SUMMARY.md and the per-issue URLs.
- Don't bypass the user-approval gate to "save time".
- Don't post more than one comment per issue per run. One revision = one comment.
- Don't strip or alter the `<!-- claude-plan -->` marker. Future runs depend on it.
- Cleanup pass runs BEFORE the Phase 2 summary, NOT after. If anything was cleared, it's a separate user-gated post.
