PNB - Not Baseball Since 2013
===

A pesäpallo game engine in C.

Build & run
-----------

All commands run inside the Nix dev shell (provides the toolchain):

```bash
nix develop --command make main      # build the game
./main --windowed                    # run it (or: make run)
```

Tests
-----

**One command before you commit:**

```bash
nix develop --command make check    # build + all five test tiers + guardrails
```

It stops at the first failure and changes nothing on disk. The five tiers are also
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

Guardrails
----------

Some architectural numbers here are only allowed to go down — files including the
`globals.h` monolith, parameters no function reads, files still awaiting the
function-quality audit. `make check` runs these as its last step; `make guardrails`
runs them alone:

```bash
nix develop --command make guardrails
nix develop --command make format        # the fixer: clang-format, in place
```

The floors live in `tools/guardrails.sh`, which is also the definition of each
measurement. Improving a number means lowering its floor in the same commit; the
script says so when it notices, so a ratchet cannot quietly slacken.

`guardrails` only ever *reports* — including the formatting row. `make format` is the
one target that edits your files, and it is deliberately separate.
