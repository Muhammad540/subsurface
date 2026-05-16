#include "raylib.h"
#include "raymath.h"
#include "stack_allocator.hpp"
#include <vector>
#include <cmath>
#include <chrono>

using namespace std;

constexpr int NUM_EVENTS = 10000;
constexpr int TOTAL_RUNS = 200;
#define STAR_COUNT 420

struct Position{
    float x,y,vx,vy;
};

int GetDeterministicRandom(int seed){
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return 10 + (std::abs(seed) % 90); 
}
volatile float dummy = 0;

Stack m_stack;
// setting aside a 50MB backing memory for stack allocator
static unsigned char m_stack_memory[1024*1024*50];

double RunVectorBench() {
    auto t0 = std::chrono::high_resolution_clock::now();
    float sum = 0;
    
    {
        // 1. we simulate a scene where a variable amount of data is received from a sensor 
        std::vector<std::vector<Position>> sensor_data;
        sensor_data.reserve(NUM_EVENTS); 

        for (int i = 0; i < NUM_EVENTS; i++) {
            int size = GetDeterministicRandom(i);

            std::vector<Position> event_data(size);

            for(int j=0; j<size; j++){
                event_data[j] = { (float)i, 0.0f, 1.5f, 1.5f }; 
            }

            sensor_data.push_back(event_data);
        }

        // 2. simulate the data processing
        for (int i = 0; i < NUM_EVENTS; i++) {
            for (size_t j=0; j < sensor_data[i].size(); j++){
                sensor_data[i][j].x += sensor_data[i][j].vx;
                sensor_data[i][j].y += sensor_data[i][j].vy;
                sum += (sensor_data[i][j].x + sensor_data[i][j].y); 
            }
        }
    } // 3. all the data is destoryed at this point 

    dummy = sum;
    auto t1 = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}


double RunStackBench() {
    auto t0 = std::chrono::high_resolution_clock::now();
    float sum = 0;

    Position** sensor_data = (Position**)stack_alloc(&m_stack, NUM_EVENTS * sizeof(Position*));
    int* event_sizes = (int*)stack_alloc(&m_stack, NUM_EVENTS*sizeof(int));
    
    if (!sensor_data || !event_sizes) return 0; 

    for (int i = 0; i < NUM_EVENTS; i++) {
        int size = GetDeterministicRandom(i);
        event_sizes[i] = size;
        sensor_data[i] = (Position*)stack_alloc(&m_stack, size*sizeof(Position));

        for(int j=0; j<size; j++){
            sensor_data[i][j] = { (float)i, 0.0f, 1.5f, 1.5f }; 
        }
    }

    for (int i = 0; i < NUM_EVENTS; i++) {
        for (int j=0; j < event_sizes[i]; j++){
            sensor_data[i][j].x += sensor_data[i][j].vx;
            sensor_data[i][j].y += sensor_data[i][j].vy;
            sum += (sensor_data[i][j].x + sensor_data[i][j].y); 
        }
    }

    dummy = sum;
    stack_free_all(&m_stack);

    auto t1 = std::chrono::high_resolution_clock::now();
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}


int main(){
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Benchmark: std::vector vs Custom Stack Allocator");

    stack_init(&m_stack, m_stack_memory, sizeof(m_stack_memory));


    std::vector<double> vtimes;
    std::vector<double> stimes;

    enum State {
        BenchVector,
        BenchStack,
        Done
    };
    State state = BenchVector;
    
    double avgVector = 0;
    double avgStack = 0;

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
                state = BenchStack;
            }
        } else if (state == BenchStack) {
            stimes.push_back(RunStackBench());
            if (stimes.size() == TOTAL_RUNS){
                avgVector = 0;
                avgStack = 0;
                for (double t : vtimes) avgVector += t;
                for (double t : stimes) avgStack += t;
                
                avgVector /= TOTAL_RUNS;
                avgStack /= TOTAL_RUNS;
                state = Done;
            }
        }

       BeginDrawing();
       ClearBackground(bgColor);

        if (state != Done){
            DrawText("Benchmarking in progress...", 270, 250, 20, GRAY);
            float speed = (state == BenchVector) ? ((float)vtimes.size()/TOTAL_RUNS)*0.5f : 0.5f + ((float)stimes.size()/TOTAL_RUNS) * 0.5f;
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
            int max_val = (avgVector>avgStack) ? avgVector : avgStack;

            int h_vec = (int)(((float)avgVector / max_val) * 200);
            int h_are = (int)(((float)avgStack / max_val) * 200);

            DrawRectangle(250, 400 - h_vec, 100, h_vec, ORANGE);
            DrawText("std::vector", 250, 410, 18, WHITE);
            DrawText(TextFormat("%.0f us", avgVector), 250, 380 - h_vec, 18, ORANGE);

            DrawRectangle(450, 400 - h_are, 100, h_are, GREEN);
            DrawText("Stack Allocator", 450, 410, 18, WHITE);
            DrawText(TextFormat("%.0f us", avgStack), 450, 380 - h_are, 18, DARKGREEN);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}