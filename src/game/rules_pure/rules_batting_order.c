#include "rules_batting_order.h"

BatterSeatVerdict batter_seat_verdict(int index, int in_turn_index, int regular_order_spent, int joker_status)
{
    if (joker_status == JOKER_STATUS_NOT_A_JOKER) {
        // §27: a varsinainen pelaaja bats when the order reaches them, and only then. The order is a
        // cycle followed for the whole match, so "not in turn" is never "has had their turn" — it is
        // simply somebody else's.
        if (index != in_turn_index) return SEAT_NOT_IN_BATTING_TURN;
        // §12(2)/(3): the order has run its course, so no further regular may be seated whatever the
        // cycle says. Only a joker can extend the turn from here.
        if (regular_order_spent) return SEAT_REGULAR_ORDER_SPENT;
        return SEAT_ALLOWED;
    }

    // §7: "Joukkue voi käyttää jokaisessa sisävuorossa kolmea eri jokeripelaajaa, kerran kutakin."
    // A joker is never in the batting order, so it is never out of turn — the only question about one
    // is whether this half-inning has already spent it.
    if (joker_status == JOKER_STATUS_SPENT) return SEAT_JOKER_ALREADY_USED;
    return SEAT_ALLOWED;
}
