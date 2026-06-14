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

Four tiers, runnable individually:

```bash
nix develop --command make test              # unit       — pure functions
nix develop --command make integration_test  # contract   — one-frame pipeline proofs
nix develop --command make scenario_test     # scenario   — full-game physics (injected intents)
nix develop --command make sim_test          # simulation — headless AI-vs-AI, real pipeline, seeded
```
The sim tier can also emit per-frame CSV traces of how game state evolves:

```bash
nix develop --command bash -c "make sim_runner && SIM_TRACE=1 ./sim_runner"
# writes sim_trace_half_inning.csv and sim_trace_homerun.csv
```
