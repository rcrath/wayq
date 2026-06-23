# Adapt RhythmEcho Skills for wayq

- [x] Find all rhythmecho-specific skills in global skills dir (C:\Users\rich\.claude\skills\) — list name and path
  - rhythmecho-code/SKILL.md, rhythmecho-web/skill.md, rhythmecho-issues/SKILL.md
  - Also found with rhythmecho content: regress/skill.md, ibuild/skill.md, sync-feedback/SKILL.md
- [x] Find all rhythmecho-specific skills in local project skills dirs (look in E:\Dropbox\audio\git\rhythmecho if it exists, and any other rhythmecho project dirs under E:\Dropbox\audio\git\)
  - rhythmecho dir exists at E:/Dropbox/audio/git/rhythmecho but no .claude/skills/ subdirectory
- [x] List every found skill with its source path
  - 6 skills total (3 named rhythmecho-*, 3 with rhythmecho content)
- [x] Create E:\Dropbox\audio\git\wayq\.claude\skills\ directory
- [x] For each rhythmecho skill found: copy it, rename it (substitute "rhythmecho" with "wayq" in filename), substitute "rhythmecho" with "wayq" in all content (skill names, references, descriptions), and adjust any absolute or relative paths to be git-relative (paths should start from the git folder level, e.g. git/wayq/... not E:\Dropbox\audio\git\wayq\...)
  - Created: wayq-code/SKILL.md, wayq-web/skill.md, wayq-issues/SKILL.md, regress/skill.md, ibuild/skill.md, sync-feedback/SKILL.md
- [x] Write summary of all source skills found and all adapted skills created to .dispatch/tasks/wayq-skills-adapt/output.md
- [x] Touch .dispatch/tasks/wayq-skills-adapt/ipc/.done
