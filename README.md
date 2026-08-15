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

Five tiers, runnable individually:

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
function-quality audit. `make guardrails` measures them all and fails if any has
crept back up, so they cannot drift between sessions:

```bash
nix develop --command make guardrails
nix develop --command make format        # clang-format in place; run before committing
```

The floors live in `tools/guardrails.sh`. Improving one means lowering its floor in
the same commit; the script says so when it notices.
