// ========== INCLUDES ==========
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
//======= notes======
//when one test cube is imported from blender in the game..it was noticed that each block of blender is eqaul..hence it will help to model 3d map accordingly :))
// map design i was thinking that it shoud be an h shaped with vertical road type..with iit jodhpur building...size 100*120 blocks(1.0f per block)

// ========== DEFINES ==========
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_BULLETS 20
#define DRONE_SPEED 0.06f
#define DETECTION_RANGE 20.0f
#define DRONE_COUNT 10
#define MAX_HEALTH  200
#define DRONEHIT 2
#define SHOOT_FRAMES 10
#define SPEED 0.1f
#define NEAR_DRONE 2.0f
#define DISTANCE_RELOADKIT 3.0f
#define DRONE_HEIGHT 7.0f
#define DISTANCE_BETWEEN_DRONES 10.0f

// ========== STRUCTS ==========
typedef struct {
    Vector3 position;
    bool isActive;
    bool isrender;
    Model model;
    int hit;
    bool ispaused;
    BoundingBox bound;
} Drone;

// ========== GLOBAL VARIABLES ==========
Model health,skybox, reloadkit,map,cube,exit;
Sound gunshot, gunchuck, reloading;
Texture2D gunUI[SHOOT_FRAMES],UI;
Image icon;

int gunFrame = 0,dronesleft;
float gunTimer = 0.0f;
bool gunAnim = false;
Drone drones[DRONE_COUNT];
int bullets = MAX_BULLETS;
float playerHealth = MAX_HEALTH;
bool isRvisible=false;

// ========== FUNCTION DECLARATIONS ==========
void LoadAssets();
void UnloadAssets();
int ispointed(BoundingBox bound, Ray cameraray);
bool isInMap(float x,float z);
bool isdronesnear(int j);
Vector3 dronepos(int j);
// ========== MAIN FUNCTION ==========
int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "EXIT HUNT!");
    SetTargetFPS(60);
    SetWindowIcon(icon);
    UnloadImage(icon);
    DisableCursor();
    InitAudioDevice();
    Vector2 mouse=GetMousePosition();
    Camera3D camera = {
        .position = (Vector3){ 1.0f, 2.0f, 0.0f },
        .target = (Vector3){mouse.x,0.0f,mouse.y},
        .up = (Vector3){ 0.0f, 1.0f, 0.0f },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    camera.position.y += sinf(GetTime() * 2.0f) * 0.01f;

    LoadAssets();

    Vector3 reloadkit_position = { 5, 0, 5 };
    BoundingBox reload_box = GetModelBoundingBox(reloadkit);
    BoundingBox base_reloadkit_box = GetModelBoundingBox(reloadkit);
    reload_box.min = Vector3Add(reloadkit_position, base_reloadkit_box.min);
    reload_box.max = Vector3Add(reloadkit_position, base_reloadkit_box.max);

    for (int i = 0; i < DRONE_COUNT; i++) {
        drones[i].model = LoadModel("drone.glb");
        drones[i].isrender = true;
        drones[i].isActive = false;
        drones[i].position = dronepos(i);
        drones[i].hit = 0;
        drones[i].ispaused=false;
        drones[i].bound = GetModelBoundingBox(drones[i].model);
    }

    while (!WindowShouldClose()) {
        UpdateCamera(&camera,CAMERA_FIRST_PERSON);
        Ray cameraray = GetMouseRay((Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, camera);
        float bob = sinf(GetTime() * 6.0f) * 0.006f;
        if((IsKeyDown(KEY_W)||IsKeyDown(KEY_A)))camera.position.y+=bob;

     /*   //==========MOVEMENT COLLISON===========
        Vector3 future_position=camera.position;
        Vector3 forward_direction=Vector3Normalize(Vector3Subtract(camera.target,camera.position));
        Vector3 perpendicular_direction=Vector3Normalize(Vector3CrossProduct(forward_direction,(Vector3){0.0f,1.0f,0.0f}));
        if(IsKeyDown(KEY_W)) future_position=Vector3Add(camera.position,Vector3Scale(forward_direction,SPEED));
        if(IsKeyDown(KEY_S))future_position=Vector3Add(camera.position,Vector3Scale(forward_direction,-SPEED));
        if(IsKeyDown(KEY_A))
        {
            future_position=Vector3Add(camera.position,Vector3Scale(perpendicular_direction,-SPEED));
        camera.target=Vector3Add(camera.target,Vector3Scale(perpendicular_direction,-SPEED));
        }
        if(IsKeyDown(KEY_D)){
            future_position=Vector3Add(camera.position,Vector3Scale(perpendicular_direction,SPEED));
            camera.target=Vector3Add(camera.target,Vector3Scale(perpendicular_direction,SPEED));
        }
        if(1){
            camera.position=future_position;
        }
*/
        // === Gun Shooting ===
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bullets > 0) {
            PlaySound(gunshot);
            bullets--;
            gunAnim = true;
            gunFrame = 0;
            gunTimer = 0.0f;
        }

        // === Gun Animation ===
        if (gunAnim) {
            gunTimer += GetFrameTime();
            if (gunTimer > 0.05f) {
                gunTimer = 0.0f;
                gunFrame++;
                if (gunFrame >= SHOOT_FRAMES) {
                    gunAnim = false;
                    gunFrame = 0;
                }
            }
        }

        // === Drone Logic ===
        for (int i = 0; i < DRONE_COUNT; i++) {
            BoundingBox baseBox = GetModelBoundingBox(drones[i].model);
            Vector3 scale = {2.0f, 2.0f, 2.0f};
            drones[i].bound.min = Vector3Add(drones[i].position, Vector3Multiply(baseBox.min, scale));
            drones[i].bound.max = Vector3Add(drones[i].position, Vector3Multiply(baseBox.max, scale));

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bullets > 0 && ispointed(drones[i].bound, cameraray)) {
                drones[i].hit++;
                if (drones[i].hit >= DRONEHIT) {
                    drones[i].isrender = false;
                    drones[i].isActive = false;
                }
            }
            //=======drone inter collison============
            drones[i].ispaused=false;
                for(int j=0;j<DRONE_COUNT;j++){
                    if(j!=i){
                        float distance = Vector3Distance(drones[i].position,drones[j].position);
                    if(distance<DISTANCE_BETWEEN_DRONES){
                        if(drones[i].position.x<=drones[j].position.x){
                            drones[i].ispaused=true;
                            break;
                        }
                    }
                    
                }
            }
         //=============================================
            float distance = Vector3Distance(camera.position, drones[i].position);
            if (distance < DETECTION_RANGE) {
                drones[i].isActive = true;
                if(drones[i].isActive)playerHealth -= 0.1f;

                Vector3 dir = Vector3Normalize(Vector3Subtract(Vector3Add(camera.position, (Vector3){1.0f, 1.0f, 1.0f}), drones[i].position));
                if (distance > NEAR_DRONE && !drones[i].ispaused) {
                    drones[i].position = Vector3Add(drones[i].position, Vector3Scale(dir, DRONE_SPEED));
                } else {
                    drones[i].position.y += sinf(GetTime() * 4.0f) * 0.01f;
                }
            }
            drones[i].position.y=DRONE_HEIGHT;
        }
        

        // === Reloading Logic ===
        if (ispointed(reload_box, cameraray) && Vector3Distance(camera.position, reloadkit_position) < DISTANCE_RELOADKIT) {
            isRvisible=true;
            if (IsKeyDown(KEY_R) && bullets < MAX_BULLETS) {
                PlaySound(reloading);
                bullets = MAX_BULLETS;
            }
            if (IsKeyPressed(KEY_R) && bullets == MAX_BULLETS) {
                PlaySound(gunchuck);
            }
        }
        else isRvisible=false;
        if(IsKeyPressed(KEY_R))PlaySound(gunchuck);
        // =========== COUNTING DRONES LEFT ==========
        dronesleft=0;
        for(int i=0;i<DRONE_COUNT;i++){
            if(drones[i].isrender==true)
            dronesleft++;
        }
             //======== TEXTS ============
          const char *text_bullets=TextFormat("AMMO LEFT : %d/%d", bullets, MAX_BULLETS);
          const char *time_text=TextFormat("Time : %0.2f", GetTime());
           const  char *drones_text=TextFormat("Total Drones left : %d/%d",dronesleft, DRONE_COUNT);
           const  char *reload_text="PRESS R to reload";
        // ========== DRAWING ==========
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            DrawGrid(100, 1.0f);
            DrawModel(cube,(Vector3){50, 0, 20},1.0f,WHITE);//test cube for size compariosn
            DrawModel(cube,(Vector3){50, 0, -20},1.0f,WHITE);//test cube for size compariosn
            DrawModel(cube,(Vector3){-50, 0, 20},1.0f,WHITE);//test cube for size compariosn
            DrawModel(cube,(Vector3){-50, 0, -20},1.0f,WHITE);//test cube for size compariosn
            DrawModel(health, (Vector3){0, 0, 0}, 0.05f, GRAY);
            DrawModel(skybox, (Vector3){0, 0, 0}, 500.0f, WHITE);
           // DrawModel(map,(Vector3){0, -10.0f, 0}, 100.0f, WHITE);
            DrawModel(reloadkit, reloadkit_position, 1.0f, WHITE);
            DrawBoundingBox(reload_box, RED);

            for (int i = 0; i < DRONE_COUNT; i++) {
                if (drones[i].isrender) {
                    DrawModel(drones[i].model, drones[i].position, 2.0f, WHITE);
                    DrawBoundingBox(drones[i].bound, RED);
                }
            }
       
        EndMode3D();

        // Gun UI
        int gunX = SCREEN_WIDTH - gunUI[gunFrame].width;
        int gunY = SCREEN_HEIGHT - gunUI[gunFrame].height;
        DrawTexture(gunUI[gunFrame], gunX, gunY, WHITE);

        // Crosshair & HUD
        DrawTextureEx(UI,(Vector2){0,0},0.0f,1.0f,WHITE);  //666 width
        DrawFPS(10, 20);
        DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2.0f, RED);
        DrawText(text_bullets,SCREEN_WIDTH-611,50, 30, YELLOW); //start 611 form right x
       /* DrawRectangle(SCREEN_WIDTH - 420, SCREEN_HEIGHT - 40, 400, 20, GRAY);
        DrawRectangle(SCREEN_WIDTH - 450, SCREEN_HEIGHT - 80, (400 * playerHealth) / MAX_HEALTH, 20, RED); //39 27 FROM THIS
        DrawText("HEALTH", SCREEN_WIDTH - 420, SCREEN_HEIGHT - 65, 20, BLUE);*/
        DrawText(time_text, (SCREEN_WIDTH - MeasureText(time_text,50))/2, 10, 50, BLACK);
        DrawText(drones_text, 611-MeasureText(drones_text,30), 50, 30, YELLOW); //end at 611 x
        if(isRvisible)DrawText(reload_text,30,200,40,RED);
        if(isInMap(camera.position.x,camera.position.z))DrawText("in the map",30,400,40,RED);
        

        EndDrawing();
    }

    void UnloadAssets();
    return 0;
}

// ========== LOAD ASSETS ==========
void LoadAssets() {
    health = LoadModel("./health.glb");
    skybox = LoadModel("./skybox.glb");
    gunshot = LoadSound("./gunshot.mp3");
    reloading = LoadSound("./reloading.mp3");
    gunchuck = LoadSound("./gunchuck.mp3");
    reloadkit = LoadModel("./reloadkit.glb");
    map=LoadModel("./testmap.glb");
    cube=LoadModel("./test cube 1m.glb");
   UI = LoadTexture("./UI.png");
   icon =LoadImage("./icon.png");
   exit=LoadModel("./exit.glb");

    char path[64];
    for (int i = 0; i < SHOOT_FRAMES; i++) {
        sprintf(path, "./gun animation shoot/shoot%d.png", i);
        gunUI[i] = LoadTexture(path);
    }
}
//============= UNLOAD ASSETS==========
 void UnloadAssets(){
 for (int i = 0; i < DRONE_COUNT; i++) UnloadModel(drones[i].model);
 for (int i = 0; i < SHOOT_FRAMES; i++) UnloadTexture(gunUI[i]);
 UnloadTexture(UI);
 UnloadModel(health);
 UnloadModel(map);
 UnloadModel(skybox);
 UnloadModel(cube);
 UnloadSound(gunshot);
 UnloadSound(reloading);
 UnloadSound(gunchuck);
 UnloadModel(reloadkit);
 UnloadModel(exit);
 CloseAudioDevice();
 CloseWindow();
}
// ========== RAY-BOX COLLISION CHECK ==========
int ispointed(BoundingBox bound, Ray cameraray) {
    RayCollision collision = GetRayCollisionBox(cameraray, bound);
    return collision.hit ? 1 : 0;
}

//==========POSITION CHECK ==============
bool isInMap(float x,float z){
    if(x<=50.0f && x>=-50.0f && z>=-20.0f && z<=20.0f)return true;  //for centre road
    else if(x<=50.0f && x>=30.0f && z>=-40.0f && z<=-20.0f)return true; //r1
    else if(x<=-30.0f && x>=-50.0f && z>=-40.0f && z<=-20.0f)return true; //r2
    else if(x<=10.0f && x>=-10.0f && z>=-40.0f && z<=-20.0f)return true; //r3

    else if(x<=-10.0f && x>=-30.0f && z>=20.0f && z<=40.0f)return true; //r4
    else if(x<=30.0f && x>=10.0f && z>=20.0f && z<=40.0f)return true; //r5
    else return false;
}
bool isdronesnear(int j){
    for(int i=0;i<DRONE_COUNT;i++){
        if(i!=j){
            float distance = Vector3Distance(drones[i].position,drones[j].position);
            if(distance<DISTANCE_BETWEEN_DRONES) return true;
        }
    }
    return false;
}
Vector3 dronepos(int j){
    Vector3 pos = {GetRandomValue(-50,50)*1.0f,DRONE_HEIGHT,GetRandomValue(-40,40)*1.0f};
    while (isInMap(pos.x,pos.z) && !isdronesnear(j))
    {
        pos = (Vector3){GetRandomValue(-50,50)*1.0f,DRONE_HEIGHT,GetRandomValue(-40,40)*1.0f};
    }
    return pos;
}
