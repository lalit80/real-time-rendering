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

GLuint vao_cube = 0;
GLuint vbo_position_cube = 0;
GLuint vbo_normal_cube = 0;
float angleCube = 0.0f;

GLuint gVao_sphere = 0;
GLuint gVbo_sphere_position = 0;
GLuint gVbo_sphere_normal = 0;
GLuint gVbo_sphere_element = 0;

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
        modelViewMatrixUniform = glGetUniformLocation(shaderProgramObject, "uModelViewMatrix");
        projectionMatrixUniform = glGetUniformLocation(shaderProgramObject, "uProjectionMatrix");
        LdUniform = glGetUniformLocation(shaderProgramObject, "uLd");
        KdUniform = glGetUniformLocation(shaderProgramObject, "uKd");
        lightPositionUniform = glGetUniformLocation(shaderProgramObject, "uLightPosition");
        LKeyPressUniform = glGetUniformLocation(shaderProgramObject, "uLKeyIsPressed");

    const GLfloat cube_position[] = {
            // front
            1.0f,  1.0f,  1.0f, // top-right of front
            -1.0f,  1.0f,  1.0f, // top-left of front
            -1.0f, -1.0f,  1.0f, // bottom-left of front
            1.0f, -1.0f,  1.0f, // bottom-right of front

            // right
            1.0f,  1.0f, -1.0f, // top-right of right
            1.0f,  1.0f,  1.0f, // top-left of right
            1.0f, -1.0f,  1.0f, // bottom-left of right
            1.0f, -1.0f, -1.0f, // bottom-right of right

            // back
            1.0f,  1.0f, -1.0f, // top-right of back
            -1.0f,  1.0f, -1.0f, // top-left of back
            -1.0f, -1.0f, -1.0f, // bottom-left of back
            1.0f, -1.0f, -1.0f, // bottom-right of back

            // left
            -1.0f,  1.0f,  1.0f, // top-right of left
            -1.0f,  1.0f, -1.0f, // top-left of left
            -1.0f, -1.0f, -1.0f, // bottom-left of left
            -1.0f, -1.0f,  1.0f, // bottom-right of left

            // top
            1.0f,  1.0f, -1.0f, // top-right of top
            -1.0f,  1.0f, -1.0f, // top-left of top
            -1.0f,  1.0f,  1.0f, // bottom-left of top
            1.0f,  1.0f,  1.0f, // bottom-right of top

            // bottom
            1.0f, -1.0f,  1.0f, // top-right of bottom
            -1.0f, -1.0f,  1.0f, // top-left of bottom
            -1.0f, -1.0f, -1.0f, // bottom-left of bottom
            1.0f, -1.0f, -1.0f, // bottom-right of bottom
        };
    
    const GLfloat cube_normal[] = {
            // front surface
            0.0f,  0.0f,  1.0f, // top-right of front
            0.0f,  0.0f,  1.0f, // top-left of front
            0.0f,  0.0f,  1.0f, // bottom-left of front
            0.0f,  0.0f,  1.0f, // bottom-right of front

            // right surface
            1.0f,  0.0f,  0.0f, // top-right of right
            1.0f,  0.0f,  0.0f, // top-left of right
            1.0f,  0.0f,  0.0f, // bottom-left of right
            1.0f,  0.0f,  0.0f, // bottom-right of right

            // back surface
            0.0f,  0.0f, -1.0f, // top-right of back
            0.0f,  0.0f, -1.0f, // top-left of back
            0.0f,  0.0f, -1.0f, // bottom-left of back
            0.0f,  0.0f, -1.0f, // bottom-right of back

            // left surface
            -1.0f,  0.0f,  0.0f, // top-right of left
            -1.0f,  0.0f,  0.0f, // top-left of left
            -1.0f,  0.0f,  0.0f, // bottom-left of left
            -1.0f,  0.0f,  0.0f, // bottom-right of left

            // top surface
            0.0f,  1.0f,  0.0f, // top-right of top
            0.0f,  1.0f,  0.0f, // top-left of top
            0.0f,  1.0f,  0.0f, // bottom-left of top
            0.0f,  1.0f,  0.0f, // bottom-right of top

            // bottom surface
            0.0f, -1.0f,  0.0f, // top-right of bottom
            0.0f, -1.0f,  0.0f, // top-left of bottom
            0.0f, -1.0f,  0.0f, // bottom-left of bottom
            0.0f, -1.0f,  0.0f, // bottom-right of bottom
        };

        // Rectangle
        // vertex array object for arrays of vertex attributes
        glGenVertexArrays(1, &vao_cube);
        glBindVertexArray(vao_cube);

        // position
        glGenBuffers(1, &vbo_position_cube);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position_cube);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_position), cube_position, GL_STATIC_DRAW);
        glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // color
        glGenBuffers(1, &vbo_normal_cube);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal_cube);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normal), cube_normal, GL_STATIC_DRAW);
        glVertexAttribPointer(AMC_ATTRIBUTE_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(AMC_ATTRIBUTE_NORMAL);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

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

    // Cube
        // transformations
        mat4 modelViewMatrix = mat4::identity();
        mat4 translationMatrix = mat4::identity();
        mat4 rotationMatrix = mat4::identity();
        mat4 rotationMatrix1 = mat4::identity();
        mat4 rotationMatrix2 = mat4::identity();
        mat4 rotationMatrix3 = mat4::identity();
        mat4 scaleMatrix = mat4::identity();
        rotationMatrix = mat4::identity();
        modelViewMatrix = mat4::identity();
        
        scaleMatrix = vmath::scale(0.75f, 0.75f, 0.75f);
        translationMatrix = vmath::translate(0.0f, 0.0f, -5.0f);
        rotationMatrix1 = vmath::rotate(angleCube, 1.0f, 0.0f, 0.0f);
        rotationMatrix2 = vmath::rotate(angleCube, 0.0f, 1.0f, 0.0f);
        rotationMatrix3 = vmath::rotate(angleCube, 0.0f, 0.0f, 1.0f);
        rotationMatrix = rotationMatrix1 * rotationMatrix2 * rotationMatrix3;
        
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
        glUseProgram(0);}

-(void) myUpdate {
    angleCube += 0.5f;
}

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
