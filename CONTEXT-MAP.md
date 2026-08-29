# Context Map

This repo is a monorepo of audio plugins. Each plugin is its own context with
its own vocabulary: "feedback" means something specific inside a delay that it
would not mean inside a compressor.

| Context     | Path                   | What it is                                              |
| ----------- | ---------------------- | ------------------------------------------------------- |
| Orbit Delay | `plugins/orbit-delay/` | Tempo-syncable delay with a damped feedback path and a WebView UI |

Per-context `CONTEXT.md` files and `plugins/<name>/docs/adr/` are written
lazily, when a term or decision actually needs pinning down - not upfront. Their
absence is normal and is not a gap to fill.

Decisions that span every plugin (build layout, shared toolchain, formats we
ship) belong in the root `docs/adr/`, not in a plugin's own.
