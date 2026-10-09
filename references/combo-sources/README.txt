REFERENCE SNAPSHOTS ONLY — NOT INSTALLED FDA MODULES

201 upstream text files read statically, not executed. Sources pinned to commits; hashes
and URLs in fetched-files.json. MIT license text and original author comments preserved.
Eroica-cpp/dota2scripts: 43 Lua scripts plus License.txt and README.
OpenHyperAI: 128 hero Lua files, selected helper/selection files, README and LICENSE.
All 127 heroes in current FDA catalog have a matching hero file with SkillsComplement.
That is symbol/file coverage, NOT 127 verified correct working player combos.
zfael/dota2-scripts: selected Rust hero modules plus README and LICENSE.
Snapshots are subsets of upstream projects: dependencies/runtime are not complete.

These files are NOT compiled by FDA build.bat or loaded/executed at runtime.
No automatic downloading/running installers, no Lua evaluator, no DLL injection changes.
Do not copy these snapshots into installed Dota folders. They are references for porting.
GPL repositories indexed only; their scripts are not included or linked into FDA.
No-license repositories indexed only; not treated as unrestricted reusable code.

ESP17 additions: two MIT OpenHyperAI data files (hero enum and position weights);
five MIT Nerve11/Umbrella-Scripts-Lua references including original LICENSE/README.
No Lua runtime is installed or executed. Derived position data is used by FDA's original
C++ read-only planner; full game scripts are NOT integrated.
