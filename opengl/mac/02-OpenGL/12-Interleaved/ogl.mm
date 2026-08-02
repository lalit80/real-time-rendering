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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode = 
        "#version 410 core\n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
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

    // load textures
    texture_marble = [self loadGLTexture:"marble.bmp"];

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
}

-(void)myUpdate {
    angleCube += 0.05f;
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

    fprintf(gpFile, "Loading texture: %s\n", [filePath UTF8String]);

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