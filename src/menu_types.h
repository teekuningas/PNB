#ifndef MENU_TYPES_H
#define MENU_TYPES_H

// Describes the reason the menu is being entered, usually from the game screen.
// This determines the menu's starting screen.
typedef enum {
	MENU_ENTRY_NORMAL = 0,          // Normal startup, show the main front page.
	MENU_ENTRY_INTER_PERIOD = 1,    // Between periods, go to batting order.
	MENU_ENTRY_SUPER_INNING = 2,    // Game tied, go to batting order for super inning.
	MENU_ENTRY_HOMERUN_CONTEST = 3, // Super inning tied, go to homerun contest setup.
	MENU_ENTRY_GAME_OVER = 4        // Game is over, show the summary screen.
} MenuMode;

typedef struct _MenuInfo {
	MenuMode mode;
} MenuInfo;

#endif /* MENU_TYPES_H */
