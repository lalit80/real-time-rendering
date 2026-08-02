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
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class GLESView extends GLSurfaceView implements OnGestureListener, OnDoubleTapListener, GLSurfaceView.Renderer {
    private Context context;
    private GestureDetector gestureDetector;
    private int shaderProgramObject;
    private int mvpMatrixUniform;
    private int vao[] = new int[1];
    private int vbo_position[] = new int[1];
    private int vbo_color[] = new int[1];
    private float perspectiveProjectionMatrix[] = new float[16];

    // tesselation related variable
    private int noOfLineSegmentsUniform;
    private int noOfLineStripsUniform;
    private int lineColorUniform;
    private int uiNoOfLineSegments = 1;
    private float lineColor[] = new float[4];
    FloatBuffer lineColorBuffer;

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
        uiNoOfLineSegments++;
        if (uiNoOfLineSegments > 20)
            uiNoOfLineSegments = 20;
        return true;
    }
    @Override public boolean onDoubleTap(MotionEvent e) {
        uiNoOfLineSegments--;
        if (uiNoOfLineSegments <= 0)
            uiNoOfLineSegments = 1;
        return true;
    }
    @Override public void onLongPress(MotionEvent e) {}
    @Override public void onShowPress(MotionEvent e) {}
    @Override public boolean onSingleTapUp(MotionEvent e) { return true; }
    @Override public boolean onDoubleTapEvent(MotionEvent e) { return true; }
    
    @Override public boolean onDown(MotionEvent e) { return true; }
    @Override public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) { return true; }

    // our custom opengl-es methods
    private int inititalize(GL10 gl) {
        printGLESInfo(gl);

        // shader
        final String vertexShaderSourceCode = String.format(
            "#version 320 es \n"+
            "precision highp int;"+
            "precision highp float;"+
            "in vec2 aPosition; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   gl_Position = vec4(aPosition, 0.0f, 1.0f); \n"+
            "} \n"
        );
        int vertexShaderObject = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject, vertexShaderSourceCode);
        GLES32.glCompileShader(vertexShaderObject);
        checkShaderError(vertexShaderObject, "vertex");

        final String tesselationControlShaderSourceCode = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "precision highp int;"+
            "layout(vertices=4) out; \n"+
            "uniform int uNoOfLineSegments; \n"+
            "uniform int uNoOfLineStrips; \n"+
            "void main(void)\n"+
            "{\n"+
            "   gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n"+
            "   gl_TessLevelOuter[0] = float(uNoOfLineStrips); \n"+
            "   gl_TessLevelOuter[1] = float(uNoOfLineSegments); \n"+
            "}\n"
        );

        int tesselationControlShaderObject = GLES32.glCreateShader(GLES32.GL_TESS_CONTROL_SHADER);
        GLES32.glShaderSource(tesselationControlShaderObject, tesselationControlShaderSourceCode);
        GLES32.glCompileShader(tesselationControlShaderObject);
        checkShaderError(tesselationControlShaderObject, "tesselation control");

        final String tesselationEvaluationShaderSourceCode = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "precision highp int;"+
            "layout(isolines) in; \n"+
            "uniform mat4 uMVPMatrix; \n"+
            "uniform int uNoOfLineSegments; \n"+
            "uniform int uNoOfLineStrips; \n"+
            "void main(void)\n"+
            "{\n"+
            "   vec3 p0 = gl_in[0].gl_Position.xyz; \n"+
            "   vec3 p1 = gl_in[1].gl_Position.xyz; \n"+
            "   vec3 p2 = gl_in[2].gl_Position.xyz; \n"+
            "   vec3 p3 = gl_in[3].gl_Position.xyz; \n"+
            "   vec3 p = p0 * ((1.0f - gl_TessCoord.x) * (1.0f - gl_TessCoord.x) * (1.0f - gl_TessCoord.x)) + (p1 * 3.0f * gl_TessCoord.x * (1.0f - gl_TessCoord.x) * (1.0f - gl_TessCoord.x)) + (p2 * 3.0f * gl_TessCoord.x * gl_TessCoord.x * (1.0f - gl_TessCoord.x)) + (p3 * gl_TessCoord.x * gl_TessCoord.x * gl_TessCoord.x); \n"+
            "   gl_Position = uMVPMatrix * vec4(p, 1.0f); \n"+
            "}\n"
        );

        int tesselationEvaluationShaderObject = GLES32.glCreateShader(GLES32.GL_TESS_EVALUATION_SHADER);
        GLES32.glShaderSource(tesselationEvaluationShaderObject, tesselationEvaluationShaderSourceCode);
        GLES32.glCompileShader(tesselationEvaluationShaderObject);
        checkShaderError(tesselationEvaluationShaderObject, "tesselation evaluation");

        final String framgmentShaderSourceCode = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "precision highp int;"+
            "uniform vec4 uLineColor; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
            "   FragColor = uLineColor; \n"+
            "}\n"
        );

        int framgmentShaderObject = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject, framgmentShaderSourceCode);
        GLES32.glCompileShader(framgmentShaderObject);
        checkShaderError(framgmentShaderObject, "fragment");

        shaderProgramObject = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject, vertexShaderObject);
        GLES32.glAttachShader(shaderProgramObject, tesselationControlShaderObject);
        GLES32.glAttachShader(shaderProgramObject, tesselationEvaluationShaderObject);
        GLES32.glAttachShader(shaderProgramObject, framgmentShaderObject);

        GLES32.glBindAttribLocation(shaderProgramObject, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glLinkProgram(shaderProgramObject);
        checkProgramError(shaderProgramObject);

        // get the required uniform location from the shader
        mvpMatrixUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uMVPMatrix");
        noOfLineSegmentsUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uNoOfLineSegments");
        noOfLineStripsUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uNoOfLineStrips");
        lineColorUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uLineColor");

        final float line_position[] = new float[] {
                                                    -1.0f, -1.0f,
                                                    -0.5f, 1.0f,
                                                    0.5f, -1.0f,
                                                    1.0f, 1.0f, 
                                            };

        // vertex array object for arrays of vertex attributes
        GLES32.glGenVertexArrays(1, vao, 0);
        GLES32.glBindVertexArray(vao[0]);

        // position
        GLES32.glGenBuffers(1, vbo_position, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, vbo_position[0]);
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(line_position.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer linePositionBuffer = byteBuffer.asFloatBuffer();
        linePositionBuffer.put(line_position);
        linePositionBuffer.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, (line_position.length * 4), linePositionBuffer, GLES32.GL_STATIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 2, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        GLES32.glBindVertexArray(0);

        // depth initialization
        GLES32.glClearDepthf(1.0f);
        GLES32.glEnable(GLES32.GL_DEPTH_TEST);
        GLES32.glDepthFunc(GLES32.GL_LEQUAL);
        
        // set the clear color
        GLES32.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        lineColor[0] = 1.0f;
        lineColor[1] = 1.0f;
        lineColor[2] = 0.0f;
        lineColor[3] = 1.0f;
        byteBuffer = ByteBuffer.allocateDirect(lineColor.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        lineColorBuffer = byteBuffer.asFloatBuffer();
        lineColorBuffer.put(lineColor);
        lineColorBuffer.position(0);

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        return 0;
    }

    private void display() {
        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);
        GLES32.glUseProgram(shaderProgramObject);

        float modelViewMatrix[] = new float[16];
        float translationMatrix[] = new float[16];
        float modelViewProjectionMatrix[] = new float[16];

        Matrix.setIdentityM(modelViewMatrix, 0);
        Matrix.setIdentityM(translationMatrix, 0);
        Matrix.setIdentityM(modelViewProjectionMatrix, 0);
        Matrix.translateM(translationMatrix, 0, 0.0f, 0.0f, -4.0f);
        
        modelViewMatrix = translationMatrix;
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, perspectiveProjectionMatrix, 0, modelViewMatrix, 0);

        // send this matrix to vertex shader in uniform
        GLES32.glUniformMatrix4fv(mvpMatrixUniform, 1, false, modelViewProjectionMatrix, 0);
        GLES32.glUniform1i(noOfLineSegmentsUniform, uiNoOfLineSegments);
        GLES32.glUniform1i(noOfLineStripsUniform, 1);
        GLES32.glUniform4fv(lineColorUniform, 1, lineColor, 0);

        // bind with vao
        GLES32.glBindVertexArray(vao[0]);

        // decide how many vertices for one batch
        GLES32.glPatchParameteri(GLES32.GL_PATCH_VERTICES, 4);

        // draw the vertex arrays
        GLES32.glDrawArrays(GLES32.GL_PATCHES, 0, 4);

        // unbind with vao
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
        if (vbo_color[0] > 0) {
            GLES32.glDeleteBuffers(1, vbo_color, 0);
            vbo_color[0] = 0;
        }
        if (vbo_position[0] > 0) {
            GLES32.glDeleteBuffers(1, vbo_position, 0);
            vbo_position[0] = 0;
        }
        if (vao[0] > 0) {
            GLES32.glDeleteVertexArrays(1, vao, 0);
            vao[0] = 0;
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
