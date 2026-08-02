// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

// opengl related header files
#include <gl/GL.h>
#include <gl/GLU.h>

// custom header files
#include "OGL.h"

// opengl related libraries
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "GLU32.lib")

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

// opengl related global variables
HDC ghdc = NULL;
HGLRC ghrc = NULL;                      // handle to graphics library rendering context

GLUquadric* quadric = NULL;

// 24 sphere
BOOL bLight = FALSE;
float lightAmbient[] = {0, 0, 0, 1};
float lightDiffuse[] = {1, 1, 1, 1};
float lightSpecular[] = {1, 1, 1, 1};
float lightPosition[] = {0, 0, 0, 1};

float lightModelAmbient[] = {0.2, 0.2, 0.2, 1};
float lightModelLocalViewer[] = {0.0};

float angleForXRotation = 0;
float angleForYRotation = 0;
float angleForZRotation = 0;
int keyPressed = -1;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // function declarations
    int  initialize(void);
    void display(void);
    void update(void);
    void uninitialize(void);

    // variable declarations
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("RTR6");
    BOOL bDone = FALSE;

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
    int result = initialize();
    if(result != 0) {
        fprintf(gpFile, "initialize() failed\n");
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    else {
        fprintf(gpFile, "initialize() successfull\n");
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
    void resize(int, int);
    void uninitialize(void);
    
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

        case WM_ERASEBKGND:
            return 0;

        case WM_SIZE:
            resize(LOWORD(lParam), HIWORD(lParam));             // (lower bits)width, (higher bits)height
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
                
                case 'L':
                case 'l':
                    if (bLight == FALSE) {
                        bLight = TRUE;
                        glEnable(GL_LIGHTING);
                    }
                    else {
                        bLight = FALSE;
                        glDisable(GL_LIGHTING);
                    }
                    break;

                case 'X':
                case 'x':
                    keyPressed = 1;
                    angleForXRotation = 0;
                    break;

                case 'y':
                case 'Y':
                    keyPressed = 2;
                    angleForYRotation = 0;
                    break;

                case 'z':
                case 'Z':
                    keyPressed = 3;
                    angleForZRotation = 0;
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

void display(void)
{
    void draw24Spheres(void);

    float radius = 100.0f;

    // code
    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set matrix to model view mode
    glMatrixMode(GL_MODELVIEW);

    // set to identity matrix
    glLoadIdentity();

    if (keyPressed == 1) {
        // rotate around x-axis
        lightPosition[1] = radius * cos(angleForXRotation * M_PI / 180.0f);
        lightPosition[2] = radius * sin(angleForXRotation * M_PI / 180.0f);
    }
    else if (keyPressed == 2) {
        lightPosition[0] = radius * cos(angleForYRotation * M_PI / 180.0f);
        lightPosition[2] = radius * sin(angleForYRotation * M_PI / 180.0f);
    }
    else if (keyPressed == 3) {
        lightPosition[0] = radius * cos(angleForZRotation * M_PI / 180.0f);
        lightPosition[1] = radius * sin(angleForZRotation * M_PI / 180.0f);
    }

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    // draw 24 spheres
    draw24Spheres();

    // swap the buffers
    SwapBuffers(ghdc);
}

void draw24Spheres(void)
{
    // variable declarations
    float materialAmbient[4];
    float materialDiffuse[4];
    float materialSpecular[4];
    float materialShininess;

    // code
    // 1st sphere of 1st column - emerald
    materialAmbient[0] = 0.0215; // r
    materialAmbient[1] = 0.1745; // g
    materialAmbient[2] = 0.0215; // b
    materialAmbient[3] = 1;
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.07568; // r
    materialDiffuse[1] = 0.61424; // g
    materialDiffuse[2] = 0.07568; // b
    materialDiffuse[3] = 1;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.633;    // r
    materialSpecular[1] = 0.727811; // g
    materialSpecular[2] = 0.633;    // b
    materialSpecular[3] = 1.0f;
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.6f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 14, 0.0f);                       // x += 6, y -= 2.5
    gluSphere(quadric, 1.0f, 30, 30);

    // 2nd sphere of 1st column - jade
    materialAmbient[0] = 0.135;  // r
    materialAmbient[1] = 0.2225; // g
    materialAmbient[2] = 0.1575; // b
    materialAmbient[3] = 1.0f;   // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.54; // r
    materialDiffuse[1] = 0.89; // g
    materialDiffuse[2] = 0.63; // b
    materialDiffuse[3] = 1.0f; // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.316228; // r
    materialSpecular[1] = 0.316228; // g
    materialSpecular[2] = 0.316228; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.1f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 11.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 3rd sphere of 1st column - obsidian
    materialAmbient[0] = 0.05375; // r
    materialAmbient[1] = 0.05;    // g
    materialAmbient[2] = 0.06625; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.18275; // r
    materialDiffuse[1] = 0.17;    // g
    materialDiffuse[2] = 0.22525; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.332741; // r
    materialSpecular[1] = 0.328634; // g
    materialSpecular[2] = 0.346435; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.3f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 9, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 4th sphere of 1st column - pearl
    materialAmbient[0] = 0.25; // r
    materialAmbient[1] = 0.20725;    // g
    materialAmbient[2] = 0.20725; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 1; // r
    materialDiffuse[1] = 0.829;    // g
    materialDiffuse[2] = 0.829; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.296648; // r
    materialSpecular[1] = 0.296648; // g
    materialSpecular[2] = 0.296648; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.088f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 6.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 5th sphere of 1st column - ruby
    materialAmbient[0] = 0.1745; // r
    materialAmbient[1] = 0.01175;    // g
    materialAmbient[2] = 0.01175; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.61424; // r
    materialDiffuse[1] = 0.04136;    // g
    materialDiffuse[2] = 0.04136; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.727811; // r
    materialSpecular[1] = 0.626959; // g
    materialSpecular[2] = 0.626959; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.6f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 4, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);
    
    // 6th sphere of 1st column - turquoise
    materialAmbient[0] = 0.1; // r
    materialAmbient[1] = 0.18725;    // g
    materialAmbient[2] = 0.1745; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.396; // r
    materialDiffuse[1] = 0.74151;    // g
    materialDiffuse[2] = 0.69102; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.297254; // r
    materialSpecular[1] = 0.30829; // g
    materialSpecular[2] = 0.306678; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.1f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(1.5f, 1.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);


    // 1st sphere of 2nd column - brass
    materialAmbient[0] = 0.329412; // r
    materialAmbient[1] = 0.223529;    // g
    materialAmbient[2] = 0.027451; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.780392; // r
    materialDiffuse[1] = 0.568627;    // g
    materialDiffuse[2] = 0.113725; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.992157; // r
    materialSpecular[1] = 0.941176; // g
    materialSpecular[2] = 0.807843; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.21794872f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 14, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 2nd sphere of 2nd column - bronze
    materialAmbient[0] = 0.2125; // r
    materialAmbient[1] = 0.1275;    // g
    materialAmbient[2] = 0.054; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.714; // r
    materialDiffuse[1] = 0.4284;    // g
    materialDiffuse[2] = 0.18144; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.393548; // r
    materialSpecular[1] = 0.271906; // g
    materialSpecular[2] = 0.166721; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.2f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 11.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 3rd sphere of 2nd column - chrome
    materialAmbient[0] = 0.25; // r
    materialAmbient[1] = 0.25;    // g
    materialAmbient[2] = 0.25; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.4; // r
    materialDiffuse[1] = 0.4;    // g
    materialDiffuse[2] = 0.4; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.774597; // r
    materialSpecular[1] = 0.774597; // g
    materialSpecular[2] = 0.774597; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.6f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 9, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 4th sphere of 2nd column - copper
    materialAmbient[0] = 0.19125; // r
    materialAmbient[1] = 0.0735;    // g
    materialAmbient[2] = 0.0225; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.7038; // r
    materialDiffuse[1] = 0.27048;    // g
    materialDiffuse[2] = 0.0828; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.256777; // r
    materialSpecular[1] = 0.137622; // g
    materialSpecular[2] = 0.086014; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.1f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 6.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 5th sphere of 2nd column - gold
    materialAmbient[0] = 0.24725; // r
    materialAmbient[1] = 0.1995;    // g
    materialAmbient[2] = 0.0745; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.75164; // r
    materialDiffuse[1] = 0.60648;    // g
    materialDiffuse[2] = 0.22648; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.628281; // r
    materialSpecular[1] = 0.555802; // g
    materialSpecular[2] = 0.366065; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.4f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 4, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 6th sphere of 2nd column - silver
    materialAmbient[0] = 0.19225; // r
    materialAmbient[1] = 0.19225;    // g
    materialAmbient[2] = 0.19225; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.50754; // r
    materialDiffuse[1] = 0.50754;    // g
    materialDiffuse[2] = 0.50754; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.508273; // r
    materialSpecular[1] = 0.508273; // g
    materialSpecular[2] = 0.508273; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.4f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(7.5f, 1.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);


    // 1st sphere of 3rd column - black
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.01; // r
    materialDiffuse[1] = 0.01;    // g
    materialDiffuse[2] = 0.01; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.50; // r
    materialSpecular[1] = 0.50; // g
    materialSpecular[2] = 0.50; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 14, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 2nd sphere of 3rd column - cyan
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0.1;    // g
    materialAmbient[2] = 0.06; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.0; // r
    materialDiffuse[1] = 0.50980392;    // g
    materialDiffuse[2] = 0.50980392; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.50196078; // r
    materialSpecular[1] = 0.50196078; // g
    materialSpecular[2] = 0.50196078; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 11.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 3rd sphere of 3rd column - green
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.1; // r
    materialDiffuse[1] = 0.35;    // g
    materialDiffuse[2] = 0.1; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.45; // r
    materialSpecular[1] = 0.55; // g
    materialSpecular[2] = 0.45; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 9, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 4th sphere of 3rd column - red
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.5; // r
    materialDiffuse[1] = 0;    // g
    materialDiffuse[2] = 0; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.7; // r
    materialSpecular[1] = 0.6; // g
    materialSpecular[2] = 0.6; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 6.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 5th sphere of 3rd column - white
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.55; // r
    materialDiffuse[1] = 0.55;    // g
    materialDiffuse[2] = 0.55; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.7; // r
    materialSpecular[1] = 0.7; // g
    materialSpecular[2] = 0.7; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 4, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 6th sphere of 3rd column - yellow
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.5; // r
    materialDiffuse[1] = 0.5;    // g
    materialDiffuse[2] = 0; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.6; // r
    materialSpecular[1] = 0.6; // g
    materialSpecular[2] = 0.5; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.25f * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(13.5f, 1.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);


    // 1st sphere of 4th column - black
    materialAmbient[0] = 0.02; // r
    materialAmbient[1] = 0.02;    // g
    materialAmbient[2] = 0.02; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.01; // r
    materialDiffuse[1] = 0.01;    // g
    materialDiffuse[2] = 0.01; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.40; // r
    materialSpecular[1] = 0.40; // g
    materialSpecular[2] = 0.40; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 14, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 2nd sphere of 4th column - cyan
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0.05;    // g
    materialAmbient[2] = 0.05; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.4; // r
    materialDiffuse[1] = 0.5;    // g
    materialDiffuse[2] = 0.5; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.04; // r
    materialSpecular[1] = 0.7; // g
    materialSpecular[2] = 0.7; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 11.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 3rd sphere of 4th column - green
    materialAmbient[0] = 0; // r
    materialAmbient[1] = 0.05;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.4; // r
    materialDiffuse[1] = 0.5;    // g
    materialDiffuse[2] = 0.4; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.04; // r
    materialSpecular[1] = 0.7; // g
    materialSpecular[2] = 0.04; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 9, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 4th sphere of 4th column - red
    materialAmbient[0] = 0.05; // r
    materialAmbient[1] = 0;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.5; // r
    materialDiffuse[1] = 0.4;    // g
    materialDiffuse[2] = 0.4; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.7; // r
    materialSpecular[1] = 0.04; // g
    materialSpecular[2] = 0.04; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 6.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 5th sphere of 4th column - white
    materialAmbient[0] = 0.05; // r
    materialAmbient[1] = 0.05;    // g
    materialAmbient[2] = 0.05; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.5; // r
    materialDiffuse[1] = 0.5;    // g
    materialDiffuse[2] = 0.5; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.7; // r
    materialSpecular[1] = 0.7; // g
    materialSpecular[2] = 0.7; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 4, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);

    // 6th sphere of 4th column - yellow
    materialAmbient[0] = 0.05; // r
    materialAmbient[1] = 0.05;    // g
    materialAmbient[2] = 0; // b
    materialAmbient[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);

    materialDiffuse[0] = 0.5; // r
    materialDiffuse[1] = 0.5;    // g
    materialDiffuse[2] = 0.4; // b
    materialDiffuse[3] = 1.0f;    // a
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

    materialSpecular[0] = 0.7; // r
    materialSpecular[1] = 0.7; // g
    materialSpecular[2] = 0.04; // b
    materialSpecular[3] = 1.0f;     // a
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    
    materialShininess = 0.078125 * 128;
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    // geometry
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(19.5f, 1.5f, 0.0f);
    gluSphere(quadric, 1.0f, 30, 30);
}

void resize(int width, int height)
{
    // code

    if (height <= 0) {
        height = 1;
    }
    // set the viewport
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // set matrix projection mode
    glMatrixMode(GL_PROJECTION);

    // set to identity matrix
    glLoadIdentity();

    if (width <= height) {
        glOrtho(0.0f,
                15.5f,
                (0.0f * ((GLfloat)height / (GLfloat)width)),
                (15.5f * ((GLfloat)height / (GLfloat)width)),
                -10.0f,
                10.0f);
    }
    else {
        glOrtho((0.0f * ((GLfloat)width / (GLfloat)height)),
            (15.5f * ((GLfloat)width / (GLfloat)height)),
            0.0f,
            15.5f,
            -10.0f,
            10.0f);
    }

    // do perspective projection
    //gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void update(void)
{
    // code
    angleForXRotation += 0.5f;
    if (angleForXRotation >= 360) {
        angleForXRotation -= 360;
    }
    angleForYRotation += 0.5f;
    if (angleForYRotation >= 360) {
        angleForYRotation -= 360;
    }
    angleForZRotation += 0.5f;
    if (angleForZRotation >= 360) {
        angleForZRotation -= 360;
    }
}

int initialize(void)
{
    // function prototype
    void printGLInfo(void);
    void resize(int, int);

    // variable declarations
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex = 0;

    // code
    ZeroMemory((void*)&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;

    // get DC
    ghdc = GetDC(ghwnd);
    if (ghdc == NULL) {
        fprintf(gpFile, "GetDC() failed\n");
        return (-1);
    }

    // get matching pixel format index using hdc and pfd
    iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
    if (iPixelFormatIndex == 0) {
        fprintf(gpFile, "ChoosePixelFormat() failed\n");
        return (-2);
    }

    // select the pixel formats of found index
    if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE) {
        fprintf(gpFile, "SetPixelFormat() failed\n");
        return (-3);
    }

    // create rendering context using hdc, pfd and chosen PixelFormatDesriptor
    ghrc = wglCreateContext(ghdc);
    if (ghrc == NULL) {
        fprintf(gpFile, "wglCreateContext() failed\n");
        return (-4);
    }
    
    // make this rendering context as current context
    if (wglMakeCurrent(ghdc, ghrc) == FALSE) {
        fprintf(gpFile, "wglMakeCurrent() failed\n");
        return (-5);
    }

    // here onwards opengl code starts
    printGLInfo();

    // depth related code
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    // tell opengl to choose the color to clear the screen
    glClearColor(0.75f, 0.75f, 0.75f, 1.0f);

    quadric = gluNewQuadric();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glEnable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightModelAmbient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lightModelLocalViewer);

    // warmup resize
    resize(WIN_WIDTH, WIN_HEIGHT);

    return 0;
}

void printGLInfo(void) {
    // code
    // print opengl information
    fprintf(gpFile, "OPENGL INFORMATION\n");
    fprintf(gpFile, "------------------\n");
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "------------------\n");
}

void uninitialize(void)
{
    // functions declaratoins
    void toggleFullScreen(void);

    // code
    // if fs restore to normal before exiting
    if (gbFullScreen == TRUE) {
        toggleFullScreen();
        gbFullScreen = FALSE;
    }

    // make hdc as current context by releasing rendering context as current context
    if (wglGetCurrentContext() == ghrc) {
        wglMakeCurrent(NULL, NULL);
    }

    // delete the rendering context
    if (ghrc) {
        wglDeleteContext(ghrc);
        ghrc = NULL;
    }

    // release the dc
    if (ghdc) {
        ReleaseDC(ghwnd, ghdc);
        ghdc = NULL;
    }

    // destory window
    if (ghwnd) {
        DestroyWindow(ghwnd);
        ghwnd = NULL;
    }

    if (quadric) {
        gluDeleteQuadric(quadric);
        quadric = NULL;
    }

    // close the file
    if(gpFile) {
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
