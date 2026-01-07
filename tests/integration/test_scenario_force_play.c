#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../../src/include/globals.h"
#include "../../src/game/rules_pure/base_logic.h"
#include "../../src/game/rules_pure/rules_outs.h"
#include "../../src/game/game_analysis.h"
#include "fixtures.h"

/*
 * TEST SCENARIO: Force Play (Pakkovaihto) §40
 *
 * Rule §40: "Ajotilanteessa on vaihtopakko... jokaisen etenijän on päästävä turvaan seuraavalle pesälle."
 * (In a loaded bases situation, there is a forced advance... every runner must reach safety at the next base.)
 */

void test_force_play_bases_loaded(void)
{
	StateInfo* state = setup_test_state();
	LocalGameInfo* game = state->localGameInfo;
	GameState* gs = &(game->gameState);

	// Setup: Bases Loaded (Ajolähtö) - Runners on 1, 2, 3
	// Player 0 on 3rd, Player 1 on 2nd, Player 2 on 1st
	int r3 = 0, r2 = 1, r1 = 2;
	
game->playerInfo[r3].bTPI.baseId = BASE_THIRD;
game->playerInfo[r3].bTPI.state = PLAYER_STATE_SAFE_ON_BASE;
game->pII.baseControlIndex[3] = r3;

	game->playerInfo[r2].bTPI.baseId = BASE_SECOND;
game->playerInfo[r2].bTPI.state = PLAYER_STATE_SAFE_ON_BASE;
game->pII.baseControlIndex[2] = r2;

	game->playerInfo[r1].bTPI.baseId = BASE_FIRST;
game->playerInfo[r1].bTPI.state = PLAYER_STATE_SAFE_ON_BASE;
game->pII.baseControlIndex[1] = r1;

	// Batter (Player 3) hits a grounder and runs to 1st
	int batter = 3;
	game->playerInfo[batter].bTPI.state = PLAYER_STATE_RUNNING; 
	// Batter is "forcing" the others because they are essentially displacing them.
	// In the code, `is_runner_forced_out` checks if the previous base is occupied/claimed.

	/* 
	 * Scenario 1: Ball arrives at 2nd Base BEFORE Runner 1 gets there.	 * Runner 1 is forced to run because Batter is coming to 1st (implicitly).
	 * Actually, the logic usually checks: "Is the player safe? No. Is the previous base occupied? Yes."
	 */

	// Simulate Ball at 2nd Base
	game->pII.hasBallIndex = PLAYERS_IN_TEAM + 5; // Fielder has ball
	// We mimic `checkForOuts` logic which calls `is_runner_forced_out`
	
	// We need to define "Force" in terms of the function signature:
	// is_runner_forced_out(current_base, is_safe, previous_base, is_advancing_freely, gameState)
	
	// Check Runner 1 (at 2nd base target, coming from 1st)
	// He is NOT safe at 2nd yet.
	// Is he forced? Yes, if 1st base is "burned" or occupied by the pusher.
	
	// Wait, the current logic in `game_analysis.c` loop:
	// It checks every base (i).
	// It sees ball is at base i (let's say 2nd base).
	// It checks if any runner is targeting this base or is "on" this base but not safe.
	
	// Let's manually invoke the pure rule check:
	// Runner 1 is effectively trying to get to 2nd. His `baseId` might still be `BASE_FIRST` if he hasn't arrived, 
	// or `BASE_SECOND` if he arrived but isn't safe?
	// `game_analysis.c` iterates players.
	// If ball is at 2nd Base:
	// It checks players who claim 2nd base is their target?
	// No, it checks `playerInfo[index].bTPI.baseId`.
	
	// CRITICAL: How does the game track "Target Base"? 
	// `runToNextBase` sets `baseId` to the NEXT base immediately when they start running?
	// Let's verify this assumption in `game_manipulation.c` or similar.
	// Yes: `game->playerInfo[index].bTPI.baseId = nextBase;` usually happens on run start.
	
		// Assume Runner 1 has started running to 2nd.
		// CRITICAL: In `game_manipulation.c`, `baseId` is updated to the NEXT base only ON ARRIVAL.
		// So while running, `baseId` is still the FROM base (BASE_FIRST).
		game->playerInfo[r1].bTPI.baseId = BASE_FIRST;
		game->playerInfo[r1].bTPI.state = PLAYER_STATE_RUNNING; 
		game->pII.baseControlIndex[1] = -1; // He left 1st
		
		// Previous base (1st) is now technically "empty" regarding Runner 1, 
		// BUT the Force rule depends on the "Chain".
		
		// Let's test `is_runner_forced_out` for Runner 1 who is en route to 2nd.
		// Ball is at 2nd Base.
		// `game_analysis.c` calculates `baseIndexId` (the base to check against) as `i - 1` = `BASE_FIRST`.
		
		// So we call with:
		// target_base (not used by func, but context) = BASE_SECOND
		// current_base (player's base) = BASE_FIRST
		// ball_at_base_index (passed param) = BASE_FIRST
		
		int is_out = is_runner_forced_out(BASE_FIRST, 0, BASE_FIRST, 0, gs);
		
		// In a normal force play, if the ball beats the runner to the base, they are OUT.
		// `is_runner_forced_out` logic:
			// If player_base == ball_at_base_index -> TRUE.
			
			assert(is_out == 1);
			
			/*
			 * Scenario 2: Runner 1 is Safe at 2nd (arrived before ball).		 * Then Ball arrives at 1st Base (behind him).
		 * Is he safe? Yes.
		 */
		game->playerInfo[r1].bTPI.baseId = BASE_SECOND; // Arrived!
		game->playerInfo[r1].bTPI.state = PLAYER_STATE_SAFE_ON_BASE; // Safe!
		
		// Ball is at 1st base (behind him).
		// `game_analysis.c` loop for i=1 (1st base):
		// ball_at_base_index = i - 1 = BASE_HOME.
		// Call: is_runner_forced_out(BASE_SECOND, 1, BASE_HOME, 0, gs);
		
			
		
			is_out = is_runner_forced_out(BASE_SECOND, 1, BASE_HOME, 0, gs);
		
			assert(is_out == 0);
		
		
		
			/*
		
			 * Scenario 3: The "Push" (Force from behind).
	 * If Runner 1 is at 1st Base (Safe).
	 * Batter hits grounder.
	 * Ball goes to 2nd Base.
	 * Runner 1 hasn't left 1st Base yet.
	 * Is he OUT?
	 * Rule: "Ajotilanteessa on vaihtopakko". He MUST advance.
	 * If he is still on 1st, and ball is at 2nd... he isn't at 2nd yet.
	 * The ball at 2nd burns 2nd base. He can't go there anymore.
	 * If he stays at 1st, he will eventually be burned if the Batter arrives at 1st.
	 */
	
	cleanup_test_state(state);
}

void run_scenario_force_play_tests(void)
{
	test_force_play_bases_loaded();
}
