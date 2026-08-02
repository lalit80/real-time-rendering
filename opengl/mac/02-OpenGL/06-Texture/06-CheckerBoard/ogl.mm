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

// rotation angles
float anglePyramid;
float angleCube;

// shader related variables
GLuint shaderProgramObject = 0;

GLuint textureSamplerUniform;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
    AMC_ATTRIBUTE_TEXTCORD,
};

GLuint vao = 0;
GLuint vbo_position = 0;
GLuint vbo_texcoord_cube = 0;

// texture related global variables
// checker board related variables
#define CHECKERBOARD_WIDTH  64
#define CHECKERBOARD_HEIGHT 64

GLubyte cb[CHECKERBOARD_HEIGHT][CHECKERBOARD_WIDTH][4];
GLuint texture_cb;
GLuint mvpMatrixUniform = 0;

mat4 perspectiveProjectionMatrix;

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
        "in vec2 aTexCoord; \n" \
        "out vec2 out_texcoord; \n" \
        "uniform mat4 uMVPMatrix; \n" \
        "void main(void) \n" \
        "{ \n" \
        "   gl_Position = uMVPMatrix * aPosition; \n" \
        "   out_texcoord = aTexCoord; \n" \
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
        "in vec2 out_texcoord; \n" \
        "uniform sampler2D uTextureSampler; \n" \
        "out vec4 FragColor; \n" \
        "void main(void)\n" \
        "{\n" \
        "   FragColor = texture(uTextureSampler, out_texcoord); \n" \
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
    glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_TEXTCORD, "aTexCoord");
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
    textureSamplerUniform = glGetUniformLocation(shaderProgramObject, "uTextureSampler");                       

    // provide vertex position, color, normal, textcord, etc
    const GLfloat rectangle_texcoords[] = {   
        // front
        1.0f, 1.0f, // top-right of front
        0.0f, 1.0f, // top-left of front
        0.0f, 0.0f, // bottom-left of front
        1.0f, 0.0f, // bottom-right of front

    };

    // vertex array object for arrays of vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo_position);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 3, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // texture
    glGenBuffers(1, &vbo_texcoord_cube);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoord_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_texcoords), rectangle_texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_TEXTCORD, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_TEXTCORD);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // depth related code
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    loadGLTexture();
    glEnable(GL_TEXTURE_2D);

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

void loadGLTexture(void) {
    // function declaration
    void makeCheckerboard(void);

    // code
    makeCheckerboard();

    // generate opengl texture object
    glGenTextures(1, &texture_cb);
    // bind to newly created object
    glBindTexture(GL_TEXTURE_2D, texture_cb);
    // unpack the image in memory for faster loading
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //gluBuild2DMipmaps(GL_TEXTURE_2D, 3, bmp.bmWidth, bmp.bmHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, bmp.bmBits);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERBOARD_WIDTH, CHECKERBOARD_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, cb);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void makeCheckerboard(void)
{
    int i, j, c;

    for (i = 0; i < CHECKERBOARD_HEIGHT; ++i) {
        for (j = 0; j < CHECKERBOARD_WIDTH; ++j) {
            c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0)) * 255;
            cb[i][j][0] = (GLubyte)c;
            cb[i][j][1] = (GLubyte)c;
            cb[i][j][2] = (GLubyte)c;
            cb[i][j][3] = (GLubyte)255;
        }
    }
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

    // for texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_cb);
    glUniform1i(textureSamplerUniform, 0);

    // bind with vao
    glBindVertexArray(vao);

    GLfloat rectangle_position[12];
    rectangle_position[0] = 0.0f;
    rectangle_position[1] = 1.0f;
    rectangle_position[2] = 0.0f;
    rectangle_position[3] = -2.0f;
    rectangle_position[4] = 1.0f;
    rectangle_position[5] = 0.0f;
    rectangle_position[6] = -2.0f;
    rectangle_position[7] = -1.0f;
    rectangle_position[8] = 0.0f;
    rectangle_position[9] = 0.0f;
    rectangle_position[10] = -1.0f;
    rectangle_position[11] = 0.0f;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_position), rectangle_position, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // second rectangle
    rectangle_position[0] = 2.41421f;
    rectangle_position[1] = 1.0f;
    rectangle_position[2] = -1.41421f;
    rectangle_position[3] = 1.0f;
    rectangle_position[4] = 1.0f;
    rectangle_position[5] = 0.0f;
    rectangle_position[6] = 1.0f;
    rectangle_position[7] = -1.0f;
    rectangle_position[8] = 0.0f;
    rectangle_position[9] = 2.41421f;
    rectangle_position[10] = -1.0f;
    rectangle_position[11] = -1.41421f;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_position), rectangle_position, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // draw the vertex arrays
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    // unbind with vao
    glBindVertexArray(0);

    // unuse shader program object
    glUseProgram(0);

    // unuse shader program object
    glUseProgram(0);
}

-(void)myUpdate {
    anglePyramid += 0.05f;
    angleCube += 0.00f;
}

-(void)resize:(int)width :(int)height {
    if (height <= 0) height = 1;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    perspectiveProjectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

-(void)uninitialize {}

// texture loading
-(GLuint)loadGLTexture:(const char*)textureFileName {
    NSString *fileName =
        [NSString stringWithUTF8String:textureFileName];

    NSString *filePath =
        [[NSBundle mainBundle] pathForResource:fileName ofType:nil];

    if (!filePath) {
        fprintf(gpFile, "Texture not found in bundle: %s\n", textureFileName);
        return 0;
    }

    NSImage *nsImage = [[NSImage alloc] initWithContentsOfFile:filePath];
    if (!nsImage) {
        fprintf(gpFile, "NSImage load failed: %s\n", textureFileName);
        return 0;
    }

    CGImageRef cgImage = [nsImage CGImageForProposedRect:nil context:nil hints:nil];
    int imageWidth = (int)CGImageGetWidth(cgImage);
    int imageHeight = (int)CGImageGetHeight(cgImage);

    CGDataProviderRef cgDataProvider = CGImageGetDataProvider(cgImage);
    CFDataRef cfData = CGDataProviderCopyData(cgDataProvider);
    void* imageData = (void*)(CFDataGetBytePtr(cfData));

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // for width not multiple of 4       
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    CFRelease(cfData);
    return texture;
}

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