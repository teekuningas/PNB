#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource_manager.h"
#include "render.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Internal structs
typedef enum { RESOURCE_TYPE_TEXTURE, RESOURCE_TYPE_MODEL } ResourceType;

typedef struct {
    char path[256];
    ResourceType type;
    union {
        GLuint texture_id;
        GLuint display_list;
    } handle;
    MeshObject* mesh; // For models
} Resource;

struct ResourceManager {
    Resource resources[MAX_RESOURCES];
    int count;
};

// Internal functions
static Resource* find_resource(ResourceManager* rm, const char* path);
static int load_texture(GLuint* texture, const char* filename);
static int load_model(MeshObject** mesh, GLuint* displayList, const char* filename, const char* objectname);

static void add_texture_resource(ResourceManager* rm, const char* path);
static void add_model_resource(ResourceManager* rm, const char* path, const char* object_name);

// --- Public API ---

ResourceManager* resource_manager_init()
{
    ResourceManager* rm = (ResourceManager*)malloc(sizeof(ResourceManager));
    if (rm == NULL) {
        printf("Failed to allocate ResourceManager\n");
        return NULL;
    }
    rm->count = 0;
    return rm;
}

void resource_manager_shutdown(ResourceManager* rm)
{
    if (rm == NULL) return;

    for (int i = 0; i < rm->count; ++i) {
        if (rm->resources[i].type == RESOURCE_TYPE_TEXTURE) {
            glDeleteTextures(1, &rm->resources[i].handle.texture_id);
        } else if (rm->resources[i].type == RESOURCE_TYPE_MODEL) {
            glDeleteLists(rm->resources[i].handle.display_list, 1);
            clean_mesh(rm->resources[i].mesh);
        }
    }
    free(rm);
}

GLuint resource_manager_get_texture(ResourceManager* rm, const char* path)
{
    Resource* res = find_resource(rm, path);
    if (res != NULL && res->type == RESOURCE_TYPE_TEXTURE) {
        return res->handle.texture_id;
    }
    printf("Texture not found: %s\n", path);
    return 0; // Return 0 if not found
}

GLuint resource_manager_get_model(ResourceManager* rm, const char* path)
{
    Resource* res = find_resource(rm, path);
    if (res != NULL && res->type == RESOURCE_TYPE_MODEL) {
        return res->handle.display_list;
    }
    printf("Model not found: %s\n", path);
    return 0; // Return 0 if not found
}

int resource_manager_load_all_menu_assets(ResourceManager* rm)
{
    add_texture_resource(rm, "data/textures/arrow.tga");
    add_texture_resource(rm, "data/textures/catcher.tga");
    add_texture_resource(rm, "data/textures/batter.tga");
    add_texture_resource(rm, "data/textures/cup_tree_slot.tga");
    add_texture_resource(rm, "data/textures/menu_trophy.tga");
    add_texture_resource(rm, "data/textures/team1.tga");
    add_texture_resource(rm, "data/textures/team2.tga");
    add_texture_resource(rm, "data/textures/team3.tga");
    add_texture_resource(rm, "data/textures/team4.tga");
    add_texture_resource(rm, "data/textures/team5.tga");
    add_texture_resource(rm, "data/textures/team6.tga");
    add_texture_resource(rm, "data/textures/team7.tga");
    add_texture_resource(rm, "data/textures/team8.tga");
    // Load background for menus
    add_texture_resource(rm, "data/textures/empty_background.tga");

    add_model_resource(rm, "data/models/plane.obj", "Plane");
    add_model_resource(rm, "data/models/hutunkeitto_bat.obj", "Sphere.001");
    add_model_resource(rm, "data/models/hutunkeitto_hand.obj", "Cube.001");

    printf("Loaded %d menu assets into resource manager.\n", rm->count);

    return 0;
}

int resource_manager_load_all_game_assets(ResourceManager* rm)
{
    int i;
    // Game Screen Textures
    add_texture_resource(rm, "data/textures/skybox.tga");
    add_texture_resource(rm, "data/textures/meter.tga");
    add_texture_resource(rm, "data/textures/selectionBall1.tga");
    add_texture_resource(rm, "data/textures/selectionBall2.tga");
    add_texture_resource(rm, "data/textures/selectionBall3.tga");
    add_texture_resource(rm, "data/textures/bases.tga");
    add_texture_resource(rm, "data/textures/basesMarker.tga");

    // Player Textures
    add_texture_resource(rm, "data/textures/team1.tga");
    add_texture_resource(rm, "data/textures/team2.tga");
    add_texture_resource(rm, "data/textures/team3.tga");
    add_texture_resource(rm, "data/textures/team4.tga");
    add_texture_resource(rm, "data/textures/team5.tga");
    add_texture_resource(rm, "data/textures/team6.tga");
    add_texture_resource(rm, "data/textures/team7.tga");
    add_texture_resource(rm, "data/textures/team8.tga");
    add_texture_resource(rm, "data/textures/team1_joker.tga");
    add_texture_resource(rm, "data/textures/team2_joker.tga");
    add_texture_resource(rm, "data/textures/team3_joker.tga");
    add_texture_resource(rm, "data/textures/team4_joker.tga");
    add_texture_resource(rm, "data/textures/team5_joker.tga");
    add_texture_resource(rm, "data/textures/team6_joker.tga");
    add_texture_resource(rm, "data/textures/team7_joker.tga");
    add_texture_resource(rm, "data/textures/team8_joker.tga");

    // Ball Texture
    add_texture_resource(rm, "data/textures/pallo.tga");

    // Immutable World Textures
    add_texture_resource(rm, "data/textures/fence.tga");
    add_texture_resource(rm, "data/textures/plate.tga");
    add_texture_resource(rm, "data/textures/grassTexture.tga");
    add_texture_resource(rm, "data/textures/kentta/osa1.tga");
    add_texture_resource(rm, "data/textures/kentta/osa2.tga");
    add_texture_resource(rm, "data/textures/kentta/osa3.tga");
    add_texture_resource(rm, "data/textures/kentta/osa4.tga");
    add_texture_resource(rm, "data/textures/kentta/osa5.tga");
    add_texture_resource(rm, "data/textures/kentta/osa6.tga");
    add_texture_resource(rm, "data/textures/kentta/osa7.tga");
    add_texture_resource(rm, "data/textures/kentta/osa8.tga");
    add_texture_resource(rm, "data/textures/kentta/osa9.tga");
    add_texture_resource(rm, "data/textures/kentta/osa10.tga");
    add_texture_resource(rm, "data/textures/kentta/osa11.tga");
    add_texture_resource(rm, "data/textures/kentta/osa12.tga");

    // --- Models ---
    add_model_resource(rm, "data/models/skybox.obj", "Cube");
    add_model_resource(rm, "data/models/plate.obj", "Cylinder");
    add_model_resource(rm, "data/models/plane.obj", "Plane");
    add_model_resource(rm, "data/models/pallo.obj", "Icosphere");
    add_model_resource(rm, "data/models/shadow.obj", "Circle");

    // Player Standing Models
    add_model_resource(rm, "data/models/player_bare_hands_standing.obj", "Sphere");
    add_model_resource(rm, "data/models/player_glove_with_ball_standing.obj", "Cube");
    add_model_resource(rm, "data/models/player_glove_without_ball_standing.obj", "Cube");

    // Player Animations
    char path[128];

    // Bare hands walking (16 frames)
    for (i = 1; i <= 16; i++) {
        sprintf(path, "data/models/player_bare_hands_walking/player_bare_hands_walking_%06d.obj", i);
        add_model_resource(rm, path, "Sphere");
    }
    // Bare hands running (20 frames)
    for (i = 1; i <= 20; i++) {
        sprintf(path, "data/models/player_bare_hands_running/player_bare_hands_running_%06d.obj", i);
        add_model_resource(rm, path, "Sphere");
    }
    // Glove without ball walking (16 frames)
    for (i = 1; i <= 16; i++) {
        sprintf(path, "data/models/player_glove_without_ball_walking/player_glove_without_ball_walking_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Glove with ball walking (16 frames)
    for (i = 1; i <= 16; i++) {
        sprintf(path, "data/models/player_glove_with_ball_walking/player_glove_with_ball_walking_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Glove without ball running (20 frames)
    for (i = 1; i <= 20; i++) {
        sprintf(path, "data/models/player_glove_without_ball_running/player_glove_without_ball_running_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Glove with ball running (20 frames)
    for (i = 1; i <= 20; i++) {
        sprintf(path, "data/models/player_glove_with_ball_running/player_glove_with_ball_running_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Pitch down (9 frames)
    for (i = 1; i <= 9; i++) {
        sprintf(path, "data/models/pitch/pitch_down_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Pitch up (13 frames)
    for (i = 1; i <= 13; i++) {
        sprintf(path, "data/models/pitch/pitch_up_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Throw load (11 frames)
    for (i = 1; i <= 11; i++) {
        sprintf(path, "data/models/throw/throw_load_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Throw release (21 frames)
    for (i = 1; i <= 21; i++) {
        sprintf(path, "data/models/throw/throw_release_%06d.obj", i);
        add_model_resource(rm, path, "Cube");
    }
    // Swing (34 frames)
    for (i = 1; i <= 34; i++) {
        sprintf(path, "data/models/batting/swing_%06d.obj", i);
        add_model_resource(rm, path, "Sphere");
    }
    // Bunt (34 frames)
    for (i = 1; i <= 34; i++) {
        sprintf(path, "data/models/batting/bunt_%06d.obj", i);
        add_model_resource(rm, path, "Sphere");
    }
    // Batting stop (34 frames)
    for (i = 1; i <= 34; i++) {
        sprintf(path, "data/models/batting/batting_stop_%06d.obj", i);
        add_model_resource(rm, path, "Sphere");
    }

    printf("Loaded game assets. Total resources: %d\n", rm->count);
    return 0;
}

// --- Internal Functions ---

static Resource* find_resource(ResourceManager* rm, const char* path)
{
    for (int i = 0; i < rm->count; ++i) {
        if (strcmp(rm->resources[i].path, path) == 0) {
            return &rm->resources[i];
        }
    }
    return NULL;
}

static void add_texture_resource(ResourceManager* rm, const char* path)
{
    if (rm->count >= MAX_RESOURCES) {
        printf("Resource manager full!\n");
        return;
    }
    if (find_resource(rm, path) != NULL) {
        return; // Already loaded
    }

    GLuint texture_id;
    if (load_texture(&texture_id, path) == 0) {
        Resource* res = &rm->resources[rm->count++];
        strncpy(res->path, path, sizeof(res->path) - 1);
        res->path[sizeof(res->path) - 1] = '\0';
        res->type = RESOURCE_TYPE_TEXTURE;
        res->handle.texture_id = texture_id;
        res->mesh = NULL;
    }
}

static void add_model_resource(ResourceManager* rm, const char* path, const char* object_name)
{
    if (rm->count >= MAX_RESOURCES) {
        printf("Resource manager full!\n");
        return;
    }
    if (find_resource(rm, path) != NULL) {
        return; // Already loaded
    }

    GLuint display_list;
    MeshObject* mesh;
    if (load_model(&mesh, &display_list, path, object_name) == 0) {
        Resource* res = &rm->resources[rm->count++];
        strncpy(res->path, path, sizeof(res->path) - 1);
        res->path[sizeof(res->path) - 1] = '\0';
        res->type = RESOURCE_TYPE_MODEL;
        res->handle.display_list = display_list;
        res->mesh = mesh;
    }
}

// --- Internal Functions ---

// --- Internal Loading Logic (from render.c) ---

static int load_texture(GLuint* texture, const char* filename)
{
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data == NULL) {
        printf("Couldn't load texture: %s\n", filename);
        return -1;
    }
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return 0;
}

static int load_model(MeshObject** mesh, GLuint* displayList, const char* filename, const char* objectname)
{
    *mesh = (MeshObject*)malloc(sizeof(MeshObject));
    if (*mesh == NULL) {
        printf("Failed to allocate MeshObject\n");
        return -1;
    }

    if (load_obj(filename, objectname, *mesh) != 0) {
        printf("\nError with load_obj for %s\n", filename);
        free(*mesh);
        return -1;
    }

    prepare_mesh(*mesh, displayList);
    return 0;
}
