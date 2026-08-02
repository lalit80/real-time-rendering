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

// shader related variables
GLuint shaderProgramObject = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
};

GLuint vao = 0;
GLuint vbo_position = 0;

GLuint mvpMatrixUniform = 0;

mat4 perspectiveProjectionMatrix;

// tesselation related variable
GLuint noOfLineSegmentsUniform;
GLuint noOfLineStripsUniform;
GLuint lineColorUniform;

u_int uiNoOfLineSegments = 1;
char colorKey = 'r';

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
        "in vec2 aPosition; \n" \
        "void main(void) \n" \
        "{ \n" \
        "   gl_Position = vec4(aPosition, 0.0f, 1.0f); \n" \
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

    // tesselation control shader
    const GLchar* tesselationControlShaderSourceCode = 
        "#version 410 core\n" \
        "layout(vertices=4) out; \n" \
        "uniform int uNoOfLineSegments; \n" \
        "uniform int uNoOfLineStrips; \n" \
        "void main(void)\n" \
        "{\n" \
        "   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n" \
        "   gl_TessLevelOuter[0] = float(uNoOfLineStrips); \n" \
        "   gl_TessLevelOuter[1] = float(uNoOfLineSegments); \n" \
        "}\n";

    GLuint tesselationControlShaderObject = glCreateShader(GL_TESS_CONTROL_SHADER);
    glShaderSource(tesselationControlShaderObject, 1, (const GLchar**)&tesselationControlShaderSourceCode   , NULL);
    glCompileShader(tesselationControlShaderObject);

    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(tesselationControlShaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(tesselationControlShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(tesselationControlShaderObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "tesselation control shader compilation log = %s\n", szInfoLog);
                free(szInfoLog);
                szInfoLog = NULL;
            }
        }
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // tesselation evaluation shader
    const GLchar* tesselationEvaluationShaderSourceCode = 
        "#version 410 core\n" \
        "layout(isolines) in; \n" \
        "uniform mat4 uMVPMatrix; \n" \
        "uniform int uNoOfLineSegments; \n" \
        "uniform int uNoOfLineStrips; \n" \
        "void main(void)\n" \
        "{\n" \
        "   vec3 p0 = gl_in[0].gl_Position.xyz; \n" \
        "   vec3 p1 = gl_in[1].gl_Position.xyz; \n" \
        "   vec3 p2 = gl_in[2].gl_Position.xyz; \n" \
        "   vec3 p3 = gl_in[3].gl_Position.xyz; \n" \
        "   vec3 p = p0 * ((1 - gl_TessCoord.x) * (1 - gl_TessCoord.x) * (1 - gl_TessCoord.x)) + (p1 * 3 * gl_TessCoord.x * (1 - gl_TessCoord.x) * (1 - gl_TessCoord.x)) + (p2 * 3 * gl_TessCoord.x * gl_TessCoord.x * (1 - gl_TessCoord.x)) + (p3 * gl_TessCoord.x * gl_TessCoord.x * gl_TessCoord.x); \n" \
        "   gl_Position = uMVPMatrix * vec4(p, 1.0f); \n" \
        "}\n";

    GLuint tesselationEvaluationShaderObject = glCreateShader(GL_TESS_EVALUATION_SHADER);
    glShaderSource(tesselationEvaluationShaderObject, 1, (const GLchar**)&tesselationEvaluationShaderSourceCode   , NULL);
    glCompileShader(tesselationEvaluationShaderObject);
    
    status = 0;
    infoLogLength = 0;
    szInfoLog = NULL;
    glGetShaderiv(tesselationEvaluationShaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderiv(tesselationEvaluationShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            szInfoLog = (GLchar*)malloc(infoLogLength * sizeof(GLchar));
            if (szInfoLog != NULL) {
                glGetShaderInfoLog(tesselationEvaluationShaderObject, infoLogLength, NULL, szInfoLog);
                fprintf(gpFile, "tesselation evaluation shader compilation log = %s\n", szInfoLog);
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
        "uniform vec4 uLineColor; \n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
        "   FragColor = uLineColor; \n" \
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
    glAttachShader(shaderProgramObject, tesselationControlShaderObject);
    glAttachShader(shaderProgramObject, tesselationEvaluationShaderObject);
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // get the required uniform location from the shader
    mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");
    noOfLineSegmentsUniform = glGetUniformLocation(shaderProgramObject, "uNoOfLineSegments");
    noOfLineStripsUniform = glGetUniformLocation(shaderProgramObject, "uNoOfLineStrips");
    lineColorUniform = glGetUniformLocation(shaderProgramObject, "uLineColor");

    const GLfloat line_position[] = { -1.0f, -1.0f,
                                      -0.5f, 1.0f,
                                       0.5f, -1.0f,
                                       1.0f, 1.0f, 
    };

    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo_position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_position), line_position, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
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

    // transformations
    mat4 modelViewMatrix = mat4::identity();
    mat4 modelViewProjectionMatrix = mat4::identity();
    mat4 translationMatrix = mat4::identity();
    translationMatrix = vmath::translate(0.0f, 0.0f, -3.0f);
    modelViewMatrix = translationMatrix;

    modelViewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

    // send this matrix to vertex shader in uniform
    glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelViewProjectionMatrix);
    
    glUniform1i(noOfLineSegmentsUniform, uiNoOfLineSegments);
    glUniform1i(noOfLineStripsUniform, 1);
    glUniform4fv(lineColorUniform, 1, vmath::vec4(1.0f, 1.0f, 0.0f, 1.0f));

    NSString *msg = [NSString stringWithFormat:@"LRC macOS Window: %d", uiNoOfLineSegments];
    //[window setTitle:msg];
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self window]) {
            [[self window] setTitle:msg];
        }
    });

    // bind with vao
    glBindVertexArray(vao);

    // decide how many vertices for one batch
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    // draw the vertex arrays
    glDrawArrays(GL_PATCHES, 0, 4);

    // unbind with vao
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);

    // unuse shader program object
    glUseProgram(0);
}

-(void)myUpdate {}

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

        
        case NSUpArrowFunctionKey:
            uiNoOfLineSegments++;
                    if (uiNoOfLineSegments > 30)
                        uiNoOfLineSegments = 30;
                    break;
        case NSDownArrowFunctionKey:
            uiNoOfLineSegments--;
                    if (uiNoOfLineSegments <= 0)
                        uiNoOfLineSegments = 1;
                    break;

        case 'r':
                    colorKey = 'r';
                    break;
                case 'g':
                    colorKey = 'g';
                    break;
                case 'b':
                    colorKey = 'b';
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