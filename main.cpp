/**
* Author: Christina Jiang
* Assignment: Lunar Lander!
* Date due: 2025-10-25, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#include "Entity.h"

// Global Constants
constexpr int SCREEN_WIDTH  = 1000,
              SCREEN_HEIGHT = 600,
              FPS           = 120;

constexpr char    BG_COLOUR[] = "#212122";
constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

constexpr int   NUMBER_OF_LANDING       = 3;
constexpr float TILE_DIMENSION          = 40.0f,
                // in m/ms², since delta time is in ms
                ACCELERATION_OF_GRAVITY = 1.0f,
                FIXED_TIMESTEP          = 1.0f / 60.0f,
                END_GAME_THRESHOLD      = 800.0f;

// Global Variables
AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f,
      gTimeAccumulator = 0.0f;

Entity *gRocket         = nullptr;
Entity *gLanding1       = nullptr;
Entity *gMovingPlatform = nullptr;

// End overlay/bookkeeping lives in main (render phase)
bool  gEndOverlayShown   = false;
float gEndOverlayShownAt = 0.0f; // seconds

// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Physics");

    std::map<Direction, std::vector<int>> rocketAnimations = {
        {DOWN,  { 0, 1, 2, 3 }},
        {RIGHT, { 4, 5, 6, 7 }},
        {LEFT,  { 8, 9, 10, 11 }},
        {UP,    { 12, 13, 14, 15 }},
    };

    gRocket = new Entity(
        {ORIGIN.x - 300.0f, ORIGIN.y - 200.0f}, // position
        {25.0f, 25.0f},                          // scale
        "spritesheet.png",                       // sprite sheet file
        ATLAS,                                   // texture type (atlas vs single)
        { 4, 4 },                               // sprite sheet dimensions (4x4 grid)
        rocketAnimations,                        // animation atlas
        PLAYER                                   // entity type
    );

    gRocket->setColliderDimensions({
        gRocket->getScale().x,
        gRocket->getScale().y
    });
    gRocket->setAcceleration({0.0f, ACCELERATION_OF_GRAVITY});

    //STABLE(LANDING) PLATFORMS
    gLanding1 = new Entity[NUMBER_OF_LANDING];

    for (int i = 0; i < NUMBER_OF_LANDING; i++) 
    {
        gLanding1[i].setTexture("landing_area.png");
        gLanding1[i].setEntityType(PLATFORM);
        gLanding1[i].setScale({TILE_DIMENSION, TILE_DIMENSION});
        gLanding1[i].setColliderDimensions({TILE_DIMENSION, TILE_DIMENSION});
        gLanding1[i].setPosition({
            200.0f + i * 8 * (TILE_DIMENSION), 
            SCREEN_HEIGHT - TILE_DIMENSION
        });
    }

    //MOVING PLATFORM
    gMovingPlatform = new Entity();
    gMovingPlatform->setEntityType(MOVING_PLATFORM);
    gMovingPlatform->setTexture("landing_area.png");
    gMovingPlatform->setScale({ TILE_DIMENSION, TILE_DIMENSION });
    gMovingPlatform->setColliderDimensions({ TILE_DIMENSION, TILE_DIMENSION });
    gMovingPlatform->setPosition({ 400.0f, SCREEN_HEIGHT / 2 });

    SetTargetFPS(FPS);
}

void processInput() 
{
    gRocket->resetMovement();

    bool thrustApplied = false;

    if (gRocket->getFuel() >= 0.1f) {
        if      (IsKeyDown(KEY_A)) {gRocket->moveLeft();gRocket->consumeFuel(0.1f); thrustApplied = true;}
        else if (IsKeyDown(KEY_D)) {gRocket->moveRight();gRocket->consumeFuel(0.1f); thrustApplied = true;}

        // to avoid faster diagonal speed
        if (GetLength(gRocket->getMovement()) > 1.0f) 
            gRocket->normaliseMovement();
    }

    if (!thrustApplied) {
        gRocket->moveDown();
    }

    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
}

void update() 
{
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Fixed timestep
    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }
    
    while (deltaTime >= FIXED_TIMESTEP)
    {
        gRocket->update(FIXED_TIMESTEP, gLanding1, NUMBER_OF_LANDING, gMovingPlatform, 1);
        
        for (int i = 0; i < NUMBER_OF_LANDING; i++) 
        {
            gLanding1[i].update(FIXED_TIMESTEP, nullptr, 0, nullptr, 0);
        }
        deltaTime -= FIXED_TIMESTEP;
    }

    gMovingPlatform->update(FIXED_TIMESTEP, nullptr, 0, nullptr, 0);


    if (gRocket->getPosition().y > SCREEN_HEIGHT || gRocket->getPosition().x > SCREEN_WIDTH
        || gRocket->getPosition().x < 0) {
        gRocket->setMissionDecided(true);
        gRocket->setMissionSuccess(false);
    }


    if (gEndOverlayShown) {
        if (GetTime() - gEndOverlayShownAt >= 5.0f) {
            gAppStatus = TERMINATED;
        }
    }


}

void render()
{
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    gRocket->render();

    for (int i = 0; i < NUMBER_OF_LANDING;  i++) gLanding1[i].render();

    gMovingPlatform->render();


    DrawText(TextFormat("Fuel: %.1f", gRocket->getFuel()), 40, 40, 20, WHITE);
    if (!gRocket->isMissionDecided()) DrawText(TextFormat("Time: %.1f s", GetTime()), 40, 60, 20, WHITE);
   
    if (gRocket->isMissionDecided()) {
        const char* msg = gRocket->isMissionSuccess() ? "MISSION ACCOMPLISHED" : "MISSION FAILED";
        int tw = MeasureText(msg, 48);
        DrawText(msg, 200, SCREEN_HEIGHT / 2 - 48, 48, RAYWHITE);

        if (!gEndOverlayShown) {
            gEndOverlayShown   = true;
            gEndOverlayShownAt = GetTime();
        }
    }
    EndDrawing();
}

void shutdown() 
{ 
    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}
