#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#define PATH_MAX_LENGTH MAX_PATH
#define SEPARATOR "\\"
#else
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#define PATH_MAX_LENGTH PATH_MAX
#define SEPARATOR "/"
#endif

int platform_ensure_save_dir(void)
{
	char* homedir;
	char pnbPath[PATH_MAX_LENGTH];
	char savesPath[PATH_MAX_LENGTH];
	int result;

#ifdef _WIN32
	homedir = getenv("USERPROFILE");
	if (homedir == NULL) {
		fprintf(stderr, "Error: Could not get user profile directory\n");
		return 1;
	}

	// Build PNB directory path
	result = snprintf(pnbPath, sizeof(pnbPath), "%s%sAppData%sLocal%sPNB",
	                  homedir, SEPARATOR, SEPARATOR, SEPARATOR);
	if (result < 0 || result >= (int)sizeof(pnbPath)) {
		fprintf(stderr, "Error: Path too long for PNB directory\n");
		return 1;
	}

	if (!CreateDirectory(pnbPath, NULL)) {
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS) {
			fprintf(stderr, "Failed to create directory %s: %lu\n", pnbPath, err);
			return 1;
		}
	}

	// Build saves directory path
	result = snprintf(savesPath, sizeof(savesPath), "%s%ssaves", pnbPath, SEPARATOR);
	if (result < 0 || result >= (int)sizeof(savesPath)) {
		fprintf(stderr, "Error: Path too long for saves directory\n");
		return 1;
	}

	if (!CreateDirectory(savesPath, NULL)) {
		DWORD err = GetLastError();
		if (err != ERROR_ALREADY_EXISTS) {
			fprintf(stderr, "Failed to create saves directory %s: %lu\n", savesPath, err);
			return 1;
		}
	}
#else
	homedir = getenv("HOME");
	if (homedir == NULL) {
		fprintf(stderr, "Error: Could not get HOME directory\n");
		return 1;
	}

	// Build PNB directory path
	result = snprintf(pnbPath, sizeof(pnbPath), "%s%s.pnb", homedir, SEPARATOR);
	if (result < 0 || result >= (int)sizeof(pnbPath)) {
		fprintf(stderr, "Error: Path too long for .pnb directory\n");
		return 1;
	}

	if (mkdir(pnbPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		if (errno != EEXIST) {
			fprintf(stderr, "Failed to create directory %s: %s\n", pnbPath, strerror(errno));
			return 1;
		}
	}

	// Build saves directory path
	result = snprintf(savesPath, sizeof(savesPath), "%s%ssaves", pnbPath, SEPARATOR);
	if (result < 0 || result >= (int)sizeof(savesPath)) {
		fprintf(stderr, "Error: Path too long for saves directory\n");
		return 1;
	}

	if (mkdir(savesPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		if (errno != EEXIST) {
			fprintf(stderr, "Failed to create saves directory %s: %s\n", savesPath, strerror(errno));
			return 1;
		}
	}
#endif

	return 0;
}

int platform_get_save_path(char* result, size_t max_len, int slot)
{
	char* homedir;
	int snprintf_result;

#ifdef _WIN32
	homedir = getenv("USERPROFILE");
	if (homedir == NULL) {
		fprintf(stderr, "Error: Could not get user profile directory\n");
		return 1;
	}

	// Build the full path directly to avoid incremental path construction
	snprintf_result = snprintf(result, max_len, "%s%sAppData%sLocal%sPNB%ssaves%scup_%d.xml",
	                           homedir, SEPARATOR, SEPARATOR, SEPARATOR, SEPARATOR, SEPARATOR, slot);
#else
	homedir = getenv("HOME");
	if (homedir == NULL) {
		fprintf(stderr, "Error: Could not get HOME directory\n");
		return 1;
	}

	// Build the full path directly
	snprintf_result = snprintf(result, max_len, "%s%s.pnb%ssaves%scup_%d.xml",
	                           homedir, SEPARATOR, SEPARATOR, SEPARATOR, slot);
#endif

	if (snprintf_result < 0 || (size_t)snprintf_result >= max_len) {
		fprintf(stderr, "Error: Save file path too long\n");
		return 1;
	}

	return 0;
}
