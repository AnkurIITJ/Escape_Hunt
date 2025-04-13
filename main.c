// ========== INCLUDES ==========
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

// ========== DEFINES ==========
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_BULLETS 20
#define DRONE_SPEED 0.1f
#define DETECTION_RANGE 35.0f
#define DRONE_COUNT 5
#define MAX_HEALTH 100
#define DRONEHIT 2
#define SHOOT_FRAMES 10
#define NEAR_DRONE 2.0f
#define DRONE_DAMAGE 0.1f
#define DISTANCE_RELOADKIT 3.0f
#define DRONE_HEIGHT 12.0f
#define DISTANCE_BETWEEN_DRONES 2.0f
#define INCREASE_HEALTH 10
#define SHOOTING_RANGE 40.0f
#define WIN_DISTANCE 5.0f

// ========== STRUCTS ==========
typedef struct
{
    Vector3 position;
    bool isActive;
    bool isrender;
    Model model;
    int hit;
    bool ispaused;
    Vector3 prevpos;
    BoundingBox bound;
} Drone;

// ========== GLOBAL VARIABLES ==========
Model health, skybox, reloadkit, map, cube, exit_game;
Sound gunshot, gunchuck, reloading, lesshealth;
Texture2D gunUI[SHOOT_FRAMES], UI, tree, redblood, screenalpha;
Image icon;

int gunFrame = 0, dronesleft;
float gunTimer = 0.0f, drone_scale = 3.0f;
bool gunAnim = false;
Drone drones[DRONE_COUNT];
int bullets = MAX_BULLETS;
float playerHealth = MAX_HEALTH;
bool isRvisible = false;
bool anydroneactive = false;
bool isHvisible = false;
Vector3 exitposition;
int minutes, seconds;

// ========== FUNCTION DECLARATIONS ==========
void LoadAssets();
void UnloadAssets();
int ispointed(BoundingBox bound, Ray cameraray);     //for reloasding and health
bool isInMap(float x, float z);     //map coordinates
bool isdronesnear(int j);  // checks  is near to this  drone with index j with drones[<j]
bool IsObstructed(Vector3 from, Vector3 to, Model mapModel);   // if a ray 
Vector3 dronepos(int j, Vector3 player);
void timeconversion(int *a, int *b);
void gamewindow();
void startmenu();
void winwindow();
void losewindow();

// ========== MAIN FUNCTION ==========
int main()
{
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    startmenu();
    return 0;
}

// ========== LOAD ASSETS ==========
void LoadAssets()
{
    health = LoadModel("./health.glb");
    skybox = LoadModel("./skybox.glb");
    gunshot = LoadSound("./gunshot.mp3");
    reloading = LoadSound("./reloading.mp3");
    gunchuck = LoadSound("./gunchuck.mp3");
    reloadkit = LoadModel("./reloadkit.glb");
    map = LoadModel("./MAP.glb");
    cube = LoadModel("./test cube 1m.glb");
    UI = LoadTexture("./UI.png");
    icon = LoadImage("./icon.png");
    exit_game = LoadModel("./exit.glb");
    tree = LoadTexture("./tree.png");
    redblood = LoadTexture("./redblood.png");
    screenalpha = LoadTexture("./screenalpha.png");
    lesshealth = LoadSound("./lessplayerhealth.mp3");

    char path[64];
    for (int i = 0; i < SHOOT_FRAMES; i++)
    {
        sprintf(path, "./gun animation shoot/shoot%d.png", i);
        gunUI[i] = LoadTexture(path);
    }
}

//============= UNLOAD ASSETS==========
void UnloadAssets()
{
    for (int i = 0; i < DRONE_COUNT; i++)
        UnloadModel(drones[i].model);
    for (int i = 0; i < SHOOT_FRAMES; i++)
        UnloadTexture(gunUI[i]);
    UnloadTexture(UI);
    UnloadModel(health);
    UnloadModel(map);
    UnloadModel(skybox);
    UnloadModel(cube);
    UnloadSound(gunshot);
    UnloadSound(reloading);
    UnloadSound(gunchuck);
    UnloadSound(lesshealth);
    UnloadModel(reloadkit);
    UnloadModel(exit_game);
    UnloadTexture(tree);
    UnloadTexture(redblood);
    UnloadTexture(screenalpha);
    CloseAudioDevice();
}

// ========== RAY-BOX COLLISION CHECK ==========
int ispointed(BoundingBox bound, Ray cameraray)
{
    RayCollision collision = GetRayCollisionBox(cameraray, bound);
    return collision.hit ? 1 : 0;
}

//==========POSITION CHECK ==============
bool isInMap(float x, float z)
{ //-+ 3 for narrowing
    if (x < 47.0f && x > -47.0f && z > -20.0f && z < 20.0f)
        return true; // for centre road
    else if (x < 47.0f && x > 33.0f && z > -60.0f && z < -20.0f)
        return true; // r1
    else if (x < -33.0f && x > -47.0f && z > -60.0f && z < -20.0f)
        return true; // r2
    else if (x < 7.0f && x > -13.0f && z > -60.0f && z < -20.0f)
        return true; // r3
    else if (x < -13.0f && x > -27.0f && z > 20.0f && z < 60.0f)
        return true; // r4
    else if (x < 27.0f && x > 13.0f && z > 20.0f && z < 60.0f)
        return true; // r5
    else
        return false;
}

bool isdronesnear(int j)
{
    for (int i = 0; i < j; i++)
    {
        float distance = Vector3Distance(drones[i].position, drones[j].position);
        if (distance < DISTANCE_BETWEEN_DRONES)
            return true;
    }
    return false;
}

Vector3 dronepos(int j, Vector3 player)
{
    Vector3 pos;
    int attempts = 0;
    do
    {
        pos = (Vector3){GetRandomValue(-50, 50) * 1.0f, DRONE_HEIGHT, GetRandomValue(-60, 60) * 1.0f};
        attempts++;
        if (attempts > 1000)
            break; // safety check
    } while (!isInMap(pos.x, pos.z) || isdronesnear(j) || Vector3Distance(player, pos) < DETECTION_RANGE);

    return pos;
}

void timeconversion(int *a, int *b)
{
    long time = GetTime();
    *a = time / 60;
    *b = time % 60;
}

bool IsObstructed(Vector3 from, Vector3 to, Model mapModel)
{
    Ray ray1 = {
        .position = from,
        .direction = Vector3Normalize(Vector3Subtract(to, from))};

    for (int i = 0; i < mapModel.meshCount; i++)
    {
        Mesh mesh = mapModel.meshes[i];
        BoundingBox box = GetMeshBoundingBox(mesh);

        // Apply model transform to the bounding box
        box.min = Vector3Transform(box.min, mapModel.transform);
        box.max = Vector3Transform(box.max, mapModel.transform);
        RayCollision inbetween = GetRayCollisionBox(ray1, box);
        if (inbetween.hit)
        {
            return true; // Something is in the way
        }
    }

    return false; // No obstruction
}

void winwindow()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "VICTORY!");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw victory message
        const char *victoryText = "VICTORY!";
        int fontSize = 100;
        int textWidth = MeasureText(victoryText, fontSize);
        DrawText(victoryText, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2 - 50, fontSize, GREEN);

        // Draw instruction
        const char *instruction = "Press ENTER key to return to menu";
        fontSize = 40;
        textWidth = MeasureText(instruction, fontSize);
        DrawText(instruction, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2 + 50, fontSize, WHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_ENTER))
        {
            break;
        }
    }

    CloseWindow();
    startmenu();
}

void losewindow()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "GAME OVER");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw game over message
        const char *gameOverText = "GAME OVER";
        int fontSize = 100;
        int textWidth = MeasureText(gameOverText, fontSize);
        DrawText(gameOverText, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2 - 50, fontSize, RED);

        // Draw instruction
        const char *instruction = "Press ENTER key to return to menu";
        fontSize = 40;
        textWidth = MeasureText(instruction, fontSize);
        DrawText(instruction, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT / 2 + 50, fontSize, WHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_ENTER))
        {
            break;
        }
    }

    CloseWindow();
    startmenu();
}

void gamewindow()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "EXIT HUNT!");
    SetWindowIcon(icon);
    UnloadImage(icon);
    InitAudioDevice();

    Camera3D camera = {
        .position = (Vector3){1.0f, 2.5f, 0.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    //=================setting position of exit and camera==============
    Vector2 fivelocation[5] = {// x,z
                               {40.0f, -59.9f},
                               {0.0f, -59.9f},
                               {-40.0f, -59.9f},
                               {20.0f, 59.9f},
                               {-20.0f, 59.9f}};

    Vector3 fourlocations[4] = {
        {15, 0, 19},
        {-15, 0, -19},
        {15, 0, 19},
        {-15, 0, -19},
    };

    //============camera and exit==============
    int random = GetRandomValue(0, 4);
    camera.position.x = fivelocation[random].x;
    camera.position.z = fivelocation[random].y;
    int random2;
    do
    {
        random2 = GetRandomValue(0, 4);
    } while (random2 == random);

    exitposition.x = fivelocation[random2].x;
    exitposition.z = fivelocation[random2].y;
    exitposition.y = 0.0f; // Ensure exit is at ground level

    LoadAssets();

    Vector3 reloadkit_position = fourlocations[GetRandomValue(0, 3)];

    BoundingBox reload_box = GetModelBoundingBox(reloadkit);
    BoundingBox base_reloadkit_box = GetModelBoundingBox(reloadkit);
    reload_box.min = Vector3Add(reloadkit_position, (Vector3Multiply)(base_reloadkit_box.min, (Vector3){2.0f, 2.0f, 2.0f}));
    reload_box.max = Vector3Add(reloadkit_position, (Vector3Multiply)(base_reloadkit_box.max, (Vector3){2.0f, 2.0f, 2.0f}));

    Vector3 health_position = reloadkit_position;

    while (Vector3Distance(health_position, reloadkit_position) == 0)
    {
        health_position = fourlocations[GetRandomValue(0, 3)];
    }

    BoundingBox health_box = GetModelBoundingBox(health);
    BoundingBox base_health_box = GetModelBoundingBox(health);
    health_box.min = Vector3Add(health_position, Vector3Multiply(base_health_box.min, (Vector3){0.1f, 0.1f, 0.1f}));
    health_box.max = Vector3Add(health_position, Vector3Multiply(base_health_box.max, (Vector3){0.1f, 0.1f, 0.1f}));

    for (int i = 0; i < DRONE_COUNT; i++)
    {
        drones[i].model = LoadModel("drone.glb");
        drones[i].isrender = true;
        drones[i].isActive = false;
        drones[i].position = dronepos(i, camera.position);
        drones[i].hit = 0;
        drones[i].ispaused = false;
        drones[i].bound = GetModelBoundingBox(drones[i].model);
    }

    SetTargetFPS(60);
    DisableCursor();

    while (!WindowShouldClose())
    {
        Vector3 oldcampos = camera.position;
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        if (!isInMap(camera.position.x, camera.position.z) && IsObstructed(oldcampos, camera.position, map))
        {
            camera.position = oldcampos;
        }
        Ray cameraray = GetMouseRay((Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, camera);
        float bob = sinf(GetTime() * 6.0f) * 0.006f;
        if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_A)))
            camera.position.y += bob;

        // === Gun Shooting ===
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bullets > 0)
        {
            PlaySound(gunshot);
            bullets--;
            gunAnim = true;
            gunFrame = 0;
            gunTimer = 0.0f;
        }

        // === Gun Animation ===
        if (gunAnim)
        {
            gunTimer += GetFrameTime();
            if (gunTimer > 0.05f)
            {
                gunTimer = 0.0f;
                gunFrame++;
                if (gunFrame >= SHOOT_FRAMES)
                {
                    gunAnim = false;
                    gunFrame = 0;
                }
            }
        }

        // === Drone Logic ===
        for (int i = 0; i < DRONE_COUNT; i++)
        {
            BoundingBox baseBox = GetModelBoundingBox(drones[i].model);
            Vector3 scale = {drone_scale, drone_scale, drone_scale};
            drones[i].bound.min = Vector3Add(drones[i].position, Vector3Multiply(baseBox.min, scale));
            drones[i].bound.max = Vector3Add(drones[i].position, Vector3Multiply(baseBox.max, scale));

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bullets > 0 && ispointed(drones[i].bound, cameraray) && Vector3Distance(camera.position, drones[i].position) < SHOOTING_RANGE)
            {
                drones[i].hit++;
                if (drones[i].hit >= DRONEHIT)
                {
                    drones[i].isrender = false;
                    drones[i].isActive = false;
                }
            }

            drones[i].ispaused = false;
            for (int j = i + 1; j < DRONE_COUNT; j++)
            {
                if (drones[i].isrender && drones[j].isrender)
                {
                    float distance = Vector3Distance(drones[i].position, drones[j].position);
                    if (distance < DISTANCE_BETWEEN_DRONES)
                    {
                        drones[i].ispaused = true;
                        break;
                    }
                }
            }

            float distance = Vector3Distance(camera.position, drones[i].position);

            if (distance < DETECTION_RANGE && drones[i].isrender && isInMap(drones[i].position.x, drones[i].position.z))
            {
                drones[i].isActive = true;
                playerHealth -= DRONE_DAMAGE;

                if (playerHealth <= 0.0f)
                {
                    playerHealth = 0.0f;
                    UnloadAssets();
                    CloseWindow();
                    losewindow();
                    return;
                }

                Vector3 dir = Vector3Normalize(Vector3Subtract(Vector3Add(camera.position, (Vector3){1.0f, 1.0f, 1.0f}), drones[i].position));
                if (distance > NEAR_DRONE && !drones[i].ispaused)
                {
                    drones[i].position = Vector3Add(drones[i].position, Vector3Scale(dir, DRONE_SPEED));
                }
                else
                {
                    drones[i].position = drones[i].prevpos;
                }
            }
            else
            {
                drones[i].isActive = false;
            }

            if (!isInMap(drones[i].position.x, drones[i].position.z))
            {
                drones[i].position = drones[i].prevpos;
            }
            drones[i].position.y = DRONE_HEIGHT;

            if (!drones[i].ispaused && isInMap(drones[i].position.x, drones[i].position.z))
            {
                drones[i].prevpos = drones[i].position;
            }
            else
            {
                drones[i].position = drones[i].prevpos;
            }
        }
        // === Reloading Logic ===
        if (ispointed(reload_box, cameraray) && Vector3Distance(camera.position, reloadkit_position) < DISTANCE_RELOADKIT)
        {
            isRvisible = true;
            if (IsKeyDown(KEY_R) && bullets < MAX_BULLETS)
            {
                PlaySound(reloading);
                bullets = MAX_BULLETS;
            }
            if (IsKeyPressed(KEY_R) && bullets == MAX_BULLETS)
            {
                PlaySound(gunchuck);
            }
        }
        else
        {
            isRvisible = false;
        }
        if (IsKeyPressed(KEY_R))
        {
            PlaySound(gunchuck);
        }

        //================healthkit==============
        if (ispointed(health_box, cameraray) && Vector3Distance(camera.position, health_position) < 5.0f)
        {
            isHvisible = true;
            if (IsKeyPressedRepeat(KEY_H))
                playerHealth += INCREASE_HEALTH;
            if (playerHealth >= MAX_HEALTH)
                playerHealth = MAX_HEALTH;
        }
        else
            isHvisible = false;

        // =========== COUNTING DRONES LEFT ==========
        dronesleft = 0;
        float alpha = 0.0f;
        anydroneactive = false;
        for (int i = 0; i < DRONE_COUNT; i++)
        {
            if (drones[i].isrender == true)
                dronesleft++;
            if (drones[i].isActive == true)
            {
                anydroneactive = true;
            }
        }

        if (anydroneactive)
        {
            alpha = 0.3f + (sin(GetTime() * 6.0f) * 0.3f);
            if (alpha < 0.0f)
                alpha = 0.0f;

            if (!IsSoundPlaying(lesshealth))
            {
                PlaySound(lesshealth);
            }
        }
        else
        {
            alpha = 0.0f;
            StopSound(lesshealth);
        }

        //======== TEXTS ============
        const char *text_bullets = TextFormat("AMMO LEFT : %d/%d", bullets, MAX_BULLETS);
        timeconversion(&minutes, &seconds);
        const char *time_text;
        if (minutes % 10 == 0 && seconds % 10 == 0)
        {
            time_text = TextFormat("Time--> 0%d:0%d", minutes, seconds);
        }
        if (minutes % 10 == 0 && seconds % 10 != 0)
        {
            time_text = TextFormat("Time--> 0%d:%d", minutes, seconds);
        }
        if (minutes % 10 != 0 && seconds % 10 == 0)
        {
            time_text = TextFormat("Time--> %d:0%d", minutes, seconds);
        }
        if (minutes % 10 != 0 && seconds % 10 != 0)
        {
            time_text = TextFormat("Time--> %d:%d", minutes, seconds);
        }

        const char *drones_text = TextFormat("Total Drones left : %d/%d", dronesleft, DRONE_COUNT);
        const char *reload_text = "PRESS R to reload";

        // ==========camera movement==========
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D))
            camera.position.y += sinf(GetTime() * 6.0f) * 0.006f;
        else
            camera.position.y += sinf(GetTime() * 2.0f) * 0.001f;

        //==================win condition check=================
        if (dronesleft == 0 && Vector3Distance(camera.position, exitposition) < WIN_DISTANCE)
        {
            UnloadAssets();
            CloseWindow();
            winwindow();
            return;
        }

        //====================drawing area==================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModel(health, health_position, 0.1f, GRAY);
        DrawModel(skybox, (Vector3){0, 0, 0}, 500.0f, WHITE);

        DrawBillboard(camera, tree, (Vector3){8, 15.0f / 2, 18}, 15.0f, WHITE);
        DrawBillboard(camera, tree, (Vector3){-8, 15.0f / 2, -18}, 15.0f, WHITE);
        DrawBillboard(camera, tree, (Vector3){8, 15.0f / 2, 18}, 15.0f, WHITE);
        DrawBillboard(camera, tree, (Vector3){-8, 15.0f / 2, -18}, 15.0f, WHITE);

        DrawModel(exit_game, exitposition, 0.5, WHITE);
        DrawModel(map, (Vector3){0, 0, 0}, 1.0f, WHITE);
        DrawModel(reloadkit, reloadkit_position, 2.0f, WHITE);

        for (int i = 0; i < DRONE_COUNT; i++)
        {
            if (drones[i].isrender)
            {
                DrawModel(drones[i].model, drones[i].position, drone_scale, WHITE);

                DrawCube((Vector3){
                             (drones[i].bound.min.x + drones[i].bound.max.x) / 2.0f,
                             DRONE_HEIGHT + 1.0f,
                             (drones[i].bound.min.z + drones[i].bound.max.z) / 2.0f},
                         DRONEHIT, 0.1f, 0.1f, GRAY);

                DrawCube((Vector3){
                             (drones[i].bound.min.x + drones[i].bound.max.x) / 2.0f,
                             DRONE_HEIGHT + 1.0f,
                             (drones[i].bound.min.z + drones[i].bound.max.z) / 2.0f},
                         DRONEHIT - drones[i].hit, 0.11f, 0.11f, RED);
            }
        }
        EndMode3D();

        // Gun UI
        int gunX = SCREEN_WIDTH - gunUI[gunFrame].width;
        int gunY = SCREEN_HEIGHT - gunUI[gunFrame].height;
        DrawTexture(gunUI[gunFrame], gunX, gunY, WHITE);

        // redblood
        DrawTexture(redblood, 0, 0, Fade(WHITE, alpha));
        DrawTexture(screenalpha, 0, 0, Fade(RED, 0.2));

        // Crosshair & HUD
        DrawRectangle(SCREEN_WIDTH - (50 + 400), SCREEN_HEIGHT - 75, 400, 20, GRAY);
        DrawRectangle(SCREEN_WIDTH - (50 + 400), SCREEN_HEIGHT - 75, playerHealth * 4, 20, RED);
        DrawText("HEALTH", SCREEN_WIDTH - 450, SCREEN_HEIGHT - 115, 20, BLACK);
        DrawTextureEx(UI, (Vector2){0, 0}, 0.0f, 1.0f, WHITE);
        DrawRectangle(611 - MeasureText(drones_text, 30), 50, MeasureText(drones_text, 30), 30, Fade(WHITE, 0.1f));
        DrawFPS(10, 20);
        DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2.0f, RED);
        DrawRectangle(SCREEN_WIDTH - 611, 50, MeasureText(text_bullets, 30), 30, Fade(WHITE, 0.1f));
        DrawText(text_bullets, SCREEN_WIDTH - 611, 50, 30, YELLOW);
        DrawText(time_text, (SCREEN_WIDTH - MeasureText(time_text, 50)) / 2, 10, 50, BLACK);
        DrawText(drones_text, 611 - MeasureText(drones_text, 30), 50, 30, YELLOW);
        if (isRvisible)
            DrawText(reload_text, 30, 200, 40, RED);
        if (isHvisible)
            DrawText("PRESS H TO INCREASE HEALTH BY 10", 30, 200, 20, RED);

        EndDrawing();
    }

    UnloadAssets();
    CloseWindow();
}

void startmenu()
{
    InitWindow(1600, 900, "Main Menu");
    InitAudioDevice();
    SetTargetFPS(60);
    DisableCursor();

    Font titleFont = LoadFont("assets/escape_font.fnt");
    Texture2D background = LoadTexture("image123.jpg");
    Music bgMusic = LoadMusicStream("assets/menu_music.mp3");
    Sound selectSound = LoadSound("assets/select.wav");

    PlayMusicStream(bgMusic);
    bgMusic.looping = true;

    int mainSelected = 0; // 0 for START, 1 for EXIT
    const char *mainOptions[2] = {"START", "EXIT"};

    bool shouldClose = false;
    bool startGame = false;

    while (!shouldClose && !WindowShouldClose())
    {
        UpdateMusicStream(bgMusic);

        // Handle input
        if (IsKeyPressed(KEY_DOWN))
        {
            PlaySound(selectSound);
            mainSelected = (mainSelected == 1) ? 0 : 1;
        }
        else if (IsKeyPressed(KEY_UP))
        {
            PlaySound(selectSound);
            mainSelected = (mainSelected == 0) ? 1 : 0;
        }

        // Handle selection
        if (IsKeyPressed(KEY_ENTER))
        {
            if (mainSelected == 1)
            { // EXIT
                shouldClose = true;
            }
            else if (mainSelected == 0)
            { // START
                startGame = true;
                shouldClose = true;
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw background image
        DrawTexture(background, 0, 0, WHITE);

        // Main Menu options
        for (int i = 0; i < 2; i++)
        {
            Color color = (i == mainSelected) ? (Color){255, 204, 0, 255} : (Color){200, 200, 200, 200};
            DrawText(mainOptions[i],
                     GetScreenWidth() / 2 - MeasureText(mainOptions[i], 40) / 2,
                     GetScreenHeight() / 1.25 + i * 60,
                     40, color);
        }

        EndDrawing();
    }

    // Cleanup
    UnloadFont(titleFont);
    UnloadMusicStream(bgMusic);
    UnloadSound(selectSound);
    UnloadTexture(background);
    CloseAudioDevice();
    CloseWindow();

    // Start game if selected
    if (startGame)
    {
        gamewindow();
    }
}
