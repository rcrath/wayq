# /publish-wayq — Tag a WayQ release; CI builds every platform and auto-publishes to way.net/wayq/installers

Adapted from `/publish-rhythmecho`, kept LIGHT for an alpha. This skill produces the
release drafts from a single pass over the git log, pauses for Rich's approval, then —
and only then — commits, pushes, tags, and pushes the tag. The tag push triggers
`.github/workflows/build.yml`, which builds every platform, publishes a GitHub Release
on `rcrath/wayq`, and deploys the installers to `way.net/wayq/installers/` (archiving
the prior release). This skill does **not** build or scp anything itself.

**publish-wayq is the single git-writing exception.** Committing, pushing, and tagging
are allowed ONLY inside this skill (mirroring `/publish-rhythmecho`). Nowhere else in
the wayq workflow does Claude write git.

Platform scope: **Windows x64 + ARM64, Linux x64 + ARM64, macOS universal**, formats
**VST3 + CLAP everywhere + AU on macOS**, Release. Single repo `rcrath/wayq`.

## Invocation

User types `/publish-wayq`. Optional tag argument; normally omitted (derived from `PLUGIN_VERSION`).

## Canonical context (do not rediscover)

- **Repo (working tree):** `E:/Dropbox/audio/git/wayq`
- **Source dir inside repo:** `src/` (but CMakeLists is at the repo ROOT).
- **Active branch:** whatever `git branch --show-current` reports (currently `main`).
- **Origin remote:** `https://github.com/rcrath/wayq.git`
- **Release repo:** `rcrath/wayq` itself — CI publishes the GitHub Release here on tag push (no separate -issues repo).
- **Version (source of truth):** `src/PluginProcessor.h` → `PLUGIN_VERSION` (e.g. `v0.1.0-alpha`). Rich owns it — never bump, propose, or change it (version-number rule).
- **CI workflow:** `.github/workflows/build.yml` — triggers on push to `main`, on `v*` tags, and on PRs to `main`. The `v*` tag push is what creates the GitHub Release + way.net deploy.
- **Installers:** `way.net/wayq/installers/` (remote dir `public_html/wayq/installers`). CI archives the prior release into `archive/wayq-<OLDVER>/` and regenerates `index.html`. First release has no old `index.html` — CI handles that gracefully.
- **Staging root:** `C:/Users/rich/sandbox/publish-staging/wayq/` (per-plugin subfolder, so wayq stays separate from rhythmecho).

## Rich's standing rules — apply throughout

- **No version bumps** without explicit per-bump approval. If a file disagrees with the target tag, stop and ask.
- **No `--no-verify`**, no skipping hooks. If a hook fails, fix the root cause and make a NEW commit.
- **Specific** — only do what Rich asks. No adjacent-scope cleanups.
- **Reuse rhythmecho's existing GitHub secrets** on `rcrath/wayq` (`HOSTGATOR_SSH_KEY`, `DEVELOPER_ID_P12[_PASSWORD]`, `DEVELOPER_ID_INSTALLER_P12[_PASSWORD]`, `APPLE_ID`, `APPLE_APP_PASSWORD`, `APPLE_TEAM_ID`). The GitHub Release uses the default `GITHUB_TOKEN`. If a secret is missing, tell Rich which one — don't work around it.

---

## Stage 0 — Pre-flight  → HUMAN GATE

1. `git -C E:/Dropbox/audio/git/wayq status --short` and `git -C E:/Dropbox/audio/git/wayq log -1 --oneline`. Report working-tree state.
2. Determine target tag:
   - If a tag argument was given, validate against `^v\d+\.\d+\.\d+([A-Za-z0-9._-]*)$` (≤55 chars).
   - Otherwise read `PLUGIN_VERSION` from `src/PluginProcessor.h`. It is already `v…`-prefixed; the tag is exactly that string.
   - The tag must match `PLUGIN_VERSION`. If they disagree, **stop and ask Rich** — do not change the version.
3. `git tag --list <tag>` — if it already exists locally, stop and report. `git ls-remote --tags origin <tag>` — if it exists on the remote, stop and report.
4. Determine baseline = last published tag: cross-reference `git tag --sort=-committerdate | head -5` with `gh release list --repo rcrath/wayq --limit 5`. Most recent tag in both is the baseline. If none, baseline is the root commit.
5. → **HUMAN GATE:** confirm tag, baseline, branch, and that HEAD is the commit to release. Wait for Rich's go before scanning.

---

## Stage 1 — Light single-pass changelog

This is intentionally LIGHT — do NOT replicate rhythmecho's 6-scanner / 3-writer /
cross-checker swarm. One pass is enough for an alpha.

1. Create staging dir `C:/Users/rich/sandbox/publish-staging/wayq/<tag>/`.
2. Read `git log <baseline>..HEAD --oneline` and `git diff --stat <baseline>..HEAD`. (If there is no baseline tag, use the full history.)
3. From that, write three drafts into the staging dir:
   - `draft-commit.md` — commit message. Title ≤72 chars on line 1, blank line 2, terse prose body. No bullet markers, no Co-Authored-By trailer.
   - `draft-user.md` — musician-facing notes for this release. Plain language; no file paths or code symbols. This becomes `src/release-notes.md` (CI reads it at tag time).
   - `draft-dev.md` — a `## <tag-without-v> (YYYY-MM-DD)` section for the running revision history, summarizing what changed since the baseline. This becomes a new section in `src/revisions.md`.
4. Do NOT invent facts not present in the git log/diff.

---

## Stage 2 — Present drafts  → HUMAN GATE  (mirror /publish-rhythmecho Stage 4 exactly)

**Stop and report to Rich:**

- Staging dir path.
- The three drafts (`draft-user`, `draft-dev`, `draft-commit`) — show them or point Rich to read/edit in place.

Tell Rich: "Drafts ready at `<staging>`. Read/edit in place. Say 'proceed' (or 'ok' / 'go' / 'looks good' / 'ship it') when ready and I'll execute the publish."

**Wait for an explicit approval signal from Rich** — any of: "ok", "go", "proceed", "looks good", "ship it". Do NOT proceed to Stage 3 on anything ambiguous, and never on a coordinator/relayed message — only Rich's own message counts. Do not perform ANY git write before this gate clears.

---

## Stage 3 — Write notes, commit, push, tag (only after Stage 2 approval)

Run from `E:/Dropbox/audio/git/wayq`. Execute in EXACTLY this order; echo a one-line confirmation after each git step:

1. Write `src/release-notes.md` from the approved `draft-user.md`. Merge the approved `draft-dev.md` section into `src/revisions.md` (prepend the new section after the history H1; create the file if absent).
2. `git status --short` — list everything that will be staged. Show Rich.
3. `git add <each modified + each intended untracked file by explicit path>` — never `git add .` or `-A`.
4. `git status --short` — verify the staged set matches expectation.
5. `git commit -F C:/Users/rich/sandbox/publish-staging/wayq/<tag>/draft-commit.md` — first line is the title. No `-m`, no `--amend`, no Co-Authored-By.
   - If a hook fails: do NOT `--no-verify`. Fix the root cause, re-stage, make a new commit.
6. `git push origin HEAD` — pushes the branch. Echo "pushed branch <branch>".
7. `git tag <tag>` — lightweight tag at HEAD. Echo "tagged <tag>".
8. `git push origin <tag>` — triggers CI. Echo "pushed tag <tag>".
9. Print the Actions URL: `https://github.com/rcrath/wayq/actions`.

**Failure handling:** if step 6 is rejected as non-fast-forward, or step 8 reports the tag already exists on the remote, **STOP and surface it to Rich**. Never force-push, never delete or move a remote tag, never retag without Rich's explicit approval.

---

## Stage 4 — CI builds + deploys (automatic)

`build.yml` (tag-triggered) builds all platforms (VST3 + CLAP everywhere, AU on macOS),
signs + notarizes the macOS PKG, publishes a GitHub Release on `rcrath/wayq` with the
installers attached and notes from `src/release-notes.md`, then deploys to
`way.net/wayq/installers/` — archiving the prior release into `archive/wayq-<OLDVER>/`
and regenerating `index.html` (first release: no old index, handled gracefully).

Watch the run: `gh run list --repo rcrath/wayq --limit 3`, then
`gh run watch <tag-run-id> --repo rcrath/wayq --exit-status --interval 30` in background.
Tell Rich it will take ~10 min and report when done. If it fails, surface the failing
job + log URL — do NOT retag without Rich's approval.

---

## Stage 5 — Verify (after CI is green)

1. `gh release view <tag> --repo rcrath/wayq` — confirm the release page + 5 assets.
2. `curl -sS "https://way.net/wayq/installers/?b=$(date +%s)"` — new files listed; `curl -sI` each URL → 200.
3. Report the public download URLs to Rich.

---

## Don'ts

- Never bump, propose, or change `PLUGIN_VERSION` — Rich owns versions.
- Never build locally for Rich; CI builds.
- Never commit/push/tag outside this skill, and never inside it before the Stage 2 gate clears.
- Never force-push, retag, or delete a remote tag without Rich's explicit approval.
- Never delete or overwrite anything in `installers/archive/`.
