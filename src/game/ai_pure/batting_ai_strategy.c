#include "batting_ai_strategy.h"
#include <math.h>
#include <stdlib.h>

BattingStrategy calculate_batting_strategy(int strikes, int fieldStatus, int power, int speed, int period)
{
    BattingStrategy strategy = {0, 0, 0}; // default
    int hasPower = (power > 2) ? 1 : 0;
    int isFast = (speed > 2) ? 1 : 0;

    if (period < 4) {
        if (strikes == 0) {
            strategy.style = 1;
            strategy.runBaseRunners = 0;
            strategy.runBatter = 0;
        } else if (strikes == 1) {
            if (fieldStatus == 0) {
                if (isFast == 0) {
                    strategy.style = 2;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                }
            } else if (fieldStatus == 1) {
                if (hasPower == 1) {
                    if (isFast == 0) {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                } else {
                    if (isFast == 0) {
                        strategy.style = 2;
                        strategy.runBaseRunners = 0;
                        strategy.runBatter = 1;
                    } else {
                        strategy.style = 0;
                        strategy.runBaseRunners = 0;
                        strategy.runBatter = 1;
                    }
                }
            } else if (fieldStatus == 2) {
                if (hasPower == 1) {
                    if (isFast == 0) {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 1;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                } else {
                    if (isFast == 0) {
                        strategy.style = 0;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 0;
                    } else {
                        strategy.style = 0;
                        strategy.runBaseRunners = 1;
                        strategy.runBatter = 1;
                    }
                }
            }
        } else if (strikes == 2) {
            if (fieldStatus == 0) {
                if (isFast == 0) {
                    strategy.style = 2;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 0;
                    strategy.runBatter = 1;
                }
            } else if (fieldStatus == 1) {
                strategy.style = 2;
                strategy.runBaseRunners = 0;
                strategy.runBatter = 1;
            } else if (fieldStatus == 2) {
                if (hasPower == 1) {
                    strategy.style = 1;
                    strategy.runBaseRunners = 1;
                    strategy.runBatter = 1;
                } else {
                    strategy.style = 0;
                    strategy.runBaseRunners = 1;
                    strategy.runBatter = 1;
                }
            }
        }
    } else {
        if (strikes == 0 || strikes == 1) {
            strategy.style = 1;
            strategy.runBaseRunners = 0;
            strategy.runBatter = 0;
        } else {
            if (hasPower == 1) {
                strategy.style = 1;
                strategy.runBaseRunners = 1;
                strategy.runBatter = 1;
            } else {
                strategy.style = 0;
                strategy.runBaseRunners = 1;
                strategy.runBatter = 0;
            }
        }
    }
    return strategy;
}

int should_change_batter(int fieldStatus, int power, int speed)
{
    if (fieldStatus == 0) {
        if (speed > 2) {
            return 0;
        } else {
            return 1;
        }
    } else if (fieldStatus == 2) {
        if (power > 2) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

int is_wrong_pitch(float vx, float vy, float gravity, float plate_width)
{
    float v_x_abs = fabsf(vx);
    float t = vy * 2.0f / gravity;
    float offset = v_x_abs * t;
    if (offset > plate_width / 2.0f) {
        return 1;
    }
    return 0;
}

float calculate_ai_batting_angle(int battingStyle, int leadBase, int randomValue)
{
    // randomValue should be a positive integer, typically from rand()
    float angle = 0.0f;
    if (battingStyle == 0) { // bunt
        int random = randomValue % 4 + 2;
        angle = (float)random / 20.0f;
    } else if (battingStyle == 1) { // normal swing
        int random;
        if (leadBase == 2) {
            random = -(randomValue % 16);
            angle = (float)random / 45.0f;
        } else if (leadBase == 1) {
            random = randomValue % 16;
            angle = (float)random / 45.0f;
        } else {
            random = randomValue % 33;
            random = random - 16;
            angle = (float)random / 45.0f;
        }
    } else if (battingStyle == 2) { // wound
        int random = randomValue % 5;
        random = random - 2;
        angle = (float)random / 20.0f;
    }
    return angle;
}
