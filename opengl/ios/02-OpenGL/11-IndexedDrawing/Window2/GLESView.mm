#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#import <QuartzCore/CADisplayLink.h>
#import "GLESView.h"

// custom header files
#include "vmath.h"
using namespace vmath;

GLuint shaderProgramObject = 0;

enum {
    AMC_ATTRIBUTE_POSITION = 0,
    AMC_ATTRIBUTE_COLOR,
};

GLuint vao = 0;
GLuint vbo_position = 0;
GLuint ibo_position = 0;

GLuint mvpMatrixUniform = 0;

mat4 perspectiveProjectionMatrix;

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
            "uniform mat4 uMVPMatrix; \n" \
            "void main(void) \n" \
            "{ \n" \
            "   gl_Position = uMVPMatrix * aPosition; \n" \
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
    "out vec4 FragColor; \n" \
            "void main(void)\n" \
            "{\n" \
            "   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); \n" \
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
        mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");

    // provide vertex position, color, normal, textcord, etc
       const GLfloat rectangle_position[] = {
                                               1, 1, 0,
                                               -1, 1, 0,
                                               -1, -1, 0,
                                               1, -1, 0
                                           };

       const unsigned int rectangle_indices[] = {
                                               0, 1, 2,
                                               2, 3, 0
                                           };

       // vertex array object for arrays of vertex attributes
       glGenVertexArrays(1, &vao);
       glBindVertexArray(vao);

       glGenBuffers(1, &vbo_position);
       glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
       glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_position), rectangle_position, GL_STATIC_DRAW);
       glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, NULL);
       glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
       glBindBuffer(GL_ARRAY_BUFFER, 0);

       glGenBuffers(1, &ibo_position);
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_position);
       glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indices), rectangle_indices, GL_STATIC_DRAW);
       //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);   don't unbind if done then bind in display

       glBindVertexArray(0);
    glClearDepthf(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    
    perspectiveProjectionMatrix = mat4::identity();
    
     return 0;
}

-(void) display {
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

        // bind with vao
        glBindVertexArray(vao);

        // draw the vertex arrays
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // unbind with vao
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

-(void) onDoubleTap:(UITapGestureRecognizer*) gestureRecognizer {}

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
