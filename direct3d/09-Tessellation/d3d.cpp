#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma warning(disable: 4838)
#include "xnamath/xnamath.h"

#include "d3d.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

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

ID3D11VertexShader* gpID3D11VertexShader = NULL;
ID3D11PixelShader* gpID3D11PixelShader = NULL;
ID3D11HullShader* gpID3D11HullShader = NULL;
ID3D11DomainShader* gpID3D11DomainShader = NULL;
ID3D11Buffer* gpID3D11Buffer_Line_PositionBuffer = NULL;
ID3D11Buffer* gpID3D11Buffer_Cube_PositionBuffer = NULL;
ID3D11Buffer* gpID3D11Buffer_Pyramid_ColorBuffer = NULL;
ID3D11Buffer* gpID3D11Buffer_Cube_ColorBuffer = NULL;
ID3D11Buffer* gpID3D11Buffer_ConstantBuffer_HullShader = NULL;
ID3D11Buffer* gpID3D11Buffer_ConstantBuffer_DomainShader = NULL;
ID3D11Buffer* gpID3D11Buffer_ConstantBuffer_PixelShader = NULL;

ID3D11RasterizerState* gpID3D11RasterizerState = NULL;

ID3D11InputLayout* gpID3D11InputLayout = NULL;

struct CBUFFER_HullShader {
    XMVECTOR Hull_Constant_Function_Params;
};
struct CBUFFER_DomainShader {
    XMMATRIX WorldViewProjectionMatrix;
};
struct CBUFFER_PixelShader {
    XMVECTOR lineColor;
};

u_int uiNoOfLineSegments = 1;

XMMATRIX PerspectiveProjectionMatrix;

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

                case VK_UP:
                    uiNoOfLineSegments++;
                    if (uiNoOfLineSegments > 30)
                        uiNoOfLineSegments = 30;
                    break;
                case VK_DOWN:
                    uiNoOfLineSegments--;
                    if (uiNoOfLineSegments <= 0)
                        uiNoOfLineSegments = 1;
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
    const char* vertexShaderSourceCode =
        "struct vertex_output {" \
        "   float4 position : POSITION;" \
        "};" \
        "vertex_output main(float2 pos : POSITION) {" \
        "   vertex_output output;" \
        "   output.position = float4(pos, 0.0f, 1.0f);" \
        "   return output;" \
        "}";

    ID3DBlob* pID3DBlob_VertexShaderCode = NULL;
    ID3DBlob* pID3DBlob_Error = NULL;
    hr = D3DCompile(vertexShaderSourceCode, lstrlenA(vertexShaderSourceCode) + 1,
                    "VS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0",
                    0, 0, &pID3DBlob_VertexShaderCode, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for vertex shader failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for vertex shader successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateVertexShader(pID3DBlob_VertexShaderCode->GetBufferPointer(),
                                            pID3DBlob_VertexShaderCode->GetBufferSize(), NULL, &gpID3D11VertexShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateVertexShader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->VSSetShader(gpID3D11VertexShader, 0, 0);

    // hull shader
    const char* hullShaderSourceCode =
        "cbuffer ConstantBuffer {" \
        "   float4 hull_constant_function_params;" \
        "}" \
        "struct vertex_output {" \
        "   float4 position : POSITION;" \
        "};" \
        "struct hull_constant_output {" \
        "   float edges[2]: SV_TESSFACTOR;" \
        "};" \
        "struct hull_output {" \
        "   float4 position : POSITION;" \
        "};" \
        "hull_constant_output hull_constant_function(void) {" \
        "   hull_constant_output output;" \
        "   float numberOfLineStrips = hull_constant_function_params[0];" \
        "   float numberOfLineSegments = hull_constant_function_params[1];" \
        "   output.edges[0] = numberOfLineStrips;" \
        "   output.edges[1] = numberOfLineSegments;" \
        "   return output;"
        "}" \
        "[domain(\"isoline\")]" \
        "[partitioning(\"integer\")]" \
        "[outputtopology(\"line\")]" \
        "[outputcontrolpoints(4)]" \
        "[patchconstantfunc(\"hull_constant_function\")]" \
        "hull_output main(InputPatch<vertex_output, 4> input_patch, uint i: SV_OUTPUTCONTROLPOINTID) {" \
        "    hull_output output;" \
        "    output.position = input_patch[i].position;" \
        "    return output;" \
        "}";

    ID3DBlob* pID3DBlob_HullShaderCode = NULL;
    pID3DBlob_Error = NULL;
    hr = D3DCompile(hullShaderSourceCode, lstrlenA(hullShaderSourceCode) + 1,
                    "HS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "hs_5_0",
                    0, 0, &pID3DBlob_HullShaderCode, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for hull shader failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for hull shader successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateHullShader(pID3DBlob_HullShaderCode->GetBufferPointer(),
                                           pID3DBlob_HullShaderCode->GetBufferSize(), NULL, &gpID3D11HullShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateHullShader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateHullShader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->HSSetShader(gpID3D11HullShader, 0, 0);

    // domain shader
    const char* domainShaderSourceCode = 
        "cbuffer ConstantBuffer {" \
        "   float4x4 worldViewProjectionMatrix;" \
        "}" \
        "struct hull_constant_output {" \
        "   float edges[2]: SV_TESSFACTOR;" \
        "};" \
        "struct hull_output {" \
        "   float4 position : POSITION;" \
        "};" \
        "struct domain_output {" \
        "   float4 position : SV_POSITION;" \
        "};" \
        "[domain(\"isoline\")]" \
        "domain_output main(hull_constant_output input, OutputPatch<hull_output, 4> output_patch, float2 tessCoord: SV_DOMAINLOCATION) {" \
        "   float3 p0 = output_patch[0].position.xyz;" \
        "   float3 p1 = output_patch[1].position.xyz;" \
        "   float3 p2 = output_patch[2].position.xyz;" \
        "   float3 p3 = output_patch[3].position.xyz;" \
        "   float3 p = p0 * ((1 - tessCoord.x) * (1 - tessCoord.x) * (1 - tessCoord.x)) + (p1 * 3 * tessCoord.x * (1 - tessCoord.x) * (1 - tessCoord.x)) + (p2 * 3 * tessCoord.x * tessCoord.x * (1 - tessCoord.x)) + (p3 * tessCoord.x * tessCoord.x * tessCoord.x); \n" \
        "   domain_output output;" \
        "   output.position = mul(worldViewProjectionMatrix, float4(p, 1.0));" \
        "   return output;" \
        "}";

    ID3DBlob* pID3DBlob_DomainShaderCode = NULL;
    pID3DBlob_Error = NULL;
    hr = D3DCompile(domainShaderSourceCode, lstrlenA(domainShaderSourceCode) + 1,
                    "DS", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ds_5_0",
                    0, 0, &pID3DBlob_DomainShaderCode, &pID3DBlob_Error);
    if(FAILED(hr)) {
        if(pID3DBlob_Error != NULL) {
            gpFile = fopen(gszLogFileName, "a+");
            fprintf(gpFile, "D3DCompile for domain shader failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for domain shader successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreateDomainShader(pID3DBlob_DomainShaderCode->GetBufferPointer(),
                                           pID3DBlob_DomainShaderCode->GetBufferSize(), NULL, &gpID3D11DomainShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateDomainShader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateDomainShader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->DSSetShader(gpID3D11DomainShader, 0, 0);
    
    // pixel shader
    const char* pixelShaderSourceCode =
        "cbuffer ConstantBuffer {" \
        "   float4 lineColor;" \
        "}" \
        "float4 main(void) : SV_TARGET {" \
        "   float4 color = lineColor;" \
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
            fprintf(gpFile, "D3DCompile for pixel shader failed: %s\n", (char*)pID3DBlob_Error->GetBufferPointer());
            fclose(gpFile);
            pID3DBlob_Error->Release();
            pID3DBlob_Error = NULL;
        }
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "D3DCompile for pixel shader successful\n");
        fclose(gpFile);
    }

    hr = gpID3D11Device->CreatePixelShader(pID3DBlob_PixelShaderCode->GetBufferPointer(),
                                           pID3DBlob_PixelShaderCode->GetBufferSize(), NULL, &gpID3D11PixelShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreatePixelShader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->PSSetShader(gpID3D11PixelShader, 0, 0);

    // initialize input layout
    D3D11_INPUT_ELEMENT_DESC d3d11InputElementDesc[1];
    ZeroMemory((void*)d3d11InputElementDesc, sizeof(D3D11_INPUT_ELEMENT_DESC) * _ARRAYSIZE(d3d11InputElementDesc));

    d3d11InputElementDesc[0].SemanticName = "POSITION";
    d3d11InputElementDesc[0].SemanticIndex = 0;
    d3d11InputElementDesc[0].Format = DXGI_FORMAT_R32G32_FLOAT;
    d3d11InputElementDesc[0].InputSlot = 0;
    d3d11InputElementDesc[0].AlignedByteOffset = 0;
    d3d11InputElementDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    d3d11InputElementDesc[0].InstanceDataStepRate = 0;

    hr = gpID3D11Device->CreateInputLayout(d3d11InputElementDesc, _ARRAYSIZE(d3d11InputElementDesc), pID3DBlob_VertexShaderCode->GetBufferPointer(),
                                            pID3DBlob_VertexShaderCode->GetBufferSize(), &gpID3D11InputLayout);
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
    pID3DBlob_VertexShaderCode->Release();
    pID3DBlob_VertexShaderCode = NULL;
    pID3DBlob_PixelShaderCode->Release();
    pID3DBlob_PixelShaderCode = NULL;

    // declare trinagle vertices
    const float line_position[] = { 
        -1.0f, -1.0f,
        -0.5f, 1.0f,
        0.5f, -1.0f,
        1.0f, 1.0f
    };

    // pyramid position buffer
    D3D11_BUFFER_DESC d3d11BufferDesc;
    ZeroMemory(&d3d11BufferDesc, sizeof(D3D11_BUFFER_DESC));
    d3d11BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11BufferDesc.ByteWidth = sizeof(float) * _ARRAYSIZE(line_position);
    d3d11BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA d3d11SubresourceData;
    ZeroMemory(&d3d11SubresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
    d3d11SubresourceData.pSysMem = line_position;
    
    hr = gpID3D11Device->CreateBuffer(&d3d11BufferDesc, &d3d11SubresourceData, &gpID3D11Buffer_Line_PositionBuffer);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for position buffer failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for position buffer successful\n");
        fclose(gpFile);
    }

    // constant buffer for hull shader
    ZeroMemory(&d3d11BufferDesc, sizeof(D3D11_BUFFER_DESC));
    d3d11BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11BufferDesc.ByteWidth = sizeof(CBUFFER_HullShader);
    d3d11BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gpID3D11Device->CreateBuffer(&d3d11BufferDesc, NULL, &gpID3D11Buffer_ConstantBuffer_HullShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for hull shader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for hull shader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->HSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer_HullShader);

    // constant buffer for domain shader
    ZeroMemory(&d3d11BufferDesc, sizeof(D3D11_BUFFER_DESC));
    d3d11BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11BufferDesc.ByteWidth = sizeof(CBUFFER_DomainShader);
    d3d11BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gpID3D11Device->CreateBuffer(&d3d11BufferDesc, NULL, &gpID3D11Buffer_ConstantBuffer_DomainShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for domain shader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for domain shader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->DSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer_DomainShader);

    // constant buffer for pixel shader
    ZeroMemory(&d3d11BufferDesc, sizeof(D3D11_BUFFER_DESC));
    d3d11BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    d3d11BufferDesc.ByteWidth = sizeof(CBUFFER_PixelShader);
    d3d11BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    hr = gpID3D11Device->CreateBuffer(&d3d11BufferDesc, NULL, &gpID3D11Buffer_ConstantBuffer_PixelShader);
    if(FAILED(hr)) {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for pixel shader failed\n");
        fclose(gpFile);
        return hr;
    } else {
        gpFile = fopen(gszLogFileName, "a+");
        fprintf(gpFile, "ID3D11Device::CreateBuffer for constant buffer for pixel shader successful\n");
        fclose(gpFile);
    }
    gpID3D11DeviceContext->PSSetConstantBuffers(0, 1, &gpID3D11Buffer_ConstantBuffer_PixelShader);

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
    gpID3D11DeviceContext->ClearRenderTargetView(gpID3D11RenderTargetView, clearColor);
    gpID3D11DeviceContext->ClearDepthStencilView(gpID3D11DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // bind vertex buffer for position
    UINT stride = sizeof(float) * 2;
    UINT offset = 0;
    gpID3D11DeviceContext->IASetVertexBuffers(0, 1, &gpID3D11Buffer_Line_PositionBuffer, &stride, &offset);
    gpID3D11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

    // transformations
    XMMATRIX worldMatrix = XMMatrixIdentity();
    XMMATRIX translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 4.0f);
    XMMATRIX viewMatrix = XMMatrixIdentity();
    worldMatrix = translationMatrix;

    XMMATRIX wvpMatrix = worldMatrix * viewMatrix * PerspectiveProjectionMatrix;

    // push data to vertex shader
    CBUFFER_DomainShader constantBuffer_DS;
    ZeroMemory(&constantBuffer_DS, sizeof(CBUFFER_DomainShader));
    constantBuffer_DS.WorldViewProjectionMatrix = wvpMatrix;
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer_DomainShader, 0, NULL, &constantBuffer_DS, 0, 0);
    
    CBUFFER_HullShader constantBuffer_HS;
    ZeroMemory(&constantBuffer_HS, sizeof(CBUFFER_HullShader));
    constantBuffer_HS.Hull_Constant_Function_Params = XMVectorSet(1, uiNoOfLineSegments, 0, 0);
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer_HullShader, 0, NULL, &constantBuffer_HS, 0, 0);
    
    CBUFFER_PixelShader constantBuffer_PS;
    ZeroMemory(&constantBuffer_PS, sizeof(CBUFFER_PixelShader));
    constantBuffer_PS.lineColor = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
    gpID3D11DeviceContext->UpdateSubresource(gpID3D11Buffer_ConstantBuffer_PixelShader, 0, NULL, &constantBuffer_PS, 0, 0);

    TCHAR str[256];
    wsprintf(str, TEXT("Lalit Choudhary : (noOfLineSegments = %d)"), uiNoOfLineSegments);
    SetWindowText(ghwnd, str);

    // draw pyramid
    gpID3D11DeviceContext->Draw(4, 0);

    // swap front and back buffers
    gpIDXGISwapChain->Present(0, 0);
}

void update(void)
{
    // code
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
    if (gpID3D11Buffer_Line_PositionBuffer) {
        gpID3D11Buffer_Line_PositionBuffer->Release();
        gpID3D11Buffer_Line_PositionBuffer = NULL;
    }
    if (gpID3D11Buffer_Cube_PositionBuffer) {
        gpID3D11Buffer_Cube_PositionBuffer->Release();
        gpID3D11Buffer_Cube_PositionBuffer = NULL;
    }
    if(gpID3D11RasterizerState) {
        gpID3D11RasterizerState->Release();
        gpID3D11RasterizerState = NULL;
    }
    if(gpID3D11InputLayout) {
        gpID3D11InputLayout->Release();
        gpID3D11InputLayout = NULL;
    }
    if(gpID3D11PixelShader) {
        gpID3D11PixelShader->Release();
        gpID3D11PixelShader = NULL;
    }
    if(gpID3D11VertexShader) {
        gpID3D11VertexShader->Release();
        gpID3D11VertexShader = NULL;
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
