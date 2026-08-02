// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// opengl related header files
#include <gl/GL.h>
#include <gl/GLU.h>

// custom header files
#include "OGL.h"
#include "Model.h"

// opengl related libraries
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "GLU32.lib")

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL loadGLTexture(GLuint* texture, TCHAR imageResourceID[]);

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

// teapot related variables
BOOL bAnimate = FALSE;
BOOL bTexture = FALSE;
BOOL bLight = FALSE;

float angle = 0.0f;
int texture_marble = 0;

float lightAmbient[] = {0.4, 0.4, 0.4, 1};
float lightDiffuse[] = {1, 1, 1, 1};
float lightSpecular[] = {1, 1, 1, 1};
float lightPosition[] = {100, 100, 100, 1};

float materialAmbient[] = {0.9, 0.5, 0.3, 1};
float materialDiffuse[] = {0.9, 0.5, 0.3, 1};
float materialSpecular[] = {0.8, 0.8, 0.8, 1};
float materialShininess[] = {128};

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
                if (bAnimate == TRUE) {
                    // update
                    update();
                }
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

                case 'A':
                case 'a':
                    if (bAnimate == FALSE) {
                        bAnimate = TRUE;
                    }
                    else {
                        bAnimate = FALSE;
                    }
                    break;

                case 'T':
                case 't':
                    if (bTexture == FALSE) {
                        glEnable(GL_TEXTURE_2D);
                        bTexture = TRUE;
                    }
                    else {
                        glDisable(GL_TEXTURE_2D);
                        bTexture = FALSE;
                    }
                    break;

                case 'L':
                case 'l':
                    if (bLight == FALSE) {
                        glEnable(GL_LIGHTING);
                        bLight = TRUE;
                    }
                    else {
                        glDisable(GL_LIGHTING);
                        bLight = FALSE;
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

    // set matrix to model view mode
    glMatrixMode(GL_MODELVIEW);

    // set to identity matrix
    glLoadIdentity();

    // translate triangle backwards by z
    glTranslatef(0.0f, 0.0f, -1.5f);

    glRotatef(angle, 0.0f, 1.0f, 0.0f);

    glBindTexture(GL_TEXTURE_2D, texture_marble);

    // triangle drawing code
    glBegin(GL_TRIANGLES);
		for (int i = 0; i < sizeof(face_indicies) / sizeof(face_indicies[0]); ++i) {
            for (int j = 0; j < 3; ++j) {
                int vertex_index = face_indicies[i][j];
                int normal_index = face_indicies[i][j + 3];
                int texture_index = face_indicies[i][j + 6];

                glTexCoord2f(textures[texture_index][0], textures[texture_index][1]);
                glNormal3f(normals[normal_index][0], normals[normal_index][1], normals[normal_index][2]);
                glVertex3f(vertices[vertex_index][0], vertices[vertex_index][1], vertices[vertex_index][2]);
            }
        }
	glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    
    // swap the buffers
    SwapBuffers(ghdc);
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

void update(void)
{
    // code
    angle += 0.01f;
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (loadGLTexture(&texture_marble, MAKEINTRESOURCE(IDBITMAP_MARBLE)) == FALSE) {
        fprintf(gpFile, "loadGLTexture failed\n");
        return (-6);
    }

    // light initialization
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);

    // material initialization
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, materialShininess);

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

    if(texture_marble) {
        glDeleteTextures(1, &texture_marble);
        texture_marble = 0;
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

BOOL loadGLTexture(GLuint* texture, TCHAR imageResourceID[]) {
    // variable declarations
    HBITMAP hBitMap = NULL;
    BITMAP bmp;
    BOOL bResult = FALSE;

    // code
    // load the bitmap as image
    hBitMap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageResourceID, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hBitMap) {
        bResult = TRUE;
        // get bitmap structure from loaded bitmap image
        GetObject(hBitMap, sizeof(BITMAP), &bmp);
        // generate opengl texture object
        glGenTextures(1, texture);
        // bind to newly created object
        glBindTexture(GL_TEXTURE_2D, *texture);
        // unpack the image in memory for faster loading
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, bmp.bmWidth, bmp.bmHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, bmp.bmBits);
        glBindTexture(GL_TEXTURE_2D, 0);
        DeleteObject(hBitMap);
        hBitMap = NULL;
    }
    return bResult;
}
