PNB - Not Baseball Since 2013
===

A pesäpallo game engine in C.

Build & run
-----------

All commands run inside the Nix dev shell (provides the toolchain):

```bash
nix develop --command make main      # build the game
./main --windowed                    # run it (or: make run)
./main --windowed --fixture super-inning      # start straight in a tied super inning
./main --windowed --fixture homerun-contest   # ... or in the home-run contest
```

`--fixture cup-final-super-inning` starts a cup final's super inning; `--control1`/`--control2`
(0 = pad 1, 1 = pad 2, 2 = AI; default pad 1 against the AI) and `--team1`/`--team2` choose who plays.

Tests
-----

**One command before you commit:**

```bash
nix develop --command make check    # build + all five test tiers + guardrails + tool tests
```

It stops at the first failure and never rewrites tracked source files. The five tiers are also
runnable individually:

```bash
nix develop --command make test              # unit       — pure functions
nix develop --command make integration_test  # contract   — one-frame pipeline proofs
nix develop --command make scenario_test     # scenario   — full-game physics (injected intents)
nix develop --command make sim_test          # simulation — headless AI-vs-AI, real pipeline, seeded
nix develop --command make scripted_test     # scripted   — scripted keys through the real input path
```
The sim tier can also emit per-frame CSV traces of how game state evolves:

```bash
nix develop --command bash -c "make sim_runner && SIM_TRACE=1 ./sim_runner"
# writes sim_trace_half_inning.csv and sim_trace_homerun.csv
```

For behaviour-preserving refactors, record 66 seeded games (AI and fuzzed human keys,
including the super inning and contest) before and after, then compare:

```bash
tools/golden/record.sh /tmp/pnb-golden/before HEAD
tools/golden/record.sh /tmp/pnb-golden/after
tools/golden/compare.sh /tmp/pnb-golden/before /tmp/pnb-golden/after
```

Recording directories must be empty and outside the repository. Failed runs, incomplete
traces, missing runs on either side, and differences return failure; sameness is not rule
correctness. `nix develop --command make golden_tool_test` tests the golden tools,
including real `int`/`bool` layout probes; `nix develop --command make naming_tool_test`
tests the naming measurements.

Guardrails
----------

Some architectural numbers here are only allowed to go down — parameters no function
reads, engine files whose includes can see past the engine, files still awaiting the
function-quality audit, mixed-case struct fields, and player abbreviations. `make check`
runs these alongside the exported-surface and tool tests; `make guardrails`
runs them alone:

```bash
nix develop --command make guardrails
nix develop --command make format        # the fixer: clang-format, in place
```

The floors live in `tools/guardrails.sh`, which is also the definition of each
measurement. Improving a number means lowering its floor in the same commit; the
script says so when it notices, so a ratchet cannot quietly slacken.

The naming measurement uses compiler-recognized file-scope fields in `src/` (not
comments, type names, or function locals), and abbreviation words in C sources/headers
under `src/`, `tests/`, and `tools/`. Names are changed by compiler-reported call sites,
not by text substitution. Fingerprints fold integers at 32 bits; this protects storage
refactors, not cross-machine floating-point agreement or a wire format.

`guardrails` only ever *reports* — including the formatting row. `make format` is the
one target that edits your files, and it is deliberately separate.
