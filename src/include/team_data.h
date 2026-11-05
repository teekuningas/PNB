#ifndef TEAM_DATA_H
#define TEAM_DATA_H

// The rosters as loaded from data/teams.xml: who is in each team, and each player's speed and power.
// Read-only once loaded; the World copies what it needs into the players at a half-inning's setup.

// struct to keep player data
typedef struct _PlayerData {
    char* id;
    char* name;
    int speed;
    int power;
} PlayerData;

// struct keeps information about teams
typedef struct _TeamData {
    char* id;
    char* name;
    int numPlayers;
    PlayerData* players;
} TeamData;

#endif /* TEAM_DATA_H */
