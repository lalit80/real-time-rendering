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

public class GLESView extends GLSurfaceView implements OnGestureListener, OnDoubleTapListener, GLSurfaceView.Renderer {
    private Context context;
    private GestureDetector gestureDetector;
    private int shaderProgramObject;
    private int mvpMatrixUniform;
    private int vao_triangle[] = new int[1];
    private int vao_rectangle[] = new int[1];
    private int rectangle_vbo_position[] = new int[1];
    private int rectangle_vbo_texcoord[] = new int[1];
    private int texCheckerBoard[] = new int[1];
    private int textureSamplerUniform;
    private float perspectiveProjectionMatrix[] = new float[16];
    private final int CHECKERBOARD_HEIGHT = 64;
    private final int CHECKERBOARD_WIDTH = 64;
    byte checkImage[] = new byte[CHECKERBOARD_HEIGHT * CHECKERBOARD_WIDTH * 4];

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
    @Override public void onLongPress(MotionEvent e) {}
    @Override public void onShowPress(MotionEvent e) {}
    @Override public boolean onSingleTapUp(MotionEvent e) { return true; }
    @Override public boolean onDoubleTap(MotionEvent e) { return true; }
    @Override public boolean onDoubleTapEvent(MotionEvent e) { return true; }
    @Override public boolean onSingleTapConfirmed(MotionEvent e) { return true; }
    @Override public boolean onDown(MotionEvent e) { return true; }
    @Override public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) { return true; }

    // our custom opengl-es methods
    private int inititalize(GL10 gl) {
        printGLESInfo(gl);

        // shader
        final String vertexShaderSourceCode = String.format(
            "#version 320 es \n"+
            "in vec4 aPosition; \n"+
            "in vec2 aTexCoord; \n"+
            "out vec2 out_texcoord; \n"+
            "uniform mat4 uMVPMatrix; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   gl_Position = uMVPMatrix * aPosition; \n"+
            "   out_texcoord = aTexCoord; \n"+
            "} \n"
        );
        int vertexShaderObject = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject, vertexShaderSourceCode);
        GLES32.glCompileShader(vertexShaderObject);
        checkShaderError(vertexShaderObject, "vertex");

        final String framgmentShaderSourceCode = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "in vec2 out_texcoord; \n"+
            "uniform highp sampler2D uTextureSampler; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
            "   FragColor = texture(uTextureSampler, out_texcoord); \n"+
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
        GLES32.glBindAttribLocation(shaderProgramObject, MyAttributes.AMC_ATTRIBUTE_TEXTCORD, "aTexCoord");
        GLES32.glLinkProgram(shaderProgramObject);
        checkProgramError(shaderProgramObject);

        // get the required uniform location from the shader
        mvpMatrixUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uMVPMatrix");
        textureSamplerUniform = GLES32.glGetUniformLocation(shaderProgramObject, "uTextureSampler");

        final float rectangle_position[] = new float[] {
            // front
            1.0f,  1.0f,  1.0f, // top-right of front
            -1.0f,  1.0f,  1.0f, // top-left of front
            -1.0f, -1.0f,  1.0f, // bottom-left of front
            1.0f, -1.0f,  1.0f, // bottom-right of front
        };

        final float rectangle_texcoord[] = new float[] {   
            // front
            1.0f, 1.0f, // top-right of front
            0.0f, 1.0f, // top-left of front
            0.0f, 0.0f, // bottom-left of front
            1.0f, 0.0f, // bottom-right of front
        };

        // vertex array object for arrays of vertex attributes
        GLES32.glGenVertexArrays(1, vao_rectangle, 0);
        GLES32.glBindVertexArray(vao_rectangle[0]);

        // position
        GLES32.glGenBuffers(1, rectangle_vbo_position, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, rectangle_vbo_position[0]);
        /*byteBuffer = ByteBuffer.allocateDirect(rectangle_position.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer rectanglePositionBuffer = byteBuffer.asFloatBuffer();
        rectanglePositionBuffer.put(rectangle_position);
        rectanglePositionBuffer.position(0);*/
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, (4 * 3 * 4), null, GLES32.GL_DYNAMIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        // texture
        GLES32.glGenBuffers(1, rectangle_vbo_texcoord, 0);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, rectangle_vbo_texcoord[0]);
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(rectangle_texcoord.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer rectangleColorBuffer = byteBuffer.asFloatBuffer();
        rectangleColorBuffer.put(rectangle_texcoord);
        rectangleColorBuffer.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, (rectangle_texcoord.length * 4), rectangleColorBuffer, GLES32.GL_STATIC_DRAW);
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

        texCheckerBoard[0] = loadGLTexture();

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        return 0;
    }

    private void display() {
        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);
        GLES32.glUseProgram(shaderProgramObject);

        float modelViewMatrix[] = new float[16];
        float translationMatrix[] = new float[16];
        float modelViewProjectionMatrix[] = new float[16];
        float rotationMatrix[] = new float[16];
        Matrix.setIdentityM(modelViewMatrix, 0);
        Matrix.setIdentityM(translationMatrix, 0);
        Matrix.setIdentityM(rotationMatrix, 0);
        Matrix.setIdentityM(modelViewProjectionMatrix, 0);
        Matrix.translateM(translationMatrix, 0, 0.0f, 0.0f, -5.0f);
        
        modelViewMatrix = translationMatrix;
        Matrix.multiplyMM(modelViewProjectionMatrix, 0, perspectiveProjectionMatrix, 0, modelViewMatrix, 0);

        // send this matrix to vertex shader in uniform
        GLES32.glUniformMatrix4fv(mvpMatrixUniform, 1, false, modelViewProjectionMatrix, 0);
            
        GLES32.glActiveTexture(GLES32.GL_TEXTURE0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, texCheckerBoard[0]);
        GLES32.glUniform1i(textureSamplerUniform, 0);
        
        GLES32.glBindVertexArray(vao_rectangle[0]);

        float rectangle_position[] = new float[12];
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

        // position
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, rectangle_vbo_position[0]);
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(rectangle_position.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        FloatBuffer rectanglePositionBuffer = byteBuffer.asFloatBuffer();
        rectanglePositionBuffer.put(rectangle_position);
        rectanglePositionBuffer.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, (4 * 3 * 4), rectanglePositionBuffer, GLES32.GL_DYNAMIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        // draw the vertex arrays
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 0, 4);

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

        // position
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, rectangle_vbo_position[0]);
        byteBuffer = ByteBuffer.allocateDirect(rectangle_position.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        rectanglePositionBuffer = byteBuffer.asFloatBuffer();
        rectanglePositionBuffer.put(rectangle_position);
        rectanglePositionBuffer.position(0);
        GLES32.glBufferData(GLES32.GL_ARRAY_BUFFER, (4 * 3 * 4), rectanglePositionBuffer, GLES32.GL_DYNAMIC_DRAW);
        GLES32.glVertexAttribPointer(MyAttributes.AMC_ATTRIBUTE_POSITION, 3, GLES32.GL_FLOAT, false, 0, 0);
        GLES32.glEnableVertexAttribArray(MyAttributes.AMC_ATTRIBUTE_POSITION);
        GLES32.glBindBuffer(GLES32.GL_ARRAY_BUFFER, 0);

        // draw the vertex arrays
        GLES32.glDrawArrays(GLES32.GL_TRIANGLE_FAN, 0, 4);

        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, 0);

        // unbind with vao_triangle
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

    private int loadGLTexture() {
        makeCheckImage();
        ByteBuffer bytebuffer = ByteBuffer.allocateDirect(CHECKERBOARD_HEIGHT * CHECKERBOARD_WIDTH * 4);
        bytebuffer.order(ByteOrder.nativeOrder());
        bytebuffer.put(checkImage);
        bytebuffer.position(0);
        Bitmap bitmap = Bitmap.createBitmap(CHECKERBOARD_WIDTH, CHECKERBOARD_HEIGHT, Bitmap.Config.ARGB_8888);
        bitmap.copyPixelsFromBuffer(bytebuffer);

        int tex[] = new int[1];
        GLES32.glGenTextures(1, tex, 0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, tex[0]);
        GLES32.glPixelStorei(GLES32.GL_UNPACK_ALIGNMENT, 4);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MAG_FILTER, GLES32.GL_NEAREST);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_MIN_FILTER, GLES32.GL_NEAREST);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_WRAP_S, GLES32.GL_REPEAT);
        GLES32.glTexParameteri(GLES32.GL_TEXTURE_2D, GLES32.GL_TEXTURE_WRAP_T, GLES32.GL_REPEAT);
        GLUtils.texImage2D(GLES32.GL_TEXTURE_2D, 0, bitmap, 0);
        GLES32.glBindTexture(GLES32.GL_TEXTURE_2D, 0);
        
        return tex[0];
    }

    private void makeCheckImage() {
        int i, j, c;
        for (i = 0; i < CHECKERBOARD_HEIGHT; ++i) {
            for (j = 0; j < CHECKERBOARD_WIDTH; ++j) {
                c = ((i&8)^(j&8)) * 255;
                checkImage[(i*64 + j) * 4 + 0] = (byte)c;
                checkImage[(i*64 + j) * 4 + 1] = (byte)c;
                checkImage[(i*64 + j) * 4 + 2] = (byte)c;
                checkImage[(i*64 + j) * 4 + 3] = (byte)0xff;
            }
        }
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
