---
name: wayq-code
description: "Get oriented on the current state of the wayq codebase: branch, latest commit, working-tree state, recent tags, src/ file listing. Trigger when the user types `/wayq-code`, when starting a session in the wayq repo, or when asked any variant of 'where are we' / 'what's the state' for wayq."
license: MIT
version: "1.0.0"
user_invocable: true
---

# /wayq-code — Orient on wayq project state

Quick orientation for the wayq codebase. Invoke manually (`/wayq-code`) or auto-fire from the SessionStart hook.

## What to gather (in parallel via Bash)

1. `git log -3 --oneline` — recent commits
2. `git status --short` — working-tree state
3. `git branch --show-current` — active branch
4. `git tag --sort=-committerdate | head -3` — recent local tags
5. `ls src/` — what source files are present

## What to report

```
wayq status
───────────
Branch:        <branch>
Last commit:   <sha> <subject>
Recent tags:   <none | list>
Working tree:  <clean | "N modified, M untracked">
src/ files:    <list of .h/.cpp files, fonts/svgs noted briefly>
```

If the working tree has modified or untracked files, list them concisely (filenames only).

## Memory context

The user's auto-memory at `~/.claude/projects/E--Dropbox-audio-git-wayq/memory/MEMORY.md` is auto-loaded into the session. Don't re-print its contents; just confirm by name that it's loaded ("Memory index loaded — N entries.") and trust it for cross-session context.

## Don'ts

- Don't make recommendations, suggestions, or take any action — this is purely orientation.
- Don't dispatch any agent. Don't commit, build, or modify anything.
- Don't re-read source files beyond the explicit gather list. Goal is fast orientation in seconds.
- If the user wants more detail on any specific area, they'll ask follow-up questions.
