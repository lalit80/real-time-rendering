#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>
#import <OpenGL/gl3.h>
#import <OpenGL/gl3ext.h>

// custom header files
#include "vmath.h"
#include "Sphere.h"
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
GLuint shaderProgramObject_PF = 0;
GLuint shaderProgramObject_PV = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
    AMC_ATTRIBUTE_NORMAL,
};

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
GLfloat materialShininiess = 128.0f;
BOOL bLight = FALSE;
BOOL bPerFragment = TRUE;
BOOL bPerVertex = FALSE;

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
    const GLchar* vertexShaderSourceCode_PF = 
        "#version 410 core \n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode_PF = 
        "#version 410 core\n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
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
    materialShininessUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uMaterialShininess");
    LKeyPressUniform_PF = glGetUniformLocation(shaderProgramObject_PF, "uLKeyIsPressed");

    // per vertex light
    const GLchar* vertexShaderSourceCode_PV = 
        "#version 410 core \n" \
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
        "uniform float uMaterialShininess; \n" \
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
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, viewerVector), 0.0f), uMaterialShininess); \n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
    }

    // fragment shader
    const GLchar* framgmentShaderSourceCode_PV = 
        "#version 410 core\n" \
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
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
        [self release];
[self uninitialize];
[NSApp terminate:self];
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
    materialShininessUniform_PV = glGetUniformLocation(shaderProgramObject_PV, "uMaterialShininess");
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
    glBufferData(GL_ARRAY_BUFFER, gNumVertices * 3 * sizeof(float), sphere_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normal vbo
    glGenBuffers(1, &gVbo_sphere_normal);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo_sphere_normal);
    glBufferData(GL_ARRAY_BUFFER, gNumVertices * 3 * sizeof(float), sphere_normals, GL_STATIC_DRAW);
    glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // element vbo
    glGenBuffers(1, &gVbo_sphere_element);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, gNumElements * sizeof(unsigned short), sphere_elements, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // unbind vao
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
    if (bPerFragment) {
        glUseProgram(shaderProgramObject_PF);

        // transformations
        mat4 modelMatrix = mat4::identity();
        mat4 viewMatrix = mat4::identity();
        mat4 translationMatrix = mat4::identity();
        translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
        modelMatrix = translationMatrix;

        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(modelMatrixUniform_PF, 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(viewMatrixUniform_PF, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PF, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == TRUE) {
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
        translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
        modelMatrix = translationMatrix;

        // send this matrix to vertex shader in uniform
        glUniformMatrix4fv(modelMatrixUniform_PV, 1, GL_FALSE, modelMatrix);
        glUniformMatrix4fv(viewMatrixUniform_PV, 1, GL_FALSE, viewMatrix);
        glUniformMatrix4fv(projectionMatrixUniform_PV, 1, GL_FALSE, perspectiveProjectionMatrix);

        if (bLight == TRUE) {
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
        case 'F':
            [[self window] toggleFullScreen:self];
            break;

                case 'f':
                    if (bPerFragment == FALSE) {
                        bPerFragment = TRUE;
                        bPerVertex = FALSE;
                    }
                    break;

                case 'v':
                case 'V':
                    if (bPerVertex == FALSE) {
                        bPerVertex = TRUE;
                        bPerFragment = FALSE;
                    }
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