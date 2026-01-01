#include "base_logic.h"

BaseID base_get_next(BaseID id)
{
	switch (id) {
	case BASE_HOME:
		return BASE_FIRST;
	case BASE_FIRST:
		return BASE_SECOND;
	case BASE_SECOND:
		return BASE_THIRD;
	case BASE_THIRD:
		return BASE_HOME_SCORED;
	case BASE_HOME_SCORED:
		return BASE_NONE;
	default:
		return BASE_NONE;
	}
}

BaseID base_get_prev(BaseID id)
{
	switch (id) {
	case BASE_FIRST:
		return BASE_HOME;
	case BASE_SECOND:
		return BASE_FIRST;
	case BASE_THIRD:
		return BASE_SECOND;
	case BASE_HOME_SCORED:
		return BASE_THIRD;
	default:
		return BASE_NONE;
	}
}

bool base_is_safe_haven(BaseID id)
{
	return (id == BASE_HOME || id == BASE_FIRST || id == BASE_SECOND || id == BASE_THIRD);
}

bool base_is_index(BaseID id)
{
	return (id == BASE_HOME || id == BASE_FIRST || id == BASE_SECOND || id == BASE_THIRD);
}

bool base_can_advance(BaseID id)
{
	return (id == BASE_HOME || id == BASE_FIRST || id == BASE_SECOND);
}

bool base_is_at_least(BaseID id, BaseID threshold)
{
	// Cast to int relies on the enum values defined in globals.h:
	// HOME=0, FIRST=1, SECOND=2, THIRD=3, HOME_SCORED=4
	return (int)id >= (int)threshold;
}

int base_cmp(BaseID a, BaseID b)
{
	return (int)a - (int)b;
}

int base_to_int_index(BaseID id)
{
	if (base_is_index(id)) {
		return (int)id;
	}
	return -1;
}