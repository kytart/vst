# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Before exploring, read these

**This repo is multi-context.** One context per plugin.

- **`CONTEXT-MAP.md`** at the repo root: it points at one `CONTEXT.md` per context. Read the ones relevant to the topic.
- **`plugins/<name>/CONTEXT.md`**: the vocabulary of that one plugin.
- **`docs/adr/`** at the root for decisions spanning every plugin, and **`plugins/<name>/docs/adr/`** for decisions scoped to one.

If any of these files don't exist, **proceed silently**. Don't flag their absence; don't suggest creating them upfront. The `/domain-modeling` skill (reached via `/grill-with-docs` and `/improve-codebase-architecture`) creates them lazily when terms or decisions actually get resolved.

## File structure

```
/
├── CONTEXT-MAP.md
├── CMakeLists.txt                     ← adds JUCE once, then each plugin
├── docs/adr/                          ← decisions spanning every plugin
├── libs/JUCE/                         ← shared submodule, compiled once
└── plugins/
    └── orbit-delay/
        ├── CMakeLists.txt
        ├── CONTEXT.md                 ← written lazily
        ├── docs/adr/                  ← decisions scoped to this plugin
        ├── src/
        └── ui/
```

## Use the glossary's vocabulary

When your output names a domain concept (in an issue title, a refactor proposal, a hypothesis, a test name), use the term as defined in `CONTEXT.md`. Don't drift to synonyms the glossary explicitly avoids.

If the concept you need isn't in the glossary yet, that's a signal: either you're inventing language the project doesn't use (reconsider) or there's a real gap (note it for `/domain-modeling`).

## Flag ADR conflicts

If your output contradicts an existing ADR, surface it explicitly rather than silently overriding:

> _Contradicts ADR-0007 (event-sourced orders), but worth reopening because…_
