#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

// open header files
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo* visualInfo = NULL;                 // hardware information (graphic card)
Window window;
Colormap colormap;

Bool bFullScreen = False;
Bool bActiveWindow = False;

// opengl related variables
GLXContext glxContext = NULL;

// file io
char gszLogFileName[] = "log.txt";
FILE* gpFile = NULL;

GLUquadric* quadric = NULL;

// light variables
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

Bool bLight = False;

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

    int framebufferAttributes[] = { GLX_DOUBLEBUFFER, GLX_RGBA,
                                    GLX_RED_SIZE, 8,
                                    GLX_GREEN_SIZE, 8,
                                    GLX_BLUE_SIZE, 8,
                                    GLX_ALPHA_SIZE, 8, None};
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

    visualInfo = glXChooseVisual(gpDisplay, defaultScreen, framebufferAttributes);
    if (visualInfo == NULL) {
        fprintf(gpFile, "glXChooseVisual failed\n");
        uninitialize();
        exit(EXIT_FAILURE);
    }

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

                            case 'L':
                case 'l':
                    if (bLight == False) {
                        bLight = True;
                        glEnable(GL_LIGHTING);
                    }
                    else {
                        bLight = False;
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
    // code
    glxContext = glXCreateContext(gpDisplay, visualInfo, NULL, True);
    if (glxContext == NULL) {
        fprintf(gpFile, "glxCreateContext failed\n");
        return (-1);
    }
    glXMakeCurrent(gpDisplay, window, glxContext);

    printGLInfo();
    glClearColor(0, 0, 0, 1);

    quadric = gluNewQuadric();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glEnable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightModelAmbient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lightModelLocalViewer);


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
}

void display(void)
{
    void draw24Spheres(void);
    // code
    float radius = 100.0f;

    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT);

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
    glXSwapBuffers(gpDisplay, window);
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
    // code
    // print opengl information
    fprintf(gpFile, "OPENGL INFORMATION\n");
    fprintf(gpFile, "------------------\n");
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "------------------\n");
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
