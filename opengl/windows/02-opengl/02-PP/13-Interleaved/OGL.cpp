// standard header files
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

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

GLuint vao_cube = 0;
GLuint vbo = 0;

GLuint modelMatrixUniform = 0;
GLuint viewMatrixUniform = 0;
GLuint projectionMatrixUniform = 0;
GLuint LaUniform = 0;                   // ambient light
GLuint LdUniform = 0;                   // diffuse
GLuint LsUniform = 0;                   // specular
GLuint KaUniform = 0;                   // ambient material
GLuint KdUniform = 0;
GLuint KsUniform = 0;
GLuint materialShininessUniform = 0;
GLuint lightPositionUniform = 0;
GLuint LKeyPressUniform = 0;

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
    mat4 modelMatrix = mat4::identity();
    mat4 viewMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    mat4 rotationMatrix = mat4::identity();
    mat4 rotationMatrix1 = mat4::identity();
    mat4 rotationMatrix2 = mat4::identity();
    mat4 rotationMatrix3 = mat4::identity();
    mat4 scaleMatrix = mat4::identity();

    scaleMatrix = vmath::scale(0.75f, 0.75f, 0.75f);
    translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
    rotationMatrix1 = vmath::rotate(angleCube, 1.0f, 0.0f, 0.0f);
    rotationMatrix2 = vmath::rotate(angleCube, 0.0f, 1.0f, 0.0f);
    rotationMatrix3 = vmath::rotate(angleCube, 0.0f, 0.0f, 1.0f);
    rotationMatrix = rotationMatrix1 * rotationMatrix2 * rotationMatrix3;
    
    modelMatrix = translationMatrix * scaleMatrix * rotationMatrix;

    // send this matrix to vertex shader in uniform
    glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, modelMatrix);
    glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, viewMatrix);
    glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, perspectiveProjectionMatrix);

    if (bLight == TRUE) {
        glUniform3fv(LaUniform, 1, lightAmbient);
        glUniform3fv(LdUniform, 1, lightDiffuse);
        glUniform3fv(LsUniform, 1, lightSpecular);
        glUniform4fv(lightPositionUniform, 1, lightPosition);
        glUniform3fv(KaUniform, 1, materialAmbient);
        glUniform3fv(KdUniform, 1, materialDiffuse);
        glUniform3fv(KsUniform, 1, materialSpecular);
        glUniform1f(materialShininessUniform, materialShininiess);
        glUniform1i(LKeyPressUniform, 1);
    } else {
        glUniform1i(LKeyPressUniform, 0);
    }

    // for texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_marble);
    glUniform1i(textureSamplerUniform, 0);

    // bind with vao
    glBindVertexArray(vao_cube);

    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
    glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

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
        "in vec3 aNormal; \n" \
        "in vec4 aColor; \n" \
        "out vec4 out_color; \n" \
        "in vec2 aTexCoord; \n" \
        "out vec2 out_texcoord; \n" \
        "out vec3 out_transformedNormals; \n" \
        "out vec3 out_lightDirection; \n" \
        "out vec3 out_viewerVector; \n" \
        "uniform mat4 uModelMatrix; \n" \
        "uniform mat4 uViewMatrix; \n" \
        "uniform mat4 uProjectionMatrix; \n" \
        "uniform vec4 uLightPosition; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) \n" \
        "{ \n" \
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n" \
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n" \
                "out_transformedNormals = normalMatrix * aNormal; \n" \
                "out_lightDirection = vec3(uLightPosition - eyeCoordinates); \n" \
                "out_viewerVector = -eyeCoordinates.xyz; \n" \
            "} \n" \
            "out_color = aColor; \n" \
            "out_texcoord = aTexCoord; \n" \
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
        "in vec4 out_color; \n" \
        "in vec2 out_texcoord; \n" \
        "uniform sampler2D uTextureSampler; \n" \
        "in vec3 out_transformedNormals; \n" \
        "in vec3 out_lightDirection; \n" \
        "in vec3 out_viewerVector; \n" \
        "out vec4 FragColor; \n" \
        "uniform vec3 uLa; \n" \
        "uniform vec3 uLd; \n" \
        "uniform vec3 uLs; \n" \
        "uniform vec3 uKa; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec3 uKs; \n" \
        "uniform float uMaterialShininess; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void)\n" \
        "{\n" \
            "vec3 phong_ads_light; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n" \
                "vec3 normalizedLightDirection = normalize(out_lightDirection); \n" \
                "vec3 normalizedViewerVector = normalize(out_viewerVector); \n" \
                "vec3 ambientLight = uLa * uKa * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n" \
                "vec3 diffuseLight = uLd * uKd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n" \
                "vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n" \
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialShininess); \n" \
                "phong_ads_light = ambientLight + diffuseLight + specularLight; \n" \
            "} \n" \
            "else { \n" \
                "phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n" \
            "} \n" \
            "vec4 tex = texture(uTextureSampler, out_texcoord); \n"
            "vec4 texColor = out_color * tex; \n" \
            "FragColor = vec4(phong_ads_light, 1.0f) * texColor; \n" \
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
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_NORMAL, "aNormal");
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_TEXTCORD, "aTexCoord");
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_COLOR, "aColor");
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
    modelMatrixUniform = glGetUniformLocation(shaderProgramObject, "uModelMatrix");
    viewMatrixUniform = glGetUniformLocation(shaderProgramObject, "uViewMatrix");
    projectionMatrixUniform = glGetUniformLocation(shaderProgramObject, "uProjectionMatrix");
    LaUniform = glGetUniformLocation(shaderProgramObject, "uLa");
    LdUniform = glGetUniformLocation(shaderProgramObject, "uLd");
    LsUniform = glGetUniformLocation(shaderProgramObject, "uLs");
    lightPositionUniform = glGetUniformLocation(shaderProgramObject, "uLightPosition");
    KaUniform = glGetUniformLocation(shaderProgramObject, "uKa");
    KdUniform = glGetUniformLocation(shaderProgramObject, "uKd");
    KsUniform = glGetUniformLocation(shaderProgramObject, "uKs");
    materialShininessUniform = glGetUniformLocation(shaderProgramObject, "uMaterialShininess");
    LKeyPressUniform = glGetUniformLocation(shaderProgramObject, "uLKeyIsPressed");
    textureSamplerUniform = glGetUniformLocation(shaderProgramObject, "uTextureSampler");

    const GLfloat cube_PCNT[] = {
        // front
	    // position				// color			 // normals				// texcoords
	     1.0f,  1.0f,  1.0f,	1.0f, 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f,	1.0f, 1.0f,
	    -1.0f,  1.0f,  1.0f,	1.0f, 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f,	0.0f, 1.0f,
	    -1.0f, -1.0f,  1.0f,	1.0f, 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f,	0.0f, 0.0f,
    	 1.0f, -1.0f,  1.0f,	1.0f, 0.0f, 0.0f,	 0.0f,  0.0f,  1.0f,	1.0f, 0.0f,
						 
	    // right			 
	    // position				// color			 // normals				// texcoords
	     1.0f,  1.0f, -1.0f,	0.0f, 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
	     1.0f,  1.0f,  1.0f,	0.0f, 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
	     1.0f, -1.0f,  1.0f,	0.0f, 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
	     1.0f, -1.0f, -1.0f,	0.0f, 0.0f, 1.0f,	 1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
						 
	    // back				 
	    // position				// color			 // normals				// texcoords
	     1.0f,  1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
	    -1.0f,  1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f,	0.0f, 1.0f,
	    -1.0f, -1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f,	0.0f, 0.0f,
	     1.0f, -1.0f, -1.0f,	1.0f, 1.0f, 0.0f,	 0.0f,  0.0f, -1.0f,	1.0f, 0.0f,
						 
	    // left				 
    	// position				// color			 // normals				// texcoords
        -1.0f,  1.0f,  1.0f,	1.0f, 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,	1.0f, 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,	1.0f, 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	0.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,	1.0f, 0.0f, 1.0f,	-1.0f,  0.0f,  0.0f,	1.0f, 0.0f,
                            
	    // top				 
	    // position				// color			 // normals				// texcoords
	     1.0f,  1.0f, -1.0f,	0.0f, 1.0f, 0.0f,	 0.0f,  1.0f,  0.0f,	1.0f, 1.0f,
	    -1.0f,  1.0f, -1.0f,	0.0f, 1.0f, 0.0f,	 0.0f,  1.0f,  0.0f,	0.0f, 1.0f,
	    -1.0f,  1.0f,  1.0f,	0.0f, 1.0f, 0.0f,	 0.0f,  1.0f,  0.0f,	0.0f, 0.0f,
    	 1.0f,  1.0f,  1.0f,	0.0f, 1.0f, 0.0f,	 0.0f,  1.0f,  0.0f,	1.0f, 0.0f,
						 
	    // bottom			 
	    // position				// color			 // normals				// texcoords
	     1.0f, -1.0f,  1.0f,	1.0f, 0.5f, 0.0f,	 0.0f, -1.0f,  0.0f,	1.0f, 1.0f,
	    -1.0f, -1.0f,  1.0f,	1.0f, 0.5f, 0.0f,	 0.0f, -1.0f,  0.0f,	0.0f, 1.0f,
	    -1.0f, -1.0f, -1.0f,	1.0f, 0.5f, 0.0f,	 0.0f, -1.0f,  0.0f,	0.0f, 0.0f,
	     1.0f, -1.0f, -1.0f,	1.0f, 0.5f, 0.0f,	 0.0f, -1.0f,  0.0f,	1.0f, 0.0f,
    };

    // Rectangle
    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao_cube);
    glBindVertexArray(vao_cube);

    // common vbo for pcnt

    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_PCNT), cube_PCNT, GL_STATIC_DRAW);                // android (24 * 11 * 4 for sizeof)
    
    // position
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(0 * sizeof(float)));
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);

    // color
    glVertexAttribPointer(AMC_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(AMC_ATTRIBUTE_COLOR);
    
    // normal
    glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);

    // texture
    glVertexAttribPointer(AMC_ATTRIBUTE_TEXTCORD, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(AMC_ATTRIBUTE_TEXTCORD);

    glBindVertexArray(0);

    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (loadGLTexture(&texture_marble, MAKEINTRESOURCE(IDBITMAP_MARBLE)) == FALSE) {
        fprintf(gpFile, "loadGLTexture failed\n");
        return (-7);
    }

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
    // free vbo of color
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = NULL;
    }
    if (vao_cube) {
        glDeleteVertexArrays(1, &vao_cube);
        vao_cube = NULL;
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
