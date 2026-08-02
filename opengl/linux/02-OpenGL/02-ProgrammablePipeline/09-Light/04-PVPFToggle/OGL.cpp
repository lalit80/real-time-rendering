#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

// open header files
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

// custom header files
#include "vmath.h"
#include "Sphere.h"
using namespace vmath;

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo* visualInfo = NULL;                 // hardware information (graphic card)
Window window;
Colormap colormap;

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
GLXFBConfig glxFBConfig;

Bool bFullScreen = False;
Bool bActiveWindow = False;

// opengl related variables
GLXContext glxContext = NULL;

// file io
char gszLogFileName[] = "log.txt";
FILE* gpFile = NULL;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_NORMAL,
};

GLuint gNumVertices;
GLuint gNumElements;
float sphere_vertices[1146];
float sphere_normals[1146];
float sphere_textures[764];
unsigned short sphere_elements[2280];

// shader related variables
// shader related variables
GLuint shaderProgramObject_PF = 0;
GLuint shaderProgramObject_PV = 0;

GLuint gVao_sphere = 0;
GLuint gVbo_sphere_position = 0;
GLuint gVbo_sphere_normal = 0;
GLuint gVbo_sphere_element = 0;

GLuint modelMatrixUniform_PV = 0;
GLuint viewMatrixUniform_PV = 0;
GLuint projectionMatrixUniform_PV = 0;
GLuint LaUniform_PV = 0;                   // ambient light
GLuint LdUniform_PV = 0;                   // diffuse
GLuint LsUniform_PV = 0;                   // specular
GLuint KaUniform_PV = 0;                   // ambient material
GLuint KdUniform_PV = 0;
GLuint KsUniform_PV = 0;
GLuint materialShininessUniform_PV = 0;
GLuint lightPositionUniform_PV = 0;
GLuint LKeyPressUniform_PV = 0;

GLuint modelMatrixUniform_PF = 0;
GLuint viewMatrixUniform_PF = 0;
GLuint projectionMatrixUniform_PF = 0;
GLuint LaUniform_PF = 0;                   // ambient light
GLuint LdUniform_PF = 0;                   // diffuse
GLuint LsUniform_PF = 0;                   // specular
GLuint KaUniform_PF = 0;                   // ambient material
GLuint KdUniform_PF = 0;
GLuint KsUniform_PF = 0;
GLuint materialShininessUniform_PF = 0;
GLuint lightPositionUniform_PF = 0;
GLuint LKeyPressUniform_PF = 0;

mat4 perspectiveProjectionMatrix;

GLfloat lightAmbient[] = {0.1f, 0.1f, 0.1f, 1.0f};
GLfloat lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat lightPosition[] = {100.0f, 100.0f, 100.0f, 1.0f};

GLfloat materialAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat materialDiffuse[] = {0.5f, 0.2f, 0.7f, 1.0f};
GLfloat materialSpecular[] = {0.7f, 0.7f, 0.7f, 1.0f};
GLfloat materialShininiess = 50.0f;
Bool bLight = False;
Bool bPerFragment = False;
Bool bPerVertex = True;

int main(void)
{
    // function declarations
    void toggleFullScreen(void);
    int initialize(void);
    void resize(int, int);
    void display(void);
    void update(void);
    void uninitialize(void);

    // variable declarations
    int defaultScreen;
    int defaultDepth;
    XSetWindowAttributes windowAttributes;
    Atom windowManagerDeleteAtom;
    XEvent event;
    Screen* screen = NULL;
    int screenWidth, screenHeight;
    KeySym keySim;
    char keys[26];
    GLXFBConfig *pGLXFBConfigs = NULL;
    GLXFBConfig bestGLXFBConfig;
    XVisualInfo* pXVisualInfo = NULL;
    int iNumFBConfigs;

    // glx double buffer true
    int framebufferAttributes[] = { 
                                    GLX_X_RENDERABLE, True,
                                    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                                    GLX_RENDER_TYPE, GLX_RGBA_BIT,
                                    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
                                    GLX_DOUBLEBUFFER, True,
                                    GLX_RED_SIZE, 8,
                                    GLX_GREEN_SIZE, 8,
                                    GLX_BLUE_SIZE, 8,
                                    GLX_ALPHA_SIZE, 8, 
                                    GLX_DEPTH_SIZE, 24, 
                                    GLX_STENCIL_SIZE, 8, None};
    Bool bDone = False;

    gpFile = fopen(gszLogFileName, "w");
    if(gpFile == NULL) {
        printf("Log file creation failed");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(gpFile, "Program started successfully\n");
    }

    // code
    // open the connection with the XServer
    gpDisplay = XOpenDisplay(NULL);     // establish a connection (client to server)
    if (gpDisplay == NULL) {
        fprintf(gpFile, "XOpenDisplay failed to connect with server\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create the default screen object
    defaultScreen = XDefaultScreen(gpDisplay);
    // get default depth
    defaultDepth = XDefaultDepth(gpDisplay, defaultScreen);

    pGLXFBConfigs = glXChooseFBConfig(gpDisplay, defaultScreen, framebufferAttributes, &iNumFBConfigs);
    if (pGLXFBConfigs == NULL) {
        fprintf(gpFile, "glXChooseFBConfig failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }
    //fprintf(gpFile, "Found no of FBConfigs %d\n", iNumFBConfigs);

    int indexOfBestFBConfig = -1, indexOfWorstFBConfig = -1;
    int bestNoOfSamples = -1, worstNoOfSamples = 999;

    for (int i = 0; i < iNumFBConfigs; ++i) {
        pXVisualInfo = glXGetVisualFromFBConfig(gpDisplay, pGLXFBConfigs[i]);
        if (pXVisualInfo) {
            int sampleBuffer, samples;
            glXGetFBConfigAttrib(gpDisplay, pGLXFBConfigs[i], GLX_SAMPLE_BUFFERS, &sampleBuffer);
            glXGetFBConfigAttrib(gpDisplay, pGLXFBConfigs[i], GLX_SAMPLES, &samples);
            if (indexOfBestFBConfig < 0 || sampleBuffer && samples > bestNoOfSamples) {
                indexOfBestFBConfig = i;
                bestNoOfSamples = samples;
            }
            if (indexOfWorstFBConfig < 0 || !sampleBuffer || samples < worstNoOfSamples) {
                indexOfWorstFBConfig = i;
                worstNoOfSamples = samples;
            }
        }
        XFree(pXVisualInfo);
        pXVisualInfo = NULL;
    }
    bestGLXFBConfig = pGLXFBConfigs[indexOfBestFBConfig];
    glxFBConfig = bestGLXFBConfig;
    XFree(pGLXFBConfigs);
    visualInfo = glXGetVisualFromFBConfig(gpDisplay, glxFBConfig);

    // set window attributes
    memset((void*)&windowAttributes, 0, sizeof(XSetWindowAttributes));
    windowAttributes.border_pixel = 0;
    windowAttributes.background_pixmap = 0;
    windowAttributes.background_pixel = XBlackPixel(gpDisplay, visualInfo->screen);
    windowAttributes.colormap = XCreateColormap(gpDisplay, 
                                                    XRootWindow(gpDisplay, visualInfo->screen),
                                                    visualInfo->visual, AllocNone);
    windowAttributes.event_mask = KeyPressMask | ButtonPressMask | FocusChangeMask | StructureNotifyMask | ExposureMask;

    colormap = windowAttributes.colormap;

    // create the window
    window = XCreateWindow(gpDisplay,
                            XRootWindow(gpDisplay, visualInfo->screen),
                            0, 0, WIN_WIDTH, WIN_HEIGHT,
                            0, visualInfo->depth, InputOutput,
                            visualInfo->visual,
                            CWBorderPixel | CWBackPixel | CWEventMask | CWColormap,
                            &windowAttributes);
    if (!window) {
        fprintf(gpFile, "XCreateWindow failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create atom for window manager to destroy the window
    windowManagerDeleteAtom = XInternAtom(gpDisplay, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(gpDisplay, window, &windowManagerDeleteAtom, 1);

    // set window title
    XStoreName(gpDisplay, window, "LRC : XWindow");

    // map the window to show it
    XMapWindow(gpDisplay, window);

    // centering of window
    screen = XScreenOfDisplay(gpDisplay, visualInfo->screen);
    screenWidth = XWidthOfScreen(screen);
    screenHeight = XHeightOfScreen(screen);
    XMoveWindow(gpDisplay, window, (screenWidth / 2) - (WIN_WIDTH / 2), (screenHeight / 2) - (WIN_HEIGHT / 2));

    // initialize
    int iResult = initialize();
    if (iResult == -1) {
        fprintf(gpFile, "initialize failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // game loop
    while (bDone == False) {
        while (XPending(gpDisplay)) {
            XNextEvent(gpDisplay, &event);
            switch (event.type) {
                case MapNotify:
                    break;

                case FocusIn:
                    bActiveWindow = True;
                    break;

                case FocusOut:
                    bActiveWindow = False;
                    break;

                case ConfigureNotify:
                    resize(event.xconfigure.width, event.xconfigure.height);
                    break;

                case KeyPress:
                    keySim = XkbKeycodeToKeysym(gpDisplay, event.xkey.keycode, 0, 0);
                    switch(keySim) {
                        case XK_Escape:
                            if (bFullScreen == False) {
                                toggleFullScreen();
                                bFullScreen = True;
                            }
                            else {
                                toggleFullScreen();
                                bFullScreen = False;
                            }
                            break;

                        default:
                            break;
                    }
                    
                    // for alphabetic keyPress
                    XLookupString(&event.xkey, keys, sizeof(keys), NULL, NULL);
                    switch(keys[0]) {
                        case 'F':
                        case 'f':
                            if (bPerFragment == False) {
                                bPerFragment = True;
                                bPerVertex = False;
                            }
                            break;

                        case 'v':
                        case 'V':
                            if (bPerVertex == False) {
                                bPerVertex = True;
                                bPerFragment = False;
                            }
                            break;

                        case 'L':
                        case 'l':
                            if (bLight == False) {
                                bLight = True;
                            } else {
                                bLight = False;
                            }
                            break;

                        case 'Q':
                        case 'q':
                            bDone = True;
                            break;

                        default:
                            break;
                    }
                    break;

                case Expose:
                    break;

                case 33:
                    bDone = True;
                    break;
                
                default:
                    break;
            }
        }

        // rendering
        if (bActiveWindow == True) {
            display();
            update();
        }
    }

    uninitialize();
    return (0);
}

int initialize(void)
{
    void printGLInfo(void);
    void uninitialize(void);

    // code
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((GLubyte*)"glXCreateContextAttribsARB");
    // declare attributes array
    GLint attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 4, 
                        GLX_CONTEXT_MINOR_VERSION_ARB, 5, 
                        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};
    glxContext = glXCreateContextAttribsARB(gpDisplay, glxFBConfig, 0, True, attribs);
    if (!glxContext) {
        GLint attribs_legacy[] = { GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
                        GLX_CONTEXT_MINOR_VERSION_ARB, 0, None};
        glxContext = glXCreateContextAttribsARB(gpDisplay, glxFBConfig, 0, True, attribs_legacy);
        fprintf(gpFile, "Cannot get opengl context 4.5 but recieved lesser context\n");
    } else {
        fprintf(gpFile, "recieved opengl context 4.5\n");
    }
    
    glXIsDirect(gpDisplay, glxContext);
    glXMakeCurrent(gpDisplay, window, glxContext);

    // initialize glew
    int glewResult = glewInit();
    if (glewResult != GLEW_OK) {
        fprintf(gpFile, "glewInit() failed\n");
        return (-6);
    }

    printGLInfo();

    // vertex shader steps
    // write the shader source code
    // create the shader object
    // give the shader src code to shader object
    // compile the shader
    // do compilation error checking
    // create shader program object
    // attach shader objects to shader program objects
    // tell to link shader objects to shader program objects
    // check for link error log

    // per freagment light
    const GLchar* vertexShaderSourceCode_PF = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "in vec3 aNormal; \n" \
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
        "in vec3 out_lightDirection; \n" \
        "in vec3 out_viewerVector; \n" \
        "out vec4 FragColor; \n" \
        "uniform vec3 uLa; \n" \
        "uniform vec3 uLd; \n" \
        "uniform vec3 uLs; \n" \
        "uniform vec3 uKa; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec3 uKs; \n" \
        "uniform float uMaterialSininess; \n" \
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
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialSininess); \n" \
                "phong_ads_light = ambientLight + diffuseLight + specularLight; \n" \
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
    LaUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLa");
    LdUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLd");
    LsUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLs");
    lightPositionUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLightPosition");
    KaUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKa");
    KdUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKd");
    KsUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uKs");
    materialShininessUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uMaterialSininess");
    LKeyPressUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLKeyIsPressed");

    // per vertex light
    const GLchar* vertexShaderSourceCode_PV = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "in vec3 aNormal; \n" \
        "out vec3 out_phong_ads_light; \n" \
        "uniform mat4 uModelMatrix; \n" \
        "uniform mat4 uViewMatrix; \n" \
        "uniform mat4 uProjectionMatrix; \n" \
        "uniform vec3 uLa; \n" \
        "uniform vec3 uLd; \n" \
        "uniform vec3 uLs; \n" \
        "uniform vec4 uLightPosition; \n" \
        "uniform vec3 uKa; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec3 uKs; \n" \
        "uniform float uMaterialSininess; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) \n" \
        "{ \n" \
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n" \
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n" \
                "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n" \
                "vec3 lightDirection = normalize(vec3(uLightPosition - eyeCoordinates)); \n" \
                "vec3 ambientLight = uLa * uKa * max(dot(lightDirection, transformedNormal), 0.0f); \n" \
                "vec3 diffuseLight = uLd * uKd * max(dot(lightDirection, transformedNormal), 0.0f); \n" \
                "vec3 reflectionVector = reflect(-lightDirection, transformedNormal); \n" \
                "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n" \
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, viewerVector), 0.0f), uMaterialSininess); \n" \
                "out_phong_ads_light = ambientLight + diffuseLight + specularLight; \n" \
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
    LaUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLa");
    LdUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLd");
    LsUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLs");
    lightPositionUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLightPosition");
    KaUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKa");
    KdUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKd");
    KsUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uKs");
    materialShininessUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uMaterialSininess");
    LKeyPressUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uLKeyIsPressed");

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

    glClearColor(0, 0, 0, 1);
    perspectiveProjectionMatrix = mat4::identity();

    return 0;
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

void display(void)
{
    // code
    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use shader program object
    if (bPerFragment) {
        glUseProgram(shaderProgramObject_PF);

        // transformations
        mat4 modelMatrix = mat4::identity();
        mat4 viewMatrix = mat4::identity();
        mat4 translationMatrix = mat4::identity();
        translationMatrix = vmath::translate(0.0f, 0.0f, -2.0f);
        modelMatrix = translationMatrix;

        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(modelMatrixUniform_PF, 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(viewMatrixUniform_PF, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PF, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == True) {
            glUniform3fv(LaUniform_PF, 1, lightAmbient);
            glUniform3fv(LdUniform_PF, 1, lightDiffuse);
            glUniform3fv(LsUniform_PF, 1, lightSpecular);
            glUniform4fv(lightPositionUniform_PF, 1, lightPosition);
            glUniform3fv(KaUniform_PF, 1, materialAmbient);
            glUniform3fv(KdUniform_PF, 1, materialDiffuse);
            glUniform3fv(KsUniform_PF, 1, materialSpecular);
            glUniform1f(materialShininessUniform_PF, materialShininiess);
            glUniform1i(LKeyPressUniform_PF, 1);
        } else {
            glUniform1i(LKeyPressUniform_PF, 0);
        }
    } 
    else {
        glUseProgram(shaderProgramObject_PV);
        // transformations
        mat4 modelMatrix = mat4::identity();
        mat4 viewMatrix = mat4::identity();
        mat4 translationMatrix = mat4::identity();
        translationMatrix = vmath::translate(0.0f, 0.0f, -2.0f);
        modelMatrix = translationMatrix;

        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(modelMatrixUniform_PV, 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(viewMatrixUniform_PV, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PV, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == True) {
            glUniform3fv(LaUniform_PV, 1, lightAmbient);
            glUniform3fv(LdUniform_PV, 1, lightDiffuse);
            glUniform3fv(LsUniform_PV, 1, lightSpecular);
            glUniform4fv(lightPositionUniform_PV, 1, lightPosition);
            glUniform3fv(KaUniform_PV, 1, materialAmbient);
            glUniform3fv(KdUniform_PV, 1, materialDiffuse);
            glUniform3fv(KsUniform_PV, 1, materialSpecular);
            glUniform1f(materialShininessUniform_PV, materialShininiess);
            glUniform1i(LKeyPressUniform_PV, 1);
        } else {
            glUniform1i(LKeyPressUniform_PV, 0);
        }
    }


    // *** bind vao ***
    glBindVertexArray(gVao_sphere);

    // *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);
    glDrawElements(GL_TRIANGLES, gNumElements, GL_UNSIGNED_SHORT, 0);

    // *** unbind vao ***
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);

    // swap the buffers
    glXSwapBuffers(gpDisplay, window);
}

void update(void)
{
    // code
}

void uninitialize(void)
{
    GLXContext currentContext = glXGetCurrentContext();
    if (currentContext && currentContext == glxContext) {
        glXMakeCurrent(gpDisplay, 0, 0);
    }
    if (glxContext) {
        glXDestroyContext(gpDisplay, glxContext);
        glxContext = NULL;
    }
    if (visualInfo) {
        free(visualInfo);
        visualInfo = NULL;
    }   
    if (window) {
        XDestroyWindow(gpDisplay, window);
    }
    if (colormap) {
        XFreeColormap(gpDisplay, colormap);
    }
    if (gpDisplay) {
        XCloseDisplay(gpDisplay);
        gpDisplay = NULL;
    }
    // close the file
    if(gpFile) {
        fprintf(gpFile, "prorgam terminated successfully\n");
        fclose(gpFile);
        gpFile = NULL;
    }
}

void toggleFullScreen(void)
{
    Atom windowManagerNormalStateAtom = XInternAtom(gpDisplay, "_NET_WM_STATE", False);
    Atom windowManagerFullScreenStateAtom = XInternAtom(gpDisplay, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent event;
    memset((void*)&event, 0, sizeof(XEvent));
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = windowManagerNormalStateAtom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = bFullScreen ? 0 : 1;
    event.xclient.data.l[1] = windowManagerFullScreenStateAtom;

    // send above event to xserver
    XSendEvent(gpDisplay,
                XRootWindow(gpDisplay, visualInfo->screen),
                False, SubstructureNotifyMask, &event);
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
