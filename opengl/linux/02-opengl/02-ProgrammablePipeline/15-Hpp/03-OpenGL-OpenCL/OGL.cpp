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
#define MESH_WIDTH  1024
#define MESH_HEIGHT 1024
#define MESH_DEPTH  4

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
    AMC_ATTRIBUTE_COLOR,
};

// shader related variables
GLuint shaderProgramObject = 0;

GLuint vao;
GLuint vbo_cpu = 0;

GLuint mvpMatrixUniform = 0;
mat4 perspectiveProjectionMatrix;

// sine wave related variables
unsigned int gMeshWidth = 512;
unsigned int gMeshHeight = 512;
unsigned int gMeshDepth = 4;
#define MESH_ARRAY_SIZE (MESH_WIDTH * MESH_HEIGHT * MESH_DEPTH)
float position[MESH_WIDTH * MESH_HEIGHT * MESH_DEPTH];
vmath::vec3 color = {1.0f, 0.0f, 0.0f};
GLuint colorUniform = 0;

GLuint vbo_gpu = 0;
float animationTime = 0.0f;
int keyPress;
Bool onGPU = False;

// OpenCL headers and related variables
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/opencl.h>

cl_int oclResult;
cl_mem oclGraphicsResource = NULL;

cl_device_id oclDeviceID;
cl_context oclContext;
cl_command_queue oclCommandQueue;
cl_program oclProgram;
cl_kernel oclkernel;

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

                        case 'c':
                        case 'C':
                            onGPU = False;
                            break;

                        case 'd':
                        case 'D':
                            onGPU = True;
                            break;

                        case 'r':
                        case 'R':
                            color = {1.0f, 0.0f, 0.0f};
                            break;

                        case 'g':
                        case 'G':
                            color = {0.0f, 1.0f, 0.0f};
                            break;

                        case 'b':
                        case 'B':
                            color = {0.0f, 0.0f, 1.0f};
                            break;

                        case '1':
                            keyPress = 1;
                            gMeshWidth = 64;
                            gMeshHeight = 64;
                            break;

                        case '2':
                            keyPress = 2;
                            gMeshWidth = 128;
                            gMeshHeight = 128;
                            break;

                        case '3':
                            keyPress = 3;
                            gMeshWidth = 256;
                            gMeshHeight = 256;
                            break;

                        case '4':
                            keyPress = 4;
                            gMeshWidth = 512;
                            gMeshHeight = 512;
                            break;

                        case '5':
                            keyPress = 5;
                            gMeshWidth = 1024;
                            gMeshHeight = 1024;
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

    // opencl related initialization
    cl_platform_id oclPlatformID;
    cl_device_id* oclDeviceIDs = NULL;
    cl_uint devCount = 0;

    // get OpenCL supporting CPU device's ID
    oclResult = clGetPlatformIDs(1, &oclPlatformID, NULL);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clGetPlatformIDs() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // get OpenCL supporting CPU device's ID
    oclResult = clGetDeviceIDs(oclPlatformID, CL_DEVICE_TYPE_GPU,
                                    0, NULL, &devCount);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clGetDeviceIDs() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    } else if (devCount == 0) {
        fprintf(gpFile, "no openlcl supported device found\n");
        uninitialize();
        exit(-1);
    }
    
    // accordingly allocate memory to out deviceID array
    oclDeviceIDs = (cl_device_id*)malloc(sizeof(cl_device_id) * devCount);
    if (!oclDeviceIDs) {
        fprintf(gpFile, "malloc() Failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }
    
    // now fill our deviceID array by calling clGetDeviceID
    oclResult = clGetDeviceIDs(oclPlatformID, CL_DEVICE_TYPE_GPU, devCount, oclDeviceIDs, NULL);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "2nd call to clGetDeviceIDs() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }
    // select one, zero'th device
    oclDeviceID = oclDeviceIDs[0];

    free(oclDeviceIDs);

    cl_context_properties oclContextProperties[] = {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),               // glxGetCurrentContext
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),              // CL_GLX_DISPLAY_KHR, glxGetCurrentDisplay
        CL_CONTEXT_PLATFORM, (cl_context_properties)oclPlatformID,
        0
    };

    // create OpenCL context from above array
    oclContext = clCreateContext(oclContextProperties, 1, &oclDeviceID, NULL, NULL, &oclResult);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clCreateContext() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create opencl command queue
    oclCommandQueue = clCreateCommandQueueWithProperties(oclContext, oclDeviceID, 0, &oclResult);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clCreateCommandQueueWithProperties() Failed %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // write opencl kernel source code
    const char* oclKernelSourceCode = 
    "__kernel void sineWaveKernel(__global float4* pos, unsigned int width, unsigned int height, float time)" \
    "{" \
    "    unsigned int i = get_global_id(0);" \
    "    unsigned int j = get_global_id(1);" \
    "    float u = ((float)i / (float)width);" \
    "    float v = ((float)j / (float)height);" \
    "    u = u * 2.0f - 1.0f;" \
    "    v = v * 2.0f - 1.0f;" \
    "    float frequency = 4.0f;" \
    "    float w = sin(u * frequency + time) * cos(v * frequency + time) * 0.5;" \
    "    pos[j * width + i] = (float4)(u, w, v, 1.0f);" \
    "}";

    // create OpenCL program using above kernel src code
    oclProgram = clCreateProgramWithSource(oclContext, 1, (const char **)&oclKernelSourceCode, NULL, &oclResult);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clCreateProgramWithSource() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // build OpenCL program
    oclResult = clBuildProgram(oclProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
    if (oclResult != CL_SUCCESS) {
        cl_int oclBuildResult;
        size_t len;
        char buffer[2048];
        oclBuildResult = clGetProgramBuildInfo(oclProgram, oclDeviceID, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        if (oclBuildResult != CL_SUCCESS) {
            fprintf(gpFile, "clGetProgramBuildInfo() Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }
        fprintf(gpFile, "Program Build Log : %s\n", buffer);
        fprintf(gpFile, "clBuildProgram() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

    // create OpenCL kernel
    oclkernel = clCreateKernel(oclProgram, "sineWaveKernel", &oclResult);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clCreateKernel() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

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
        "uniform vec3 uColor; \n" \
        "out vec3 out_color; \n" \
        "uniform mat4 uMVPMatrix; \n" \
        "void main(void) \n" \
        "{ \n" \
        "   gl_Position = uMVPMatrix * aPosition; \n" \
        "   out_color = uColor; \n" \
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
        "in vec3 out_color; \n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
        "   FragColor = vec4(out_color, 1.0f); \n" \
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
    colorUniform = glGetUniformLocation(shaderProgramObject, "uColor");

    // provide vertex position, color, normal, textcord, etc
    memset(position, 0, sizeof(position));
    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo_cpu);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_cpu);
    glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vbo_gpu);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_gpu);
    glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // create opencl graphics resource
    oclGraphicsResource = clCreateFromGLBuffer(oclContext, CL_MEM_WRITE_ONLY, vbo_gpu, &oclResult);
    if (oclResult != CL_SUCCESS) {
        fprintf(gpFile, "clCreateFromGLBuffer() Failed: %d\n", oclResult);
        uninitialize();
        exit(EXIT_FAILURE);
    }

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
    void sineWave(unsigned int mesh_width, unsigned int mesh_height, float time);
    void uninitialize(void);
    
    // code
    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use shader program object
    glUseProgram(shaderProgramObject);
    
    // transformations
    mat4 modelViewMatrix = mat4::identity();
    mat4 modelViewProjectionMatrix = mat4::identity();

    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    // send this matrix to vertex shader in uniform
    glUniform3fv(colorUniform, 1, color);
    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);

    // bind with vao
    glBindVertexArray(vao);

    if (onGPU == True) {
        // set 0th kernel argument (pos)
        oclResult = clSetKernelArg(oclkernel, 0, sizeof(cl_mem), (void*)&oclGraphicsResource);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clSetKernelArg(0) Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }
        // set 1st kernel argument (width)
        oclResult = clSetKernelArg(oclkernel, 1, sizeof(unsigned int), (void*)&gMeshWidth);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clSetKernelArg(1) Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }
        // set 2nd kernel argument (height)
        oclResult = clSetKernelArg(oclkernel, 2, sizeof(unsigned int), (void*)&gMeshHeight);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clSetKernelArg(2) Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }
        // set 3rd kernel argument (time)
        oclResult = clSetKernelArg(oclkernel, 3, sizeof(float), (void*)&animationTime);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clSetKernelArg(3) Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }

        // acquire opencl graphics resource
        oclResult = clEnqueueAcquireGLObjects(oclCommandQueue, 1, &oclGraphicsResource, 0, NULL, NULL);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clEnqueueAcquireGLObjects() Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }

        // call opencl kernel
        size_t globalWorkSize[2];
        globalWorkSize[0] = gMeshWidth;
        globalWorkSize[1] = gMeshHeight;
        size_t localWorkSize[2];
        localWorkSize[0] = 8;
        localWorkSize[1] = 8;

        oclResult = clEnqueueNDRangeKernel(oclCommandQueue, oclkernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clEnqueueNDRangeKernel() Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }

        // release opencl graphics resource
        oclResult = clEnqueueReleaseGLObjects(oclCommandQueue, 1, &oclGraphicsResource, 0, NULL, NULL);
        if (oclResult != CL_SUCCESS) {
            fprintf(gpFile, "clEnqueueReleaseGLObjects() Failed: %d\n", oclResult);
            uninitialize();
            exit(EXIT_FAILURE);
        }

        // finish the command queue
        clFinish(oclCommandQueue);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo_gpu);
    } else {
        sineWave(gMeshWidth, gMeshHeight, animationTime);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_cpu);
        glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), position, GL_DYNAMIC_DRAW);
    }
    
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);

    glDrawArrays(GL_POINTS, 0, gMeshWidth * gMeshHeight);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
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
    animationTime += 0.01;
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
    if (vbo_cpu) {
        glDeleteBuffers(1, &vbo_cpu);
        vbo_cpu = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
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

void sineWave(unsigned int mesh_width, unsigned int mesh_height, float time)
{
    int index = 0;
    for (int i = 0; i < (int)gMeshWidth; ++i) {
        for (int j = 0; j < (int)gMeshHeight; ++j) {
                float u = ((float)i / (float)mesh_width);
                float v = ((float)j / (float)mesh_height);

                u = u * 2.0f - 1.0f;
                v = v * 2.0f - 1.0f;
                
                float frequency = 4.0f;
                float w = sin(u * frequency + time) * cos(v * frequency + time) * 0.5;

                position[index + 0] = u; 
                position[index + 1] = w; 
                position[index + 2] = v; 
                position[index + 3] = 1.0f;
                index += 4;
        }
    }
}
