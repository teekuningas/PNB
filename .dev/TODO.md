# TODO - Current Tasks

### Phase 3: Ascent Stage 1 - Rendering & UI (Next)
- [ ] Refactor `src/renderer/player_renderer.c` to use `PlayerUnitState` and `BaseID` enums instead of flags.
    - Replace usage of `bTPI.isOnBase`, `bTPI.joker` (if relevant to state), etc. with switch on `bTPI.state`.
    - **Note:** Do NOT remove the old flags yet. Just change the reader to use the Enums.
- [ ] Refactor `src/game/game_screen.c` (Overlay/HUD) to use Enums.
    - Update `drawStatistics` to check `gAI.event` (enum) instead of `gAI.gameInfoEvent` (int).
    - Update `drawStatistics` to check `bTPI.baseId` instead of `bTPI.base` for base markers.
- [ ] Verify visually.
