// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

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

// rotation angles
float anglePyramid = 0.0f;

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

void display(void)
{
    // code

    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Triangle
    // set matrix to model view mode
    glMatrixMode(GL_MODELVIEW);

    // set to identity matrix
    glLoadIdentity();

    // translate triangle backwards by z
    glTranslatef(0.0f, 0.0f, -5.0f);

    glRotatef(anglePyramid, 0.0f, 1.0f, 0.0f);

    // pyramid drawing code
    glBegin(GL_TRIANGLES);
        // front face
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);

        // right face
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

        // back face
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

        // left face
		glVertex3f(0.0f, 1.0f, 0.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
    glEnd();

    // swap the buffers
    SwapBuffers(ghdc);
}

void update(void)
{
    // code
    anglePyramid += 0.02f;
    if (anglePyramid >= 360.0f) {
        anglePyramid = anglePyramid - 360.0f;
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
    glEnable(GL_DEPTH_TEST);                    // depth test enabled (one of the 8 test)
    glDepthFunc(GL_LEQUAL);                     // pass those fragments whose value <= glClearDepth
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

    // do perspective projection
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
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
