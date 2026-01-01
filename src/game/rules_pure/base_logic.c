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
		return BASE_HOME_SCORED; // Or BASE_HOME? Usually implies scoring.
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
