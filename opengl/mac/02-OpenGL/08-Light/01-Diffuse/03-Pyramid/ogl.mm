#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>

// custom header files
#include "vmath.h"
using namespace vmath;

CVReturn myDisplayLinkCallback(CVDisplayLinkRef, const CVTimeStamp*, const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void*);

FILE* gpFile = NULL;

#define SPHERE_VERTICES 2883
#define SPHERE_TEXCOORDS 1922
#define SPHERE_ELEMENTS 5400

float sphere_vertices[SPHERE_VERTICES];
float sphere_normals[SPHERE_VERTICES];
float sphere_textures[SPHERE_TEXCOORDS];
unsigned short sphere_elements[SPHERE_ELEMENTS];

// shader related variables
GLuint shaderProgramObject = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
    AMC_ATTRIBUTE_NORMAL,
};

GLuint gVao_sphere = 0;
GLuint gVbo_sphere_position = 0;
GLuint gVbo_sphere_normal = 0;
GLuint gVbo_sphere_element = 0;

GLuint vao_pyramid = 0;
GLuint vbo_position_pyramid = 0;
float anglePyramid = 0.0f;

GLuint mvpMatrixUniform = 0;
GLuint modelViewMatrixUniform = 0;
GLuint projectionMatrixUniform = 0;
GLuint LdUniform = 0;
GLuint KdUniform = 0;
GLuint lightPositionUniform = 0;
GLuint LKeyPressUniform = 0;

mat4 perspectiveProjectionMatrix;

GLfloat lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat materialDiffuse[] = {0.4f, 0.4f, 0.4f, 1.0f};
GLfloat lightPosition[] = {0, 0, 2.0f, 1.0f};
BOOL bLight = FALSE;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@end

@interface GLView : NSOpenGLView
-(void)uninitialize;
@end

int main(int argc, char* argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSApp = [NSApplication sharedApplication];
    [NSApp setDelegate:[[AppDelegate alloc] init]];
    [NSApp run];
    [pool release];
    return 0;
}

@implementation AppDelegate
{
    @private
    NSWindow *window;
    GLView *glView;
}

-(void)applicationDidFinishLaunching:(NSNotification*)notification {
    NSBundle *appBundle = [NSBundle mainBundle];
    NSString *appDirPath = [appBundle bundlePath];
    NSString *parentDirPath = [appDirPath stringByDeletingLastPathComponent];
    NSString *logFileNameWithPath = [NSString stringWithFormat:@"%@/Log.txt", parentDirPath];
    const char* pszLogFileNameWithPath = [logFileNameWithPath cStringUsingEncoding:NSASCIIStringEncoding];
    
    gpFile = fopen(pszLogFileNameWithPath, "w");
    if(gpFile == NULL) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert setMessageText:@"Log file creation failed"];
        [alert addButtonWithTitle:@"Exit"];
        [alert runModal];
        [alert release];
        [NSApp terminate:self];
    } else {
        fprintf(gpFile, "Program started successfully\n");
    }

    NSRect winRect = NSMakeRect(0.0, 0.0, 800.0, 600.0);
    window = [[NSWindow alloc] initWithContentRect:winRect
                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    [window setTitle:@"LRC: macOS Window"];
    [window center];

    glView = [[GLView alloc] initWithFrame:winRect];

    [window setContentView:glView];
    [window setDelegate:self];
    [window makeKeyAndOrderFront:self];
}

-(void)applicationWillTerminate:(NSNotification*)notification {
    if(gpFile) {
        fprintf(gpFile, "program terminated successfully");
        fclose(gpFile);
        gpFile = NULL;
    }
}

-(BOOL)windowShouldClose:(NSWindow*)aWindow { 
    [glView uninitialize];
    return YES;
}

-(void)windowWillClose:(NSNotification*)notification {
    [NSApp terminate:self];
}

-(void)dealloc {
    [glView release];
    [window release];
    [super dealloc];
}
@end

@implementation GLView {
    @private
    CVDisplayLinkRef displayLink;
}

-(id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        NSOpenGLPixelFormatAttribute attributes[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
            NSOpenGLPFAScreenMask, CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),
            NSOpenGLPFANoRecovery,
            NSOpenGLPFAAccelerated,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADoubleBuffer,
            0
        };

        NSOpenGLPixelFormat *pixelFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
        if (pixelFormat == nil) {
            fprintf(gpFile, "NSOpenGLPixelFormat failed \n");
            [NSApp terminate:self];
        }

        NSOpenGLContext *glContext = [[[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil] autorelease];
        if (glContext == nil) {
            fprintf(gpFile, "NSOpenGLContext failed \n");
            [NSApp terminate:self];
        }

        [self setPixelFormat:pixelFormat];
        [self setOpenGLContext:glContext];
        
        [self setWantsLayer:YES];
        [[self layer] setBackgroundColor:[[NSColor blackColor] CGColor]];
    }
    return self;
}

-(int)initialize {
    const GLchar* vertexShaderSourceCode = 
        "#version 410 core \n" \
        "in vec4 aPosition; \n" \
        "in vec3 aNormal; \n" \
        "out vec3 outDiffuseLight; \n" \
        "uniform mat4 uModelViewMatrix; \n" \
        "uniform mat4 uProjectionMatrix; \n" \
        "uniform vec3 uLd; \n" \
        "uniform vec3 uKd; \n" \
        "uniform vec4 uLightPosition; \n" \
        "uniform int uLKeyIsPressed; \n" \
        "void main(void) \n" \
        "{ \n" \
            "gl_Position = uProjectionMatrix * uModelViewMatrix * aPosition; \n" \
            "if (uLKeyIsPressed == 1) { \n" \
                "vec4 eyeCoordinates = uModelViewMatrix * aPosition; \n" \
                "mat3 normalMatrix = mat3(transpose(inverse(uModelViewMatrix))); \n" \
                "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n" \
                "vec3 lightSource = vec3(uLightPosition - eyeCoordinates); \n" \
                "outDiffuseLight = uLd * uKd * max(dot(lightSource, transformedNormal), 0.0f); \n" \
            "} \n" \
            "else { \n" \
                "outDiffuseLight = vec3(1.0f, 1.0f, 1.0f); \n" \
            "} \n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode = 
        "#version 410 core\n" \
        "in vec3 outDiffuseLight; \n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
            "FragColor = vec4(outDiffuseLight, 1.0f); \n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // create, attach, link
    shaderProgramObject = glCreateProgram();
    glAttachShader(shaderProgramObject, vertexShaderObject);
    glAttachShader(shaderProgramObject, framgmentShaderObject);

    // bind shader attribute at a certain index in shader
    // to same index in host program
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_POSITION, "aPosition");
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_NORMAL, "aNormal");
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // get the required uniform location from the shader
    modelViewMatrixUniform = glGetUniformLocation(shaderProgramObject, "uModelViewMatrix");
    projectionMatrixUniform = glGetUniformLocation(shaderProgramObject, "uProjectionMatrix");
    LdUniform = glGetUniformLocation(shaderProgramObject, "uLd");
    KdUniform = glGetUniformLocation(shaderProgramObject, "uKd");
    lightPositionUniform = glGetUniformLocation(shaderProgramObject, "uLightPosition");
    LKeyPressUniform = glGetUniformLocation(shaderProgramObject, "uLKeyIsPressed");

    // provide vertex position, color, normal, textcord, etc
    const GLfloat pyramid_position[] = {   
        // front
        0.0f,  1.0f,  0.0f, // front-top
        -1.0f, -1.0f,  1.0f, // front-left
        1.0f, -1.0f,  1.0f, // front-right
        
        // right
        0.0f,  1.0f,  0.0f, // right-top
        1.0f, -1.0f,  1.0f, // right-left
        1.0f, -1.0f, -1.0f, // right-right

        // back
        0.0f,  1.0f,  0.0f, // back-top
        1.0f, -1.0f, -1.0f, // back-left
        -1.0f, -1.0f, -1.0f, // back-right

        // left
        0.0f,  1.0f,  0.0f, // left-top
        -1.0f, -1.0f, -1.0f, // left-left
        -1.0f, -1.0f,  1.0f, // left-right
    
    };

    const GLfloat pyramid_normal[] = {   
        // front
        0.000000f, 0.447214f,  0.894427f, // front-top
        0.000000f, 0.447214f,  0.894427f, // front-left
        0.000000f, 0.447214f,  0.894427f, // front-right
                                
        // right			    
        0.894427f, 0.447214f,  0.000000f, // right-top
        0.894427f, 0.447214f,  0.000000f, // right-left
        0.894427f, 0.447214f,  0.000000f, // right-right

        // back
        0.000000f, 0.447214f, -0.894427f, // back-top
        0.000000f, 0.447214f, -0.894427f, // back-left
        0.000000f, 0.447214f, -0.894427f, // back-right

        // left
        -0.894427f, 0.447214f,  0.000000f, // left-top
        -0.894427f, 0.447214f,  0.000000f, // left-left
        -0.894427f, 0.447214f,  0.000000f, // left-right
    };

    // Rectangle
    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao_pyramid);
    glBindVertexArray(vao_pyramid);

    // position
    glGenBuffers(1, &vbo_position_pyramid);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_pyramid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid_position), pyramid_position, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // color
    glGenBuffers(1, &vbo_position_pyramid);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position_pyramid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid_normal), pyramid_normal, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // tell opengl to choose the color to clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    perspectiveProjectionMatrix = mat4::identity();

    return 0;
}

-(void)printGLInfo {
    fprintf(gpFile, "OPENGL INFORMATION\n");
    fprintf(gpFile, "------------------\n");
    fprintf(gpFile, "OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    fprintf(gpFile, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    fprintf(gpFile, "OpenGL Version : %s\n", glGetString(GL_VERSION));
    fprintf(gpFile, "GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    fprintf(gpFile, "------------------\n");
}

-(void)display {
    // code
    // clear opengl buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use shader program object
    glUseProgram(shaderProgramObject);

    mat4 modelViewMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    mat4 rotationMatrix = mat4::identity();
    mat4 scaleMatrix = mat4::identity();
    rotationMatrix = mat4::identity();
    modelViewMatrix = mat4::identity();
    
    scaleMatrix = vmath::scale(0.75f, 0.75f, 0.75f);
    translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
    rotationMatrix = vmath::rotate(anglePyramid, 0.0f, 1.0f, 0.0f);
    
    modelViewMatrix = translationMatrix * scaleMatrix * rotationMatrix;
    
    // send this matrix to vertex shader in uniform
    glUniformMatrix4fv(modelViewMatrixUniform, 1, GL_FALSE, modelViewMatrix);
    glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, perspectiveProjectionMatrix);

    if (bLight == TRUE) {
        glUniform3fv(LdUniform, 1, lightDiffuse);
        glUniform3fv(KdUniform, 1, materialDiffuse);
        glUniform4fv(lightPositionUniform, 1, lightPosition);
        glUniform1i(LKeyPressUniform, 1);
    } else {
        glUniform1i(LKeyPressUniform, 0);
    }

    // bind with vao
    glBindVertexArray(vao_pyramid);

    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLES, 0, 12);

    // unbind with vao
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);
}

-(void)myUpdate {
    anglePyramid += 0.1f;
}

-(void)resize:(int)width :(int)height {
    if (height <= 0) height = 1;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    perspectiveProjectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

-(void)uninitialize {}

-(CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [self drawView];
    [pool release];
    return kCVReturnSuccess;
}

-(void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];

    GLint swapInterval = 1;
    [[self openGLContext] setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval];

    [self printGLInfo];

    if ([self initialize] != 0) {
        [NSApp terminate:self];
    }

    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink, &myDisplayLinkCallback, self);
    CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
    CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
    CVDisplayLinkStart(displayLink);
}

-(void)reshape {
    [super reshape];
    [[self openGLContext]makeCurrentContext];
    CGLLockContext([[self openGLContext]CGLContextObj]);
    NSRect viewRect = [self bounds];
    int width = viewRect.size.width;
    int height = viewRect.size.height;
    [self resize:width :height];
    CGLUnlockContext([[self openGLContext]CGLContextObj]);
}

-(void)drawView {
    [[self openGLContext] makeCurrentContext];
    CGLLockContext([[self openGLContext] CGLContextObj]);
    [self display];
    [self myUpdate];
    CGLFlushDrawable([[self openGLContext] CGLContextObj]);
    CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

-(void)drawRect:(NSRect)dirtyRect {
    [self drawView];
}

-(BOOL)acceptsFirstResponder {
    [[self window]makeFirstResponder:self];
    return YES;
}

-(void)keyDown:(NSEvent*)event {
    unichar key = [[event charactersIgnoringModifiers] characterAtIndex:0];
    switch(key) {
        case 27:
            [self uninitialize];
            [NSApp terminate:self];
            break;
        case 'f':
        case 'F':
            [[self window] toggleFullScreen:self];
            break;

        case 'L':
                case 'l':
                    if (bLight == FALSE) {
                        bLight = TRUE;
                    } else {
                        bLight = FALSE;
                    }
                    break;
    }
}

-(void)dealloc {
    CVDisplayLinkStop(displayLink);
    CVDisplayLinkRelease(displayLink);
    [super dealloc];
}
@end

CVReturn myDisplayLinkCallback(CVDisplayLinkRef displayLinkRef, const CVTimeStamp* current, const CVTimeStamp* output,
                                CVOptionFlags inputFlags, CVOptionFlags* outputFlags, void* view)
{
    return [(GLView*)view getFrameForTime:output];
}