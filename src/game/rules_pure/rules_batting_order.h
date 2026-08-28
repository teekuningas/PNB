#ifndef RULES_BATTING_ORDER_H
#define RULES_BATTING_ORDER_H

/*
 * §27 Lyöntijärjestys, §12 Vuoronvaihto, §7 Joukkue — WHO may be seated as the next batter.
 *
 * One question, asked in two places for two different purposes, and that is the point of it being a
 * function rather than a branch. A client asks it to ENUMERATE — so a human is never shown an option
 * the rules forbid, and a cursor re-derives its candidates from the world rather than remembering
 * them. The INGEST gate asks it to REFUSE — so a producer that is wrong, lagging, scripted, or one
 * day a peer on a wire cannot seat an illegal batter whatever its client believed. Two callers, one
 * rule; the engine never trusts the client, and does not need a second copy of the rule to distrust
 * it.
 *
 * Written in plain ints and includes nothing, for the reason rules_side_change.h gives: who may bat
 * is a statement about a batting order, a joker's status and a spent turn, not about bodies, bases
 * or a ball. Joker status is passed as an int carrying the JokerStatus values, so this file needs no
 * headers at all — see JOKER_STATUS_* below.
 *
 * Rule text: the official pesäpallo rulebook, §7, §12, §27.
 */

/* The JokerStatus values, restated here so this header includes nothing. A caller passes
 * `(int)playerInfo[i].bTPI.joker`. The names below say what the enum's do not: the field answers
 * "is this a jokeripelaaja, and is he spent?", and its first value is the "no, a varsinainen
 * pelaaja" case. */
#define JOKER_STATUS_NOT_A_JOKER 0 /* varsinainen pelaaja — in the batting order */
#define JOKER_STATUS_UNUSED 1 /* jokeripelaaja, still available this half-inning */
#define JOKER_STATUS_SPENT 2 /* jokeripelaaja, already used this half-inning */

/**
 * @brief Why a player may not be seated, or SEAT_ALLOWED if they may.
 *
 * The refusals are distinct because they are different rules, and the gate hands each one back as a
 * RuleId: a refusal whose reason is thrown away cannot be explained on screen, replayed, or asked of
 * a rule set later.
 */
typedef enum {
    SEAT_ALLOWED = 0,
    SEAT_NOT_IN_BATTING_TURN, /* §27: a regular who is not the lyöntivuoroinen player */
    SEAT_REGULAR_ORDER_SPENT, /* §12: the order has come round; only a joker may extend the turn */
    SEAT_JOKER_ALREADY_USED /* §7: three jokers per half-inning, each once */
} BatterSeatVerdict;

/**
 * @brief May this player take the bat right now?
 *
 * §27 gives the candidate set directly: the *lyöntivuoroinen* player, plus any joker, because
 * *"Jokeripelaajan voi asettaa lyömään, ellei lyöntivuoroinen pelaaja ole asettunut lyömään"* — the
 * choice stays open until somebody takes the bat, and is irrevocable afterwards ("Lyömään
 * asettunutta pelaajaa ei voi vaihtaa pois lyömästä"), which is why the question is asked at the
 * moment of seating and not when the prompt is raised.
 *
 * §12 removes the regular from that set once the batting order has come round to the designated last
 * batter. The rulebook's own worked example is explicit that seating one anyway is an error rather
 * than a nuance — *"peliteot mitätöidään ja vihelletään vuoronvaihto, koska tullessaan lyömään
 * numero 7 vei joukkueelta jokereiden käyttömahdollisuuden"*. Coming to bat IS the offence, so the
 * engine's job is to make it unreachable, not to undo it afterwards.
 *
 * §7 removes a joker from the set once it has batted this half-inning.
 *
 * @param index               The player being asked about.
 * @param in_turn_index       The *lyöntivuoroinen* player: batterOrder[batterOrderIndex].
 * @param regular_order_spent halfInningState.lastBatter.regularOrderSpent — §12's batting-order
 *                            clause on its own, never `turnExhausted` (bug #9).
 * @param joker_status        JOKER_STATUS_* for `index`.
 * @return SEAT_ALLOWED, or the rule that refuses.
 */
BatterSeatVerdict batter_seat_verdict(int index, int in_turn_index, int regular_order_spent, int joker_status);

#endif /* RULES_BATTING_ORDER_H */
