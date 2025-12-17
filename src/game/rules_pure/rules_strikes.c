#include "rules_strikes.h"

// §26 Syötön tuomitseminen
int should_change_batter_on_strikes(int strikes, int safe_on_first_base_index) {
	if (strikes == 3 && safe_on_first_base_index != -1) {
		return 1;
	}
	return 0;
}
