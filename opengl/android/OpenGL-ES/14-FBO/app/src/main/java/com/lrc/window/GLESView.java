package com.lrc.window;

import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.GestureDetector.OnGestureListener;
import android.view.GestureDetector.OnDoubleTapListener;
import android.content.Context;

// opengl
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;
import android.opengl.GLES32;

// io
import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class GLESView extends GLSurfaceView implements OnGestureListener, OnDoubleTapListener, GLSurfaceView.Renderer {
    private Context context;
    private GestureDetector gestureDetector;

    private final int FBO_WIDTH = 512;
    private final int FBO_HEIGHT = 512;

    private int shaderProgramObject_cube;
    private int shaderProgramObject_sphere;

    private int modelMatrixUniform_sphere = 0;
    private int viewMatrixUniform_sphere = 0;
    private int projectionMatrixUniform_sphere = 0;
    private int LaUniform_sphere = 0;
    private int LdUniform_sphere = 0;
    private int LsUniform_sphere = 0;
    private int KaUniform_sphere = 0;
    private int KdUniform_sphere = 0;
    private int KsUniform_sphere = 0;
    private int materialShininessUniform_sphere = 0;
    private int lightPositionUniform_sphere = 0;
    private int SingleTapPressUniform_sphere = 0;

    private int mvpMatrixUniform_cube = 0;
    private int textureSamplerUniform_cube = 0;

    private int[] vao_sphere = new int[1];
    private int[] vbo_sphere_position = new int[1];
    private int[] vbo_sphere_normal = new int[1];
    private int[] vbo_sphere_element = new int[1];
    private int numVertices;
    private int numElements;

    private int[] vao_cube = new int[1];
    private int[] vbo_position_cube = new int[1];
    private int[] vbo_texcoord_cube = new int[1];

    private int[] fbo = new int[1];
    private int[] rbo = new int[1];
    private int[] fbo_texture = new int[1];
    private int fboResult = -1;

    private float perspectiveProjectionMatrix[] = new float[16];
    private float perspectiveProjectionMatrix_sphere[] = new float[16];
    private float angleCube = 0.0f;
    private int winWidth;
    private int winHeight;

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

        initialize_sphere();

        // shader
        final String vertexShaderSourceCode = 
            "#version 320 es \n" +
            "in vec4 aPosition; \n" +
            "in vec2 aTexCoord; \n" +
            "out vec2 out_texcoord; \n" +
            "uniform mat4 uMVPMatrix; \n" +
            "void main(void) \n" +
            "{ \n" +
            "   gl_Position = uMVPMatrix * aPosition; \n" +
            "   out_texcoord = aTexCoord; \n" +
            "} \n";

        int vertexShaderObject = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject, vertexShaderSourceCode);
        GLES32.glCompileShader(vertexShaderObject);
        checkShaderError(vertexShaderObject, "vertex cube");

        final String framgmentShaderSourceCode = 
            "#version 320 es\n" +
            "precision highp float;\n" +
            "in vec2 out_texcoord; \n" +
            "uniform sampler2D uTextureSampler; \n" +
            "out vec4 FragColor; \n" +
            "void main(void)\n" +
            "{\n" +
            "   FragColor = texture(uTextureSampler, out_texcoord); \n" +
            "}\n";

        int framgmentShaderObject = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject, framgmentShaderSourceCode);
        GLES32.glCompileShader(framgmentShaderObject);
        checkShaderError(framgmentShaderObject, "fragment cube");

        shaderProgramObject_cube = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject_cube, vertexShaderObject);
        GLES32.glAttachShader(shaderProgramObject_cube, framgmentShaderObject);

        GLES32.glBindAttribLocation(shaderProgramObject_cube, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glBindAttribLocation(shaderProgramObject_cube, MyAttributes.AMC_ATTRIBUTE_TEXTCORD, "aTexCoord");
        GLES32.glLinkProgram(shaderProgramObject_cube);
        checkProgramError(shaderProgramObject_cube);

        mvpMatrixUniform_cube = GLES32.glGetUniformLocation(shaderProgramObject_cube, "uMVPMatrix");
        textureSamplerUniform_cube = GLES32.glGetUniformLocation(shaderProgramObject_cube, "uTextureSampler");

        final float cube_position[] = {   
            // front
            1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f,  1.0f,
            // right
            1.0f,  1.0f, -1.0f, 1.0f,  1.0f,  1.0f, 1.0f, -1.0f,  1.0f, 1.0f, -1.0f, -1.0f,
            // back
            1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
            // left
            -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
            // top
            1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
            // bottom
            1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
        };

        final float cube_texcoords[] = {   
            // front
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // right
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // back
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // left
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // top
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            // bottom
            1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        };

        GLES32.glGenVertexArrays(1, vao_cube, 0);
        GLES32.glBindVertexArray(vao_cube[0]);

        // position
        GLES32.glGenBuffers(1, vbo_position_cube, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vbo_position_cube[0]);
        ByteBuffer bb = ByteBuffer.allocateDirect(cube_position.length * 4);
        bb.order(ByteOrder.nativeOrder());
        FloatBuffer fb = bb.asFloatBuffer();
        fb.put(cube_position);
        fb.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, cube_position.length * 4, fb, GLES32.GL_STATIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        // texture
        GLES32.glGenBuffers(1, vbo_texcoord_cube, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vbo_texcoord_cube[0]);
        bb = ByteBuffer.allocateDirect(cube_texcoords.length * 4);
        bb.order(ByteOrder.nativeOrder());
        fb = bb.asFloatBuffer();
        fb.put(cube_texcoords);
        fb.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, cube_texcoords.length * 4, fb, GLES32.GL_STATIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_TEXTCORD, 2, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_TEXTCORD);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        GLES32.glBindVertexArray(0);

        // depth initialization
        GLES32.glClearDepthf(1.0f);
        GLES32.glEnable(GLES32.GL_DEPTH_TEST);
        GLES32.glDepthFunc(GLES32.GL_LEQUAL);
        
        // set the clear color
        GLES32.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        if (createAndPrepareFBOForDrawing(FBO_WIDTH, FBO_HEIGHT) == true) {
            System.out.println("lrc: fbo creation successful\n");
            fboResult = initialize_sphere();

            if (fboResult != 0) {
                System.out.println("lrc: initialize_sphere failed");
                return (-1);
            } else {
                System.out.println("lrc: initialize_sphere success");
            }
        } else {
            System.out.println("lrc: fbo creation failed\n");
        }

        bLight = false;
        light[0] = new Light();
        material[0] = new Material();

        light[0].ambient[0] = 0.1f;
        light[0].ambient[1] = 0.1f;
        light[0].ambient[2] = 0.1f;
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

        material[0].diffuse[0] = 0.5f; 
        material[0].diffuse[1] = 0.2f; 
        material[0].diffuse[2] = 0.7f; 
        material[0].diffuse[3] = 1.0f;

        material[0].specular[0] = 0.7f; 
        material[0].specular[1] = 0.7f; 
        material[0].specular[2] = 0.7f; 
        material[0].specular[3] = 1.0f;

        material[0].shininiess = 128.0f;

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        return 0;
    }

    private void display() {
        if (fboResult == 0) {
            display_sphere();
        }

        GLES32.glBindFramebuffer(GLES32.GL_FRAMEBUFFER, 0);
        resize(winWidth, winHeight);
        
        GLES32.glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);
        GLES32.glUseProgram(shaderProgramObject_cube);

        float modelViewMatrix[] = new float[16];
        float translationMatrix[] = new float[16];
        float rotationMatrix[] = new float[16];
        float modelViewProjectionMatrix[] = new float[16];
        Matrix.setIdentityM(modelViewMatrix, 0);
        Matrix.setIdentityM(translationMatrix, 0);
        Matrix.setIdentityM(rotationMatrix, 0);
        Matrix.setIdentityM(modelViewProjectionMatrix, 0);
        Matrix.translateM(translationMatrix, 0, 0.0f, 0.0f, -5.0f);
        Matrix.setRotateM(rotationMatrix, 0, angleCube, 0.0f, 1.0f, 0.0f);
        
        //modelViewMatrix = translationMatrix * rotationMatrix;
        Matrix.multiplyMM(modelViewMatrix, 0, translationMatrix, 0, rotationMatrix, 0);
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, perspectiveProjectionMatrix, 0, modelViewMatrix, 0);

        // send this matrix to vertex shader in uniform
        GLES32.glUniformMatrix4fv(mvpMatrixUniform_cube, 1, false, modelViewProjectionMatrix, 0);

        // Bind FBO texture
        GLES32.glActiveTexture(GLES32.GL_TEXTURE0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, fbo_texture[0]);
        GLES32.glUniform1i(textureSamplerUniform_cube, 0);

        // bind with vao
        GLES32.glBindVertexArray(vao_cube[0]);

        // draw the vertex arrays
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 0, 4);
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 4, 4);
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 8, 4);
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 12, 4);
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 16, 4);
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 20, 4);

        // unbind with vao
        GLES32.glBindVertexArray(0);

        GLES32.glUseProgram(0);

        requestRender();        // swap buffer
    }

    private void display_sphere() {
        GLES32.glBindFramebuffer(GLES32.GL_FRAMEBUFFER, fbo[0]);
        resize_sphere(FBO_WIDTH, FBO_HEIGHT);
        
        GLES32.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);
        
        GLES32.glUseProgram(shaderProgramObject_sphere);

        float modelMatrix[] = new float[16];
        float viewMatrix[] = new float[16];
        float translationMatrix[] = new float[16];
        
        Matrix.setIdentityM(modelMatrix, 0);
        Matrix.setIdentityM(viewMatrix, 0);
        Matrix.setIdentityM(translationMatrix, 0);
        Matrix.translateM(translationMatrix, 0, 0.0f, 0.0f, -2.0f);
        modelMatrix = translationMatrix;

        GLES32.glUniformMatrix4fv(modelMatrixUniform_sphere, 1, false, modelMatrix, 0);
        GLES32.glUniformMatrix4fv(viewMatrixUniform_sphere, 1, false, viewMatrix, 0);
        GLES32.glUniformMatrix4fv(projectionMatrixUniform_sphere, 1, false, perspectiveProjectionMatrix_sphere, 0);

        if (bLight == true) {
            GLES32.glUniform3fv(LaUniform_sphere, 1, light[0].ambient, 0);
            GLES32.glUniform3fv(LdUniform_sphere, 1, light[0].diffuse, 0);
            GLES32.glUniform3fv(LsUniform_sphere, 1, light[0].specular, 0);
            GLES32.glUniform4fv(lightPositionUniform_sphere, 1, light[0].position, 0);
            GLES32.glUniform3fv(KaUniform_sphere, 1, material[0].ambient, 0);
            GLES32.glUniform3fv(KdUniform_sphere, 1, material[0].diffuse, 0);
            GLES32.glUniform3fv(KsUniform_sphere, 1, material[0].specular, 0);
            GLES32.glUniform1f(materialShininessUniform_sphere, material[0].shininiess);
            GLES32.glUniform1i(SingleTapPressUniform_sphere, 1);
        } else {
            GLES32.glUniform1i(SingleTapPressUniform_sphere, 0);
        }

        GLES32.glBindVertexArray(vao_sphere[0]);
        GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, vbo_sphere_element[0]);
        GLES32.glDrawElements(GLES32.GL_TRIANGLES, numElements, GLES32.GL_UNSIGNED_SHORT, 0);
        GLES32.glBindVertexArray(0);
        GLES32.glUseProgram(0);
    }

    private void update() {
        angleCube += 0.5f;
    }

    private void resize(int width, int height) {
        if (height <= 0) height = 1;
        winWidth = width;
        winHeight = height;
        GLES32.glViewport(0, 0, width, height);
        Matrix.perspectiveM(perspectiveProjectionMatrix, 0, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
    }

    private void resize_sphere(int width, int height) {
        if (height <= 0) height = 1;
        GLES32.glViewport(0, 0, width, height);
        Matrix.perspectiveM(perspectiveProjectionMatrix_sphere, 0, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
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
        if(vao_sphere[0] != 0) { GLES32.glDeleteVertexArrays(1, vao_sphere, 0); vao_sphere[0]=0; }
        if(vbo_sphere_position[0] != 0) { 
            GLES32.glDeleteBuffers(1, vbo_sphere_position, 0); 
            vbo_sphere_position[0]=0; 
        }
        if(vbo_sphere_normal[0] != 0) { 
            GLES32.glDeleteBuffers(1, vbo_sphere_normal, 0); 
            vbo_sphere_normal[0]=0; 
        }
        if(vbo_sphere_element[0] != 0) { 
            GLES32.glDeleteBuffers(1, vbo_sphere_element, 0); 
            vbo_sphere_element[0]=0; 
        }

        // destroy cube buffers
        if(vao_cube[0] != 0) { 
            GLES32.glDeleteVertexArrays(1, vao_cube, 0); 
            vao_cube[0]=0; 
        }
        if(vbo_position_cube[0] != 0) { 
            GLES32.glDeleteBuffers(1, vbo_position_cube, 0); 
            vbo_position_cube[0]=0; 
        }
        if(vbo_texcoord_cube[0] != 0) { 
            GLES32.glDeleteBuffers(1, vbo_texcoord_cube, 0); 
            vbo_texcoord_cube[0]=0; 
        }

        // destroy fbo
        if(fbo[0] != 0) { 
            GLES32.glDeleteFramebuffers(1, fbo, 0); 
            fbo[0]=0; 
        }
        if(rbo[0] != 0) { 
            GLES32.glDeleteRenderbuffers(1, rbo, 0); 
            rbo[0]=0; 
        }
        if(fbo_texture[0] != 0) { 
            GLES32.glDeleteTextures(1, fbo_texture, 0); 
            fbo_texture[0]=0; 
        }

        // detach, delete shader objects and delete shader program object
        if (shaderProgramObject_cube > 0) {
            GLES32.glUseProgram(shaderProgramObject_cube);
            int retVal[] = new int[1];
            GLES32.glGetProgramiv(shaderProgramObject_cube, GLES32.GL_ATTACHED_SHADERS, retVal, 0);
            int numShaders = retVal[0];
            if (numShaders > 0) {
                int pShaders[] = new int[numShaders];
                GLES32.glGetAttachedShaders(shaderProgramObject_cube, numShaders, retVal, 0, pShaders, 0);
                for (int i = 0; i < numShaders; ++i) {
                    GLES32.glDetachShader(shaderProgramObject_cube, pShaders[i]);
                    GLES32.glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
            }
            GLES32.glUseProgram(0);
            GLES32.glDeleteProgram(shaderProgramObject_cube);
            shaderProgramObject_cube = 0;
        }
    }

    private int initialize_sphere() {
        final String vertexShaderSourceCode = 
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
            "} \n";

        int vertexShaderObject = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject, vertexShaderSourceCode);
        GLES32.glCompileShader(vertexShaderObject);
        checkShaderError(vertexShaderObject, "vertex sphere");

        final String framgmentShaderSourceCode = 
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
            "}\n";

        int framgmentShaderObject = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject, framgmentShaderSourceCode);
        GLES32.glCompileShader(framgmentShaderObject);
        checkShaderError(framgmentShaderObject, "fragment sphere");

        shaderProgramObject_sphere = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject_sphere, vertexShaderObject);
        GLES32.glAttachShader(shaderProgramObject_sphere, framgmentShaderObject);

        GLES32.glBindAttribLocation(shaderProgramObject_sphere, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glBindAttribLocation(shaderProgramObject_sphere, MyAttributes.AMC_ATTRIBUTE_NORMAL, "aNormal");
        GLES32.glLinkProgram(shaderProgramObject_sphere);
        checkProgramError(shaderProgramObject_sphere);

        modelMatrixUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uModelMatrix");
        viewMatrixUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uViewMatrix");
        projectionMatrixUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uProjectionMatrix");
        LaUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uLa");
        LdUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uLd");
        LsUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uLs");
        lightPositionUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uLightPosition");
        KaUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uKa");
        KdUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uKd");
        KsUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uKs");
        materialShininessUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uMaterialShininess");
        SingleTapPressUniform_sphere = GLES32.glGetUniformLocation(shaderProgramObject_sphere, "uSingleTapPress");


        Sphere sphere = new Sphere();
        float sphere_vertices[] = new float[1146];
        float sphere_normals[] = new float[1146];
        float sphere_textures[] = new float[764];
        short sphere_elements[] = new short[2280];
        sphere.getSphereVertexData(sphere_vertices, sphere_normals, sphere_textures, sphere_elements);
        numVertices = sphere.getNumberOfSphereVertices();
        numElements = sphere.getNumberOfSphereElements();

        GLES32.glGenVertexArrays(1, vao_sphere, 0);
        GLES32.glBindVertexArray(vao_sphere[0]);
        
        // Position
        GLES32.glGenBuffers(1, vbo_sphere_position, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vbo_sphere_position[0]);
        ByteBuffer bb = ByteBuffer.allocateDirect(sphere_vertices.length * 4);
        bb.order(ByteOrder.nativeOrder());
        FloatBuffer fb = bb.asFloatBuffer();
        fb.put(sphere_vertices);
        fb.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, sphere_vertices.length * 4, fb, GLES32.GL_STATIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);
        
        // Normal
        GLES32.glGenBuffers(1, vbo_sphere_normal, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vbo_sphere_normal[0]);
        bb = ByteBuffer.allocateDirect(sphere_normals.length * 4);
        bb.order(ByteOrder.nativeOrder());
        fb = bb.asFloatBuffer();
        fb.put(sphere_normals);
        fb.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, sphere_normals.length * 4, fb, GLES32.GL_STATIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_NORMAL, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_NORMAL);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);
        
        // Element
        GLES32.glGenBuffers(1, vbo_sphere_element, 0);
        GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, vbo_sphere_element[0]);
        bb = ByteBuffer.allocateDirect(sphere_elements.length * 2);
        bb.order(ByteOrder.nativeOrder());
        ShortBuffer sb = bb.asShortBuffer();
        sb.put(sphere_elements);
        sb.position(0);
        GLES32.glBufferData(GLES32.GL_ELEMENT_ARRAY_BUFFER, sphere_elements.length * 2, sb, GLES32.GL_STATIC_DRAW);

        GLES32.glBindVertexArray(0);

        // depth initialization
        GLES32.glClearDepthf(1.0f);
        GLES32.glEnable(GLES32.GL_DEPTH_TEST);
        GLES32.glDepthFunc(GLES32.GL_LEQUAL);
        GLES32.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        Matrix.setIdentityM(perspectiveProjectionMatrix_sphere, 0);

        return 0;
    }

    private boolean createAndPrepareFBOForDrawing(int texture_width, int texture_height) {
        int[] maxRenderBufferSize = new int[1];
        GLES32.glGetIntegerv(GLES32.GL_MAX_RENDERBUFFER_SIZE, maxRenderBufferSize, 0);
        if (maxRenderBufferSize[0] < texture_width || maxRenderBufferSize[0] < texture_height) {
            System.out.println("lrc: fbo width/height exceeding max size");
            return false;
        }

        GLES32.glGenFramebuffers(1, fbo, 0);
        GLES32.glBindFramebuffer(GLES32.GL_FRAMEBUFFER, fbo[0]);
        
        GLES32.glGenRenderbuffers(1, rbo, 0);
        GLES32.glBindRenderbuffer(GLES32.GL_RENDERBUFFER, rbo[0]);
        GLES32.glRenderbufferStorage(GLES32.GL_RENDERBUFFER, GLES32.GL_DEPTH_COMPONENT16, texture_width, texture_height);
        
        GLES32.glGenTextures(1, fbo_texture, 0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, fbo_texture[0]);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_WRAP_S, GLES32.GL_CLAMP_TO_EDGE);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_WRAP_T, GLES32.GL_CLAMP_TO_EDGE);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MAG_FILTER, GLES32.GL_LINEAR);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MIN_FILTER, GLES32.GL_LINEAR);
        
        GLES32.glTexImage2D(GLES32.GL_TEXTURE_2D, 0, GLES32.GL_RGB, texture_width, texture_height, 0, GLES32.GL_RGB, GLES32.GL_UNSIGNED_SHORT_5_6_5, null);
        GLES32.glFramebufferTexture2D(GLES32.GL_FRAMEBUFFER, GLES32.GL_COLOR_ATTACHMENT0, GLES32.GL_TEXTURE_2D, fbo_texture[0], 0);
        GLES32.glFramebufferRenderbuffer(GLES32.GL_FRAMEBUFFER, GLES32.GL_DEPTH_ATTACHMENT, GLES32.GL_RENDERBUFFER, rbo[0]);

        if (GLES32.glCheckFramebufferStatus(GLES32.GL_FRAMEBUFFER) != GLES32.GL_FRAMEBUFFER_COMPLETE) {
            System.out.println("lrc: fbo completion incomplete");
            return false;
        }

        GLES32.glBindFramebuffer(GLES32.GL_FRAMEBUFFER, 0);

        return true;
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
