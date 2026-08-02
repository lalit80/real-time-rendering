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
    private int shaderProgramObject_PF;
    private int shaderProgramObject_PV;
    
    // uniforms
    private int modelMatrixUniform_PV = 0;
    private int viewMatrixUniform_PV = 0;
    private int projectionMatrixUniform_PV = 0;
    private int LaUniform_PV[] = new int[1];
    private int LdUniform_PV[] = new int[1];
    private int LsUniform_PV[] = new int[1];
    private int KaUniform_PV = 0;
    private int KdUniform_PV = 0;
    private int KsUniform_PV = 0;
    private int materialShininessUniform_PV = 0;
    private int lightPositionUniform_PV[] = new int[1];
    private int SingleTapPressUniform_PV = 0;

    private int modelMatrixUniform_PF = 0;
    private int viewMatrixUniform_PF = 0;
    private int projectionMatrixUniform_PF = 0;
    private int LaUniform_PF[] = new int[1];
    private int LdUniform_PF[] = new int[1];
    private int LsUniform_PF[] = new int[1];
    private int KaUniform_PF = 0;                   
    private int KdUniform_PF = 0;
    private int KsUniform_PF = 0;
    private int materialShininessUniform_PF = 0;
    private int lightPositionUniform_PF[] = new int[1];
    private int SingleTapPressUniform_PF = 0;

    private int[] vao_sphere = new int[1];
    private int[] vbo_sphere_position = new int[1];
    private int[] vbo_sphere_normal = new int[1];
    private int[] vbo_sphere_element = new int[1];
    private int numVertices;
    private int numElements;
    private float perspectiveProjectionMatrix[] = new float[16];

    private boolean bLight;    
    private boolean bPerFragment;
    private boolean bPerVertex;
    private int singleTapCount = 0;
    private Light light[] = new Light[1];
    private float lightAngle = 0.0f;
    private Material material[] = new Material[24];

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
        singleTapCount++;
        if (singleTapCount > 2) singleTapCount = 0;
        return true;
    }
    @Override public boolean onDoubleTap(MotionEvent e) { 
        if (bPerFragment == true) {
            bPerFragment = false;
            bPerVertex = true;
        }
        else {
            bPerFragment = true;
            bPerVertex = false;
        }
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

        // shader per fragment light
        final String vertexShaderSourceCode_PF = String.format(
            "#version 320 es \n"+
            "precision highp float;"+
            "precision highp int; \n"+
            "in vec4 aPosition; \n"+
            "in vec3 aNormal; \n"+
            "out vec3 out_transformedNormals; \n"+
            "out vec3 out_lightDirection[1]; \n"+
            "out vec3 out_viewerVector; \n"+
            "uniform mat4 uModelMatrix; \n"+
            "uniform mat4 uViewMatrix; \n"+
            "uniform mat4 uProjectionMatrix; \n"+
            "uniform vec4 uLightPosition[1]; \n"+
            "uniform int uSingleTapPress; \n"+
            "void main(void) { \n"+
                "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
                "if (uSingleTapPress == 1) { \n"+
                    "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                    "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                    "out_transformedNormals = normalize(normalMatrix * aNormal); \n"+
                    "out_viewerVector = normalize(-eyeCoordinates.xyz); \n"+
                    "for (int i = 0; i < 1; ++i) { \n"+
                        "out_lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n"+
                    "} \n"+
                "} \n"+
            "} \n"
        );
        int vertexShaderObject_PF = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject_PF, vertexShaderSourceCode_PF);
        GLES32.glCompileShader(vertexShaderObject_PF);
        checkShaderError(vertexShaderObject_PF, "vertex");

        final String framgmentShaderSourceCode_PF = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "precision highp int; \n"+
            "in vec3 out_transformedNormals; \n"+
            "in vec3 out_lightDirection[1]; \n"+
            "in vec3 out_viewerVector; \n"+
            "out vec4 FragColor; \n"+
            "uniform vec3 uLa[1]; \n"+
            "uniform vec3 uLd[1]; \n"+
            "uniform vec3 uLs[1]; \n"+
            "uniform vec4 uLightPosition[1]; \n"+
            "uniform vec3 uKa; \n"+
            "uniform vec3 uKd; \n"+
            "uniform vec3 uKs; \n"+
            "uniform float uMaterialSininess; \n"+
            "uniform int uSingleTapPress; \n"+
            "void main(void) { \n"+
                "vec3 phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n"+
                "if (uSingleTapPress == 1) { \n"+
                    "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
                    "vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
                    "vec3 normalizedLightDirection[1]; \n"+
                    "vec3 lightDirection[1]; \n"+
                    "vec3 ambientLight[1]; \n"+
                    "vec3 diffuseLight[1]; \n"+
                    "vec3 reflectionVector[1]; \n"+
                    "vec3 specularLight[1]; \n"+
                    "for (int i = 0; i < 1; ++i) { \n"+
                        "normalizedLightDirection[i] = normalize(out_lightDirection[i]); \n"+
                        "ambientLight[i] = uLa[i] * uKa * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n"+
                        "diffuseLight[i] = uLd[i] * uKd * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n"+
                        "reflectionVector[i] = reflect(-normalizedLightDirection[i], normalizedTransformedNormals); \n"+
                        "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], normalizedViewerVector), 0.0f), uMaterialSininess); \n"+
                        "phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n"+
                    "} \n"+
                "} \n"+
                "else { \n"+
                    "phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n"+
                "} \n"+
                "FragColor = vec4(phong_ads_light, 1.0f); \n"+
            "}\n"
        );

        int framgmentShaderObject_PF = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject_PF, framgmentShaderSourceCode_PF);
        GLES32.glCompileShader(framgmentShaderObject_PF);
        checkShaderError(framgmentShaderObject_PF, "fragment");

        shaderProgramObject_PF = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject_PF, vertexShaderObject_PF);
        GLES32.glAttachShader(shaderProgramObject_PF, framgmentShaderObject_PF);

        GLES32.glBindAttribLocation(shaderProgramObject_PF, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glBindAttribLocation(shaderProgramObject_PF, MyAttributes.AMC_ATTRIBUTE_NORMAL, "aNormal");
        GLES32.glLinkProgram(shaderProgramObject_PF);
        checkProgramError(shaderProgramObject_PF);

        // get the required uniform location from the shader
        modelMatrixUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uModelMatrix");
        viewMatrixUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uViewMatrix");
        projectionMatrixUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uProjectionMatrix");
        KaUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uKa");
        KdUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uKd");
        KsUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uKs");
        materialShininessUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uMaterialSininess");
        SingleTapPressUniform_PF = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uSingleTapPress");
        LaUniform_PF[0] = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uLa[0]");
        LdUniform_PF[0] = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uLd[0]");
        LsUniform_PF[0] = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uLs[0]");
        lightPositionUniform_PF[0] = GLES32.glGetUniformLocation(shaderProgramObject_PF, "uLightPosition[0]");


        // shader per vertex light
        final String vertexShaderSourceCode_PV = String.format(
            "#version 320 es \n"+
            "in vec4 aPosition; \n"+
            "in vec3 aNormal; \n"+
            "out vec3 out_phong_ads_light; \n"+
            "uniform mat4 uModelMatrix; \n"+
            "uniform mat4 uViewMatrix; \n"+
            "uniform mat4 uProjectionMatrix; \n"+
            "uniform vec3 uLa[1]; \n"+
            "uniform vec3 uLd[1]; \n"+
            "uniform vec3 uLs[1]; \n"+
            "uniform vec4 uLightPosition[1]; \n"+
            "uniform vec3 uKa; \n"+
            "uniform vec3 uKd; \n"+
            "uniform vec3 uKs; \n"+
            "uniform float uMaterialSininess; \n"+
            "uniform int uSingleTapPress; \n"+
            "void main(void) \n"+
            "{ \n"+
                "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
                "if (uSingleTapPress == 1) { \n"+
                    "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                    "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                    "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n"+
                    "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n"+
                    "vec3 lightDirection[1]; \n"+
                    "vec3 ambientLight[1]; \n"+
                    "vec3 diffuseLight[1]; \n"+
                    "vec3 reflectionVector[1]; \n"+
                    "vec3 specularLight[1]; \n"+
                    "out_phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n"+
                    "for (int i = 0; i < 1; ++i) { \n"+
                        "lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n"+
                        "ambientLight[i] = uLa[i] * uKa * max(dot(lightDirection[i], transformedNormal), 0.0f); \n"+
                        "diffuseLight[i] = uLd[i] * uKd * max(dot(lightDirection[i], transformedNormal), 0.0f); \n"+
                        "reflectionVector[i] = reflect(-lightDirection[i], transformedNormal); \n"+
                        "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], viewerVector), 0.0f), uMaterialSininess); \n"+
                        "out_phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n"+
                    "} \n"+
                "} \n"+
                "else { \n"+
                    "out_phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n"+
                "} \n"+
            "} \n"
        );
        int vertexShaderObject_PV = GLES32.glCreateShader(GLES32.GL_VERTEX_SHADER);
        GLES32.glShaderSource(vertexShaderObject_PV, vertexShaderSourceCode_PV);
        GLES32.glCompileShader(vertexShaderObject_PV);
        checkShaderError(vertexShaderObject_PV, "vertex");

        final String framgmentShaderSourceCode_PV = String.format(
            "#version 320 es\n"+
            "precision highp float;"+
            "in vec3 out_phong_ads_light; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
                "FragColor = vec4(out_phong_ads_light, 1.0f); \n"+
            "}\n"
        );

        int framgmentShaderObject_PV = GLES32.glCreateShader(GLES32.GL_FRAGMENT_SHADER);
        GLES32.glShaderSource(framgmentShaderObject_PV, framgmentShaderSourceCode_PV);
        GLES32.glCompileShader(framgmentShaderObject_PV);
        checkShaderError(framgmentShaderObject_PV, "fragment");

        shaderProgramObject_PV = GLES32.glCreateProgram();
        GLES32.glAttachShader(shaderProgramObject_PV, vertexShaderObject_PV);
        GLES32.glAttachShader(shaderProgramObject_PV, framgmentShaderObject_PV);

        GLES32.glBindAttribLocation(shaderProgramObject_PV, MyAttributes.AMC_ATTRIBUTE_POSITION, "aPosition");
        GLES32.glBindAttribLocation(shaderProgramObject_PV, MyAttributes.AMC_ATTRIBUTE_NORMAL, "aNormal");
        GLES32.glLinkProgram(shaderProgramObject_PV);
        checkProgramError(shaderProgramObject_PV);

        // get the required uniform location from the shader
        modelMatrixUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uModelMatrix");
        viewMatrixUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uViewMatrix");
        projectionMatrixUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uProjectionMatrix");
        KaUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uKa");
        KdUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uKd");
        KsUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uKs");
        materialShininessUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uMaterialSininess");
        SingleTapPressUniform_PV = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uSingleTapPress");
        LaUniform_PV[0] = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uLa[0]");
        LdUniform_PV[0] = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uLd[0]");
        LsUniform_PV[0] = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uLs[0]");
        lightPositionUniform_PV[0] = GLES32.glGetUniformLocation(shaderProgramObject_PV, "uLightPosition[0]");


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
        bLight = true;
        bPerVertex = false;
        bPerFragment = true;
        light[0] = new Light();
        for (int i = 0; i < 24; ++i) material[i] = new Material();

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

        light[0].position[0] = 0.0f;
        light[0].position[1] = 0.0f;
        light[0].position[2] = 0.0f;
        light[0].position[3] = 1.0f;

        fillMaterialProperties();

        lightAngle = 0.0f;

        Matrix.setIdentityM(perspectiveProjectionMatrix, 0);

        return 0;
    }

    private void display() {
        GLES32.glClear(GLES32.GL_COLOR_BUFFER_BIT | GLES32.GL_DEPTH_BUFFER_BIT);

        if (bPerFragment == true) {
            GLES32.glUseProgram(shaderProgramObject_PF);

            float modelMatrix[] = new float[16];
            float viewMatrix[] = new float[16];
            float translationMatrix[] = new float[16];
            Matrix.setIdentityM(viewMatrix, 0);
            
            GLES32.glUniformMatrix4fv(viewMatrixUniform_PF, 1, false, viewMatrix, 0);
            GLES32.glUniformMatrix4fv(projectionMatrixUniform_PF, 1, false, perspectiveProjectionMatrix, 0);

            if (bLight == true) {
                GLES32.glUniform3fv(LaUniform_PF[0], 1, light[0].ambient, 0);
                GLES32.glUniform3fv(LdUniform_PF[0], 1, light[0].diffuse, 0);
                GLES32.glUniform3fv(LsUniform_PF[0], 1, light[0].specular, 0);
                GLES32.glUniform4fv(lightPositionUniform_PF[0], 1, light[0].position, 0);
                GLES32.glUniform1i(SingleTapPressUniform_PF, 1);
            } else {
                GLES32.glUniform1i(SingleTapPressUniform_PF, 0);
            }

                // bind vao
            GLES32.glBindVertexArray(vao_sphere[0]);
            
            // *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, vbo_sphere_element[0]);

            float xTranslate = -2.5f;
            float yTranslate = 0.0f;
            for (int i = 0, k = 0; i < 4; ++i) {
                yTranslate = 3.0f;
                for (int j = 0; j < 6; ++j) {
                    Matrix.setIdentityM(modelMatrix, 0);
                    Matrix.setIdentityM(translationMatrix, 0);
                    Matrix.translateM(translationMatrix, 0, xTranslate, yTranslate, -8.5f);
                    modelMatrix = translationMatrix;
                    GLES32.glUniformMatrix4fv(modelMatrixUniform_PF, 1, false, modelMatrix, 0);

                    GLES32.glUniform3fv(KaUniform_PF, 1, material[k].ambient, 0);
                    GLES32.glUniform3fv(KdUniform_PF, 1, material[k].diffuse, 0);
                    GLES32.glUniform3fv(KsUniform_PF, 1, material[k].specular, 0);
                    GLES32.glUniform1f(materialShininessUniform_PF, material[k].shininess);

                    GLES32.glDrawElements(GLES32.GL_TRIANGLES, numElements, GLES32.GL_UNSIGNED_SHORT, 0);
                    
                    ++k;
                    yTranslate -= 1.2f;
                }
                xTranslate += 1.5f;
            }    
            // unbind vao
            GLES32.glBindVertexArray(0);
            GLES32.glUseProgram(0);
        }
        else {
            GLES32.glUseProgram(shaderProgramObject_PV);

            float modelMatrix[] = new float[16];
            float viewMatrix[] = new float[16];
            float translationMatrix[] = new float[16];
            Matrix.setIdentityM(viewMatrix, 0);
            
            GLES32.glUniformMatrix4fv(viewMatrixUniform_PV, 1, false, viewMatrix, 0);
            GLES32.glUniformMatrix4fv(projectionMatrixUniform_PV, 1, false, perspectiveProjectionMatrix, 0);

            if (bLight == true) {
                GLES32.glUniform3fv(LaUniform_PV[0], 1, light[0].ambient, 0);
                GLES32.glUniform3fv(LdUniform_PV[0], 1, light[0].diffuse, 0);
                GLES32.glUniform3fv(LsUniform_PV[0], 1, light[0].specular, 0);
                GLES32.glUniform4fv(lightPositionUniform_PV[0], 1, light[0].position, 0);
                GLES32.glUniform1i(SingleTapPressUniform_PV, 1);
            } else {
                GLES32.glUniform1i(SingleTapPressUniform_PV, 0);
            }

            // bind vao
            GLES32.glBindVertexArray(vao_sphere[0]);
            
            // *** draw, either by glDrawTriangles() or glDrawArrays() or glDrawElements()
            GLES32.glBindBuffer(GLES32.GL_ELEMENT_ARRAY_BUFFER, vbo_sphere_element[0]);

            float xTranslate = -2.5f;
            float yTranslate = 0.0f;
            for (int i = 0, k = 0; i < 4; ++i) {
                yTranslate = 3.0f;
                for (int j = 0; j < 6; ++j) {
                    Matrix.setIdentityM(modelMatrix, 0);
                    Matrix.setIdentityM(translationMatrix, 0);
                    Matrix.translateM(translationMatrix, 0, xTranslate, yTranslate, -8.5f);
                    modelMatrix = translationMatrix;
                    GLES32.glUniformMatrix4fv(modelMatrixUniform_PV, 1, false, modelMatrix, 0);

                    GLES32.glUniform3fv(KaUniform_PV, 1, material[k].ambient, 0);
                    GLES32.glUniform3fv(KdUniform_PV, 1, material[k].diffuse, 0);
                    GLES32.glUniform3fv(KsUniform_PV, 1, material[k].specular, 0);
                    GLES32.glUniform1f(materialShininessUniform_PV, material[k].shininess);

                    GLES32.glDrawElements(GLES32.GL_TRIANGLES, numElements, GLES32.GL_UNSIGNED_SHORT, 0);
                    
                    ++k;
                    yTranslate -= 1.2f;
                }
                xTranslate += 1.5f;
            }
            // unbind vao
            GLES32.glBindVertexArray(0);
            GLES32.glUseProgram(0);
        }

        requestRender();        // swap buffer
    }

    private void update() {
        final float radius = 50.0f;

        lightAngle += 0.007f;

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        if (singleTapCount == 0) {
            // update position of light 0 (x-y plane)
            x = (float)(Math.cos(lightAngle) * radius);
            y = (float)(Math.sin(lightAngle) * radius);
            light[0].position[0] = x;
            light[0].position[1] = y;
        } else if (singleTapCount == 1) {
            // update position of light 1 (x-z plane)
            x = (float)(Math.cos(lightAngle) * radius);
            z = (float)(Math.sin(lightAngle) * radius);
            light[0].position[0] = x;
            light[0].position[2] = z;
        } else {
            // update position of light 2 (z-y plane)
            z = (float)(Math.cos(lightAngle) * radius);
            y = (float)(Math.sin(lightAngle) * radius);
            light[0].position[1] = y;
            light[0].position[2] = z;
        }
    }

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

    private void fillMaterialProperties() {
        // Material 0: Emerald
        material[0].ambient = new float[]{0.0215f, 0.1745f, 0.0215f, 1.0f};
        material[0].diffuse = new float[]{0.07568f, 0.61424f, 0.07568f, 1.0f};
        material[0].specular = new float[]{0.633f, 0.727811f, 0.633f, 1.0f};
        material[0].shininess = 0.6f * 128;

        // Material 1: Jade
        material[1].ambient = new float[]{0.135f, 0.2225f, 0.1575f, 1.0f};
        material[1].diffuse = new float[]{0.54f, 0.89f, 0.63f, 1.0f};
        material[1].specular = new float[]{0.316228f, 0.316228f, 0.316228f, 1.0f};
        material[1].shininess = 0.1f * 128;

        // Material 2: Obsidian
        material[2].ambient = new float[]{0.05375f, 0.05f, 0.06625f, 1.0f};
        material[2].diffuse = new float[]{0.18275f, 0.17f, 0.22525f, 1.0f};
        material[2].specular = new float[]{0.332741f, 0.328634f, 0.346435f, 1.0f};
        material[2].shininess = 0.3f * 128; 

        // Material 3: Pearl
        material[3].ambient = new float[]{0.25f, 0.20725f, 0.20725f, 1.0f};
        material[3].diffuse = new float[]{1.0f, 0.829f, 0.829f, 1.0f};
        material[3].specular = new float[]{0.296648f, 0.296648f, 0.296648f, 1.0f};
        material[3].shininess = 0.088f * 128; 

        // Material 4: Ruby
        material[4].ambient = new float[]{0.1745f, 0.01175f, 0.01175f, 1.0f};
        material[4].diffuse = new float[]{0.61424f, 0.04136f, 0.04136f, 1.0f};
        material[4].specular = new float[]{0.727811f, 0.626959f, 0.626959f, 1.0f};
        material[4].shininess = 0.6f * 128;

        // Material 5: Turquoise
        material[5].ambient = new float[]{0.1f, 0.18725f, 0.1745f, 1.0f};
        material[5].diffuse = new float[]{0.396f, 0.396f, 0.69102f, 1.0f};
        material[5].specular = new float[]{0.297254f, 0.30829f, 0.306678f, 1.0f};
        material[5].shininess = 0.1f * 128; 

        // Material 6: Gold
        material[6].ambient = new float[]{0.329412f, 0.223529f, 0.027451f, 1.0f};
        material[6].diffuse = new float[]{0.780392f, 0.568627f, 0.113725f, 1.0f};
        material[6].specular = new float[]{0.992157f, 0.941176f, 0.807843f, 1.0f};
        material[6].shininess = 0.21794872f * 128; 

        // Material 7: Bronze
        material[7].ambient = new float[]{0.2125f, 0.1275f, 0.054f, 1.0f};
        material[7].diffuse = new float[]{0.714f, 0.4284f, 0.18144f, 1.0f};
        material[7].specular = new float[]{0.393548f, 0.271906f, 0.166721f, 1.0f};
        material[7].shininess = 0.2f * 128; 

        // Material 8: Chrome
        material[8].ambient = new float[]{0.25f, 0.25f, 0.25f, 1.0f};
        material[8].diffuse = new float[]{0.4f, 0.4f, 0.4f, 1.0f};
        material[8].specular = new float[]{0.774597f, 0.774597f, 0.774597f, 1.0f};
        material[8].shininess = 0.6f * 128; 

        // Material 9: Copper
        material[9].ambient = new float[]{0.19125f, 0.0735f, 0.0225f, 1.0f};
        material[9].diffuse = new float[]{0.7038f, 0.27048f, 0.0828f, 1.0f};
        material[9].specular = new float[]{0.256777f, 0.137622f, 0.086014f, 1.0f};
        material[9].shininess = 0.1f * 128; 

        // Material 10: Silver
        material[10].ambient = new float[]{0.24725f, 0.1995f, 0.0745f, 1.0f};
        material[10].diffuse = new float[]{0.75164f, 0.60648f, 0.22648f, 1.0f};
        material[10].specular = new float[]{0.628281f, 0.555802f, 0.366065f, 1.0f};
        material[10].shininess = 0.4f * 128; 

        // Material 11: Polished Silver
        material[11].ambient = new float[]{0.19225f, 0.19225f, 0.19225f, 1.0f};
        material[11].diffuse = new float[]{0.50754f, 0.50754f, 0.50754f, 1.0f};
        material[11].specular = new float[]{0.508273f, 0.508273f, 0.508273f, 1.0f};
        material[11].shininess = 0.4f * 128; 

        // Material 12: Black Plastic
        material[12].ambient = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
        material[12].diffuse = new float[]{0.01f, 0.01f, 0.01f, 1.0f};
        material[12].specular = new float[]{0.50f, 0.50f, 0.50f, 1.0f};
        material[12].shininess = 0.25f * 128; 

        // Material 13: Cyan Plastic
        material[13].ambient = new float[]{0.0f, 0.1f, 0.06f, 1.0f};
        material[13].diffuse = new float[]{0.0f, 0.50980392f, 0.50980392f, 1.0f};
        material[13].specular = new float[]{0.50196078f, 0.50196078f, 0.50196078f, 1.0f};
        material[13].shininess = 0.25f * 128;

        // Material 14: Green Plastic
        material[14].ambient = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
        material[14].diffuse = new float[]{0.1f, 0.35f, 0.1f, 1.0f};
        material[14].specular = new float[]{0.45f, 0.55f, 0.45f, 1.0f};
        material[14].shininess = 0.25f * 128; 

        // Material 15: Red Plastic
        material[15].ambient = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
        material[15].diffuse = new float[]{0.5f, 0.0f, 0.0f, 1.0f};
        material[15].specular = new float[]{0.7f, 0.6f, 0.6f, 1.0f};
        material[15].shininess = 0.25f * 128; 

        // Material 16: White Plastic
        material[16].ambient = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
        material[16].diffuse = new float[]{0.55f, 0.55f, 0.55f, 1.0f};
        material[16].specular = new float[]{0.7f, 0.7f, 0.7f, 1.0f};
        material[16].shininess = 0.25f * 128; 

        // Material 17: Yellow Plastic
        material[17].ambient = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
        material[17].diffuse = new float[]{0.5f, 0.5f, 0.0f, 1.0f};
        material[17].specular = new float[]{0.6f, 0.6f, 0.5f, 1.0f};
        material[17].shininess = 0.25f * 128; 

        // Material 18: Black Rubber
        material[18].ambient = new float[]{0.02f, 0.02f, 0.02f, 1.0f};
        material[18].diffuse = new float[]{0.01f, 0.01f, 0.01f, 1.0f};
        material[18].specular = new float[]{0.40f, 0.40f, 0.40f, 1.0f};
        material[18].shininess = 0.078125f * 128;

        // Material 19: Cyan Rubber
        material[19].ambient = new float[]{0.0f, 0.05f, 0.05f, 1.0f};
        material[19].diffuse = new float[]{0.4f, 0.5f, 0.5f, 1.0f};
        material[19].specular = new float[]{0.04f, 0.7f, 0.7f, 1.0f};
        material[19].shininess = 0.078125f * 128;

        // Material 20: Green Rubber
        material[20].ambient = new float[]{0.0f, 0.05f, 0.0f, 1.0f};
        material[20].diffuse = new float[]{0.4f, 0.5f, 0.4f, 1.0f};
        material[20].specular = new float[]{0.04f, 0.7f, 0.04f, 1.0f};
        material[20].shininess = 0.078125f * 128; 

        // Material 21: Red Rubber
        material[21].ambient = new float[]{0.05f, 0.0f, 0.0f, 1.0f};
        material[21].diffuse = new float[]{0.5f, 0.4f, 0.4f, 1.0f};
        material[21].specular = new float[]{0.7f, 0.04f, 0.04f, 1.0f};
        material[21].shininess = 0.078125f * 128; 

        // Material 22: White Rubber
        material[22].ambient = new float[]{0.05f, 0.05f, 0.05f, 1.0f};
        material[22].diffuse = new float[]{0.5f, 0.5f, 0.5f, 1.0f};
        material[22].specular = new float[]{0.7f, 0.7f, 0.7f, 1.0f};
        material[22].shininess = 0.078125f * 128; 

        // Material 23: Yellow Rubber
        material[23].ambient = new float[]{0.05f, 0.05f, 0.0f, 1.0f};
        material[23].diffuse = new float[]{0.5f, 0.5f, 0.4f, 1.0f};
        material[23].specular = new float[]{0.7f, 0.7f, 0.04f, 1.0f};
        material[23].shininess = 0.078125f * 128;
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
        if (shaderProgramObject_PF > 0) {
            GLES32.glUseProgram(shaderProgramObject_PF);
            int retVal[] = new int[1];
            GLES32.glGetProgramiv(shaderProgramObject_PF, GLES32.GL_ATTACHED_SHADERS, retVal, 0);
            int numShaders = retVal[0];
            if (numShaders > 0) {
                int pShaders[] = new int[numShaders];
                GLES32.glGetAttachedShaders(shaderProgramObject_PF, numShaders, retVal, 0, pShaders, 0);
                for (int i = 0; i < numShaders; ++i) {
                    GLES32.glDetachShader(shaderProgramObject_PF, pShaders[i]);
                    GLES32.glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
            }
            GLES32.glUseProgram(0);
            GLES32.glDeleteProgram(shaderProgramObject_PF);
            shaderProgramObject_PF = 0;
        }

        if (shaderProgramObject_PV > 0) {
            GLES32.glUseProgram(shaderProgramObject_PV);
            int retVal[] = new int[1];
            GLES32.glGetProgramiv(shaderProgramObject_PV, GLES32.GL_ATTACHED_SHADERS, retVal, 0);
            int numShaders = retVal[0];
            if (numShaders > 0) {
                int pShaders[] = new int[numShaders];
                GLES32.glGetAttachedShaders(shaderProgramObject_PV, numShaders, retVal, 0, pShaders, 0);
                for (int i = 0; i < numShaders; ++i) {
                    GLES32.glDetachShader(shaderProgramObject_PV, pShaders[i]);
                    GLES32.glDeleteShader(pShaders[i]);
                    pShaders[i] = 0;
                }
            }
            GLES32.glUseProgram(0);
            GLES32.glDeleteProgram(shaderProgramObject_PV);
            shaderProgramObject_PV = 0;
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
    float shininess;
}
