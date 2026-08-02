#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// Xlib header files
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

// open header files
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include <SOIL/SOIL.h>

// macros
#define WIN_WIDTH   800
#define WIN_HEIGHT  600

#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define MAX_LIMIT_CUBE_TRANSLATE_X  8.0f
#define MIN_LIMIT_CUBE_TRANSLATE_X  -8.0f
#define MAX_LIMIT_CUBE_TRANSLATE_Y  4.0f
#define MIN_LIMIT_CUBE_TRANSLATE_Y  1.5f
#define CLOSET_LIMIT_CUBE_TRANSLATE_Z  5.0f
#define FARTHEST_LIMIT_CUBE_TRANSLATE_Z  -20.0f

#define INCREMENT 0.05f
#define DECREMENT 0.05f

// global variables
Display* gpDisplay = NULL;              // interface between XServer - XClient
XVisualInfo* visualInfo = NULL;                 // hardware information (graphic card)
Window window;
Colormap colormap;

Bool bFullScreen = False;
Bool bActiveWindow = False;

Bool loadGLTexture(GLuint* texture, const char* imagePath);

// opengl related variables
GLXContext glxContext = NULL;

// file io
char gszLogFileName[] = "log.txt";
FILE* gpFile = NULL;

// rotation angles
float angleCube = 0.0f;

// light variables
float lightAmbient[] = {0, 0, 0, 1};
float lightDiffuse[] = {1, 1, 1, 1};
float lightSpecular[] = {1, 1, 1, 1};
float lightPosition[] = {0, 5, 10, 1};

float materialAmbient[] = {0, 0, 0, 1};
float materialDiffuse[] = {1, 0, 0, 1};
float materialSpecular[] = {1, 1, 1, 1};
float materialShininess = 128;

// special effect related global variables
GLuint texture_marble;
float translateCubeX = 0;
float translateCubeY = 2;
float translateCubeZ = 0;
float cubeScale = 0.25f;

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

                        case XK_Right:
                            if (translateCubeX < MAX_LIMIT_CUBE_TRANSLATE_X) {
                                translateCubeX += INCREMENT;
                            }
                            break;

                        case XK_Left:
                            if (translateCubeX > MIN_LIMIT_CUBE_TRANSLATE_X) {
                                translateCubeX -= DECREMENT;
                            }
                            break;

                        case XK_Up:
                            if (translateCubeY < MAX_LIMIT_CUBE_TRANSLATE_Y) {
                                translateCubeY += INCREMENT;
                            }
                            break;

                        case XK_Down:
                            if (translateCubeY > MIN_LIMIT_CUBE_TRANSLATE_Y) {
                                translateCubeY -= DECREMENT;
                            }
                            break;

                        case XK_KP_Add:
                            if (translateCubeZ > FARTHEST_LIMIT_CUBE_TRANSLATE_Z) {
                                translateCubeZ -= DECREMENT;
                            }
                            break;

                        case XK_KP_Subtract:
                            if (translateCubeZ < CLOSET_LIMIT_CUBE_TRANSLATE_Z) {
                                translateCubeZ += INCREMENT;
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
    // code
    glxContext = glXCreateContext(gpDisplay, visualInfo, NULL, True);
    if (glxContext == NULL) {
        fprintf(gpFile, "glxCreateContext failed\n");
        return (-1);
    }
    glXMakeCurrent(gpDisplay, window, glxContext);

    printGLInfo();
    glClearColor(0, 0, 0, 1);

    if (loadGLTexture(&texture_marble, "marble.bmp") == False) {
        fprintf(gpFile, "loadGLTexture failed\n");
        return (-6);
    }

    glEnable(GL_TEXTURE_2D);

    // light configuration
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);

    glEnable(GL_NORMALIZE);
    glEnable(GL_AUTO_NORMAL);

    glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);
    
    return 0;
}

void resize(int width, int height)
{
    // code
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // set matrix projection mode
    glMatrixMode(GL_PROJECTION);

    // set matrix projection mode
    glMatrixMode(GL_PROJECTION);

    // set to identity matrix
    glLoadIdentity();

    // do perspective projection
    gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

void display(void)
{
    // code

    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT);

    // set matrix to model view mode
    glMatrixMode(GL_MODELVIEW);

    // set to identity matrix
    glLoadIdentity();

    // set up camera
    gluLookAt(0, 7, 9, 0, 0, 0, 0, 1, 0);

    // render the actual cube
    glPushMatrix();
    // translate cube backwards by z
    glTranslatef(translateCubeX, translateCubeY, translateCubeZ);
    glScalef(cubeScale, cubeScale, cubeScale);
    glRotatef(angleCube, 1.0f, 0.0f, 0.0f);
    glRotatef(angleCube, 0.0f, 1.0f, 0.0f);
    glRotatef(angleCube, 0.0f, 0.0f, 1.0f);
    drawLitCube();
    glPopMatrix();

    // render the reflected cube
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 1);                 // always pass the stencil test
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    drawFloor();                                    // this will not render the floor (for stencil)

    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 1);                  // draw only where there is 1 stored in stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    glPushMatrix();
    glScalef(1, -1, 1);
    // translate cube backwards by z
    glTranslatef(translateCubeX, translateCubeY, translateCubeZ);
    glScalef(cubeScale, cubeScale, cubeScale);
    glRotatef(angleCube, 1.0f, 1.0f, 1.0f);
    drawLitCube();
    glPopMatrix();
    
    glDisable(GL_STENCIL_TEST);

    // render the floor
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, 1, 1, 0.7f);
    drawFloor();
    glDisable(GL_BLEND);

    // swap the buffers
    glXSwapBuffers(gpDisplay, window);
}

void update(void)
{
    // code
    angleCube += 1;
    if (angleCube >= 360.0f) {
        angleCube = angleCube - 360.0f;
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

void drawLitCube(void)
{
    // code
    glEnable(GL_LIGHTING);

    glBegin(GL_QUADS);
        // front face
        glNormal3f(0, 0, 1);
        glVertex3f(1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);

        // right face
        glNormal3f(1, 0, 0);
        glVertex3f(1.0f, 1.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);

        // back face
        glNormal3f(0, 0, -1);
        glVertex3f(1.0f, 1.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);

        // left face
        glNormal3f(-1, 0, 0);
        glVertex3f(-1.0f, 1.0f, 1.0f); 
        glVertex3f(-1.0f, 1.0f, -1.0f); 
        glVertex3f(-1.0f, -1.0f, -1.0f); 
        glVertex3f(-1.0f, -1.0f, 1.0f);

        // top face
        glNormal3f(0, 1, 0);
        glVertex3f(1.0f, 1.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);

        // bottom face
        glNormal3f(0, -1, 0);
        glVertex3f(1.0f, -1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();

    glDisable(GL_LIGHTING);
}

void drawFloor(void)
{
    // code
    // pushing camera matrix set in display
    glPushMatrix();
    glTranslatef(0.0f, 1.0f, -2.0f);
    glRotatef(90, 1, 0, 0);
    glScalef(5, 5, 1);

    glBindTexture(GL_TEXTURE_2D, texture_marble);

    glBegin(GL_QUADS);
        glTexCoord2f(1, 1);
        glVertex3f(1.0f, 1.0f, 0.0f);
        glTexCoord2f(0, 1);
		glVertex3f(-1.0f, 1.0f, 0.0f);
        glTexCoord2f(0, 0);
		glVertex3f(-1.0f, -1.0f, 0.0f);
        glTexCoord2f(1, 0);
		glVertex3f(1.0f, -1.0f, 0.0f);
	glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
}

Bool loadGLTexture(GLuint* texture, const char* imagePath) {
    // variable declarations
    int width, height;
    unsigned char* imageData = NULL;

    imageData = SOIL_load_image(imagePath, &width, &height, NULL, SOIL_LOAD_RGB);

    if (imageData == NULL) {
        return False;
    }

    // code
    glGenTextures(1, texture);
    // bind to newly created object
    glBindTexture(GL_TEXTURE_2D, *texture);
    // unpack the image in memory for faster loading
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, imageData);
    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(imageData);
    
    return True;
}
