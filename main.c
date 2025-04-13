// ========== INCLUDES ==========
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
//======= notes======
//when one test cube is imported from blender in the game..it was noticed that each block of blender is eqaul..hence it will help to model 3d map accordingly :))
// map design i was thinking that it shoud be an h shaped with vertical road type..with iit jodhpur building...size 100*120 blocks(1.0f per block)
//healthbar should end 50 from right,base of heatlth bar 75 from bottom,hieght 20each text and bar


// ========== DEFINES ==========
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_BULLETS 20
#define DRONE_SPEED 0.2f
#define DETECTION_RANGE 20.0f
#define DRONE_COUNT 1
#define MAX_HEALTH  100
#define DRONEHIT 3
#define SHOOT_FRAMES 10
#define NEAR_DRONE 2.0f
#define DRONE_DAMAGE 0.1f
#define DISTANCE_RELOADKIT 3.0f
#define DRONE_HEIGHT 12.0f
#define DISTANCE_BETWEEN_DRONES 0.5f
#define DRONE_TIMER 1.0f
#define INCREASE_HEALTH 10

// ========== STRUCTS ==========
typedef struct {
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
Model health,skybox, reloadkit,map,cube,exit_game;
Sound gunshot, gunchuck, reloading,lesshealth;
Texture2D gunUI[SHOOT_FRAMES],UI,tree,redblood,screenalpha;
Image icon;

int gunFrame = 0,dronesleft;
float gunTimer = 0.0f,drone_scale=3.0f;
bool gunAnim = false;
Drone drones[DRONE_COUNT];
int bullets = MAX_BULLETS;
float playerHealth = MAX_HEALTH;
bool isRvisible=false;
bool anydroneactive=false;
bool isHvisible=false;
Vector3 exitposition;
int minutes,seconds;

// ========== FUNCTION DECLARATIONS ==========
void LoadAssets();
void UnloadAssets();
int ispointed(BoundingBox bound, Ray cameraray);
bool isInMap(float x,float z);
bool isdronesnear(int j);
bool IsObstructed(Vector3 from, Vector3 to, Model mapModel);
Vector3 dronepos(int j,Vector3 player);
void timeconversion(int *a,int*b);
void gamewindow();
void menu();
// ========== MAIN FUNCTION ==========
int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "ESCAPE HUNT!");
  gamewindow();
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
    map=LoadModel("./MAP.glb");
    cube=LoadModel("./test cube 1m.glb");
   UI = LoadTexture("./UI.png");
   icon =LoadImage("./icon.png");
   exit_game=LoadModel("./exit.glb");
   tree=LoadTexture("./tree.png");
   redblood=LoadTexture("./redblood.png");
   screenalpha=LoadTexture("./screenalpha.png");
   lesshealth=LoadSound("./lessplayerhealth.mp3");
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
 UnloadSound(lesshealth);
 UnloadModel(reloadkit);
 UnloadModel(exit_game);
 UnloadTexture(tree);
 UnloadTexture(redblood);
 UnloadTexture(screenalpha);
 CloseAudioDevice();
 CloseWindow();
}
// ========== RAY-BOX COLLISION CHECK ==========
int ispointed(BoundingBox bound, Ray cameraray) {
    RayCollision collision = GetRayCollisionBox(cameraray, bound);
    return collision.hit ? 1 : 0;
}

//==========POSITION CHECK ==============
bool isInMap(float x,float z){   //-+ 3 for narrowing
    if(x<47.0f && x>-47.0f && z>-20.0f && z<20.0f)return true;  //for centre road
    else if(x<47.0f && x>33.0f && z>-60.0f && z<-20.0f)return true; //r1
    else if(x<-33.0f && x>-47.0f && z>-60.0f && z<-20.0f)return true; //r2
    else if(x<7.0f && x>-13.0f && z>-60.0f && z<-20.0f)return true; //r3

    else if(x<-13.0f && x>-27.0f && z>20.0f && z<60.0f)return true; //r4
    else if(x<27.0f && x>13.0f && z>20.0f && z<60.0f)return true; //r5
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
Vector3 dronepos(int j,Vector3 player){
    Vector3 pos={0,0,0};
    while (!isInMap(pos.x,pos.z)||isdronesnear(j)||Vector3Distance(player,pos)<DETECTION_RANGE){
        pos = (Vector3){GetRandomValue(-50,50)*1.0f,DRONE_HEIGHT,GetRandomValue(-40,40)*1.0f};
    }
    return pos;
}
void timeconversion(int *a,int*b){
    long time=GetTime();
    *a=time/60;
   *b=time%60;
}

void gamewindow(){
SetTargetFPS(60);
SetWindowIcon(icon);
UnloadImage(icon);
DisableCursor();
InitAudioDevice();
Camera3D camera = {
    .position = (Vector3){ 1.0f, 2.0f, 0.0f },
    .target = (Vector3){0.0f,0.0f,0.0f},
    .up = (Vector3){ 0.0f, 1.0f, 0.0f },
    .fovy = 60.0f,
    .projection = CAMERA_PERSPECTIVE,
};
//=================setting position of exit and camera==============
Vector2 fivelocation[5]={                                //x,z
{40.0f,-59.9f},
{0.0f,-59.9f},
{-40.0f,-59.9f},
{20.0f,59.9f},
{-20.0f,59.9f}
};
Vector3 fourlocations[4]={
    {15,0,19},
    {-15,0,-19},
    {15,0,19},
    {-15,0,-19},
};
//============camera and exit==============
int random=GetRandomValue(0,4);
camera.position.x=fivelocation[random].x;
camera.position.z=fivelocation[random].y;
int random2;
do{
random2=GetRandomValue(0,4);
}
while(random2==random);
    exitposition.x=fivelocation[random2].x;
    exitposition.z=fivelocation[random2].y;

//===================================

LoadAssets();

Vector3 reloadkit_position = fourlocations[GetRandomValue(0,3)];


BoundingBox reload_box = GetModelBoundingBox(reloadkit);
BoundingBox base_reloadkit_box = GetModelBoundingBox(reloadkit);
reload_box.min = Vector3Add(reloadkit_position, base_reloadkit_box.min);
reload_box.max = Vector3Add(reloadkit_position, base_reloadkit_box.max);

Vector3 health_position=reloadkit_position;

while( Vector3Distance(health_position,reloadkit_position)==0 ){
    health_position = fourlocations[GetRandomValue(0,3)];
}


BoundingBox health_box = GetModelBoundingBox(health);
BoundingBox base_health_box = GetModelBoundingBox(health);
health_box.min = Vector3Add(health_position, Vector3Multiply(base_health_box.min,(Vector3){0.1f,0.1f,0.1f}));
health_box.max = Vector3Add(health_position, Vector3Multiply(base_health_box.max,(Vector3){0.1f,0.1f,0.1f}));


for (int i = 0; i < DRONE_COUNT; i++) {
    drones[i].model = LoadModel("drone.glb");
    drones[i].isrender = true;
    drones[i].isActive = false;
    drones[i].position = dronepos(i,camera.position);
    drones[i].hit = 0;
    drones[i].ispaused=false;
    drones[i].bound = GetModelBoundingBox(drones[i].model);
}

//=====================================
while (!WindowShouldClose()) {
    Vector3 oldcampos=camera.position;
    UpdateCamera(&camera,CAMERA_FIRST_PERSON);
    if(!isInMap(camera.position.x,camera.position.z)&& IsObstructed(oldcampos,camera.position,map)){
        camera.position=oldcampos;
    }
    Ray cameraray = GetMouseRay((Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2}, camera);
    float bob = sinf(GetTime() * 6.0f) * 0.006f;
    if((IsKeyDown(KEY_W)||IsKeyDown(KEY_A)))camera.position.y+=bob;

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
        Vector3 scale = {drone_scale,drone_scale,drone_scale};
        drones[i].bound.min = Vector3Add(drones[i].position, Vector3Multiply(baseBox.min, scale));
        drones[i].bound.max = Vector3Add(drones[i].position, Vector3Multiply(baseBox.max, scale));

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bullets > 0 && ispointed(drones[i].bound, cameraray)) {
            drones[i].hit++;
            if (drones[i].hit >= DRONEHIT) {
                drones[i].isrender = false;
                drones[i].isActive = false;
            }
        }
        //=======drone inter collison===========
        drones[i].ispaused = false;
        for (int j = i + 1; j < DRONE_COUNT; j++) {
            if (drones[i].isrender && drones[j].isrender) {
                float distance = Vector3Distance(drones[i].position, drones[j].position);
                if (distance < DISTANCE_BETWEEN_DRONES) {
                    // Apply pause to one side only to avoid conflict
                    drones[i].ispaused = true;
                    break; // Optional: only react to first nearby drone
                }
            }
        }
        
     //=============================================
        float distance = Vector3Distance(camera.position, drones[i].position);
      
        if (distance < DETECTION_RANGE && drones[i].isrender && isInMap(drones[i].position.x,drones[i].position.z)) {
            drones[i].isActive = true;
            playerHealth -= DRONE_DAMAGE;
        
            if(playerHealth<0){
                playerHealth=0.0f;
              //  UnloadAssets();
            }

            Vector3 dir = Vector3Normalize(Vector3Subtract(Vector3Add(camera.position, (Vector3){1.0f, 1.0f, 1.0f}), drones[i].position));
            if (distance > NEAR_DRONE && !drones[i].ispaused) {
                drones[i].position = Vector3Add(drones[i].position,Vector3Scale(dir,DRONE_SPEED));
            }
             else {
                drones[i].position = drones[i].prevpos;
            }
        }
         else{
            drones[i].isActive=false;
         }
        //====================collison maps===========
        if(!isInMap(drones[i].position.x,drones[i].position.z)){
            drones[i].position=drones[i].prevpos;
        }
        drones[i].position.y=DRONE_HEIGHT;
        //==================================================
        if (!drones[i].ispaused && isInMap(drones[i].position.x, drones[i].position.z)) {
            drones[i].prevpos = drones[i].position;
        } else {
            drones[i].position = drones[i].prevpos;
        }
        

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
    else {
        isRvisible=false;
    }
    if(IsKeyPressed(KEY_R)){
        PlaySound(gunchuck);
    }
    //================healthkit==============
    if (ispointed(health_box, cameraray) && Vector3Distance(camera.position,health_position) < DISTANCE_RELOADKIT){
        isHvisible=true;
        if(IsKeyPressedRepeat(KEY_H))playerHealth+=INCREASE_HEALTH;
        if(playerHealth>=MAX_HEALTH) playerHealth=MAX_HEALTH;
    }
    else isHvisible=false;
    // =========== COUNTING DRONES LEFT ==========
    dronesleft=0;
    float alpha=0.0f;
    anydroneactive=false;
    for(int i=0;i<DRONE_COUNT;i++){
        if(drones[i].isrender==true)  dronesleft++;
        if(drones[i].isActive==true){
       anydroneactive=true;
    }
}
    if (anydroneactive) {
        alpha = 0.3f+(sin(GetTime()*6.0f)*0.3f);
        if (alpha < 0.0f)alpha=0.0f;
    
        if (!IsSoundPlaying(lesshealth)) {
            PlaySound(lesshealth); // Play only once while active
        }
    } 
    else {
        alpha = 0.0f;
        StopSound(lesshealth);
    }           
         //======== TEXTS ============

      const char *text_bullets=TextFormat("AMMO LEFT : %d/%d", bullets, MAX_BULLETS);
      timeconversion( &minutes, &seconds);
      const char *time_text=TextFormat("Time--> %d:%d",minutes,seconds);
       const  char *drones_text=TextFormat("Total Drones left : %d/%d",dronesleft, DRONE_COUNT);
       const  char *reload_text="PRESS R to reload";
    // ==========camera movement==========
    if(IsKeyDown(KEY_W)||IsKeyDown(KEY_A)||IsKeyDown(KEY_S)||IsKeyDown(KEY_D))camera.position.y += sinf(GetTime() * 6.0f) * 0.006f;
    else camera.position.y += sinf(GetTime() * 2.0f) * 0.001f;
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(camera);
       // DrawGrid(100, 1.0f);
   /*   DrawModel(cube,(Vector3){50, 0, 20},1.0f,WHITE);//test cube for size compariosn
        DrawModel(cube,(Vector3){50, 0, -20},1.0f,WHITE);//test cube for size compariosn
        DrawModel(cube,(Vector3){-50, 0, 20},1.0f,WHITE);//test cube for size compariosn
        DrawModel(cube,(Vector3){-50, 0, -20},1.0f,WHITE);//test cube for size compariosn */
        DrawModel(health, health_position,0.1f, GRAY);
        DrawModel(skybox, (Vector3){0, 0, 0}, 500.0f, WHITE);

        DrawBillboard(camera,tree,(Vector3){8,15.0f/2,18},15.0f,WHITE);
        DrawBillboard(camera,tree,(Vector3){-8,15.0f/2,-18},15.0f,WHITE);
        DrawBillboard(camera,tree,(Vector3){8,15.0f/2,18},15.0f,WHITE);
        DrawBillboard(camera,tree,(Vector3){-8,15.0f/2,-18},15.0f,WHITE);

      
        

        DrawModel(exit_game,exitposition,0.5,WHITE);
        DrawModel(map,(Vector3){0, 0, 0}, 1.0f, WHITE); //2.6417 IN Y
        DrawModel(reloadkit, reloadkit_position, 1.0f, WHITE);
    //  DrawBoundingBox(reload_box, RED);
    //   DrawBoundingBox(health_box, RED);

      for (int i = 0; i < DRONE_COUNT; i++) {
            if (drones[i].isrender) {
                DrawModel(drones[i].model, drones[i].position, drone_scale, WHITE);
               // DrawBoundingBox(drones[i].bound, RED);
                DrawLine3D(drones[i].position,camera.position,RED);
              
            }
        }
            
   
    EndMode3D();

    // Gun UI
    int gunX = SCREEN_WIDTH - gunUI[gunFrame].width;
    int gunY = SCREEN_HEIGHT - gunUI[gunFrame].height;
    DrawTexture(gunUI[gunFrame], gunX, gunY, WHITE);
    //redblood
    DrawTexture(redblood,0,0,Fade(WHITE,alpha));
    DrawTexture(screenalpha,0,0,Fade(RED,0.2));
    // Crosshair & HUD
    DrawRectangle(SCREEN_WIDTH-(50+400),SCREEN_HEIGHT - 75, 400, 20, GRAY);  //length 400
    DrawRectangle(SCREEN_WIDTH-(50+400),SCREEN_HEIGHT - 75, playerHealth*4, 20, RED); 
    DrawText("HEALTH", SCREEN_WIDTH - 450, SCREEN_HEIGHT - 115, 20, BLACK);
    DrawTextureEx(UI,(Vector2){0,0},0.0f,1.0f,WHITE);  //666 width
    DrawRectangle(611-MeasureText(drones_text,30), 50,MeasureText(drones_text,30),30,Fade(WHITE,0.1f));
    DrawFPS(10, 20);
    DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 2.0f, RED);
    DrawRectangle(SCREEN_WIDTH-611,50,MeasureText(text_bullets,30),30,Fade(WHITE,0.1f));
    DrawText(text_bullets,SCREEN_WIDTH-611,50, 30, YELLOW); //start 611 form right x
    DrawText(time_text, (SCREEN_WIDTH - MeasureText(time_text,50))/2, 10, 50, BLACK);
    DrawText(drones_text, 611-MeasureText(drones_text,30), 50, 30, YELLOW); //end at 611 x
    if(isRvisible)DrawText(reload_text,30,200,40,RED);
    if(isHvisible)DrawText("PRESS H TO INCREASE HEALTH BY 10",30,200,20,RED);
   // if(isInMap(camera.position.x,camera.position.z))DrawText("in the map",30,400,40,RED);
    

    EndDrawing();
}



void UnloadAssets();
}

bool IsObstructed(Vector3 from, Vector3 to, Model mapModel) {
    Ray ray1 = {
        .position = from,
        .direction = Vector3Normalize(Vector3Subtract(to, from))
    };

    for (int i = 0; i < mapModel.meshCount; i++) {
        Mesh mesh = mapModel.meshes[i];
        BoundingBox box = GetMeshBoundingBox(mesh);

        // Apply model transform to the bounding box
        box.min = Vector3Transform(box.min, mapModel.transform);
        box.max = Vector3Transform(box.max, mapModel.transform);
        RayCollision inbetween=GetRayCollisionBox(ray1, box);
        if (inbetween.hit) {
            return true; // Something is in the way
        }
    }

    return false; // No obstruction
}

/*void menu(){
    InitAudioDevice();
    SetTargetFPS(60);
    DisableCursor();

    Font titleFont = LoadFont("assets/escape_font.fnt");

    // Load background music and sound
    Music bgMusic = LoadMusicStream("assets/menu_music.mp3");
    Sound selectSound = LoadSound("assets/select.wav");
    PlayMusicStream(bgMusic);
    bgMusic.looping = true;

    MenuState menuState = MENU_MAIN;
    MainOption mainSelected = MAIN_START;


    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const char *mainOptions[2] = {"START", "EXIT"};

    while (!WindowShouldClose()) {
        UpdateMusicStream(bgMusic);

        // Handle input
        if (menuState == MENU_MAIN) {
            if (IsKeyPressed(KEY_DOWN)) {
                PlaySound(selectSound);
                mainSelected = (mainSelected == MAIN_EXIT) ? MAIN_START : MAIN_EXIT;
            } else if (IsKeyPressed(KEY_UP)) {
                PlaySound(selectSound);
                mainSelected = (mainSelected == MAIN_START) ? MAIN_EXIT : MAIN_START;
            } else if (IsKeyPressed(KEY_RIGHT) && mainSelected == MAIN_START) {
                PlaySound(selectSound);
                gamewindow();
            }
        } 
        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // Title
        const char *titleText = "ESCAPE PROTOCOL";
        int titleSize = 80;
        Vector2 titleSizeVec = MeasureTextEx(titleFont, titleText, titleSize, 5);
        DrawTextEx(titleFont, titleText,
                   (Vector2){(screenWidth - titleSizeVec.x-705)/ 2, screenHeight / 6},
                   titleSize, 5, WHITE);

        // Main Menu
        if (menuState == MENU_MAIN) {
            for (int i = 0; i < 2; i++) {
                Color color = (i == mainSelected) ? YELLOW : GRAY;
                DrawText(mainOptions[i],
                         screenWidth / 2 - MeasureText(mainOptions[i], 40) / 2,
                         screenHeight / 2 + i * 60,
                         40, color);
            }
        }

         
        }

        EndDrawing();

    

    // Cleanup
    UnloadFont(titleFont);
    UnloadMusicStream(bgMusic);
    UnloadSound(selectSound);
    UnloadAssets();
}*/
