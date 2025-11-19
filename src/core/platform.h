#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

// Ensures the application's save directory exists.
// ~/.pnb/saves/ on Linux
// %USERPROFILE%/AppData/Local/PNB/saves/ on Windows
// Returns 0 on success, 1 on failure.
int platform_ensure_save_dir(void);

// Constructs the full, platform-specific path for a given save slot.
// result: A buffer to write the path into.
// slot: The save slot number (0-4).
// Returns 0 on success, 1 on failure (e.g., path too long).
int platform_get_save_path(char* result, size_t max_len, int slot);

#endif // PLATFORM_H
