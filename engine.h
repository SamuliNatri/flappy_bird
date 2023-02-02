#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <stdio.h>
#include <windows.h>
#include <d3d11_1.h>
#include <assert.h>
#include <time.h>

typedef struct { float X, Y, Z; } v3;
typedef struct { float R, G, B, A; } color;
typedef struct { float M[4][4]; } matrix;

typedef struct {
    ID3D11Buffer* Buffer;
    int Stride;
    int NumVertices;
    int Offset;
} mesh;

typedef struct {
    matrix Model;
    matrix View;
    matrix Projection;
    color Color;
} constants;

typedef struct {
    LARGE_INTEGER StartingCount;
    LARGE_INTEGER EndingCount;
    LARGE_INTEGER TotalCount;
    LARGE_INTEGER CountsPerSecond;
    double ElapsedMilliSeconds;
} timer;

// Globals

float DeltaTime = 1.0f / 60.0f;

mesh MeshRectangle;

v3 CameraPosition = {25.0f, 30.0f, -35.0f};

enum {
    UP, LEFT, DOWN, RIGHT, SPACE, 
    W, A, S, D, Q, E, P, M, 
    KEYSAMOUNT
};

int KeyDown[KEYSAMOUNT];
int KeyPressed[KEYSAMOUNT];

int Running = 1;

int WindowWidth = 400;
int WindowHeight = 600;
int ClientWidth;
int ClientHeight;
int ScreenWidth;
int ScreenHeight;

D3D11_VIEWPORT Viewport;

ID3D11Device1* Device;
ID3D11DeviceContext1* Context;
ID3D11Buffer* Buffer;
ID3D11Buffer* ConstantBuffer;

matrix ProjectionMatrix;
matrix ViewMatrix;

// Declarations

void Init();
void Input();
void Update();
void Draw();

void DrawOne(v3 Position, color Color, mesh Mesh);
void Debug(char *Format, ...);

v3 AddV3(v3 A, v3 B);
int IsZeroV3(v3 Vector);
int CompareV3(v3 A, v3 B);
int IsOppositeV3(v3 A, v3 B);

LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

mesh CreateMesh(float *Vertices,
                size_t Size,
                int Stride,
                int Offset) {
    
    mesh Mesh = {0};
    Mesh.Stride = Stride * sizeof(float);
    Mesh.NumVertices = Size / Stride;
    Mesh.Offset = Offset;
    
    D3D11_BUFFER_DESC BufferDesc = {
        Size,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0, 0, 0
    };
    
    D3D11_SUBRESOURCE_DATA InitialData = { Vertices };
    
    ID3D11Device1_CreateBuffer(Device,
                               &BufferDesc,
                               &InitialData,
                               &Mesh.Buffer);
    return Mesh;
}

void DrawOne(v3 Position, color Color, mesh Mesh) {
    ID3D11DeviceContext1_IASetPrimitiveTopology(Context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext1_IASetVertexBuffers(Context, 0, 1, &Mesh.Buffer, &Mesh.Stride, &Mesh.Offset);
    
    D3D11_MAPPED_SUBRESOURCE MappedSubresource;
    
    ID3D11DeviceContext1_Map(Context, (ID3D11Resource*)ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
    constants* Constants = (constants*)MappedSubresource.pData;
    Constants->Model = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        Position.X, 
        Position.Y, 
        Position.Z, 1.0f
    };
    
    Constants->View = ViewMatrix;
    Constants->Projection = ProjectionMatrix;
    Constants->Color = Color;
    
    ID3D11DeviceContext1_Unmap(Context, (ID3D11Resource*)ConstantBuffer, 0);
    ID3D11DeviceContext1_Draw(Context, Mesh.NumVertices, 0);
}

int WINAPI 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow) {
    
    srand(time(NULL));
    
    WNDCLASS WindowClass = {0};
    const char ClassName[] = "Window";
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = ClassName;
    WindowClass.hCursor = LoadCursor(NULL, IDC_CROSS);
    
    if(!RegisterClass(&WindowClass)) {
        MessageBox(0, "RegisterClass failed", 0, 0);
        return GetLastError();
    }
    
    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    HWND Window = CreateWindowEx(0, ClassName, ClassName,
                                 WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                 ScreenWidth / 2 - WindowWidth / 2,
                                 ScreenHeight / 2 - WindowHeight / 2,
                                 WindowWidth,
                                 WindowHeight,
                                 0, 0, Instance, 0);
    
    
    if(!Window) {
        MessageBox(0, "CreateWindowEx failed", 0, 0);
        return GetLastError();
    }
    
    // Get client width and height
    
    RECT ClientRect = {0};
    if(GetClientRect(Window, &ClientRect)) {
        ClientWidth = ClientRect.right;
        ClientHeight = ClientRect.bottom;
    } else {
        MessageBox(0, "GetClientRect() failed", 0, 0);
        return GetLastError();
    }
    
    // Device & Context
    
    ID3D11Device* BaseDevice;
    ID3D11DeviceContext* BaseContext;
    
    UINT CreationFlags = 0;
#ifdef _DEBUG
    CreationFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0
    };
    
    HRESULT Result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
                                       CreationFlags, FeatureLevels,
                                       ARRAYSIZE(FeatureLevels),
                                       D3D11_SDK_VERSION, &BaseDevice, 0,
                                       &BaseContext);    
    
    if(FAILED(Result)) {
        MessageBox(0, "D3D11CreateDevice failed", 0, 0);
        return GetLastError();
    }
    
    
    Result = ID3D11Device1_QueryInterface(BaseDevice, &IID_ID3D11Device1, (void**)&Device);
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(BaseDevice);
    
    Result = ID3D11DeviceContext1_QueryInterface(BaseContext, &IID_ID3D11DeviceContext1, (void**)&Context);
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(BaseContext);
    
    // Swap chain
    
    IDXGIDevice2* DxgiDevice;
    Result = ID3D11Device1_QueryInterface(Device, &IID_IDXGIDevice2, (void**)&DxgiDevice); 
    assert(SUCCEEDED(Result));
    
    IDXGIAdapter* DxgiAdapter;
    Result = IDXGIDevice2_GetAdapter(DxgiDevice, &DxgiAdapter); 
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(DxgiDevice);
    
    IDXGIFactory2* DxgiFactory;
    Result = IDXGIDevice2_GetParent(DxgiAdapter, &IID_IDXGIFactory2, (void**)&DxgiFactory); 
    assert(SUCCEEDED(Result));
    IDXGIAdapter_Release(DxgiAdapter);
    
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {0};
    SwapChainDesc.Width = 0;
    SwapChainDesc.Height = 0;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 1;
    SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    SwapChainDesc.Flags = 0;
    
    IDXGISwapChain1* SwapChain;
    Result = IDXGIFactory2_CreateSwapChainForHwnd(DxgiFactory, (IUnknown*)Device, Window,
                                                  &SwapChainDesc, 0, 0, &SwapChain);
    assert(SUCCEEDED(Result));
    IDXGIFactory2_Release(DxgiFactory);
    
    // Render target view
    
    ID3D11Texture2D* FrameBuffer;
    Result = IDXGISwapChain1_GetBuffer(SwapChain, 0, &IID_ID3D11Texture2D, (void**)&FrameBuffer);
    assert(SUCCEEDED(Result));
    
    ID3D11RenderTargetView* RenderTargetView;
    Result = ID3D11Device1_CreateRenderTargetView(Device, (ID3D11Resource*)FrameBuffer, 0, &RenderTargetView);
    assert(SUCCEEDED(Result));
    ID3D11Texture2D_Release(FrameBuffer);
    
    // Shaders
    
    ID3D10Blob* VSBlob;
    D3DCompileFromFile(L"shaders.hlsl", 0, 0, "vs_main", "vs_5_0", 0, 0, &VSBlob, 0);
    ID3D11VertexShader* VertexShader;
    Result = ID3D11Device1_CreateVertexShader(Device,
                                              ID3D10Blob_GetBufferPointer(VSBlob),
                                              ID3D10Blob_GetBufferSize(VSBlob),
                                              0,
                                              &VertexShader);
    assert(SUCCEEDED(Result));
    
    ID3D10Blob* PSBlob;
    D3DCompileFromFile(L"shaders.hlsl", 0, 0, "ps_main", "ps_5_0", 0, 0, &PSBlob, 0);
    ID3D11PixelShader* PixelShader;
    Result = ID3D11Device1_CreatePixelShader(Device,
                                             ID3D10Blob_GetBufferPointer(PSBlob),
                                             ID3D10Blob_GetBufferSize(PSBlob),
                                             0,
                                             &PixelShader);
    assert(SUCCEEDED(Result));
    
    // Data layout
    
    D3D11_INPUT_ELEMENT_DESC InputElementDesc[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
            0, 0, 
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };
    
    ID3D11InputLayout* InputLayout;
    Result = ID3D11Device1_CreateInputLayout(Device, 
                                             InputElementDesc,
                                             ARRAYSIZE(InputElementDesc),
                                             ID3D10Blob_GetBufferPointer(VSBlob),
                                             ID3D10Blob_GetBufferSize(VSBlob),
                                             &InputLayout
                                             );
    assert(SUCCEEDED(Result));
    
    // Constant buffer
    
    D3D11_BUFFER_DESC ConstantBufferDesc = {0};
    ConstantBufferDesc.ByteWidth  = sizeof(constants);
    ConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    ConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    ConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    
    Result = ID3D11Device1_CreateBuffer(Device, &ConstantBufferDesc, NULL, &ConstantBuffer);
    assert(SUCCEEDED(Result));
    
    // Viewport
    
    Viewport = (D3D11_VIEWPORT){
        .Width = (float)ClientWidth, 
        .Height = (float)ClientHeight, 
        .MaxDepth = 1.0f,
    };
    
    // Projection matrix
    
    float AspectRatio = (float)ClientWidth / (float)ClientHeight;
    float Height = 1.0f;
    float Near = 1.0f;
    float Far = 100.0f;
    
    ProjectionMatrix = (matrix){
        2.0f * Near / AspectRatio, 0.0f, 0.0f, 0.0f, 
        0.0f, 2.0f * Near / Height, 0.0f, 0.0f, 
        0.0f, 0.0f, Far / (Far - Near), 1.0f, 
        0.0f, 0.0f, Near * Far / (Near - Far), 0.0f 
    };
    
    // View matrix
    
    ViewMatrix = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -CameraPosition.X, -CameraPosition.Y, -CameraPosition.Z, 1.0f,
    };
    
    // Default meshes
    
    float RectangleVertexData[] = {
        -0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };
    
    MeshRectangle = CreateMesh(RectangleVertexData, sizeof(RectangleVertexData),
                               3, 0);
    
    Init();
    
    while(Running) {
        MSG Message;
        while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
            if(Message.message == WM_QUIT) Running = 0;
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
        Input();
        Update();
        
        // Clear
        
        float Color[] = {0.3f, 0.3f, 0.3f, 1.0f};
        ID3D11DeviceContext1_ClearRenderTargetView(Context, RenderTargetView, Color);
        
        // Set stuff
        
        ID3D11DeviceContext1_RSSetViewports(Context, 1, &Viewport);
        ID3D11DeviceContext1_OMSetRenderTargets(Context, 1, &RenderTargetView, 0);
        ID3D11DeviceContext1_IASetInputLayout(Context, InputLayout);
        ID3D11DeviceContext1_VSSetShader(Context, VertexShader, 0, 0);
        ID3D11DeviceContext1_VSSetConstantBuffers(Context, 0, 1, &ConstantBuffer);
        ID3D11DeviceContext1_PSSetShader(Context, PixelShader, 0, 0);
        
        Draw();
        
        // Swap
        
        IDXGISwapChain1_Present(SwapChain, 1, 0);
        
    }
    
    return 0;
}

int IsRepeat(LPARAM LParam) {
    return (HIWORD(LParam) & KF_REPEAT);
}

LRESULT CALLBACK 
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    switch(Message) {
        case WM_KEYUP:
        case WM_KEYDOWN: {
            int IsKeyDown = (Message == WM_KEYDOWN ? 1 : 0);
            switch(WParam) {
                case 'W': {
                    KeyDown[W] = IsKeyDown;
                } break;
                case 'A': {
                    KeyDown[A] = IsKeyDown;
                } break;
                case 'S': {
                    KeyDown[S] = IsKeyDown;
                } break;
                case 'D': {
                    KeyDown[D] = IsKeyDown;
                } break;
                case 'Q': {
                    KeyDown[Q] = IsKeyDown;
                } break;
                case 'E': {
                    KeyDown[E] = IsKeyDown;
                } break;
                case VK_SPACE: {
                    KeyDown[SPACE] = IsKeyDown;
                } break;
                case 'P': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[P] = 1;
                    } 
                } break;
                case 'M': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[M] = 1;
                    }
                } break;
                case 'O': { 
                    DestroyWindow(Window); 
                } break;
            }
        } break;
        case WM_DESTROY: { PostQuitMessage(0); } break;
        
        default: {
            return DefWindowProc(Window, Message, WParam,  LParam);
        }
    }
    
    return 0;
}


// Timer

void StartTimer(timer *Timer) {
    QueryPerformanceCounter(&Timer->StartingCount);
}

void UpdateTimer(timer *Timer) {
    // get current tick
    QueryPerformanceCounter(&Timer->EndingCount);
    // get tick count
    Timer->TotalCount.QuadPart = Timer->EndingCount.QuadPart - Timer->StartingCount.QuadPart;
    // for precision
    Timer->TotalCount.QuadPart *= 1000000;
    // calculate elapsed milliseconds
    Timer->ElapsedMilliSeconds = (double)(Timer->TotalCount.QuadPart / Timer->CountsPerSecond.QuadPart) / 1000;
}

DWORD InitTimer(timer *Timer) {
    if(!QueryPerformanceFrequency(&Timer->CountsPerSecond)) {
        return GetLastError();
    }
    StartTimer(Timer);
    return 0;
}

// Misc

v3 AddV3(v3 A, v3 B) {
    v3 Result = {0};
    Result.X += A.X + B.X;
    Result.Y += A.Y + B.Y;
    Result.Z += A.Z + B.Z;
    return Result;
}

v3 AddV3Scalar(v3 A, float B) {
    v3 Result = {0};
    Result.X += A.X + B;
    Result.Y += A.Y + B;
    Result.Z += A.Z + B;
    return Result;
}

v3 MultiplyV3Scalar(v3 A, float B) {
    v3 Result = {0};
    Result.X = A.X * B;
    Result.Y = A.Y * B;
    Result.Z = A.Z * B;
    return Result;
}

int IsZeroV3(v3 Vector) {
    if(Vector.X == 0.0f &&
       Vector.Y == 0.0f &&
       Vector.Z == 0.0f) {
        return 1; 
    }
    return 0;
}

int CompareV3(v3 A, v3 B) {
    if(A.X == B.X &&
       A.Y == B.Y &&
       A.Z == B.Z) {
        return 1; 
    }
    return 0;
}


float GetRandomZeroToOne() {
    return (float)rand() / (float)RAND_MAX ;
}

color GetRandomColor() {
    return (color){
        GetRandomZeroToOne(),
        GetRandomZeroToOne(),
        GetRandomZeroToOne(),
    };
}

color GetRandomShadeOfGray() {
    float Shade = GetRandomZeroToOne();
    return (color){
        Shade,
        Shade,
        Shade,
    };
}

void Debug(char *Format, ...) {
    va_list Arguments;
    va_start(Arguments, Format);
    char String[1024] = {0};
    vsprintf(String, Format, Arguments);
    OutputDebugString(String);
    va_end(Arguments);
}

