#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "globals.h"
#include "loadobj.h"

// Using a simple fixed-size array for now. Can be improved later.
#define MAX_RESOURCES 256

// Opaque struct for the public API
typedef struct ResourceManager ResourceManager;

ResourceManager* resource_manager_init();
void resource_manager_shutdown(ResourceManager* rm);

// Eager-loads all menu resources.
int resource_manager_load_all_menu_assets(ResourceManager* rm);

// Getter functions to retrieve already-loaded resources.
GLuint resource_manager_get_texture(ResourceManager* rm, const char* path);
GLuint resource_manager_get_model(ResourceManager* rm, const char* path);

#endif /* RESOURCE_MANAGER_H */
