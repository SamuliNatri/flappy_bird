#include "engine.h"

#define MAX_ARRAY_LENGTH 4096
#define MAX_TRAIL_LENGTH 15
#define MAX_PIPES_AMOUNT 10

enum {BIRD, PIPE, TRAIL};

typedef struct { 
    v3 Position;
    v3 Velocity;
    v3 MaxVelocity;
    mesh Mesh;
    color Color;
    int Type;
    int Hit;
} entity;

typedef struct {
    entity Entities[MAX_ARRAY_LENGTH];
    int Length;
    int Capacity;
    int Index;
} entityArray;

typedef struct {
    float Left;
    float Right;
    float Top;
    float Bottom;
} rectangle;

// Globals

int PracticeMode = 1;
float Gravity = 9.81f;
int XTiles = 60;
int YTiles = 60;
int Pause;

timer PipeTimer;
timer TrailTimer;

color ColorBird = {0.1f, 0.9f, 0.3f, 1.0f};
color ColorPipe= {0.4f, 0.2f, 0.9f, 1.0f};
color ColorPipePractice = {0.3f, 0.3f, 0.3f, 1.0f};
color ColorPipeHit = {0.5f, 0.5f, 0.5f, 1.0f};
color ColorBackground = {0.2f, 0.2f, 0.2f, 1.0f};
color ColorBackgroundLighter = {0.21f, 0.21f, 0.21f, 1.0f};
color ColorTrail = {0.3f, 0.3f, 0.3f, 1.0f};

entity Bird;
entityArray Trail = {.Capacity = MAX_TRAIL_LENGTH};
entityArray Background = {.Capacity = MAX_ARRAY_LENGTH};
entityArray Pipes = {.Capacity = MAX_PIPES_AMOUNT};

float PipeStartX = 45.0f; 
float PipeStartY = 10.0f;
float PipeWidth = 4.0f;
float PipeHeight = 25.0f;
float PipeVerticalSpace = 10.0f;

int   PipeSpawnRate = 500; // milliseconds
int   TrailSpawnRate = 30; // milliseconds

float PipeSpeed = 20.0f;
float BirdSpeed = 2.0f;
float CameraSpeed = 50.0f;

float BirdWidth = 1.0f;
float BirdHeight = 1.0f;

mesh MeshPipe;

int RectanglesIntersect(rectangle* A, rectangle* B) {
    if(A->Left > B->Right) return 0;
    if(B->Left > A->Right) return 0;
    if(A->Bottom > B->Top) return 0;
    if(B->Bottom > A->Top) return 0;
    return 1;
}

void AddEntityToArray(entity* Entity, entityArray* Array) {
    Array->Entities[Array->Index++] = *Entity;
    if(Array->Length < Array->Capacity) {
        ++Array->Length;
    }
    if(Array->Index >= Array->Capacity) {
        Array->Index = 0;
    }
}

void DrawEntity(entity* Entity) {
    
    color Color = Entity->Color;
    
    if(PracticeMode && Entity->Type == PIPE) {
        Color = ColorPipePractice;
        if(Entity->Hit) {
            Color = ColorPipeHit;
        }
    }
    
    DrawOne(Entity->Position, Color, Entity->Mesh);
}

void Draw() {
    
    // Background
    
    for(int Index = 0; Index < Background.Length; ++Index) {
        DrawEntity(&Background.Entities[Index]);
    }
    
    // Pipes
    
    for(int Index = 0; Index < Pipes.Length; ++Index) {
        DrawEntity(&Pipes.Entities[Index]);
    }
    
    // Trail
    
    for(int Index = 0; Index < Trail.Length; ++Index) {
        DrawEntity(&Trail.Entities[Index]);
    }
    
    DrawEntity(&Bird);
}

void Init() {
    
    InitTimer(&PipeTimer);
    InitTimer(&TrailTimer);
    
    // Bird
    
    Bird = (entity){
        .Position = {20.0f, 40.0f, 0.0f},
        .Color = ColorBird,
        .Mesh = MeshRectangle,
        .Velocity = {0.0f, 0.0f, 0.0f},
        .MaxVelocity = {0.0f, 10.0f, 0.0f},
    };
    
    // Pipe mesh
    
    float PipeVertexData[] = {
        -PipeWidth / 2.0f, -PipeHeight / 2.0f, 0.0f,
        -PipeWidth / 2.0f, PipeHeight / 2.0f, 0.0f,
        PipeWidth / 2.0f, PipeHeight / 2.0f, 0.0f,
        -PipeWidth / 2.0f, -PipeHeight / 2.0f, 0.0f,
        PipeWidth / 2.0f, PipeHeight / 2.0f, 0.0f,
        PipeWidth / 2.0f, -PipeHeight / 2.0f, 0.0f,
    };
    
    MeshPipe = CreateMesh(PipeVertexData, sizeof(PipeVertexData),
                          3, 0);
    
    // Background
    
    for(int Y = 0; Y < YTiles; ++Y) {
        for(int X = 0; X < XTiles; ++X) {
            entity Entity = {
                .Position = {X, Y},
                .Color = ColorBackground,
                .Mesh = MeshRectangle,
            };
            
            // Some lighter tiles
            
            if((rand() % 100) < 3) {
                Entity.Color = ColorBackgroundLighter;
            }
            AddEntityToArray(&Entity, &Background);
        }
    }
    
}

void Input() {
    
    if(KeyPressed[P]) {
        Pause = (Pause ? 0 : 1);
        KeyPressed[P] = 0;
    }
    if(KeyPressed[M]) {
        PracticeMode = (PracticeMode ? 0 : 1);
        KeyPressed[M] = 0;
    }
    
    // Camera
    
    v3 CameraAcceleration = {0};
    
    if(KeyDown[W]) {
        CameraAcceleration.Y = 1.0f; 
    }
    if(KeyDown[A]) {
        CameraAcceleration.X = -1.0f; 
    }
    if(KeyDown[S]) {
        CameraAcceleration.Y = -1.0f; 
    }
    if(KeyDown[D]) {
        CameraAcceleration.X = 1.0f; 
    }
    if(KeyDown[Q]) {
        CameraAcceleration.Z = -1.0f; 
    }
    if(KeyDown[E]) {
        CameraAcceleration.Z = 1.0f; 
    }
    
    v3 CameraVelocity = {0};
    
    CameraVelocity = AddV3(CameraVelocity, 
                           MultiplyV3Scalar(CameraAcceleration, DeltaTime * CameraSpeed));
    CameraPosition = AddV3(CameraPosition, MultiplyV3Scalar(CameraVelocity, DeltaTime * CameraSpeed));
    
    ViewMatrix = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -CameraPosition.X, -CameraPosition.Y, -CameraPosition.Z, 1.0f,
    };
    
}

void Update() {
    
    if(Pause) return; 
    
    // Bird
    
    v3 Acceleration = {0.0f, -Gravity, 0.0f};
    
    if(KeyDown[SPACE]) { 
        Acceleration.Y = 4 * Gravity; 
    }
    
    // velocity += acceleration * dt * speed
    
    Bird.Velocity = AddV3(Bird.Velocity, 
                          MultiplyV3Scalar(Acceleration, DeltaTime * BirdSpeed));
    
    if(Bird.Velocity.Y >= Bird.MaxVelocity.Y) {
        Bird.Velocity.Y = Bird.MaxVelocity.Y;
    }
    
    // position += velocity * dt * speed
    
    Bird.Position = AddV3(Bird.Position, MultiplyV3Scalar(Bird.Velocity, DeltaTime * BirdSpeed));
    
    rectangle BirdRectangle = {
        Bird.Position.X - BirdWidth / 2.0f,
        Bird.Position.X + BirdWidth / 2.0f,
        Bird.Position.Y + BirdHeight / 2.0f,
        Bird.Position.Y - BirdHeight / 2.0f,
    };
    
    // Pipes
    
    for(int Index = 0; Index < Pipes.Length; ++Index) {
        
        entity* Pipe = &Pipes.Entities[Index];
        
        // Check collision with the bird
        
        rectangle PipeRectangle = {
            Pipe->Position.X - PipeWidth / 2.0f,
            Pipe->Position.X + PipeWidth / 2.0f,
            Pipe->Position.Y + PipeHeight / 2.0f,
            Pipe->Position.Y - PipeHeight / 2.0f,
        };
        
        if(RectanglesIntersect(&BirdRectangle, &PipeRectangle)) {
            
            if(PracticeMode) {
                Pipe->Hit = 1;
            } else {
                Running = 0;
            }
        }
        
        // Move pipe
        
        Pipe->Position.X -= DeltaTime * PipeSpeed;
    }
    
    // Add trail entity
    
    UpdateTimer(&TrailTimer);
    
    if(TrailTimer.ElapsedMilliSeconds > TrailSpawnRate) {
        AddEntityToArray(&(entity){
                             .Position = Bird.Position,
                             .Color = ColorTrail,
                             .Mesh = MeshRectangle,
                             .Type = TRAIL,
                         }, &Trail);
        StartTimer(&TrailTimer);
    }
    
    // Move & color trail entities
    
    for(int Index = 0; Index < Trail.Length; ++Index) {
        entity *Entity = &Trail.Entities[Index];
        
        Entity->Position.X -= DeltaTime * PipeSpeed;
        
        Entity->Color.R -= (Entity->Color.R > ColorBackground.R) ? 0.005f : 0;
        Entity->Color.G -= (Entity->Color.G > ColorBackground.G) ? 0.005f : 0;
        Entity->Color.B -= (Entity->Color.B > ColorBackground.B) ? 0.005f : 0;
    }
    
    // Spawn new pipe pair
    
    UpdateTimer(&PipeTimer);
    
    if(PipeTimer.ElapsedMilliSeconds > PipeSpawnRate) {
        float Offset = rand() % 10 - 5;
        
        AddEntityToArray(&(entity){
                             .Position = {
                                 PipeStartX, 
                                 PipeStartY + Offset,
                             },
                             .Color = ColorPipe,
                             .Mesh = MeshPipe,
                             .Type = PIPE,
                         }, &Pipes);
        
        AddEntityToArray(&(entity){
                             .Position = {
                                 PipeStartX, 
                                 PipeStartY + Offset + PipeHeight + PipeVerticalSpace,
                             },
                             .Color = ColorPipe,
                             .Mesh = MeshPipe,
                             .Type = PIPE,
                         }, &Pipes);
        
        StartTimer(&PipeTimer);
    }
    
}

