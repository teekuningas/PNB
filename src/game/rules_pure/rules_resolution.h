#ifndef RULES_RESOLUTION_H
#define RULES_RESOLUTION_H

/*
 * §21 Etenijän pääseminen lopulliseen ratkaisuun — when a runner has reached a final resolution.
 *
 * Written in plain ints and includes nothing, matching rules_side_change.h and
 * rules_batting_order.h: whether a runner's turn on the bases has concluded is a statement about
 * the four outcomes §21 enumerates, not about bodies, coordinates, or the ball.
 *
 * Rule text: the official pesäpallo rulebook, §21 (Etenijän pääseminen lopulliseen ratkaisuun).
 */

/**
 * @brief Has a runner reached a final resolution (*lopullinen ratkaisu*)?
 *
 * §21 defines the four terminal outcomes for a runner:
 *   1) scored a regular run (*tuo juoksun*),
 *   2) wounded (*haavoittuu*) — terminal wound, not merely marked or pending,
 *   3) burnt (*palaa*), or
 *   4) their advance has been voided (*eteneminen on mitätöity*).
 *
 * A *kunniajuoksu* (run of honour, §42) leaves the player alive on third base, so only a regular
 * run satisfies clause (1).
 *
 * @param has_scored     1 if the runner scored a regular run (§21(1)).
 * @param is_wounded     1 if the runner is terminally wounded (§21(2)).
 * @param is_burnt       1 if the runner is burnt (§21(3)).
 * @param advance_voided 1 if the runner's advance has been voided (§21(4)).
 * @return 1 if any of §21's four clauses holds, 0 otherwise.
 */
int player_has_reached_final_resolution(int has_scored, int is_wounded, int is_burnt, int advance_voided);

#endif /* RULES_RESOLUTION_H */
