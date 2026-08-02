// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define USE_MATH_DEFINES
#include <math.h>

// opengl related header files
#include <gl/GLEW.h>
#include <gl/GL.h>

// custom header files
#include "OGL.h"
#include "vmath.h"
#include "Sphere.h"
using namespace vmath;

// opengl related libraries
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib,"Sphere.lib")

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

GLuint gNumVertices;
GLuint gNumElements;
float sphere_vertices[1146];
float sphere_normals[1146];
float sphere_textures[764];
unsigned short sphere_elements[2280];

// shader related variables
GLuint shaderProgramObject_PF = 0;
GLuint shaderProgramObject_PV = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_NORMAL,
};

GLuint gVao_sphere = 0;
GLuint gVbo_sphere_position = 0;
GLuint gVbo_sphere_normal = 0;
GLuint gVbo_sphere_element = 0;

GLuint modelMatrixUniform_PV = 0;
GLuint viewMatrixUniform_PV = 0;
GLuint projectionMatrixUniform_PV = 0;
GLuint LaUniform_PV[1];
GLuint LdUniform_PV[1];                   // diffuse
GLuint LsUniform_PV[1];                   // specular
GLuint LAngleUniform_PV[1];
GLuint KaUniform_PV = 0;                   // ambient material
GLuint KdUniform_PV = 0;
GLuint KsUniform_PV = 0;
GLuint materialShininessUniform_PV = 0;
GLuint lightPositionUniform_PV[1];
GLuint LKeyPressUniform_PV = 0;

GLuint modelMatrixUniform_PF = 0;
GLuint viewMatrixUniform_PF = 0;
GLuint projectionMatrixUniform_PF = 0;
GLuint LaUniform_PF[1];
GLuint LdUniform_PF[1];                   // diffuse
GLuint LsUniform_PF[1];                   // specular
GLuint LAngleUniform_PF[1];
GLuint KaUniform_PF = 0;                   // ambient material
GLuint KdUniform_PF = 0;
GLuint KsUniform_PF = 0;
GLuint materialShininessUniform_PF = 0;
GLuint lightPositionUniform_PF[1];
GLuint LKeyPressUniform_PF = 0;

mat4 perspectiveProjectionMatrix;

struct Light {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;
    GLfloat angle;
};

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    GLfloat shininiess;
};

struct Light light[1];
struct Material material[24];

BOOL bLight = FALSE;
BOOL bPerFragment = FALSE;
BOOL bPerVertex = TRUE;
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

        case WM_CHAR:
            switch(wParam) {
                case 'Q':
                case 'q':
                    gbEscapKeyIsPressed = TRUE;
                    break;
                    
                case 'L':
                case 'l':
                    if (bLight == FALSE) {
                        bLight = TRUE;
                    } else {
                        bLight = FALSE;
                    }
                    break;

                case 'F':
                case 'f':
                    if (bPerFragment == FALSE) {
                        bPerFragment = TRUE;
                        bPerVertex = FALSE;
                    }
                    break;

                case 'v':
                case 'V':
                    if (bPerVertex == FALSE) {
                        bPerVertex = TRUE;
                        bPerFragment = FALSE;
                    }
                    break;

                case 'X':
                case 'x':
                    keyPressed = 1;
                    break;

                case 'y':
                case 'Y':
                    keyPressed = 2;
                    break;

                case 'z':
                case 'Z':
                    keyPressed = 3;
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
    if (bPerFragment == TRUE) {
        glUseProgram(shaderProgramObject_PF);

        // transformations
        mat4 viewMatrix = mat4::identity();
        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(viewMatrixUniform_PF, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PF, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == TRUE) {
            glUniform3fv(LaUniform_PF[0], 1, light[0].ambient);
            glUniform3fv(LdUniform_PF[0], 1, light[0].diffuse);
            glUniform3fv(LsUniform_PF[0], 1, light[0].specular);
            glUniform4fv(lightPositionUniform_PF[0], 1, light[0].position);
            glUniform1i(LKeyPressUniform_PF, 1);
        } else {
            glUniform1i(LKeyPressUniform_PF, 0);
        }
        
        // *** bind vao ***
        glBindVertexArray(gVao_sphere);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);

        GLfloat xTranslate = -2.5f, yTranslate = 0.0f;
        for (int i = 0, k = 0; i < 4; ++i) {
            yTranslate = 3.0f;
            for (int j = 0; j < 6; ++j) {
                mat4 modelMatrix = mat4::identity();
                mat4 translationMatrix = mat4::identity();
                translationMatrix = vmath::translate(xTranslate, yTranslate, -8.5f);
                modelMatrix = translationMatrix;
                glUniformMatrix4fv(modelMatrixUniform_PF, 1, GL_FALSE, modelMatrix);
                glUniform3fv(KaUniform_PF, 1, material[k].ambient);
                glUniform3fv(KdUniform_PF, 1, material[k].diffuse);
                glUniform3fv(KsUniform_PF, 1, material[k].specular);
                glUniform1f(materialShininessUniform_PF, material[k].shininiess);

                glDrawElements(GL_TRIANGLES, gNumElements, GL_UNSIGNED_SHORT, 0);
                
                ++k;
                yTranslate -= 1.2f;
            }
            xTranslate += 1.5f;
        }
            
        // *** unbind vao ***
        glBindVertexArray(0);
        // unuse shader program object
        glUseProgram(0);
    } else {
        glUseProgram(shaderProgramObject_PV);

        // transformations
        mat4 viewMatrix = mat4::identity();
        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(viewMatrixUniform_PV, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PV, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == TRUE) {
            glUniform3fv(LaUniform_PV[0], 1, light[0].ambient);
            glUniform3fv(LdUniform_PV[0], 1, light[0].diffuse);
            glUniform3fv(LsUniform_PV[0], 1, light[0].specular);
            glUniform4fv(lightPositionUniform_PV[0], 1, light[0].position);
            glUniform1i(LKeyPressUniform_PV, 1);
        } else {
            glUniform1i(LKeyPressUniform_PV, 0);
        }
        
        // *** bind vao ***
        glBindVertexArray(gVao_sphere);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);

        GLfloat xTranslate = -2.5f, yTranslate = 0.0f;
        for (int i = 0, k = 0; i < 4; ++i) {
            yTranslate = 3.0f;
            for (int j = 0; j < 6; ++j) {
                mat4 modelMatrix = mat4::identity();
                mat4 translationMatrix = mat4::identity();
                translationMatrix = vmath::translate(xTranslate, yTranslate, -8.5f);
                modelMatrix = translationMatrix;
                glUniformMatrix4fv(modelMatrixUniform_PV, 1, GL_FALSE, modelMatrix);
                glUniform3fv(KaUniform_PV, 1, material[k].ambient);
                glUniform3fv(KdUniform_PV, 1, material[k].diffuse);
                glUniform3fv(KsUniform_PV, 1, material[k].specular);
                glUniform1f(materialShininessUniform_PV, material[k].shininiess);

                glDrawElements(GL_TRIANGLES, gNumElements, GL_UNSIGNED_SHORT, 0);
                
                ++k;
                yTranslate -= 1.2f;
            }
            xTranslate += 1.5f;
        }

        // *** unbind vao ***
        glBindVertexArray(0);
        // unuse shader program object
        glUseProgram(0);
    }

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
    #define RADIUS  50.0f
    GLfloat x = 0.0f, y = 0.0f, z = 0.0f;

    light[0].angle += 0.003f;
    if (keyPressed == 1) {
        // update position of light 0 (z-y plane)
        z = cos(light[0].angle) * RADIUS;
        y = sin(light[0].angle) * RADIUS;
        light[0].position = vec4(0.0f, y, z, 1.0f);
    }
    else if (keyPressed == 2) {
        // update position of light 0 (x-z plane)
        x = cos(light[0].angle) * RADIUS;
        z = sin(light[0].angle) * RADIUS;
        light[0].position = vec4(x, 0.0f, z, 1.0f);
    }
    else if (keyPressed == 3) {
        // update position of light 0 (x-y plane)
        x = cos(light[0].angle) * RADIUS;
        y = sin(light[0].angle) * RADIUS;
        light[0].position = vec4(x, y, 0.0f, 1.0f);
    }
}

int initialize(void)
{
    // function prototype
    void printGLInfo(void);
    void resize(int, int);
    void uninitialize(void);
    void fillMaterialProperties(void);

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

    // per freagment light
    const GLchar* vertexShaderSourceCode_PF = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "in vec3 aNormal; \n" \
        "out vec3 out_transformedNormals; \n" \
        "out vec3 out_lightDirection[1]; \n" \
        "out vec3 out_viewerVector; \n" \
        "uniform mat4 uModelMatrix; \n" \
        "uniform mat4 uViewMatrix; \n" \
        "uniform mat4 uProjectionMatrix; \n" \
        "uniform vec4 uLightPosition[1]; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) { \n" \
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n" \
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n" \
                "out_transformedNormals = normalize(normalMatrix * aNormal); \n" \
                "out_viewerVector = normalize(-eyeCoordinates.xyz); \n" \
                "for (int i = 0; i < 1; ++i) { \n" \
                    "out_lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n" \
                "} \n" \
            "} \n" \
        "} \n";

    GLuint vertexShaderObject_PF = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderObject_PF, 1, (const GLchar**)&vertexShaderSourceCode_PF, NULL);
    glCompileShader(vertexShaderObject_PF);
    
    GLint status = 0;
    GLint infoLogLength = 0;
    GLchar* szInfoLog = NULL;
    glGetShaderiv(vertexShaderObject_PF, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(vertexShaderObject_PF, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(vertexShaderObject_PF, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "vertex shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode_PF = 
        "#version 460 core\n" \
        "in vec3 out_transformedNormals; \n" \
        "in vec3 out_lightDirection[1]; \n" \
        "in vec3 out_viewerVector; \n" \
        "out vec4 FragColor; \n" \
        "uniform vec3 uLa[1]; \n" \
        "uniform vec3 uLd[1]; \n" \
        "uniform vec3 uLs[1]; \n" \
        "uniform vec4 uLightPosition[1]; \n" \
        "uniform vec3 uKa; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec3 uKs; \n" \
        "uniform float uMaterialShininess; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) { \n" \
            "vec3 phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n" \
                "vec3 normalizedViewerVector = normalize(out_viewerVector); \n" \
                "vec3 normalizedLightDirection[1]; \n" \
                "vec3 lightDirection[1]; \n" \
                "vec3 ambientLight[1]; \n" \
                "vec3 diffuseLight[1]; \n" \
                "vec3 reflectionVector[1]; \n" \
                "vec3 specularLight[1]; \n" \
                "for (int i = 0; i < 1; ++i) { \n" \
                    "normalizedLightDirection[i] = normalize(out_lightDirection[i]); \n" \
                    "ambientLight[i] = uLa[i] * uKa * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n" \
                    "diffuseLight[i] = uLd[i] * uKd * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n" \
                    "reflectionVector[i] = reflect(-normalizedLightDirection[i], normalizedTransformedNormals); \n" \
                    "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], normalizedViewerVector), 0.0f), uMaterialShininess); \n" \
                    "phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n" \
                "} \n" \
            "} \n" \
            "else { \n" \
                "phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n" \
            "} \n" \
            "FragColor = vec4(phong_ads_light, 1.0f); \n" \
        "}\n";

    GLuint framgmentShaderObject_PF = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(framgmentShaderObject_PF, 1, (const GLchar**)&framgmentShaderSourceCode_PF, NULL);
    glCompileShader(framgmentShaderObject_PF);
    
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(framgmentShaderObject_PF, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(framgmentShaderObject_PF, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(framgmentShaderObject_PF, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "fragment shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // create, attach, link
    shaderProgramObject_PF = glCreateProgram();
    glAttachShader(shaderProgramObject_PF, vertexShaderObject_PF);
    glAttachShader(shaderProgramObject_PF, framgmentShaderObject_PF);

    // bind shader attribute at a certain index in shader
    // to same index in host program
    glBindAttribLocation(shaderProgramObject_PF, AMC_ATTRIBUTE_POSITION, "aPosition");
    glBindAttribLocation(shaderProgramObject_PF, AMC_ATTRIBUTE_NORMAL, "aNormal");
    glLinkProgram(shaderProgramObject_PF);

    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetProgramiv(shaderProgramObject_PF, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        glGetProgramiv(shaderProgramObject_PF, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetProgramInfoLog(shaderProgramObject_PF, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "shader program link log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // get the required uniform location from the shader
    modelMatrixUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uModelMatrix");
    viewMatrixUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uViewMatrix");
    projectionMatrixUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uProjectionMatrix");
    KaUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKa");
    KdUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKd");
    KsUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKs");
    materialShininessUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uMaterialShininess");
    LKeyPressUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLKeyIsPressed");
    LaUniform_PF[0] = glGetUniformLocation(shaderProgramObject_PF, "uLa[0]");
    LdUniform_PF[0] = glGetUniformLocation(shaderProgramObject_PF, "uLd[0]");
    LsUniform_PF[0] = glGetUniformLocation(shaderProgramObject_PF, "uLs[0]");
    lightPositionUniform_PF[0] = glGetUniformLocation(shaderProgramObject_PF, "uLightPosition[0]");

    
    // per vertex light
    const GLchar* vertexShaderSourceCode_PV = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "in vec3 aNormal; \n" \
        "out vec3 out_phong_ads_light; \n" \
        "uniform mat4 uModelMatrix; \n" \
        "uniform mat4 uViewMatrix; \n" \
        "uniform mat4 uProjectionMatrix; \n" \
        "uniform vec3 uLa[1]; \n" \
        "uniform vec3 uLd[1]; \n" \
        "uniform vec3 uLs[1]; \n" \
        "uniform vec4 uLightPosition[1]; \n" \
        "uniform vec3 uKa; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec3 uKs; \n" \
        "uniform float uMaterialShininess; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) \n" \
        "{ \n" \
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n" \
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n" \
                "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n" \
                "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n" \
                "vec3 lightDirection[3]; \n" \
                "vec3 ambientLight[3]; \n" \
                "vec3 diffuseLight[3]; \n" \
                "vec3 reflectionVector[3]; \n" \
                "vec3 specularLight[3]; \n" \
                "out_phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n" \
                "for (int i = 0; i < 1; ++i) { \n" \
                    "lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n" \
                    "ambientLight[i] = uLa[i] * uKa * max(dot(lightDirection[i], transformedNormal), 0.0f); \n" \
                    "diffuseLight[i] = uLd[i] * uKd * max(dot(lightDirection[i], transformedNormal), 0.0f); \n" \
                    "reflectionVector[i] = reflect(-lightDirection[i], transformedNormal); \n" \
                    "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], viewerVector), 0.0f), uMaterialShininess); \n" \
                    "out_phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n" \
                "} \n" \
            "} \n" \
            "else { \n" \
                "out_phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n" \
            "} \n" \
        "} \n";

    GLuint vertexShaderObject_PV = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderObject_PV, 1, (const GLchar**)&vertexShaderSourceCode_PV, NULL);
    glCompileShader(vertexShaderObject_PV);
    
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(vertexShaderObject_PV, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(vertexShaderObject_PV, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(vertexShaderObject_PV, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "vertex shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode_PV = 
        "#version 460 core\n" \
        "in vec3 out_phong_ads_light; \n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
            "FragColor = vec4(out_phong_ads_light, 1.0f); \n" \
        "}\n";

    GLuint framgmentShaderObject_PV = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(framgmentShaderObject_PV, 1, (const GLchar**)&framgmentShaderSourceCode_PV, NULL);
    glCompileShader(framgmentShaderObject_PV);
    
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(framgmentShaderObject_PV, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(framgmentShaderObject_PV, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(framgmentShaderObject_PV, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "fragment shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // create, attach, link
    shaderProgramObject_PV = glCreateProgram();
    glAttachShader(shaderProgramObject_PV, vertexShaderObject_PV);
    glAttachShader(shaderProgramObject_PV, framgmentShaderObject_PV);

    // bind shader attribute at a certain index in shader
    // to same index in host program
    glBindAttribLocation(shaderProgramObject_PV, AMC_ATTRIBUTE_POSITION, "aPosition");
    glBindAttribLocation(shaderProgramObject_PV, AMC_ATTRIBUTE_NORMAL, "aNormal");
    glLinkProgram(shaderProgramObject_PV);

    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetProgramiv(shaderProgramObject_PV, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        glGetProgramiv(shaderProgramObject_PV, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetProgramInfoLog(shaderProgramObject_PV, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "shader program link log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        uninitialize();
    }

    // get the required uniform location from the shader
    modelMatrixUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uModelMatrix");
    viewMatrixUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uViewMatrix");
    projectionMatrixUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uProjectionMatrix");
    KaUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKa");
    KdUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKd");
    KsUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKs");
    materialShininessUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uMaterialShininess");
    LKeyPressUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLKeyIsPressed");
    LaUniform_PV[0] = glGetUniformLocation(shaderProgramObject_PV, "uLa[0]");
    LdUniform_PV[0] = glGetUniformLocation(shaderProgramObject_PV, "uLd[0]");
    LsUniform_PV[0] = glGetUniformLocation(shaderProgramObject_PV, "uLs[0]");
    lightPositionUniform_PV[0] = glGetUniformLocation(shaderProgramObject_PV, "uLightPosition[0]");

    getSphereVertexData(sphere_vertices, sphere_normals, sphere_textures, sphere_elements);
    gNumVertices = getNumberOfSphereVertices();
    gNumElements = getNumberOfSphereElements();

    // vertex array object for arrays of vertex attributes
    // vao
    glGenVertexArrays(1, &gVao_sphere);
    glBindVertexArray(gVao_sphere);

    // position vbo
    glGenBuffers(1, &gVbo_sphere_position);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo_sphere_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_vertices), sphere_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normal vbo
    glGenBuffers(1, &gVbo_sphere_normal);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo_sphere_normal);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphere_normals), sphere_normals, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // element vbo
    glGenBuffers(1, &gVbo_sphere_element);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphere_elements), sphere_elements, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    // unbind vao
    glBindVertexArray(0);


    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.75f, 0.75f, 0.75f, 1.0f);

    perspectiveProjectionMatrix = mat4::identity();

    // warmup resize
    resize(WIN_WIDTH, WIN_HEIGHT);

    // initialization of light
    light[0].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    light[0].diffuse = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    light[0].specular = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    light[0].position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    light[0].angle = 0.0f;

    fillMaterialProperties();

    return 0;
}

void fillMaterialProperties(void)
{
    material[0].ambient = vec4(0.0215f, 0.1745f, 0.0215f, 1.0f);
    material[0].diffuse = vec4(0.07568f, 0.61424f, 0.07568f, 1.0f);
    material[0].specular = vec4(0.633f, 0.727811f, 0.633f, 1.0f);
    material[0].shininiess = 0.6f * 128;

    material[1].ambient = vec4(0.135f, 0.2225f, 0.1575f, 1.0f);
    material[1].diffuse = vec4(0.54f, 0.89f, 0.63f, 1.0f);
    material[1].specular = vec4(0.316228f, 0.316228f, 0.316228f, 1.0f);
    material[1].shininiess = 0.1f * 128;

    material[2].ambient = vec4(0.05375f, 0.05f, 0.06625f, 1.0f);
    material[2].diffuse = vec4(0.18275f, 0.17f, 0.22525f, 1.0f);
    material[2].specular = vec4(0.332741f, 0.328634f, 0.346435f, 1.0f);
    material[2].shininiess = 0.3f * 128;

    material[3].ambient = vec4(0.25f, 0.20725f, 0.20725f, 1.0f);
    material[3].diffuse = vec4(1.0f, 0.829f, 0.829f, 1.0f);
    material[3].specular = vec4(0.296648f, 0.296648f, 0.296648f, 1.0f);
    material[3].shininiess = 0.088f * 128;

    material[4].ambient = vec4(0.1745f, 0.01175f, 0.01175f, 1.0f);
    material[4].diffuse = vec4(0.61424f, 0.04136f, 0.04136f, 1.0f);
    material[4].specular = vec4(0.727811f, 0.626959f, 0.626959f, 1.0f);
    material[4].shininiess = 0.6f * 128;

    material[5].ambient = vec4(0.1f, 0.18725f, 0.1745f, 1.0f);
    material[5].diffuse = vec4(0.396f, 0.396f, 0.69102f, 1.0f);
    material[5].specular = vec4(0.297254f, 0.30829f, 0.306678f, 1.0f);
    material[5].shininiess = 0.1f * 128;

    material[6].ambient = vec4(0.329412f, 0.223529f, 0.027451f, 1.0f);
    material[6].diffuse = vec4(0.780392f, 0.568627f, 0.113725f, 1.0f);
    material[6].specular = vec4(0.992157f, 0.941176f, 0.807843f, 1.0f);
    material[6].shininiess = 0.21794872f * 128;

    material[7].ambient = vec4(0.2125f, 0.1275f, 0.054f, 1.0f);
    material[7].diffuse = vec4(0.714f, 0.4284f, 0.18144f, 1.0f);
    material[7].specular = vec4(0.393548f, 0.271906f, 0.166721f, 1.0f);
    material[7].shininiess = 0.2f * 128;

    material[8].ambient = vec4(0.25f, 0.25f, 0.25f, 1.0f);
    material[8].diffuse = vec4(0.4f, 0.4f, 0.4f, 1.0f);
    material[8].specular = vec4(0.774597f, 0.774597f, 0.774597f, 1.0f);
    material[8].shininiess = 0.6f * 128;

    material[9].ambient = vec4(0.19125f, 0.0735f, 0.0225f, 1.0f);
    material[9].diffuse = vec4(0.7038f, 0.27048f, 0.0828f, 1.0f);
    material[9].specular = vec4(0.256777f, 0.137622f, 0.086014f, 1.0f);
    material[9].shininiess = 0.1f * 128;

    material[10].ambient = vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
    material[10].diffuse = vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
    material[10].specular = vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
    material[10].shininiess = 0.4f * 128;

    material[11].ambient = vec4(0.19225f, 0.19225f, 0.19225f, 1.0f);
    material[11].diffuse = vec4(0.50754f, 0.50754f, 0.50754f, 1.0f);
    material[11].specular = vec4(0.508273f, 0.508273f, 0.508273f, 1.0f);
    material[11].shininiess = 0.4f * 128;

    material[12].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[12].diffuse = vec4(0.01f, 0.01f, 0.01f, 1.0f);
    material[12].specular = vec4(0.50f, 0.50f, 0.50f, 1.0f);
    material[12].shininiess = 0.25f * 128;

    material[13].ambient = vec4(0.0f, 0.1f, 0.06f, 1.0f);
    material[13].diffuse = vec4(0.0f, 0.50980392f, 0.50980392f, 1.0f);
    material[13].specular = vec4(0.50196078f, 0.50196078f, 0.50196078f, 1.0f);
    material[13].shininiess = 0.25f * 128;

    material[14].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[14].diffuse = vec4(0.1f, 0.35f, 0.1f, 1.0f);
    material[14].specular = vec4(0.45f, 0.55f, 0.45f, 1.0f);
    material[14].shininiess = 0.25f * 128;

    material[15].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[15].diffuse = vec4(0.5f, 0.0f, 0.0f, 1.0f);
    material[15].specular = vec4(0.7f, 0.6f, 0.6f, 1.0f);
    material[15].shininiess = 0.25f * 128;

    material[16].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[16].diffuse = vec4(0.55f, 0.55f, 0.55f, 1.0f);
    material[16].specular = vec4(0.7f, 0.7f, 0.7f, 1.0f);
    material[16].shininiess = 0.25f * 128;

    material[17].ambient = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    material[17].diffuse = vec4(0.5f, 0.5f, 0.0f, 1.0f);
    material[17].specular = vec4(0.6f, 0.6f, 0.5f, 1.0f);
    material[17].shininiess = 0.25f * 128;

    material[18].ambient = vec4(0.02f, 0.02f, 0.02f, 1.0f);
    material[18].diffuse = vec4(0.01f, 0.01f, 0.01f, 1.0f);
    material[18].specular = vec4(0.40f, 0.40f, 0.40f, 1.0f);
    material[18].shininiess = 0.078125f * 128;

    material[19].ambient = vec4(0.0f, 0.05f, 0.05f, 1.0f);
    material[19].diffuse = vec4(0.4f, 0.5f, 0.5f, 1.0f);
    material[19].specular = vec4(0.04f, 0.7f, 0.7f, 1.0f);
    material[19].shininiess = 0.078125f * 128;

    material[20].ambient = vec4(0.0f, 0.05f, 0.0f, 1.0f);
    material[20].diffuse = vec4(0.4f, 0.5f, 0.4f, 1.0f);
    material[20].specular = vec4(0.04f, 0.7f, 0.04f, 1.0f);
    material[20].shininiess = 0.078125f * 128;

    material[21].ambient = vec4(0.05f, 0.0f, 0.0f, 1.0f);
    material[21].diffuse = vec4(0.5f, 0.4f, 0.4f, 1.0f);
    material[21].specular = vec4(0.7f, 0.04f, 0.04f, 1.0f);
    material[21].shininiess = 0.078125f * 128;

    material[22].ambient = vec4(0.05f, 0.05f, 0.05f, 1.0f);
    material[22].diffuse = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    material[22].specular = vec4(0.7f, 0.7f, 0.7f, 1.0f);
    material[22].shininiess = 0.078125f * 128;

    material[23].ambient = vec4(0.05, 0.05, 0.0f, 1.0f);
    material[23].diffuse = vec4(0.5f, 0.5f, 0.4f, 1.0f);
    material[23].specular = vec4(0.7f, 0.7f, 0.04f, 1.0f);
    material[23].shininiess = 0.078125f * 128;
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

    // free vbo of position
    if (gVbo_sphere_position) {
        glDeleteBuffers(1, &gVbo_sphere_position);
        gVbo_sphere_position = NULL;
    }

    // free vao
    if (gVao_sphere) {
        glDeleteVertexArrays(1, &gVao_sphere);
        gVao_sphere = NULL;
    }

    // detach, delete shader objects and delete shader program object
    if (shaderProgramObject_PF) {
        glUseProgram(shaderProgramObject_PF);
        GLint numShaders;
        glGetProgramiv(shaderProgramObject_PF, GL_ATTACHED_SHADERS, &numShaders);
        if (numShaders > 0) {
            GLuint* pShaders = (GLuint*)malloc(numShaders * sizeof(GLuint));
            if (pShaders) {
                glGetAttachedShaders(shaderProgramObject_PF, numShaders, NULL, pShaders);
                for (GLint i = 0; i < numShaders; ++i) {
                    glDetachShader(shaderProgramObject_PF, pShaders[i]);
                    glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
                free(pShaders);
            }
        }
        glUseProgram(0);
        glDeleteProgram(shaderProgramObject_PF);
        shaderProgramObject_PF = 0;
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
