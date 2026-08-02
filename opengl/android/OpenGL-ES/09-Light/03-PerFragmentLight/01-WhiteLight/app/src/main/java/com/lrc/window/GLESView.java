package com.lrc.window;

import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.GestureDetector.OnDoubleTapListener;
import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap;

// opengl
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;
import android.opengl.GLES32;
import android.opengl.GLUtils;

// io
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

public class GLESView extends GLSurfaceView implements OnGestureListener, OnDoubleTapListener, GLSurfaceView.Renderer {
    private Context context;
    private GestureDetector gestureDetector;
    private int shaderProgramObject;
    
    // uniforms
    private int modelMatrixUniform = 0;
    private int viewMatrixUniform = 0;
    private int projectionMatrixUniform = 0;
    private int LaUniform = 0;
    private int LdUniform = 0;
    private int LsUniform = 0;
    private int KaUniform = 0;
    private int KdUniform = 0;
    private int KsUniform = 0;
    private int materialShininessUniform = 0;
    private int lightPositionUniform = 0;
    private int SingleTapPressUniform = 0;

    private int[] vao_sphere = new int[1];
    private int[] vbo_sphere_position = new int[1];
    private int[] vbo_sphere_normal = new int[1];
    private int[] vbo_sphere_element = new int[1];
    private int numVertices;
    private int numElements;
    private float perspectiveProjectionMatrix[] = new float[16];

    private boolean bLight;
    private Light light[] = new Light[1];
    private Material material[] = new Material[1];

    public GLESView(Context _context) {
        super(_context);
        context = _context;
        gestureDetector = new GestureDetector(context, this, null, false);
        gestureDetector.setOnDoubleTapListener(this);

        // inititalize opengl-es
        setEGLContextClientVersion(3);
        setRenderer(this);
        setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    // renderer methods
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        int iResult = inititalize(gl);
        if (iResult != 0) {
            System.out.println("lrc: inititalize() failed");
            System.exit(0);
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) { resize(width, height); }
    @Override
    public void onDrawFrame(GL10 gl) { display(); update(); }

    // listener
    @Override
    public boolean onTouchEvent(MotionEvent e) {
        if (!gestureDetector.onTouchEvent(e)) {
            super.onTouchEvent(e);
        }
        return true;
    }
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) { 
        uninitialize(); 
        System.exit(0);
        return true;
    }
    @Override public boolean onSingleTapConfirmed(MotionEvent e) { 
        if (bLight == true) bLight = false;
        else bLight = true;
        return true;
    }
    @Override public void onLongPress(MotionEvent e) {}
    @Override public void onShowPress(MotionEvent e) {}
    @Override public boolean onSingleTapUp(MotionEvent e) { return true; }
    @Override public boolean onDoubleTap(MotionEvent e) { return true; }
    @Override public boolean onDoubleTapEvent(MotionEvent e) { return true; }
    @Override public boolean onDown(MotionEvent e) { return true; }
    @Override public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) { return true; }

    // our custom opengl-es methods
    private int inititalize(GL10 gl) {
        printGLESInfo(gl);

        // shader
        final String vertexShaderSourceCode = String.format(
            "#version 320 es \n"+
            "precision highp float;"+
            "precision highp int; \n"+
            "in vec4 aPosition; \n"+
            "in vec3 aNormal; \n"+
            "out vec3 out_transformedNormals; \n"+
            "out vec3 out_lightDirection; \n"+
            "out vec3 out_viewerVector; \n"+
            "uniform mat4 uModelMatrix; \n"+
            "uniform mat4 uViewMatrix; \n"+
            "uniform mat4 uProjectionMatrix; \n"+
            "uniform vec4 uLightPosition; \n"+
            "uniform int uSingleTapPress; \n"+
            "void main(void) \n"+
            "{ \n"+
                "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
                "if (uSingleTapPress == 1) { \n"+
                    "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                    "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                    "out_transformedNormals = normalMatrix * aNormal; \n"+
                    "out_lightDirection = vec3(uLightPosition - eyeCoordinates); \n"+
                    "out_viewerVector = -eyeCoordinates.xyz; \n"+
                "} \n"+
            "} \n"
        );
        int vertexShaderObject = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject, vertexShaderSourceCode);
        GLES32.glCompileShader(vertexShaderObject);
        checkShaderError(vertexShaderObject, "vertex");

        final String framgmentShaderSourceCode = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "precision highp int; \n"+
            "in vec3 out_transformedNormals; \n"+
            "in vec3 out_lightDirection; \n"+
            "in vec3 out_viewerVector; \n"+
            "out vec4 FragColor; \n"+
            "uniform vec3 uLa; \n"+
            "uniform vec3 uLd; \n"+
            "uniform vec3 uLs; \n"+
            "uniform vec3 uKa; \n"+
            "uniform vec3 uKd; \n"+
            "uniform vec3 uKs; \n"+
            "uniform float uMaterialShininess; \n"+
            "uniform int uSingleTapPress; \n"+
            "void main(void)\n"+
            "{\n"+
                "vec3 phong_ads_light; \n"+
                "if (uSingleTapPress == 1) { \n"+
                    "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
                    "vec3 normalizedLightDirection = normalize(out_lightDirection); \n"+
                    "vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
                    "vec3 ambientLight = uLa * uKa * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
                    "vec3 diffuseLight = uLd * uKd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
                    "vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n"+
                    "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialShininess); \n"+
                    "phong_ads_light = ambientLight + diffuseLight + specularLight; \n"+
                "} \n"+
                "else { \n"+
                    "phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n"+
                "} \n"+
                "FragColor = vec4(phong_ads_light, 1.0f); \n"+
            "}\n"
        );

        int framgmentShaderObject = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject, framgmentShaderSourceCode);
        GLES32.glCompileShader(framgmentShaderObject);
        checkShaderError(framgmentShaderObject, "fragment");

        shaderProgramObject = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject, vertexShaderObject);
        GLES32.glAttachShader(shaderProgramObject, framgmentShaderObject);

        GLES32.glBindAttribLocation(shaderProgramObject, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glBindAttribLocation(shaderProgramObject, MyAttributes.AMC_ATTRIBUTE_NORMAL, "aNormal");
        GLES32.glLinkProgram(shaderProgramObject);
        checkProgramError(shaderProgramObject);

        // get the required uniform location from the shader
        modelMatrixUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uModelMatrix");
        viewMatrixUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uViewMatrix");
        projectionMatrixUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uProjectionMatrix");
        LaUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uLa");
        LdUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uLd");
        LsUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uLs");
        lightPositionUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uLightPosition");
        KaUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uKa");
        KdUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uKd");
        KsUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uKs");
        materialShininessUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uMaterialShininess");
        SingleTapPressUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uSingleTapPress");

        Sphere sphere=new Sphere();
        float sphere_vertices[]=new float[1146];
        float sphere_normals[]=new float[1146];
        float sphere_textures[]=new float[764];
        short sphere_elements[]=new short[2280];
        sphere.getSphereVertexData(sphere_vertices, sphere_normals, sphere_textures, sphere_elements);
        numVertices = sphere.getNumberOfSphereVertices();
        numElements = sphere.getNumberOfSphereElements();

        // vao
        GLES32.glGenVertexArrays(1,vao_sphere,0);
        GLES32.glBindVertexArray(vao_sphere[0]);
        
        // position vbo
        GLES32.glGenBuffers(1,vbo_sphere_position,0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER,vbo_sphere_position[0]);
        
        ByteBuffer byteBuffer=ByteBuffer.allocateDirect(sphere_vertices.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer verticesBuffer=byteBuffer.asFloatBuffer();
        verticesBuffer.put(sphere_vertices);
        verticesBuffer.position(0);
        
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER,
                            sphere_vertices.length * 4,
                            verticesBuffer,
                            GLES32.GL_STATIC_DRAW);
        
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION,
                                     3,
                                     GLES32.GL_FLOAT,
                                     false,0,0);
        
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER,0);
        
        // normal vbo
        GLES32.glGenBuffers(1,vbo_sphere_normal,0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER,vbo_sphere_normal[0]);
        
        byteBuffer=ByteBuffer.allocateDirect(sphere_normals.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        verticesBuffer=byteBuffer.asFloatBuffer();
        verticesBuffer.put(sphere_normals);
        verticesBuffer.position(0);
        
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER,
                            sphere_normals.length * 4,
                            verticesBuffer,
                            GLES32.GL_STATIC_DRAW);
        
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_NORMAL,
                                     3,
                                     GLES32.GL_FLOAT,
                                     false,0,0);
        
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_NORMAL);
        
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER,0);
        
        // element vbo
        GLES32.glGenBuffers(1,vbo_sphere_element,0);
        GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER,vbo_sphere_element[0]);
        
        byteBuffer=ByteBuffer.allocateDirect(sphere_elements.length * 2);
        byteBuffer.order(ByteOrder.nativeOrder());
        ShortBuffer elementsBuffer=byteBuffer.asShortBuffer();
        elementsBuffer.put(sphere_elements);
        elementsBuffer.position(0);
        
        GLES32.glBufferData(GLES32.GL_ELEMENT_ARRAY_BUFFER,
                            sphere_elements.length * 2,
                            elementsBuffer,
                            GLES32.GL_STATIC_DRAW);
        
        GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER,0);

        GLES32.glBindVertexArray(0);

        // depth initialization
        GLES32.glClearDepthf(1.0f);
        GLES32.glEnable(GLES32.GL_DEPTH_TEST);
        GLES32.glDepthFunc(GLES32.GL_LEQUAL);
        
        // set the clear color
        GLES32.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // light initialization
        bLight = false;
        light[0] = new Light();
        material[0] = new Material();

        light[0].ambient[0] = 0.0f;
        light[0].ambient[1] = 0.0f;
        light[0].ambient[2] = 0.0f;
        light[0].ambient[3] = 1.0f;

        light[0].diffuse[0] = 1.0f;
        light[0].diffuse[1] = 1.0f;
        light[0].diffuse[2] = 1.0f;
        light[0].diffuse[3] = 1.0f;

        light[0].specular[0] = 1.0f;
        light[0].specular[1] = 1.0f;
        light[0].specular[2] = 1.0f;
        light[0].specular[3] = 1.0f;

        light[0].position[0] = 100.0f;
        light[0].position[1] = 100.0f;
        light[0].position[2] = 100.0f;
        light[0].position[3] = 1.0f;

        material[0].ambient[0] = 0.0f;
        material[0].ambient[1] = 0.0f;
        material[0].ambient[2] = 0.0f;
        material[0].ambient[3] = 1.0f;

        material[0].diffuse[0] = 1.0f;
        material[0].diffuse[1] = 1.0f;
        material[0].diffuse[2] = 1.0f;
        material[0].diffuse[3] = 1.0f;

        material[0].specular[0] = 1.0f;
        material[0].specular[1] = 1.0f;
        material[0].specular[2] = 1.0f;
        material[0].specular[3] = 1.0f;
        
        material[0].shininiess = 50.0f;

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        return 0;
    }

    private void display() {
        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);
        GLES32.glUseProgram(shaderProgramObject);

        float modelMatrix[] = new float[16];
        float viewMatrix[] = new float[16];
        float translationMatrix[] = new float[16];
        float rotationMatrix[] = new float[16];
        Matrix.setIdentityM(modelMatrix, 0);
        Matrix.setIdentityM(viewMatrix, 0);
        Matrix.setIdentityM(translationMatrix, 0);
        Matrix.setIdentityM(rotationMatrix, 0);
        Matrix.translateM(translationMatrix, 0, 0.0f, 0.0f, -2.0f);
        
        modelMatrix = translationMatrix;
        //Matrix.multiplyMM(modelViewProjectionMatrix, 0, perspectiveProjectionMatrix, 0, modelViewMatrix, 0);

        // send this matrix to vertex shader in uniform
        GLES32.glUniformMatrix4fv(modelMatrixUniform, 1, false, modelMatrix, 0);
        GLES32.glUniformMatrix4fv(viewMatrixUniform, 1, false, viewMatrix, 0);
        GLES32.glUniformMatrix4fv(projectionMatrixUniform, 1, false, perspectiveProjectionMatrix, 0);

        if (bLight == true) {
            GLES32.glUniform3fv(LaUniform, 1, light[0].ambient, 0);
            GLES32.glUniform3fv(LdUniform, 1, light[0].diffuse, 0);
            GLES32.glUniform3fv(LsUniform, 1, light[0].specular, 0);
            GLES32.glUniform4fv(lightPositionUniform, 1, light[0].position, 0);
            GLES32.glUniform3fv(KaUniform, 1, material[0].ambient, 0);
            GLES32.glUniform3fv(KdUniform, 1, material[0].diffuse, 0);
            GLES32.glUniform3fv(KsUniform, 1, material[0].specular, 0);
            GLES32.glUniform1f(materialShininessUniform, material[0].shininiess);
            GLES32.glUniform1i(SingleTapPressUniform, 1);
        } else {
            GLES32.glUniform1i(SingleTapPressUniform, 0);
        }

        // bind vao
        GLES32.glBindVertexArray(vao_sphere[0]);
        
        // *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
        GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, vbo_sphere_element[0]);
        GLES32.glDrawElements(GLES32.GL_TRIANGLES, numElements, GLES32.GL_UNSIGNED_SHORT, 0);
        
        // unbind vao
        GLES32.glBindVertexArray(0);

        GLES32.glUseProgram(0);

        requestRender();        // swap buffer
    }

    private void update() {}

    private void resize(int width, int height) {
        if (height <= 0) height = 1;
        GLES32.glViewport(0, 0, width, height);
        Matrix.perspectiveM(perspectiveProjectionMatrix, 0, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
    }

    private int loadGLTexture(int imageFileResourceID) {
        // decide whether android should scale texture or not	
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        // create android compatible bitmap of our image
        Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), imageFileResourceID, options);

        int tex[] = new int[1];
        GLES32.glGenTextures(1, tex, 0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, tex[0]);
        GLES32.glPixelStorei(GLES32.GL_UNPACK_ALIGNMENT, 4);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MAG_FILTER, GLES32.GL_LINEAR);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MIN_FILTER, GLES32.GL_LINEAR_MIPMAP_LINEAR);
        GLUtils.texImage2D(GLES32.GL_TEXTURE_2D, 0, bitmap, 0);
        GLES32.glGenerateMipmap(GLES32.GL_TEXTURE_2D);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, 0);
        
        return tex[0];
    }

    private void printGLESInfo(GL10 gl) {
        String gles_vendor = gl.glGetString(GL10.GL_VENDOR);
        String gles_version = gl.glGetString(GL10.GL_VERSION);
        String gles_renderer = gl.glGetString(GL10.GL_RENDERER);
        
        System.out.println("lrc: " + gles_vendor);
        System.out.println("lrc: " + gles_version);
        System.out.println("lrc: " + gles_renderer);
    }

    private void checkShaderError(int shaderObject, String shader) {
        int status[] = new int[1];
        int infoLogLength[] = new int[1];
        String szInfoLog = null;
        GLES32.glGetShaderiv(shaderObject, GLES32.GL_COMPILE_STATUS, status, 0);
        if (status[0] == GLES32.GL_FALSE) {
            GLES32.glGetShaderiv(shaderObject, GLES32.GL_INFO_LOG_LENGTH, infoLogLength, 0);
            if (infoLogLength[0] > 0) {
                szInfoLog = GLES32.glGetShaderInfoLog(shaderObject);
                System.out.println("lrc: " + shader + " shader compilation log = " + szInfoLog);
            }
            uninitialize();
        }
    }

    private void checkProgramError(int programObject) {
        int status[] = new int[1];
        int infoLogLength[] = new int[1];
        String szInfoLog = null;
        GLES32.glGetProgramiv(programObject, GLES32.GL_LINK_STATUS, status, 0);
        if (status[0] == GLES32.GL_FALSE) {
            GLES32.glGetProgramiv(programObject, GLES32.GL_INFO_LOG_LENGTH, infoLogLength, 0);
            if (infoLogLength[0] > 0) {
                szInfoLog = GLES32.glGetProgramInfoLog(programObject);
                System.out.println("lrc: shader program link log = " + szInfoLog);
            }
            uninitialize();
        }
    }

    private void uninitialize() {
        // destroy vao
        if(vao_sphere[0] != 0) {
            GLES32.glDeleteVertexArrays(1, vao_sphere, 0);
            vao_sphere[0]=0;
        }
        // destroy position vbo
        if(vbo_sphere_position[0] != 0) {
            GLES32.glDeleteBuffers(1, vbo_sphere_position, 0);
            vbo_sphere_position[0]=0;
        }
        // destroy normal vbo
        if(vbo_sphere_normal[0] != 0) {
            GLES32.glDeleteBuffers(1, vbo_sphere_normal, 0);
            vbo_sphere_normal[0]=0;
        }
        // destroy element vbo
        if(vbo_sphere_element[0] != 0) {
            GLES32.glDeleteBuffers(1, vbo_sphere_element, 0);
            vbo_sphere_element[0]=0;
        }

        // detach, delete shader objects and delete shader program object
        if (shaderProgramObject > 0) {
            GLES32.glUseProgram(shaderProgramObject);
            int retVal[] = new int[1];
            GLES32.glGetProgramiv(shaderProgramObject, GLES32.GL_ATTACHED_SHADERS, retVal, 0);
            int numShaders = retVal[0];
            if (numShaders > 0) {
                int pShaders[] = new int[numShaders];
                GLES32.glGetAttachedShaders(shaderProgramObject, numShaders, retVal, 0, pShaders, 0);
                for (int i = 0; i < numShaders; ++i) {
                    GLES32.glDetachShader(shaderProgramObject, pShaders[i]);
                    GLES32.glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
            }
            GLES32.glUseProgram(0);
            GLES32.glDeleteProgram(shaderProgramObject);
            shaderProgramObject = 0;
        }
    }
}

class Light {
    float ambient[] = new float[4];
    float diffuse[] = new float[4];
    float specular[] = new float[4];
    float position[] = new float[4];
}

class Material {
    float ambient[] = new float[4];
    float diffuse[] = new float[4];
    float specular[] = new float[4];
    float shininiess;
}
