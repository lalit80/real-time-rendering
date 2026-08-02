// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// opengl related header files
#include <gl/GLEW.h>
#include <gl/GL.h>

// custom header files
#include "OGL.h"
#include "vmath.h"
using namespace vmath;

// opengl related libraries
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")

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

// rotation angles
float anglePyramid = 0.0f;
float angleCube = 0.0f;

// shader related variables
GLuint shaderProgramObject = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_NORMAL,
    AMC_ATTRIBUTE_COLOR,
    AMC_ATTRIBUTE_TEXTCORD,
};

// texture
GLuint texture_marble;
GLuint textureSamplerUniform = 0;

GLuint vao = 0;
GLuint vbo_position = 0;
GLuint vbo_texture = 0; 
GLuint vbo_normal = 0;  
GLuint ebo = 0;

GLuint mvpMatrixUniform;

mat4 perspectiveProjectionMatrix;

GLfloat lightAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat lightPosition[] = {100.0f, 100.0f, 100.0f, 1.0f};

GLfloat materialAmbient[] = {0.25f, 0.25f, 0.25f, 1.0f};
GLfloat materialDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat materialSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat materialShininiess = 128.0f;
BOOL bAnimation = FALSE;
BOOL bLight = FALSE;

// model loading related variables
struct vec_int {
    int* p;
    int size;
};
typedef struct vec_int vec_int;

struct vec_float {
    float* p;
    int size;
};
typedef struct vec_float vec_float;

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE];

FILE* gp_mesh_file = NULL;
vec_float* gp_vertex, *gp_texture, *gp_normal;
vec_float* gp_vertex_sorted, *gp_texture_sorted, *gp_normal_sorted;
vec_int* gp_vertex_indices, *gp_texture_indices, *gp_normal_indices;

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
                if (bAnimation == TRUE) {
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

    // use shader program object
    glUseProgram(shaderProgramObject);

    // transformations
    mat4 modelViewMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    mat4 modelViewProjectionMatrix = mat4::identity();

    translationMatrix = vmath::translate(0.0f, 0.0f, -4.0f);    
    modelViewMatrix = translationMatrix;
    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);

    // bind with vao
    glBindVertexArray(vao);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);      // wireframe

    // draw the vertex arrays
    int num_vertices = gp_vertex_sorted->size / 3;
    glDrawElements(GL_TRIANGLES, gp_vertex_indices->size, GL_UNSIGNED_INT, NULL);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // unbind with vao
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);

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
    
    perspectiveProjectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void update(void)
{
    // code
    angleCube -= 0.01f;
}

int initialize(void)
{
    // function prototype
    void printGLInfo(void);
    void resize(int, int);
    void uninitialize(void);
    void load_mesh(void);

    // variable declarations
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormatIndex = 0;
    GLenum glewResult;

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

    // initialize glew
    glewResult = glewInit();
    if (glewResult != GLEW_OK) {
        fprintf(gpFile, "glewInit() failed\n");
        return (-6);
    }

    // here onwards opengl code starts
    printGLInfo();

    const GLchar* vertexShaderSourceCode = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "uniform mat4 uMVPMatrix; \n" \
        "void main(void) \n" \
        "{ \n" \
        "   gl_Position = uMVPMatrix * aPosition; \n" \
        "} \n";

    GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderObject, 1, (const GLchar**)&vertexShaderSourceCode, NULL);
    glCompileShader(vertexShaderObject);
    
    GLint status = 0;
    GLint infoLogLength = 0;
    GLchar* szInfoLog = NULL;
    glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(vertexShaderObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "vertex shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode = 
        "#version 460 core\n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
        "   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); \n" \
        "}\n";

    GLuint framgmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(framgmentShaderObject, 1, (const GLchar**)&framgmentShaderSourceCode, NULL);
    glCompileShader(framgmentShaderObject);
    
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(framgmentShaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(framgmentShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(framgmentShaderObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "fragment shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // create, attach, link
    shaderProgramObject = glCreateProgram();
    glAttachShader(shaderProgramObject, vertexShaderObject);
    glAttachShader(shaderProgramObject, framgmentShaderObject);

    // bind shader attribute at a certain index in shader
    // to same index in host program
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_POSITION, "aPosition");
    glLinkProgram(shaderProgramObject);

    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        glGetProgramiv(shaderProgramObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetProgramInfoLog(shaderProgramObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "shader program link log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // get the required uniform location from the shader
    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");

    load_mesh();

    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo_position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (gp_vertex_sorted->size), gp_vertex_sorted->p, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vbo_normal);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (gp_normal_sorted->size), gp_normal_sorted->p, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    //glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vbo_texture);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_texture);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (gp_texture_sorted->size), gp_texture_sorted->p, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_TEXTCORD, 2, GL_FLOAT, GL_FALSE, 0, NULL); 
    //glEnableVertexAttribArray(AMC_ATTRIBUTE_TEXTCORD);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * (gp_vertex_indices->size), gp_vertex_indices->p, GL_STATIC_DRAW);
    // Do NOT unbind the EBO while the VAO is bound! The VAO saves the EBO binding.
    
    glBindVertexArray(0);

    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    perspectiveProjectionMatrix = mat4::identity();

    // warmup resize
    resize(WIN_WIDTH, WIN_HEIGHT);

    return 0;
}

void printGLInfo(void)
{
    int numExtensions, i;

    // code
    //glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    // print opengl information
    fprintf(gpFile, "OPENGL INFORMATION\n");
    fprintf(gpFile, "------------------\n");
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(gpFile, "------------------\n");

    // print opengl information
    /*for (i = 0; i < numExtensions; ++i) {
        fprintf(gpFile, "%s\n", glGetStringi(GL_EXTENSIONS, i));
    }*/
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
    // free vbo_position of color
    if (vbo_position) {
        glDeleteBuffers(1, &vbo_position);
        vbo_position = NULL;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = NULL;
    }

    // detach, delete shader objects and delete shader program object
    if (shaderProgramObject) {
        glUseProgram(shaderProgramObject);
        GLint numShaders;
        glGetProgramiv(shaderProgramObject, GL_ATTACHED_SHADERS, &numShaders);
        if (numShaders > 0) {
            GLuint* pShaders = (GLuint*)malloc(numShaders * sizeof(GLuint));
            if (pShaders) {
                glGetAttachedShaders(shaderProgramObject, numShaders, NULL, pShaders);
                for (GLint i = 0; i < numShaders; ++i) {
                    glDetachShader(shaderProgramObject, pShaders[i]);
                    glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
                free(pShaders);
            }
        }
        glUseProgram(0);
        glDeleteProgram(shaderProgramObject);
        shaderProgramObject = 0;
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmp.bmWidth, bmp.bmHeight, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bmp.bmBits);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        DeleteObject(hBitMap);
        hBitMap = NULL;
    }
    return bResult;
}

void load_mesh(void) {
    vec_float* create_vec_float();
    vec_int* create_vec_int();
    int push_back_vec_float(vec_float* p_vec_float, float data);
    int push_back_vec_int(vec_int* p_vec_int, int data);
    int destroy_vec_float(vec_float* p_vec_float);
    int destroy_vec_int(vec_int* p_vec_int);
    void show_vec_float(vec_float* p_vec_float);
    void show_vec_int(vec_int* p_vec_int);

    char *space = " ", *slash = "/", *first_token = NULL, *token = NULL;
    char *f_entries[3] = {NULL, NULL, NULL};
    int nr_pos_cords = 0, nr_tex_cords = 0, nr_normal_cords = 0, nr_faces = 0;
    int i, vi;

    gp_mesh_file = fopen("Suzanne.obj", "r");
    if (gp_mesh_file == NULL) {
        fprintf(gpFile, "Cannot open mesh file Suzanne.obj\n");
        return;
    }

    gp_vertex = create_vec_float();
    gp_texture = create_vec_float();
    gp_normal = create_vec_float();
    gp_vertex_indices = create_vec_int();
    gp_texture_indices = create_vec_int();
    gp_normal_indices = create_vec_int();

    while (fgets(buffer, BUFFER_SIZE, gp_mesh_file) != NULL) {
        first_token = strtok(buffer, space);

        if (strcmp(first_token, "v") == 0) {
            nr_pos_cords++;
            while ((token = strtok(NULL, space)) != NULL) {
                push_back_vec_float(gp_vertex, atof(token));
            }
        }
        else if (strcmp(first_token, "vt") == 0) {
            nr_tex_cords++;
            while ((token = strtok(NULL, space)) != NULL) {
                push_back_vec_float(gp_texture, atof(token));
            }
        }
        else if (strcmp(first_token, "vn") == 0) {
            nr_normal_cords++;
            while ((token = strtok(NULL, space)) != NULL) {
                push_back_vec_float(gp_normal, atof(token));
            }
        }
        else if (strcmp(first_token, "f") == 0) {
            nr_faces++;
            for (i = 0; i < 3; ++i)
                f_entries[i] = strtok(NULL, space);

            for (i = 0; i < 3; ++i) {
                token = strtok(f_entries[i], slash);
                push_back_vec_int(gp_vertex_indices, atoi(token) - 1);
                token = strtok(NULL, slash);
                push_back_vec_int(gp_texture_indices, atoi(token) - 1);
                token = strtok(NULL, slash);
                push_back_vec_int(gp_normal_indices, atoi(token) - 1);
            }
        }
    }

    gp_vertex_sorted = create_vec_float();
    for (int i = 0; i < gp_vertex_indices->size; ++i)
        push_back_vec_float(gp_vertex_sorted, gp_vertex->p[i]);
    
    gp_texture_sorted = create_vec_float();
    for (int i = 0; i < gp_texture_indices->size; ++i)
        push_back_vec_float(gp_texture_sorted, gp_texture->p[i]);
    
    gp_normal_sorted = create_vec_float();
    for (int i = 0; i < gp_normal_indices->size; ++i)
        push_back_vec_float(gp_normal_sorted, gp_normal->p[i]);
    
    fclose(gp_mesh_file);
    gp_mesh_file = NULL;

    /*fprintf(gpFile, "Vertex Array\n");
    show_vec_float(gp_vertex);
    fprintf(gpFile, "Normal Array\n");
    show_vec_float(gp_normal);
    fprintf(gpFile, "Vertex Array Indices\n");
    show_vec_int(gp_vertex_indices);
    fprintf(gpFile, "Normal Array Indices\n");
    show_vec_int(gp_normal_indices);*/
}

vec_int* create_vec_int() {
    vec_int* vec = (vec_int*)malloc(sizeof(vec_int));
    assert(vec != NULL);
    memset(vec, 0, sizeof(vec_int));
    return vec;
}

vec_float* create_vec_float() {
    vec_float* vec = (vec_float*)malloc(sizeof(vec_float));
    assert(vec != NULL);
    memset(vec, 0, sizeof(vec_float));
    return vec;
}

int push_back_vec_int(vec_int* p_vec_int, int data) {
    p_vec_int->p = (int*)realloc(p_vec_int->p, (p_vec_int->size + 1) * sizeof(int));
    assert(p_vec_int->p != NULL);
    p_vec_int->p[p_vec_int->size] = data;
    p_vec_int->size += 1;
    return 0;
}

int push_back_vec_float(vec_float* p_vec_float, float data) {
    p_vec_float->p = (float*)realloc(p_vec_float->p, (p_vec_float->size + 1) * sizeof(float));
    assert(p_vec_float->p != NULL);
    p_vec_float->p[p_vec_float->size] = data;
    p_vec_float->size += 1;
    return 0;
}

int destroy_vec_int(vec_int* p_vec_int) {
    if (p_vec_int) {
        if (p_vec_int->p) {
            free(p_vec_int->p);
            p_vec_int->p = NULL;
        }
        free(p_vec_int);
        p_vec_int = NULL;
    }
    return 0;
}

int destroy_vec_float(vec_float* p_vec_float) {
    if (p_vec_float) {
        if (p_vec_float->p) {
            free(p_vec_float->p);
            p_vec_float->p = NULL;
        }
        free(p_vec_float);
        p_vec_float = NULL;
    }
    return 0;
}

void show_vec_int(vec_int* p_vec_int) {
    if (p_vec_int) {
        for (int i = 0; i < p_vec_int->size; ++i) {
            fprintf(gpFile, "%d\n", p_vec_int->p[i]);
        }
    }
}

void show_vec_float(vec_float* p_vec_float) {
    if (p_vec_float) {
        for (int i = 0; i < p_vec_float->size; ++i) {
            fprintf(gpFile, "%f\n", p_vec_float->p[i]);
        }
    }
}
