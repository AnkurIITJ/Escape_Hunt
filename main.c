#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "rcamera.h"
#include "raymath.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 800
#define MAX_BULLETS 20
#define DRONE_SPEED 0.4f
#define DETECTION_RANGE 20.0f
#define DRONE 16
int HEALTHBAR_WIDTH=10,HEALTHBAR_LENGTH=1600;

typedef struct {
    Vector3 position;
    bool isActive;
    bool isrender;
    Model model;
    int hit;
} Drone;

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "3D Enemy Drone AI");
    SetTargetFPS(60);
    DisableCursor();
    InitAudioDevice();

    
    Camera3D camera = { 0 };
    camera.position = (Vector3){2.0f, 2.0f, 1.0f};
    camera.target = (Vector3){0.0f, -1.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

   
    Model gun = LoadModel("./gun.glb");
    Model health = LoadModel("./health.glb");
    Sound gunshot = LoadSound("./gunshot.mp3");


   
    int current_bullets = MAX_BULLETS;
    char ammoText[20];

    Drone drone[DRONE];
    for(int i=0;i<DRONE;i++){
        drone[i].model=LoadModel("./drone.glb");
        drone[i].isActive=false;
        drone[i].isrender=true;
        drone[i].position=(Vector3){GetRandomValue(-100.0f,100.0f),5.0f,GetRandomValue(-100.0f,100.0f)};
        drone[i].hit=0;
    }

  
    while (!WindowShouldClose()) {
       
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

       
        Vector3 gunPosition = { 
            camera.position.x+1.0f,
            camera.position.y-1.0f,
            camera.position.z-1.0f,
        };
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
            gunPosition.x=-0.0006f;
        }

        
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (current_bullets > 0) {
                PlaySound(gunshot);
                current_bullets--;
            }
        }
        if(IsKeyDown(KEY_W)||IsKeyDown(KEY_S)){
            camera.position.y += sin(7 * PI * GetTime()) * 0.007f;
        }

        for (int i = 0; i < DRONE; i++) {  // Loop through all drones
    
            // Check if drone is close to the camera target and mouse button is clicked
            if (Vector3Distance(camera.target, drone[i].position) < 5.0f && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && current_bullets!=0) {
                 drone[i].hit++;
                if(drone[i].hit==3) {
                drone[i].isActive = false;
                drone[i].isrender=false;
            }
            }
            // Check if the drone is within the detection range (anytime they are near)
            float distanceToPlayer = Vector3Distance(drone[i].position, gunPosition);  // Use player.position instead of gunPosition
            
            if (distanceToPlayer < DETECTION_RANGE) {
                drone[i].isActive = true;
                HEALTHBAR_LENGTH-=100;
                // Calculate direction towards player
                Vector3 direction = Vector3Normalize(Vector3Subtract(gunPosition, drone[i].position));
                
                // Move drone towards player with a speed limit
                drone[i].position = Vector3Add(drone[i].position, Vector3Scale(direction, DRONE_SPEED));
            } 
            else {
                // Drone stays inactive if out of range
                drone[i].isActive = false;
            }
        
            // Allow drone to be shot anytime it is within 5.0f distance of the camera or when mouse is clicked
            if (Vector3Distance(camera.target, drone[i].position) < 5.0f && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                drone[i].isrender = false;  // Disable rendering if shot
                drone[i].isActive = false;  // Deactivate the drone
            }
        
            // Optional: Prevent jitter by stopping movement if the drone is very close to the player
            if (distanceToPlayer < 1.0f) {  // Small threshold to stop movement when close
                drone[i].position = gunPosition;  // Align drone position with player to avoid jitter
            }
        }
        

        

    sprintf(ammoText, "%d/%d", current_bullets, MAX_BULLETS);
        
        BeginDrawing();
        ClearBackground(GRAY);

        BeginMode3D(camera);
        DrawPlane((Vector3){0.0f,0.0f,0.0f},(Vector2){100,100},BLACK);
        DrawLine3D(gunPosition,camera.target,WHITE);
        DrawModel(health,(Vector3){0.0f,0.0f,0.0f},0.1f,GRAY);
            DrawModel(gun, gunPosition, 0.7, WHITE);
            for(int i=0;i<DRONE;i++){
            if(drone[i].isrender)  DrawModel(drone[i].model, drone[i].position, 5, WHITE);
            }
        EndMode3D();

        DrawText(ammoText, 20, SCREEN_HEIGHT - 50, 30, WHITE);
        DrawFPS(10, 20);
        DrawCircle(SCREEN_WIDTH/2,SCREEN_HEIGHT/2,2.0f,RED);
        DrawRectangle(SCREEN_WIDTH/2,SCREEN_HEIGHT-(HEALTHBAR_WIDTH/2),HEALTHBAR_WIDTH,10000,GREEN);
       

        EndDrawing();
    }

  
    UnloadModel(gun);
    for(int i=0;i<DRONE;i++){
    UnloadModel(drone[i].model);
    }
    UnloadModel(health);
    UnloadSound(gunshot);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
