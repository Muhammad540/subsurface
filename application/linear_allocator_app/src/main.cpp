#include "raylib.h"
#include "raymath.h"
#include "linear_allocator.hpp"
#include <vector>
#include <chrono>

using namespace std;

constexpr int NUM_PARTICLES = 1'000'000;
constexpr int TOTAL_RUNS = 200;
#define STAR_COUNT 420

struct Particle{
    float x,y,vx,vy;
};

volatile float dummy = 0;

Arena g_arena;
// setting aside a 50MB backing memory
static unsigned char g_arena_memory[1024*1024*50];

double RunVectorBench() {
    auto t0 = std::chrono::high_resolution_clock::now();
    
    std::vector<Particle> particles;
    particles.reserve(NUM_PARTICLES); 

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles.push_back({ (float)i, 0.0f, 1.5f, 1.5f });
    }

    float sum = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].x += particles[i].vx;
        sum += particles[i].x;
    }
    dummy = sum;

    auto t1 = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

double RunArenaBench() {
    arena_free_all(&g_arena); // Reset bump pointer
    auto t0 = std::chrono::high_resolution_clock::now();

    Particle* particles = (Particle*)arena_alloc(&g_arena, NUM_PARTICLES * sizeof(Particle));
    if (!particles) return 0; // Guard against full arena

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i] = { (float)i, 0.0f, 1.5f, 1.5f };
    }

    float sum = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].x += particles[i].vx;
        sum += particles[i].x;
    }
    dummy = sum;

    auto t1 = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}


int main(){
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Benchmark: std::vector vs Custom Linear Arena Allocator");

    arena_init(&g_arena, g_arena_memory, sizeof(g_arena_memory));

    std::vector<double> vtimes;
    std::vector<double> atimes;

    enum State {
        BenchVector,
        BenchArena,
        Done
    };
    State state = BenchVector;
    
    double avgVector = 0;
    double avgArena = 0;

    // Animation
    Color bgColor = ColorLerp(DARKBLUE, BLACK, 0.69f);
    bool drawLines = true;
    Vector3 stars[STAR_COUNT] = { 0 };
    Vector2 starsScreenPos[STAR_COUNT] = { 0 };
    // Setup the stars with a random position
    for (int i = 0; i < STAR_COUNT; i++){
        stars[i].x = (float)GetRandomValue(-screenWidth / 2, (int)screenWidth / 2);
        stars[i].y = (float)GetRandomValue(-screenHeight / 2, (int)screenHeight / 2);
        stars[i].z = 1.0f;
    }
    SetTargetFPS(60);
    
    while(!WindowShouldClose()){
        if (state == BenchVector){
            vtimes.push_back(RunVectorBench());
            if(vtimes.size() == TOTAL_RUNS){
                state = BenchArena;
            }
        } else if (state == BenchArena) {
            atimes.push_back(RunArenaBench());
            if (atimes.size() == TOTAL_RUNS){
                avgVector = 0;
                avgArena = 0;
                for (double t : vtimes) avgVector += t;
                for (double t : atimes) avgArena += t;
                
                avgVector /= TOTAL_RUNS;
                avgArena /= TOTAL_RUNS;
                state = Done;
            }
        }

       BeginDrawing();
       ClearBackground(bgColor);

        if (state != Done){
            DrawText("Benchmarking in progress...", 270, 250, 20, GRAY);
            float speed = (state == BenchVector) ? ((float)vtimes.size()/TOTAL_RUNS)*0.5f : 0.5f + ((float)atimes.size()/TOTAL_RUNS) * 0.5f;
            float dt = GetFrameTime();
            for (int i = 0; i < STAR_COUNT; i++){
                // Update star's timer
                stars[i].z -= dt*speed;

                // Calculate the screen position
                starsScreenPos[i] = (Vector2){
                    screenWidth*0.5f + stars[i].x/stars[i].z,
                    screenHeight*0.5f + stars[i].y/stars[i].z,
                };

                // If the star is too old, or offscreen, it dies and we make a new random one
                if ((stars[i].z < 0.0f) || (starsScreenPos[i].x < 0) || (starsScreenPos[i].y < 0.0f) ||
                    (starsScreenPos[i].x > screenWidth) || (starsScreenPos[i].y > screenHeight))
                {
                    stars[i].x = (float)GetRandomValue(-screenWidth / 2, screenWidth / 2);
                    stars[i].y = (float)GetRandomValue(-screenHeight / 2, screenHeight / 2);
                    stars[i].z = 1.0f;
                }
            }
            for (int i = 0; i < STAR_COUNT; i++){
                if (drawLines)
                {
                    // Get the time a little while ago for this star, but clamp it
                    float t = Clamp(stars[i].z + 1.0f/32.0f, 0.0f, 1.0f);

                    // If it's different enough from the current time, we proceed
                    if ((t - stars[i].z) > 1e-3)
                    {
                        // Calculate the screen position of the old point
                        Vector2 startPos = (Vector2){
                            screenWidth*0.5f + stars[i].x/t,
                            screenHeight*0.5f + stars[i].y/t,
                        };

                        // Draw a line connecting the old point to the current point
                        DrawLineV(startPos, starsScreenPos[i], RAYWHITE);
                    }
                }
                else
                {
                    // Make the radius grow as the star ages
                    float radius = Lerp(stars[i].z, 1.0f, 5.0f);

                    // Draw the circle
                    DrawCircleV(starsScreenPos[i], radius, RAYWHITE);
                }
            }
        } else {
            DrawText("Avg Execution Time", 270, 100, 30, WHITE);
            int max_val = (avgVector>avgArena) ? avgVector : avgArena;

            int h_vec = (int)(((float)avgVector / max_val) * 200);
            int h_are = (int)(((float)avgArena / max_val) * 200);

            DrawRectangle(250, 400 - h_vec, 100, h_vec, ORANGE);
            DrawText("std::vector", 250, 410, 18, WHITE);
            DrawText(TextFormat("%.0f us", avgVector), 250, 380 - h_vec, 18, ORANGE);

            DrawRectangle(450, 400 - h_are, 100, h_are, GREEN);
            DrawText("Linear Arena Allocator", 450, 410, 18, WHITE);
            DrawText(TextFormat("%.0f us", avgArena), 450, 380 - h_are, 18, DARKGREEN);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}