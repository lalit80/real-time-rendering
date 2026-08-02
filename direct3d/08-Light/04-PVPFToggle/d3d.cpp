#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma warning(disable: 4838)
#include "xnamath/xnamath.h"

#include "d3d.h"
#include "Sphere.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "Sphere.lib")

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// global variable declarations
// fullscreen
BOOL gbFullScreen = FALSE;
HWND ghwnd = NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev;

// file io
char gszLogFileName[] = "Log.txt";
FILE* gpFile = NULL;

// active window related variable
BOOL gbActiveWindow = FALSE;

// exit key press related
BOOL gbEscapKeyIsPressed = FALSE;

// d3d related global variables
IDXGISwapChain* gpIDXGISwapChain = NULL;
ID3D11Device* gpID3D11Device = NULL;
ID3D11DeviceContext* gpID3D11DeviceContext = NULL;
ID3D11RenderTargetView* gpID3D11RenderTargetView = NULL;
ID3D11DepthStencilView *gpID3D11DepthStencilView = NULL;
float clearColor[4];

ID3D11VertexShader* gpID3D11VertexShader_PP = NULL;
ID3D11PixelShader* gpID3D11PixelShader_PP = NULL;
ID3D11VertexShader* gpID3D11VertexShader_PV = NULL;
ID3D11PixelShader* gpID3D11PixelShader_PV = NULL;
ID3D11Buffer *gpID3D11Buffer_VertexBuffer_Position = NULL;
ID3D11Buffer *gpID3D11Buffer_VertexBuffer_Normal = NULL;
ID3D11Buffer *gpID3D11Buffer_IndexBuffer = NULL;
float sphere_vertices[1146];
float sphere_normals[1146];
float sphere_textures[764];
unsigned short sphere_elements[2280];
unsigned int gNumElements;
unsigned int gNumVertices;
ID3D11Buffer* gpID3D11Buffer_ConstantBuffer = NULL;

ID3D11RasterizerState* gpID3D11RasterizerState = NULL;

ID3D11InputLayout* gpID3D11InputLayout = NULL;

struct CBUFFER {
    // transformation matrices
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix;
    XMMATRIX projectionMatrix;

    // lighting related
    XMVECTOR La;
    XMVECTOR Ld;
    XMVECTOR Ls;
    XMVECTOR LightPosition;

    XMVECTOR Ka;
    XMVECTOR Kd;
    XMVECTOR Ks;
    float MaterialShininess;

    unsigned int LKeyPress;
};

XMMATRIX PerspectiveProjectionMatrix;

// light related variables
BOOL bAnimation = FALSE;
BOOL bLight = FALSE;

float lightAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
float lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
float lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
float lightPosition[] = {100.0f, 100.0f, -100.0f, 1.0f};

float materialAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
float materialDiffuse[] = {0.5f, 0.2f, 0.7f, 1.0f};
float materialSpecular[] = {0.7f, 0.7f, 0.7f, 1.0f};
float materialShininiess = 50.0f;

// rotation angles
float anglePyramid = 0.0f;
float angleCube = 0.0f;

// which shader to use
BOOL usePixel = FALSE;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // function declarations
    HRESULT initialize(void);
    void display(void);
    void update(void);
    void uninitialize(void);

    // variable declarations
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("RTR6");
    BOOL bDone = FALSE;
    HRESULT hr = S_OK;

    // code
    // create log file
    gpFile = fopen(gszLogFileName, "w");
    if(gpFile == NULL) {
        MessageBox(NULL, TEXT("Log file creation failed"), TEXT("File io error"), MB_OK);
        exit(0);
    }
    else {
        fprintf(gpFile, "Program started successfully\n");
    }

    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName = NULL;
    wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MYICON));

    RegisterClassEx(&wndclass);

    hwnd = CreateWindowEx(WS_EX_APPWINDOW, szAppName, TEXT("Lalit Choudhary"),
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, 
                            ((GetSystemMetrics(SM_CXSCREEN) - WIN_WIDTH) / 2), ((GetSystemMetrics(SM_CYSCREEN) - WIN_HEIGHT) / 2), 
                            WIN_WIDTH, WIN_HEIGHT,
                            NULL, NULL, hInstance, NULL);
    ghwnd = hwnd;

    ShowWindow(hwnd, iCmdShow);

    UpdateWindow(hwnd);

    /*while(GetMessage(&msg, NULL, 0, 0))           // get meassage waits if queue is empty PeekMessage returns quickly
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }*/

    // initialize
    hr = initialize();
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "initialize() failed\n");
        fclose(gpFile);
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "initialize() successfull\n");
        fclose(gpFile);
    }

    // set this window as foreground and active window
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    // game loop
    while(bDone == FALSE) {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                bDone = TRUE;
            }
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            if(gbActiveWindow == TRUE) {                    // don't render if another window is active
                if(gbEscapKeyIsPressed == TRUE) {
                    bDone = TRUE;
                }
                // render
                display();
                // update
                update();
            }
        }
    }

    uninitialize();

    return ((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    void toggleFullScreen(void);
    HRESULT resize(int, int);
    void uninitialize(void);
    HRESULT hr;
    
    // code
    switch(iMsg)
    {
        case WM_CREATE:
            ZeroMemory((void*)&wpPrev, sizeof(WINDOWPLACEMENT));
            wpPrev.length = sizeof(WINDOWPLACEMENT);
            break;
        
        case WM_SETFOCUS:
            gbActiveWindow = TRUE;
            break;
            
        case WM_KILLFOCUS:
            gbActiveWindow = FALSE;
            break;

        case WM_SIZE:
            if (gpID3D11DeviceContext) {
                hr = resize(LOWORD(lParam), HIWORD(lParam));             // (lower bits)width, (higher bits)height
                if(FAILED(hr)) {
                    gpFile = fopen(gszLogFileName, "a+");
                    fprintf(gpFile, "resize() failed\n");
                    fclose(gpFile);
                    return(hr);
                }
            }
            break;

        case WM_KEYDOWN:
            switch(wParam) {
                case VK_ESCAPE:
                    gbEscapKeyIsPressed = TRUE;
                    break;
                default:
                    break;
            }
            break;

        case WM_CHAR:
            switch(wParam) {
                case 'F':
                case 'f':
                    if (gbFullScreen == FALSE) {
                        toggleFullScreen();
                        gbFullScreen = TRUE;
                    }
                    else {
                        toggleFullScreen();
                        gbFullScreen = FALSE;
                    }
                    break;

                case 'a':
                case 'A':
                    if (bAnimation == FALSE) {
                        bAnimation = TRUE;
                    }
                    else {
                        bAnimation = FALSE;
                    }
                    break;

                case 'L':
                case 'l':
                    if (bLight == FALSE) {
                        bLight = TRUE;
                    }
                    else {
                        bLight = FALSE;
                    }
                    break;

                case 'p':
                case 'P':
                    usePixel = ~usePixel;
                default:
                    break;
            }
            break;

        case WM_CLOSE:
            uninitialize();
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            break;
    }
    
    return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

HRESULT initialize(void)
{
    int printDXInfo(void);
    HRESULT resize(int, int);
    // variable declarations
    HRESULT hr = S_OK;

    // code
    DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
    ZeroMemory((void*)&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    dxgiSwapChainDesc.BufferDesc.Width = WIN_WIDTH;
    dxgiSwapChainDesc.BufferDesc.Height = WIN_HEIGHT;
    dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    dxgiSwapChainDesc.BufferCount = 1;
    dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.SampleDesc.Count = 1;
    dxgiSwapChainDesc.SampleDesc.Quality = 0;
    dxgiSwapChainDesc.OutputWindow = ghwnd;
    dxgiSwapChainDesc.Windowed = TRUE;

    // get dxgi swapchain, d3d11 device, d3d11 device context supported driver and supported feature level
    D3D_DRIVER_TYPE  d3dDriverType;
    D3D_DRIVER_TYPE  d3dDriverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_SOFTWARE,
        D3D_DRIVER_TYPE_REFERENCE
    };
    D3D_FEATURE_LEVEL d3dFeatureLevel_required = D3D_FEATURE_LEVEL_11_0;
    D3D_FEATURE_LEVEL d3dFeatureLevel_acquired = D3D_FEATURE_LEVEL_10_0;
    UINT numDriverTypes = sizeof(d3dDriverTypes) / sizeof(d3dDriverTypes[0]);
    
    for (UINT i = 0; i < numDriverTypes; i++) {
        d3dDriverType = d3dDriverTypes[i];
        hr = D3D11CreateDeviceAndSwapChain(
            NULL,
            d3dDriverType,
            NULL,
            0,
            &d3dFeatureLevel_required,
            1,
            D3D11_SDK_VERSION,
            &dxgiSwapChainDesc,
            &gpIDXGISwapChain,
            &gpID3D11Device,
            &d3dFeatureLevel_acquired,
            &gpID3D11DeviceContext
        );
        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3D11CreateDeviceAndSwapChain failed\n", hr);
        fclose(gpFile);
    }

    // check which driver it uses and which feature level is acquired
    gpFile = fopen(gszLogFileName, "a+");
    if (d3dDriverType == D3D_DRIVER_TYPE_HARDWARE) fprintf(gpFile, "HARDWARE chosen\n");
    else if (d3dDriverType == D3D_DRIVER_TYPE_WARP) fprintf(gpFile, "WARP chosen\n");
    else if (d3dDriverType == D3D_DRIVER_TYPE_SOFTWARE) fprintf(gpFile, "SOFTWARE chosen\n");
    else if (d3dDriverType == D3D_DRIVER_TYPE_REFERENCE) fprintf(gpFile, "REFERENCE chosen\n");
    else fprintf(gpFile, "Unknown d3d driver type\n");

    if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_11_0) fprintf(gpFile, "D3D_FEATURE_LEVEL_11_0 acquired\n");
    else if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_1) fprintf(gpFile, "D3D_FEATURE_LEVEL_10_1 acquired\n");
    else if (d3dFeatureLevel_acquired == D3D_FEATURE_LEVEL_10_0) fprintf(gpFile, "D3D_FEATURE_LEVEL_10_0 acquired\n");
    else fprintf(gpFile, "very old feature level acquired\n");
    fclose(gpFile);

    printDXInfo();

    // vertex shader
    const char* vertexShaderSourceCode_PP =
        "cbuffer ConstantBuffer {" \
        "   float4x4 worldMatrix;" \
        "   float4x4 viewMatrix;" \
        "   float4x4 projectionMatrix;" \
        "   float4 la;" \
        "   float4 ld;" \
        "   float4 ls;" \
        "   float4 lightPosition;" \
        "   float4 ka;" \
        "   float4 kd;" \
        "   float4 ks;" \
        "   float materialShininess;" \
        "   uint lKeyIsPressed;" \
        "}" \
        "struct vertex_output {" \
        "   float4 position : SV_POSITION;" \
        "   float3 out_transformedNormals : TEXCOORD0; \n" \
        "   float3 out_lightDirection : TEXCOORD1; \n" \
        "   float3 out_viewerVector : TEXCOORD2; \n" \
        "};" \
        "vertex_output main(float4 pos : POSITION, float4 norm : NORMAL) {" \
        "   vertex_output output;" \
        "   if (lKeyIsPressed == 1) { \n" \
        "       float4 eyeCoordinates = mul(viewMatrix, mul(worldMatrix, pos));" \
        "       float3x3 normalMatrix = (float3x3)(mul(viewMatrix, worldMatrix));" \
        "       output.out_transformedNormals = mul(normalMatrix, norm.xyz); \n" \
        "       output.out_lightDirection = (lightPosition - eyeCoordinates).xyz; \n" \
        "       output.out_viewerVector = -eyeCoordinates.xyz; \n" \
        "   }" \
        "   output.position = mul(projectionMatrix, mul(viewMatrix, mul(worldMatrix, pos)));" \
        "   return output;" \
        "}";

    ID3DBlob* pID3DBlob_VertexShaderCode_PP = NULL;
    ID3DBlob* pID3DBlob_Error = NULL;
    hr = D3DCompile(vertexShaderSourceCode_PP, lstrlenA(vertexShaderSourceCode_PP) + 1,
                    "VS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0",
                    0, 0, &pID3DBlob_VertexShaderCode_PP, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for vertex shader for pp failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for vertex shader for pp successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateVertexShader(pID3DBlob_VertexShaderCode_PP->GetBufferPointer(),
                                            pID3DBlob_VertexShaderCode_PP->GetBufferSize(), NULL, &gpID3D11VertexShader_PP);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader for pp failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader for pp successful\n");
        fclose(gpFile);
    }

    // pixel shader
    const char* pixelShaderSourceCode_PP =
        "cbuffer ConstantBuffer {" \
        "   float4x4 worldMatrix;" \
        "   float4x4 viewMatrix;" \
        "   float4x4 projectionMatrix;" \
        "   float4 la;" \
        "   float4 ld;" \
        "   float4 ls;" \
        "   float4 lightPosition;" \
        "   float4 ka;" \
        "   float4 kd;" \
        "   float4 ks;" \
        "   float materialShininess;" \
        "   uint lKeyIsPressed;" \
        "}" \
        "struct vertex_output {" \
        "   float4 position : SV_POSITION;" \
        "   float3 out_transformedNormals: TEXCOORD0; \n" \
        "   float3 out_lightDirection: TEXCOORD1; \n" \
        "   float3 out_viewerVector: TEXCOORD2; \n" \
        "};" \
        "float4 main(vertex_output input) : SV_TARGET {" \
        "   float3 phong_ads_light;" \
        "   if (lKeyIsPressed == 1) { \n" \
        "       float3 normalizedTransformedNormals = normalize(input.out_transformedNormals); \n" \
        "       float3 normalizedLightDirection = normalize(input.out_lightDirection); \n" \
        "       float3 normalizedViewerVector = normalize(input.out_viewerVector); \n" \
        "       float3 ambientLight = la * ks * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n" \
        "       float3 diffuseLight = ld * kd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n" \
        "       float3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n" \
        "       float3 specularLight = ls * ks * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), materialShininess); \n" \
        "       phong_ads_light = ambientLight + diffuseLight + specularLight; \n" \
        "   }" \
        "   else { \n" \
        "       phong_ads_light = float3(1.0f, 1.0f, 1.0f); \n" \
        "   } \n" \
        "   return float4(phong_ads_light, 1.0f);" \
        "}";

    ID3DBlob* pID3DBlob_PixelShaderCode_PP = NULL;
    pID3DBlob_Error = NULL;
    hr = D3DCompile(pixelShaderSourceCode_PP, lstrlenA(pixelShaderSourceCode_PP) + 1,
                    "PS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0",
                    0, 0, &pID3DBlob_PixelShaderCode_PP, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for pixel shader for pp failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for pixel shader for pp successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreatePixelShader(pID3DBlob_PixelShaderCode_PP->GetBufferPointer(),
                                           pID3DBlob_PixelShaderCode_PP->GetBufferSize(), NULL, &gpID3D11PixelShader_PP);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader for pp failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader for pp successful\n");
        fclose(gpFile);
    }

    // vertex shader
    const char* vertexShaderSourceCode =
        "cbuffer ConstantBuffer {" \
        "   float4x4 worldMatrix;" \
        "   float4x4 viewMatrix;" \
        "   float4x4 projectionMatrix;" \
        "   float4 la;" \
        "   float4 ld;" \
        "   float4 ls;" \
        "   float4 lightPosition;" \
        "   float4 ka;" \
        "   float4 kd;" \
        "   float4 ks;" \
        "   float materialShininess;" \
        "   uint lKeyIsPressed;" \
        "}" \
        "struct vertex_output {" \
        "   float4 position : SV_POSITION;" \
        "   float4 out_phong_ads_light : COLOR;" \
        "};" \
        "vertex_output main(float4 pos : POSITION, float4 norm : NORMAL) {" \
        "   vertex_output output;" \
        "   if (lKeyIsPressed == 1) { \n" \
        "       float4 eyeCoordinates = mul(viewMatrix, mul(worldMatrix, pos));" \
        "       float3x3 normalMatrix = (float3x3)(mul(viewMatrix, worldMatrix));" \
        "       float3 transformedNormal = normalize(mul(normalMatrix, (float3)norm));" \
        "       float3 lightSource = (float3)normalize((lightPosition - eyeCoordinates));" \
        "       float3 ambientLight = la * ka * max(dot(lightSource, transformedNormal), 0.0f);" \
        "       float3 diffuseLight = ld * kd * max(dot(lightSource, transformedNormal), 0.0f);" \
        "       float3 reflectionVector = reflect(-lightSource, transformedNormal);" \
        "       float3 viewerVector = normalize(-eyeCoordinates.xyz);" \
        "       float3 specularLight = ls * ks * pow(max(dot(reflectionVector, viewerVector), 0.0f), materialShininess); \n" \
                "output.out_phong_ads_light = float4(ambientLight + diffuseLight + specularLight, 1.0f); \n" \
        "   } else {" \
        "       output.out_phong_ads_light = float4(1.0f, 1.0f, 1.0f, 1.0f);" \
        "   }" \
        "   output.position = mul(projectionMatrix, mul(viewMatrix, mul(worldMatrix, pos)));" \
        "   return output;" \
        "}";

    ID3DBlob* pID3DBlob_VertexShaderCode = NULL;
    pID3DBlob_Error = NULL;
    hr = D3DCompile(vertexShaderSourceCode, lstrlenA(vertexShaderSourceCode) + 1,
                    "VS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0",
                    0, 0, &pID3DBlob_VertexShaderCode, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for vertex shader for pv failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for vertex shader for pv successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateVertexShader(pID3DBlob_VertexShaderCode->GetBufferPointer(),
                                            pID3DBlob_VertexShaderCode->GetBufferSize(), NULL, &gpID3D11VertexShader_PV);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader for pv failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader for pv successful\n");
        fclose(gpFile);
    }

    // pixel shader
    const char* pixelShaderSourceCode =
        "struct vertex_output {" \
        "   float4 position : SV_POSITION;" \
        "   float4 out_phong_ads_light : COLOR;" \
        "};" \
        "float4 main(vertex_output input) : SV_TARGET {" \
        "   float4 color = input.out_phong_ads_light;" \
        "   return color;" \
        "}";

    ID3DBlob* pID3DBlob_PixelShaderCode = NULL;
    pID3DBlob_Error = NULL;
    hr = D3DCompile(pixelShaderSourceCode, lstrlenA(pixelShaderSourceCode) + 1,
                    "PS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0",
                    0, 0, &pID3DBlob_PixelShaderCode, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for pixel shader for pv failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for pixel shader for pv successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreatePixelShader(pID3DBlob_PixelShaderCode->GetBufferPointer(),
                                           pID3DBlob_PixelShaderCode->GetBufferSize(), NULL, &gpID3D11PixelShader_PV);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader for pv failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader for pv successful\n");
        fclose(gpFile);
    }

    // initialize input layout
    D3D11_INPUT_ELEMENT_DESC d3d11InputElementDesc[2];
    ZeroMemory((void*)d3d11InputElementDesc, sizeof(D3D11_INPUT_ELEMENT_DESC) * _ARRAYSIZE(d3d11InputElementDesc));

    d3d11InputElementDesc[0].SemanticName = "POSITION";
    d3d11InputElementDesc[0].SemanticIndex = 0;
    d3d11InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    d3d11InputElementDesc[0].InputSlot = 0;
    d3d11InputElementDesc[0].AlignedByteOffset = 0;
    d3d11InputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    d3d11InputElementDesc[0].InstanceDataStepRate = 0;

    d3d11InputElementDesc[1].SemanticName = "NORMAL";
    d3d11InputElementDesc[1].SemanticIndex = 0;
    d3d11InputElementDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    d3d11InputElementDesc[1].InputSlot = 1;
    d3d11InputElementDesc[1].AlignedByteOffset = 0;
    d3d11InputElementDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    d3d11InputElementDesc[1].InstanceDataStepRate = 0;

    hr = gpID3D11Device->CreateInputLayout(d3d11InputElementDesc, _ARRAYSIZE(d3d11InputElementDesc), pID3DBlob_VertexShaderCode_PP->GetBufferPointer(),
                                            pID3DBlob_VertexShaderCode_PP->GetBufferSize(), &gpID3D11InputLayout);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateInputLayout failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateInputLayout successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->IASetInputLayout(gpID3D11InputLayout);
    pID3DBlob_VertexShaderCode_PP->Release();
    pID3DBlob_VertexShaderCode_PP = NULL;
    pID3DBlob_PixelShaderCode_PP->Release();
    pID3DBlob_PixelShaderCode_PP = NULL;

    getSphereVertexData(sphere_vertices, sphere_normals, sphere_textures, sphere_elements);
    gNumVertices = getNumberOfSphereVertices();
    gNumElements = getNumberOfSphereElements();

    // position buffer
    D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = gNumVertices * 3 * sizeof(float);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
	ZeroMemory((void*)&d3d11SubresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	d3d11SubresourceData.pSysMem = sphere_vertices;
	hr = gpID3D11Device->CreateBuffer(&bufferDesc, &d3d11SubresourceData, &gpID3D11Buffer_VertexBuffer_Position);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for vertex buffer position failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for vertex buffer position successful\n");
        fclose(gpFile);
    }

    // normal buffer
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = gNumVertices * 3 * sizeof(float);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ZeroMemory((void*)&d3d11SubresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	d3d11SubresourceData.pSysMem = sphere_normals;
	hr = gpID3D11Device->CreateBuffer(&bufferDesc, &d3d11SubresourceData, &gpID3D11Buffer_VertexBuffer_Normal);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for vertex buffer normal failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for vertex buffer normal successful\n");
        fclose(gpFile);
    }

    // index buffer
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = gNumElements * sizeof(short);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ZeroMemory((void*)&d3d11SubresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	d3d11SubresourceData.pSysMem = sphere_elements;
	hr = gpID3D11Device->CreateBuffer(&bufferDesc, &d3d11SubresourceData, &gpID3D11Buffer_IndexBuffer);
	    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for index buffer failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for index buffer successful\n");
        fclose(gpFile);
    }

    // constant buffer
    D3D11_BUFFER_DESC d3d11BufferDesc;
    ZeroMemory(&d3d11BufferDesc, sizeof(D3D11_BUFFER_DESC));
    d3d11BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11BufferDesc.ByteWidth = sizeof(CBUFFER);
    d3d11BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gpID3D11Device->CreateBuffer(&d3d11BufferDesc, NULL, &gpID3D11Buffer_ConstantBuffer);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->VSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer);

    // set rasterizer state to disable backface culling
    D3D11_RASTERIZER_DESC d3d11RasterizerDesc;
    ZeroMemory(&d3d11RasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    d3d11RasterizerDesc.AntialiasedLineEnable = FALSE;
    d3d11RasterizerDesc.CullMode = D3D11_CULL_NONE;
    d3d11RasterizerDesc.DepthBias = 0;
    d3d11RasterizerDesc.DepthBiasClamp = 0.0f;
    d3d11RasterizerDesc.DepthClipEnable = TRUE;
    d3d11RasterizerDesc.FillMode = D3D11_FILL_SOLID;
    d3d11RasterizerDesc.FrontCounterClockwise = FALSE;
    d3d11RasterizerDesc.MultisampleEnable = FALSE;
    d3d11RasterizerDesc.ScissorEnable = FALSE;
    d3d11RasterizerDesc.SlopeScaledDepthBias = 0.0f;
    hr = gpID3D11Device->CreateRasterizerState(&d3d11RasterizerDesc, &gpID3D11RasterizerState);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateRasterizerState failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateRasterizerState successful\n");
        fclose(gpFile);
    }
    // set rasterizer state
    gpID3D11DeviceContext->RSSetState(gpID3D11RasterizerState);

    // set clear color
    clearColor[0] = 0.0f;
    clearColor[1] = 0.0f;
    clearColor[2] = 0.0f;
    clearColor[3] = 1.0f;

    PerspectiveProjectionMatrix = XMMatrixIdentity();

    hr = resize(WIN_WIDTH, WIN_HEIGHT);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "resize() failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "resize() successfull\n");
        fclose(gpFile);
    }

    return hr;
}

void display(void)
{
    // code
    
    if (usePixel) {
        gpID3D11DeviceContext->VSSetShader(gpID3D11VertexShader_PP, 0, 0);
        gpID3D11DeviceContext->PSSetShader(gpID3D11PixelShader_PP, 0, 0);
    } else {
        gpID3D11DeviceContext->VSSetShader(gpID3D11VertexShader_PV, 0, 0);
        gpID3D11DeviceContext->PSSetShader(gpID3D11PixelShader_PV, 0, 0);
    }

    gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView, clearColor);
    gpID3D11DeviceContext->ClearDepthStencilView(gpID3D11DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // render cube
    // bind vertex buffer for position
    UINT stride = sizeof(float) * 3;
    UINT offset = 0;
    gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_VertexBuffer_Position, &stride, &offset);
    gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // bind vertex buffer for normal
    stride = sizeof(float) * 3;
    offset = 0;
    gpID3D11DeviceContext->IASetVertexBuffers(1, 1, &gpID3D11Buffer_VertexBuffer_Normal, &stride, &offset);

    // bind index buffer
    gpID3D11DeviceContext->IASetIndexBuffer(gpID3D11Buffer_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    // transformations
    XMMATRIX worldMatrix = XMMatrixIdentity();
    XMMATRIX viewMatrix = XMMatrixIdentity();
    XMMATRIX translationMatrix = XMMatrixIdentity();
    translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 3.0f);
    worldMatrix = translationMatrix;

    XMMATRIX wvpMatrix = worldMatrix * viewMatrix * PerspectiveProjectionMatrix;

    // push data to vertex shader
    CBUFFER constantBuffer;
    ZeroMemory(&constantBuffer, sizeof(CBUFFER));
    constantBuffer.worldMatrix = worldMatrix;
    constantBuffer.viewMatrix = viewMatrix;
    constantBuffer.projectionMatrix = PerspectiveProjectionMatrix;
    if(bLight == TRUE) {
        constantBuffer.La = XMVectorSet(lightAmbient[0], lightAmbient[1], lightAmbient[2], lightAmbient[3]);
        constantBuffer.Ld = XMVectorSet(lightDiffuse[0], lightDiffuse[1], lightDiffuse[2], lightDiffuse[3]);
        constantBuffer.Ls = XMVectorSet(lightSpecular[0], lightSpecular[1], lightSpecular[2], lightSpecular[3]);
        constantBuffer.LightPosition = XMVectorSet(lightPosition[0], lightPosition[1], lightPosition[2], lightPosition[3]);
        constantBuffer.Ka = XMVectorSet(materialAmbient[0], materialAmbient[1], materialAmbient[2], materialAmbient[3]);
        constantBuffer.Kd = XMVectorSet(materialDiffuse[0], materialDiffuse[1], materialDiffuse[2], materialDiffuse[3]);
        constantBuffer.Ks = XMVectorSet(materialSpecular[0], materialSpecular[1], materialSpecular[2], materialSpecular[3]);
        constantBuffer.MaterialShininess = materialShininiess;
        constantBuffer.LKeyPress = 1;
    } else {
        constantBuffer.LKeyPress = 0;
    }
    
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer, 0, NULL, &constantBuffer, 0, 0);
    
    // Bind to shaders
    gpID3D11DeviceContext->VSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer);
    gpID3D11DeviceContext->PSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer);

    // draw sphere
    gpID3D11DeviceContext->DrawIndexed(gNumElements, 0, 0);

    // swap front and back buffers
    gpIDXGISwapChain->Present(0, 0);
}

void update(void)
{
    // code
    if (bAnimation == TRUE) {
        anglePyramid += 0.01f;
        angleCube += 0.01f;
    }
}

HRESULT resize(int width, int height)
{
    // code
    HRESULT hr = S_OK;

    // release depth stencil view
    if(gpID3D11DepthStencilView) {
        gpID3D11DepthStencilView->Release();
        gpID3D11DepthStencilView = NULL;
    }
    
    // release existing render target view
    if(gpID3D11RenderTargetView) {
        gpID3D11RenderTargetView->Release();
        gpID3D11RenderTargetView = NULL;
    }

    // resize swap chain buffers accordingly
    gpIDXGISwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

    // get buffer from render target view
    ID3D11Texture2D* pID3D11Texture2D_BackBuffer = NULL;
    gpIDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pID3D11Texture2D_BackBuffer);

    // create render target view using above back buffer
    hr = gpID3D11Device->CreateRenderTargetView(pID3D11Texture2D_BackBuffer, NULL, &gpID3D11RenderTargetView);
    if(FAILED(hr)) {
        pID3D11Texture2D_BackBuffer->Release();
        pID3D11Texture2D_BackBuffer = NULL;
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateRenderTargetView() failed\n");
        fclose(gpFile);
        return hr;
    }
    pID3D11Texture2D_BackBuffer->Release();
    pID3D11Texture2D_BackBuffer = NULL;

    D3D11_TEXTURE2D_DESC d3d11Texture2DDesc;
    ZeroMemory(&d3d11Texture2DDesc, sizeof(D3D11_TEXTURE2D_DESC));
    d3d11Texture2DDesc.Width = (UINT)width;
    d3d11Texture2DDesc.Height = (UINT)height;
    d3d11Texture2DDesc.ArraySize = 1;
    d3d11Texture2DDesc.MipLevels = 1;
    d3d11Texture2DDesc.SampleDesc.Count = 1;
    d3d11Texture2DDesc.SampleDesc.Quality = 0;
    d3d11Texture2DDesc.Format = DXGI_FORMAT_D32_FLOAT;
    d3d11Texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11Texture2DDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    // create 2d texture from above structure
    ID3D11Texture2D* pID3D11Texture2D_DepthStencilBuffer = NULL;
    hr = gpID3D11Device->CreateTexture2D(&d3d11Texture2DDesc, NULL, &pID3D11Texture2D_DepthStencilBuffer);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateTexture2D for depth buffer failed\n");
        fclose(gpFile);
        return hr;
    }

    // create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC d3d11DepthStencilViewDesc;
    ZeroMemory(&d3d11DepthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    d3d11DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    d3d11DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    hr = gpID3D11Device->CreateDepthStencilView(pID3D11Texture2D_DepthStencilBuffer, &d3d11DepthStencilViewDesc, &gpID3D11DepthStencilView);
    if(FAILED(hr)) {
        pID3D11Texture2D_DepthStencilBuffer->Release();
        pID3D11Texture2D_DepthStencilBuffer = NULL;
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateDepthStencilView failed\n");
        fclose(gpFile);
        return hr;
    }
    pID3D11Texture2D_DepthStencilBuffer->Release();
    pID3D11Texture2D_DepthStencilBuffer = NULL;

    // set render target view and depth stencil view in pipeline
    gpID3D11DeviceContext->OMSetRenderTargets(1, &gpID3D11RenderTargetView, gpID3D11DepthStencilView);
    
    // set viewport
    D3D11_VIEWPORT d3dViewPort;
    ZeroMemory((void*)&d3dViewPort, sizeof(D3D11_VIEWPORT));
    d3dViewPort.TopLeftX = 0;
    d3dViewPort.TopLeftY = 0;
    d3dViewPort.Width = (float)width;
    d3dViewPort.Height = (float)height;
    d3dViewPort.MinDepth = 0.0f;
    d3dViewPort.MaxDepth = 1.0f;
    gpID3D11DeviceContext->RSSetViewports(1, &d3dViewPort);
    
    // set perspective projection matrix
    PerspectiveProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    return hr;
}

void uninitialize(void)
{
    // code
    if (gpID3D11Buffer_ConstantBuffer) {
        gpID3D11Buffer_ConstantBuffer->Release();
        gpID3D11Buffer_ConstantBuffer = NULL;
    }
    if (gpID3D11Buffer_IndexBuffer)
	{
		gpID3D11Buffer_IndexBuffer->Release();
		gpID3D11Buffer_IndexBuffer = NULL;
	}
    if(gpID3D11RasterizerState) {
        gpID3D11RasterizerState->Release();
        gpID3D11RasterizerState = NULL;
    }
    if(gpID3D11InputLayout) {
        gpID3D11InputLayout->Release();
        gpID3D11InputLayout = NULL;
    }
    if(gpID3D11PixelShader_PP) {
        gpID3D11PixelShader_PP->Release();
        gpID3D11PixelShader_PP = NULL;
    }
    if(gpID3D11VertexShader_PP) {
        gpID3D11VertexShader_PP->Release();
        gpID3D11VertexShader_PP = NULL;
    }
    if(gpID3D11DepthStencilView) {
        gpID3D11DepthStencilView->Release();
        gpID3D11DepthStencilView = NULL;
    }
    if(gpID3D11RenderTargetView) {
        gpID3D11RenderTargetView->Release();
        gpID3D11RenderTargetView = NULL;
    }
    if(gpIDXGISwapChain) {
        gpIDXGISwapChain->Release();
        gpIDXGISwapChain = NULL;
    }
    if(gpID3D11DeviceContext) {
        gpID3D11DeviceContext->Release();
        gpID3D11DeviceContext = NULL;
    }
    if(gpID3D11Device) {
        gpID3D11Device->Release();
        gpID3D11Device = NULL;
    }

    // close the file
    if(gpFile) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "prorgam terminated successfully");
        fclose(gpFile);
        gpFile = NULL;
    }
}

void toggleFullScreen(void)
{
    MONITORINFO mi;

    if (gbFullScreen == FALSE) {
        dwStyle = GetWindowLong(ghwnd, GWL_STYLE);
        if(dwStyle & WS_OVERLAPPEDWINDOW) {
            ZeroMemory((void*)&mi, sizeof(MONITORINFO));
            mi.cbSize = sizeof(MONITORINFO);

            if(GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi)) {
                SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                                mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                                SWP_NOZORDER | SWP_FRAMECHANGED);
            }
        }
        ShowCursor(FALSE);
    }
    else {
        SetWindowPlacement(ghwnd, &wpPrev);
        SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
        ShowCursor(TRUE);
    }
}

int printDXInfo(void) {
    IDXGIFactory* pIDXGIFactory = NULL;
    IDXGIAdapter* pIDXGIAdapter = NULL;
    DXGI_ADAPTER_DESC dxgiAdapterDesc;
    HRESULT hr = S_OK;
    char str[256];

    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);
    if (FAILED(hr)) {
        fprintf(gpFile, "CreateDXGIFactory failed. hr = 0x%08x\n", hr);
        goto cleanup;
    }
    
    if (pIDXGIFactory->EnumAdapters(0, &pIDXGIAdapter) != DXGI_ERROR_NOT_FOUND) {
        ZeroMemory(&dxgiAdapterDesc, sizeof(DXGI_ADAPTER_DESC));
        pIDXGIAdapter->GetDesc(&dxgiAdapterDesc);

        // to convert wchar type into char type
        WideCharToMultiByte(CP_ACP, 0, dxgiAdapterDesc.Description, 256, str, 256, NULL, NULL);
        fprintf(gpFile, "Graphic Device Name = %s\n", str);
        fprintf(gpFile, "VRAM(in bytes) = %I64d\n", (__int64)dxgiAdapterDesc.DedicatedVideoMemory);
        fprintf(gpFile, "VRAM(in GB) = %d\n", (int)ceil((double)dxgiAdapterDesc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0)));
    } else {
        fprintf(gpFile, "IDXGIFactory::EnumAdapters failed.\n");
        goto cleanup;
    }
    
cleanup:
    if (pIDXGIAdapter) {
        pIDXGIAdapter->Release();
        pIDXGIAdapter = NULL;
    }
    if (pIDXGIFactory) {
        pIDXGIFactory->Release();
        pIDXGIFactory = NULL;
    }

    return 0;
}
