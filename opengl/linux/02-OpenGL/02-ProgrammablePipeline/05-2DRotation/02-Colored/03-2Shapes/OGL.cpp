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

float angleTriangle;
float angleRectangle;

// opengl related variables
GLXContext glxContext = NULL;

// file io
char gszLogFileName[] = "log.txt";
FILE* gpFile = NULL;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
};

// shader related variables
GLuint shaderProgramObject = 0;

GLuint vao_triangle           = 0;
GLuint vao_rectangle          = 0;
GLuint vbo_position_triangle  = 0;
GLuint vbo_position_rectangle = 0;
GLuint vbo_color_triangle     = 0;
GLuint vbo_color_rectangle    = 0;

GLuint mvpMatrixUniform = 0;
mat4 perspectiveProjectionMatrix;

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
                            bDone = True;
                            break;

                        default:
                            break;
                    }
                    
                    // for alphabetic keyPress
                    XLookupString(&event.xkey, keys, sizeof(keys), NULL, NULL);
                    switch(keys[0]) {
                        case 'F':
                        case 'f':
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

    const GLchar* vertexShaderSourceCode = 
        "#version 460 core \n" \
        "in vec4 aPosition; \n" \
        "in vec4 aColor; \n" \
        "out vec4 out_color; \n" \
        "uniform mat4 uMVPMatrix; \n" \
        "void main(void) \n" \
        "{ \n" \
        "   gl_Position = uMVPMatrix * aPosition; \n" \
        "   out_color = aColor; \n" \
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
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
        "   FragColor = out_color; \n" \
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
    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");

    // provide vertex position, color, normal, textcord, etc
    const GLfloat triangle_position[] = {   
                                            0.0f, 1.0f, 0.0f, 
                                            -1.0f, -1.0f, 0.0f,
                                            1.0f, -1.0f, 0.0f 
                                        };

    const GLfloat rectangle_position[] = {   
                                            1, 1, 0,
                                            -1, 1, 0,
                                            -1, -1, 0,
                                            1, -1, 0
                                        };

    const GLfloat triangle_color[] = {   
                                            1.0f, 0.0f, 0.0f, 
                                            0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f 
                                        };

    const GLfloat rectangle_color[] = {   
                                            0, 0, 1,
                                            0, 0, 1,
                                            0, 0, 1,
                                            0, 0, 1
                                        };

    // Triangle
    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao_triangle);
    glBindVertexArray(vao_triangle);

    // position
    glGenBuffers(1, &vbo_position_triangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_triangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_position), triangle_position, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // color
    glGenBuffers(1, &vbo_color_triangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color_triangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_color), triangle_color, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // Rectangle
    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao_rectangle);
    glBindVertexArray(vao_rectangle);

    // position
    glGenBuffers(1, &vbo_position_rectangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_rectangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_position), rectangle_position, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // color
    glGenBuffers(1, &vbo_color_rectangle);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_color_rectangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_color), rectangle_color, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_COLOR, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_COLOR);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

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
    glUseProgram(shaderProgramObject);
    
    // Triangle
    // transformations
    mat4 modelViewMatrix = mat4::identity();
    mat4 modelViewProjectionMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    translationMatrix = vmath::translate(-1.5f, 0.0f, -5.0f);
    mat4 rotationMatrix = mat4::identity();
    rotationMatrix = vmath::rotate(angleTriangle, 0.0f, 1.0f, 0.0f);

    modelViewMatrix = translationMatrix * rotationMatrix;

    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    // send this matrix to vertex shader in uniform
    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);

    // bind with vao
    glBindVertexArray(vao_triangle);

    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // unbind with vao
    glBindVertexArray(0);

    // Rectangle
    // transformations
    modelViewMatrix = mat4::identity();
    modelViewProjectionMatrix = mat4::identity();
    translationMatrix = mat4::identity();
    translationMatrix = vmath::translate(1.5f, 0.0f, -5.0f);
    rotationMatrix = mat4::identity();
    rotationMatrix = vmath::rotate(angleRectangle, 1.0f, 0.0f, 0.0f);

    modelViewMatrix = translationMatrix * rotationMatrix;

    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    // send this matrix to vertex shader in uniform
    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);

    // bind with vao
    glBindVertexArray(vao_rectangle);

    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // unbind with vao
    glBindVertexArray(0);


    // unuse shader program object
    glUseProgram(0);

    // swap the buffers
    glXSwapBuffers(gpDisplay, window);
}

void update(void)
{
    // code
    angleTriangle += 0.2f;
    angleRectangle += 0.2f;
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
    if (vbo_position_rectangle) {
        glDeleteBuffers(1, &vbo_position_rectangle);
        vbo_position_rectangle = 0;
    }
    if (vao_rectangle) {
        glDeleteVertexArrays(1, &vao_rectangle);
        vao_rectangle = 0;
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
