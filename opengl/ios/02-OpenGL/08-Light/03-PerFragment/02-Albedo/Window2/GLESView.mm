#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <QuartzCore/CADisplayLink.h>
#import "GLESView.h"

// custom header files
#include "vmath.h"
#include "Sphere.h"
using namespace vmath;

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

GLfloat materialAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat materialDiffuse[] = {0.5f, 0.2f, 0.7f, 1.0f};
GLfloat materialSpecular[] = {0.7f, 0.7f, 0.7f, 1.0f};
GLfloat materialShininiess = 50.0f;
BOOL bLight = FALSE;

@implementation GLESView {
    EAGLContext* eaglContext;
    GLuint framebuffer;
    GLuint colorRenderbuffer;
    GLuint depthRenderbuffer;
    CADisplayLink* displayLink;
    CAFrameRateRange frameRateChange;
    BOOL isDisplayLink;
}

-(int) initialize {
    [self printGLESInfo];
    
    const GLchar* vertexShaderSourceCode =
            "#version 300 es \n" \
            "precision highp float; \n" \
            "precision highp int; \n" \
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
                    printf("vertex shader compilation log = %s\n", szInfoLog);
                    free(szInfoLog);
                    szInfoLog = NULL;
                }
            }
            [self release];
    [self uninitialize];        }

        // fragment shader
        const GLchar* framgmentShaderSourceCode =
            "#version 300 es\n" \
            "precision highp float; \n" \
            "precision highp int; \n" \
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
                    printf("fragment shader compilation log = %s\n", szInfoLog);
                    free(szInfoLog);
                    szInfoLog = NULL;
                }
            }
            [self release];
    [self uninitialize];
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
                    printf("shader program link log = %s\n", szInfoLog);
                    free(szInfoLog);
                    szInfoLog = NULL;
                }
            }
            [self release];
    [self uninitialize];
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
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // unbind vao
        glBindVertexArray(0);
    
    glClearDepthf(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    perspectiveProjectionMatrix = mat4::identity();
    
     return 0;
}

-(void) display {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // use shader program object
        glUseProgram(shaderProgramObject);

    // transformations
       mat4 modelMatrix = mat4::identity();
       mat4 viewMatrix = mat4::identity();
       mat4 translationMatrix = mat4::identity();
       translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
       modelMatrix = translationMatrix;

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

       // *** bind vao ***
       glBindVertexArray(gVao_sphere);

       // *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gVbo_sphere_element);
       glDrawElements(GL_TRIANGLES, gNumElements, GL_UNSIGNED_SHORT, 0);

       // *** unbind vao ***
       glBindVertexArray(0);

        // unuse shader program object
        glUseProgram(0);}

-(void) myUpdate {}

-(void)awakeFromNib {
    // code
    [super awakeFromNib];
    [self setBackgroundColor:[UIColor blackColor]];
    
    // get drawable layer
    CAEAGLLayer* eaglLayer = (CAEAGLLayer*)[super layer];
    [eaglLayer setOpaque:YES];
    NSDictionary* dictionary = [NSDictionary dictionaryWithObjectsAndKeys:kEAGLDrawablePropertyRetainedBacking, [NSNumber numberWithBool:NO], kEAGLDrawablePropertyColorFormat, kEAGLColorFormatRGBA8,nil];
    [eaglLayer setDrawableProperties:dictionary];
    
    eaglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    if (eaglContext == nil) {
        printf("OpenGL-ES context creation failed\n");
        return;
    }
    [EAGLContext setCurrentContext:eaglContext];
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glGenRenderbuffers(1, &colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:eaglLayer];
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
    GLint width;
    GLint height;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("initWithFrame() FrameBuffer is not complete\n");
        [self uninitialize];
        return;
    }
    
    displayLink = nil;
    frameRateChange.minimum = 30.0f;
    frameRateChange.maximum = 60.0f;
    frameRateChange.preferred = 60.0f;
    isDisplayLink = NO;
    
    int result = [self initialize];
    if (result != 0) {
        printf("initialize failed\n");
        return;
    }
    
    // event handling
    UITapGestureRecognizer* singleTapGestureRecognizer = [[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(onSingleTap:)];
    [singleTapGestureRecognizer setNumberOfTapsRequired:1];
    [singleTapGestureRecognizer setNumberOfTouchesRequired:1];
    [singleTapGestureRecognizer setDelegate:self];
    [self addGestureRecognizer:singleTapGestureRecognizer];
    
    UITapGestureRecognizer* doubleTapGestureRecognizer = [[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(onDoubleTap:)];
    [doubleTapGestureRecognizer setNumberOfTapsRequired:2];
    [doubleTapGestureRecognizer setNumberOfTouchesRequired:1];
    [doubleTapGestureRecognizer setDelegate:self];
    [self addGestureRecognizer:doubleTapGestureRecognizer];
    
    [singleTapGestureRecognizer requireGestureRecognizerToFail:doubleTapGestureRecognizer];
    
    UISwipeGestureRecognizer* swipeGestureRecognizer = [[UISwipeGestureRecognizer alloc]initWithTarget:self action:@selector(onSwipe:)];
    [swipeGestureRecognizer setDelegate:self];
    [self addGestureRecognizer:swipeGestureRecognizer];
    
    UILongPressGestureRecognizer* longPressGestureRecognizer = [[UILongPressGestureRecognizer alloc]initWithTarget:self action:@selector(onLongPress:)];
    [longPressGestureRecognizer setDelegate:self];
    [self addGestureRecognizer:longPressGestureRecognizer];
    
    return ;
}

+(Class)layerClass {
    return [CAEAGLLayer class];
}

-(void) resize:(int) width :(int)height {
    if (height <= 0) height = 1;
    
    glViewport(0, 0, (GLsizei) width, (GLsizei) height);
    
    perspectiveProjectionMatrix = vmath::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);
}

/*- (void)drawRect:(CGRect)rect {
    // code
}*/

-(void) layoutSubviews {
    [super layoutSubviews];
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)[self layer]];
    GLint width;
    GLint height;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("layoutSubviews() FrameBuffer is not complete\n");
        return;
    }
    
    [self resize:width :height];
    [self drawView:self];
}

-(void) drawView:(id) sender {
    [EAGLContext setCurrentContext:eaglContext];
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
    [self myUpdate];
    [self display];
    
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [eaglContext presentRenderbuffer:colorRenderbuffer];
    
}

-(void) startDisplayLink {
    if(isDisplayLink == NO) {
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView:)];
        [displayLink setPreferredFrameRateRange:frameRateChange];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        isDisplayLink = YES;
    }
}

-(void) stopDisplayLink {
    if(isDisplayLink == YES) {
        [displayLink invalidate];
        isDisplayLink = NO;
    }
}

-(BOOL) becomeFirstResponder {
    return YES;
}

-(void) onSingleTap:(UITapGestureRecognizer*) gestureRecognizer {}

-(void) onDoubleTap:(UITapGestureRecognizer*) gestureRecognizer {
    if (bLight == FALSE) {
                            bLight = TRUE;
                        } else {
                            bLight = FALSE;
                        }
}

-(void) onSwipe:(UISwipeGestureRecognizer*) gestureRecognizer {
    // code
    [self uninitialize];
    [self release];
    exit(0);
}

-(void) onLongPress:(UILongPressGestureRecognizer*) gestureRecognizer {}

-(void) touchesBegan:(UITouch*)touches withEvent:(UIEvent *)event {}

-(void) printGLESInfo {
    printf("OPENGL INFORMATION\n");
    printf("------------------\n");
    printf("OpenGL Vendor : %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version : %s\n", glGetString(GL_VERSION));
    printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("------------------\n");
}

-(void) uninitialize {
    if (depthRenderbuffer) glDeleteRenderbuffers(1, &depthRenderbuffer);
    if (colorRenderbuffer) glDeleteRenderbuffers(1, &colorRenderbuffer);
    if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
    if ([EAGLContext currentContext] == eaglContext) [EAGLContext setCurrentContext:nil];
}

-(void) dealloc {
    [self uninitialize];
    [self release];
    [super dealloc];
}

@end
