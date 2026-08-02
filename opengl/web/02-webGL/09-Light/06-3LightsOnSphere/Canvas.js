var canvas = null;
var gl = null;
var bFullScreen = false;
var canvas_original_width = 0;
var canvas_original_height = 0;
var requestAnimationFrame = window.requestAnimationFrame || window.webkitRequestAnimationFrame || window.mozRequestAnimationFrame || window.oRequestAnimationFrame || window.msRequestAnimationFrame;

// webGL related variables
const MyAttributes = {
    LRC_ATTRIBUTE_POSITION: 0,
    LRC_ATTRIBUTE_COLOR: 1,
    LRC_ATTRIBUTE_NORMAL: 2,
    LRC_ATTRIBUTE_TEXCOORD: 3,
};

var shaderProgramObject_PF = null; 
var shaderProgramObject_PV = null; 

var vao = null;
var sphere = null;

// Uniforms for per fragment
var modelMatrixUniform_PF = null;
var viewMatrixUniform_PF = null;
var projectionMatrixUniform_PF = null;
var LaUniform_PF = [null, null, null];
var LdUniform_PF = [null, null, null];
var LsUniform_PF = [null, null, null];
var KaUniform_PF = null;
var KdUniform_PF = null;
var KsUniform_PF = null;
var materialShininessUniform_PF = null;
var lightPositionUniform_PF = [null, null, null];
var LKeyPressUniform_PF = null;

// Uniforms for per vertex
var modelMatrixUniform_PV = null;
var viewMatrixUniform_PV = null;
var projectionMatrixUniform_PV = null;
var LaUniform_PV = [null, null, null];
var LdUniform_PV = [null, null, null];
var LsUniform_PV = [null, null, null];
var KaUniform_PV = null;
var KdUniform_PV = null;
var KsUniform_PV = null;
var materialShininessUniform_PV = null;
var lightPositionUniform_PV = [null, null, null];
var LKeyPressUniform_PV = null;

var perspectiveProjectionMatrix = null;

var light = [
    {   ambient: [0.0,0.0,0.0], 
        diffuse: [1.0,0.0,0.0],
        specular: [1.0,0.0,0.0],
        position: [0.0,0.0,0.0,1.0],
        angle: 0.0 },
    {   ambient: [0.0,0.0,0.0],
        diffuse: [0.0,0.0,1.0], 
        specular: [0.0,0.0,1.0], 
        position: [0.0,0.0,0.0,1.0], 
        angle: 0.0 },
    {   ambient: [0.0,0.0,0.0], 
        diffuse: [0.0,1.0,0.0], 
        specular: [0.0,1.0,0.0], 
        position: [0.0,0.0,0.0,1.0], 
        angle: 0.0 }
];

var materialAmbient = [0.0, 0.0, 0.0]; 
var materialDiffuse = [1.0, 1.0, 1.0]; 
var materialSpecular = [1.0, 1.0, 1.0];
var materialShininiess = 128.0;
var bLight = false;
var bPerFragment = true;
var bPerVertex = false;

function main() {
    // get canvas
    canvas = document.getElementById("lrc");
    if (canvas == null) console.log("Canvas element cannot be obtained\n");
    else console.log("Canvas element succesfully obtained\n");

    canvas_original_width = canvas.width;
    canvas_original_height = canvas.height;

    // register our callback functions as event listeners
    window.addEventListener("keydown", keyDown, false);
    window.addEventListener("click", mouseDown, false);
    window.addEventListener("resize", resize, false);

    // initialize WebGL
    initialize();
    resize();
    display();
}

function initialize() {
    // get 2D context form canvas
    gl = canvas.getContext("webgl2");
    if (gl == null) console.log("webGl2 Context element cannot be obtained\n");
    else console.log("webGl2 Context element succesfully obtained\n");

    // set viewport width and height
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;

    var vertexShaderObject_PF = gl.createShader(gl.VERTEX_SHADER);
    var vertexShaderSourceCode_PF = 
        "#version 300 es \n"+
        "precision highp float; \n"+
        "precision highp int; \n"+
        "in vec4 aPosition; \n"+
        "in vec3 aNormal; \n"+
        "out vec3 out_transformedNormals; \n"+
        "out vec3 out_lightDirection[3]; \n"+
        "out vec3 out_viewerVector; \n"+
        "uniform mat4 uModelMatrix; \n"+
        "uniform mat4 uViewMatrix; \n"+
        "uniform mat4 uProjectionMatrix; \n"+
        "uniform vec4 uLightPosition[3]; \n"+
        "uniform int uLKeyIsPressed; \n"+
        "void main(void) \n"+
        "{ \n"+
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
            "if (uLKeyIsPressed == 1) { \n"+
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                "out_transformedNormals = normalMatrix * aNormal; \n"+
                "out_viewerVector = -eyeCoordinates.xyz; \n"+
                "for (int i = 0; i < 3; ++i) { \n"+
                    "out_lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n"+
                "} \n"+
            "} \n"+
        "} \n";
    gl.shaderSource(vertexShaderObject_PF, vertexShaderSourceCode_PF);
    gl.compileShader(vertexShaderObject_PF);
    if (gl.getShaderParameter(vertexShaderObject_PF, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(vertexShaderObject_PF);
        if (error.length > 0) {
            alert("PF Vertex Shader Compilation Error: " + error);
            uninitialize();
        }
    }

    var fragmentShaderObject_PF = gl.createShader(gl.FRAGMENT_SHADER);
    var fragmentShaderSourceCode_PF = 
            "#version 300 es \n"+
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec3 out_transformedNormals; \n"+
        "in vec3 out_lightDirection[3]; \n"+
        "in vec3 out_viewerVector; \n"+
        "out vec4 FragColor; \n"+
        "uniform vec3 uLa[3]; \n"+
        "uniform vec3 uLd[3]; \n"+
        "uniform vec3 uLs[3]; \n"+
        "uniform vec4 uLightPosition[3]; \n"+
        "uniform vec3 uKa; \n"+
        "uniform vec3 uKd; \n"+
        "uniform vec3 uKs; \n"+
        "uniform float uMaterialShininess; \n"+
        "uniform int uLKeyIsPressed; \n"+
        "void main(void) { \n"+
            "vec3 phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n"+
            "if (uLKeyIsPressed == 1) { \n"+
                "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
                "vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
                "vec3 normalizedLightDirection[3]; \n"+
                "vec3 lightDirection[3]; \n"+
                "vec3 ambientLight[3]; \n"+
                "vec3 diffuseLight[3]; \n"+
                "vec3 reflectionVector[3]; \n"+
                "vec3 specularLight[3]; \n"+
                "for (int i = 0; i < 3; ++i) { \n"+
                    "normalizedLightDirection[i] = normalize(out_lightDirection[i]); \n"+
                    "ambientLight[i] = uLa[i] * uKa * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n"+
                    "diffuseLight[i] = uLd[i] * uKd * max(dot(normalizedLightDirection[i], normalizedTransformedNormals), 0.0f); \n"+
                    "reflectionVector[i] = reflect(-normalizedLightDirection[i], normalizedTransformedNormals); \n"+
                    "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], normalizedViewerVector), 0.0f), uMaterialShininess); \n"+
                    "phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n"+
                "} \n"+
            "} \n"+
            "else { \n"+
                "phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n"+
            "} \n"+
            "FragColor = vec4(phong_ads_light, 1.0f); \n"+
        "}\n";
    gl.shaderSource(fragmentShaderObject_PF, fragmentShaderSourceCode_PF);
    gl.compileShader(fragmentShaderObject_PF);
    if (gl.getShaderParameter(fragmentShaderObject_PF, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(fragmentShaderObject_PF);
        if (error.length > 0) {
            alert("PF Fragment Shader Compilation Error: " + error);
            uninitialize();
        }   
    }

    // PF Shader Program
    shaderProgramObject_PF = gl.createProgram();
    gl.attachShader(shaderProgramObject_PF, vertexShaderObject_PF);
    gl.attachShader(shaderProgramObject_PF, fragmentShaderObject_PF);
    gl.bindAttribLocation(shaderProgramObject_PF, MyAttributes.LRC_ATTRIBUTE_POSITION, "aPosition");
    gl.bindAttribLocation(shaderProgramObject_PF, MyAttributes.LRC_ATTRIBUTE_NORMAL, "aNormal");
    gl.linkProgram(shaderProgramObject_PF);
    if (gl.getProgramParameter(shaderProgramObject_PF, gl.LINK_STATUS) == false) {
        var error = gl.getProgramInfoLog(shaderProgramObject_PF);  
        if (error.length > 0) {
            alert("PF Shader Program Linking Error: " + error);
            uninitialize();
        }
    }

    modelMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uModelMatrix");
    viewMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uViewMatrix");
    projectionMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uProjectionMatrix");
    KaUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKa");
    KdUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKd");
    KsUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKs");
    materialShininessUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uMaterialShininess");
    LKeyPressUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLKeyIsPressed");
    LaUniform_PF[0] = gl.getUniformLocation(shaderProgramObject_PF, "uLa[0]");
    LdUniform_PF[0] = gl.getUniformLocation(shaderProgramObject_PF, "uLd[0]");
    LsUniform_PF[0] = gl.getUniformLocation(shaderProgramObject_PF, "uLs[0]");
    lightPositionUniform_PF[0] = gl.getUniformLocation(shaderProgramObject_PF, "uLightPosition[0]");
    LaUniform_PF[1] = gl.getUniformLocation(shaderProgramObject_PF, "uLa[1]");
    LdUniform_PF[1] = gl.getUniformLocation(shaderProgramObject_PF, "uLd[1]");
    LsUniform_PF[1] = gl.getUniformLocation(shaderProgramObject_PF, "uLs[1]");
    lightPositionUniform_PF[1] = gl.getUniformLocation(shaderProgramObject_PF, "uLightPosition[1]");
    LaUniform_PF[2] = gl.getUniformLocation(shaderProgramObject_PF, "uLa[2]");
    LdUniform_PF[2] = gl.getUniformLocation(shaderProgramObject_PF, "uLd[2]");
    LsUniform_PF[2] = gl.getUniformLocation(shaderProgramObject_PF, "uLs[2]");
    lightPositionUniform_PF[2] = gl.getUniformLocation(shaderProgramObject_PF, "uLightPosition[2]");

    var vertexShaderObject_PV = gl.createShader(gl.VERTEX_SHADER);
    var vertexShaderSourceCode_PV = 
        "#version 300 es \n"+
        "precision highp float; \n"+
        "precision highp int; \n"+
        "in vec4 aPosition; \n"+
        "in vec3 aNormal; \n"+
        "out vec3 out_phong_ads_light; \n"+
        "uniform mat4 uModelMatrix; \n"+
        "uniform mat4 uViewMatrix; \n"+
        "uniform mat4 uProjectionMatrix; \n"+
        "uniform vec3 uLa[3]; \n"+
        "uniform vec3 uLd[3]; \n"+
        "uniform vec3 uLs[3]; \n"+
        "uniform vec4 uLightPosition[3]; \n"+
        "uniform vec3 uKa; \n"+
        "uniform vec3 uKd; \n"+
        "uniform vec3 uKs; \n"+
        "uniform float uMaterialShininess; \n"+
        "uniform int uLKeyIsPressed; \n"+
        "void main(void) \n"+
        "{ \n"+
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
            "if (uLKeyIsPressed == 1) { \n"+
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n"+
                "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n"+
                "vec3 lightDirection[3]; \n"+
                "vec3 ambientLight[3]; \n"+
                "vec3 diffuseLight[3]; \n"+
                "vec3 reflectionVector[3]; \n"+
                "vec3 specularLight[3]; \n"+
                "out_phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n"+
                "for (int i = 0; i < 3; ++i) { \n"+
                    "lightDirection[i] = normalize(vec3(uLightPosition[i] - eyeCoordinates)); \n"+
                    "ambientLight[i] = uLa[i] * uKa * max(dot(lightDirection[i], transformedNormal), 0.0f); \n"+
                    "diffuseLight[i] = uLd[i] * uKd * max(dot(lightDirection[i], transformedNormal), 0.0f); \n"+
                    "reflectionVector[i] = reflect(-lightDirection[i], transformedNormal); \n"+
                    "specularLight[i] = uLs[i] * uKs * pow(max(dot(reflectionVector[i], viewerVector), 0.0f), uMaterialShininess); \n"+
                    "out_phong_ads_light += ambientLight[i] + diffuseLight[i] + specularLight[i]; \n"+
                "} \n"+
            "} \n"+
            "else { \n"+
                "out_phong_ads_light = vec3(1.0f, 1.0f, 1.0f); \n"+
            "} \n"+
        "} \n";
    gl.shaderSource(vertexShaderObject_PV, vertexShaderSourceCode_PV);
    gl.compileShader(vertexShaderObject_PV);
    if (gl.getShaderParameter(vertexShaderObject_PV, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(vertexShaderObject_PV);
        if (error.length > 0) {
            alert("PV Vertex Shader Compilation Error: " + error);
            uninitialize();
        }
    }

    var fragmentShaderObject_PV = gl.createShader(gl.FRAGMENT_SHADER);
    var fragmentShaderSourceCode_PV = 
            "#version 300 es \n"+
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec3 out_phong_ads_light; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
                "FragColor = vec4(out_phong_ads_light, 1.0f); \n"+
            "}\n";
    gl.shaderSource(fragmentShaderObject_PV, fragmentShaderSourceCode_PV);
    gl.compileShader(fragmentShaderObject_PV);
    if (gl.getShaderParameter(fragmentShaderObject_PV, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(fragmentShaderObject_PV);
        if (error.length > 0) {
            alert("PV Fragment Shader Compilation Error: " + error);
            uninitialize();
        }   
    }

    // PV Shader Program
    shaderProgramObject_PV = gl.createProgram();
    gl.attachShader(shaderProgramObject_PV, vertexShaderObject_PV);
    gl.attachShader(shaderProgramObject_PV, fragmentShaderObject_PV);
    gl.bindAttribLocation(shaderProgramObject_PV, MyAttributes.LRC_ATTRIBUTE_POSITION, "aPosition");
    gl.bindAttribLocation(shaderProgramObject_PV, MyAttributes.LRC_ATTRIBUTE_NORMAL, "aNormal");
    gl.linkProgram(shaderProgramObject_PV);
    if (gl.getProgramParameter(shaderProgramObject_PV, gl.LINK_STATUS) == false) {
        var error = gl.getProgramInfoLog(shaderProgramObject_PV);  
        if (error.length > 0) {
            alert("PV Shader Program Linking Error: " + error);
            uninitialize();
        }
    }

    modelMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uModelMatrix");
    viewMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uViewMatrix");
    projectionMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uProjectionMatrix");
    KaUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKa");
    KdUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKd");
    KsUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKs");
    materialShininessUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uMaterialShininess");
    LKeyPressUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLKeyIsPressed");
    LaUniform_PV[0] = gl.getUniformLocation(shaderProgramObject_PV, "uLa[0]");
    LdUniform_PV[0] = gl.getUniformLocation(shaderProgramObject_PV, "uLd[0]");
    LsUniform_PV[0] = gl.getUniformLocation(shaderProgramObject_PV, "uLs[0]");
    lightPositionUniform_PV[0] = gl.getUniformLocation(shaderProgramObject_PV, "uLightPosition[0]");
    LaUniform_PV[1] = gl.getUniformLocation(shaderProgramObject_PV, "uLa[1]");
    LdUniform_PV[1] = gl.getUniformLocation(shaderProgramObject_PV, "uLd[1]");
    LsUniform_PV[1] = gl.getUniformLocation(shaderProgramObject_PV, "uLs[1]");
    lightPositionUniform_PV[1] = gl.getUniformLocation(shaderProgramObject_PV, "uLightPosition[1]");
    LaUniform_PV[2] = gl.getUniformLocation(shaderProgramObject_PV, "uLa[2]");
    LdUniform_PV[2] = gl.getUniformLocation(shaderProgramObject_PV, "uLd[2]");
    LsUniform_PV[2] = gl.getUniformLocation(shaderProgramObject_PV, "uLs[2]");
    lightPositionUniform_PV[2] = gl.getUniformLocation(shaderProgramObject_PV, "uLightPosition[2]");
    
    sphere = new Mesh();
	makeSphere(sphere, 2.0, 30, 30);

    // enable depth
    gl.clearDepth(1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);

    perspectiveProjectionMatrix = mat4.create();

    // set clear color
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
}

function display() {
    // clear the color buffer
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    var modelMatrix = mat4.create();
    var viewMatrix = mat4.create();
    var translationMatrix = mat4.create();

    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -7.0]);
    modelMatrix = translationMatrix;

    if (bPerFragment) {
        gl.useProgram(shaderProgramObject_PF);

        gl.uniformMatrix4fv(modelMatrixUniform_PF, false, modelMatrix);
        gl.uniformMatrix4fv(viewMatrixUniform_PF, false, viewMatrix);
        gl.uniformMatrix4fv(projectionMatrixUniform_PF, false, perspectiveProjectionMatrix);

        if (bLight == true) {
            gl.uniform3fv(LaUniform_PF[0], light[0].ambient);
            gl.uniform3fv(LdUniform_PF[0], light[0].diffuse);
            gl.uniform3fv(LsUniform_PF[0], light[0].specular);
            gl.uniform4fv(lightPositionUniform_PF[0], light[0].position);
            gl.uniform3fv(LaUniform_PF[1], light[1].ambient);
            gl.uniform3fv(LdUniform_PF[1], light[1].diffuse);
            gl.uniform3fv(LsUniform_PF[1], light[1].specular);
            gl.uniform4fv(lightPositionUniform_PF[1], light[1].position);
            gl.uniform3fv(LaUniform_PF[2], light[2].ambient);
            gl.uniform3fv(LdUniform_PF[2], light[2].diffuse);
            gl.uniform3fv(LsUniform_PF[2], light[2].specular);
            gl.uniform4fv(lightPositionUniform_PF[2], light[2].position);
            gl.uniform3fv(KaUniform_PF, materialAmbient);
            gl.uniform3fv(KdUniform_PF, materialDiffuse);
            gl.uniform3fv(KsUniform_PF, materialSpecular);
            gl.uniform1f(materialShininessUniform_PF, materialShininiess);
            gl.uniform1i(LKeyPressUniform_PF, 1);
        } else {
            gl.uniform1i(LKeyPressUniform_PF, 0);
        }
    } else {
        gl.useProgram(shaderProgramObject_PV);

        gl.uniformMatrix4fv(modelMatrixUniform_PV, false, modelMatrix);
        gl.uniformMatrix4fv(viewMatrixUniform_PV, false, viewMatrix);
        gl.uniformMatrix4fv(projectionMatrixUniform_PV, false, perspectiveProjectionMatrix);

        if (bLight == true) {
            gl.uniform3fv(LaUniform_PV[0], light[0].ambient);
            gl.uniform3fv(LdUniform_PV[0], light[0].diffuse);
            gl.uniform3fv(LsUniform_PV[0], light[0].specular);
            gl.uniform4fv(lightPositionUniform_PV[0], light[0].position);
            gl.uniform3fv(LaUniform_PV[1], light[1].ambient);
            gl.uniform3fv(LdUniform_PV[1], light[1].diffuse);
            gl.uniform3fv(LsUniform_PV[1], light[1].specular);
            gl.uniform4fv(lightPositionUniform_PV[1], light[1].position);
            gl.uniform3fv(LaUniform_PV[2], light[2].ambient);
            gl.uniform3fv(LdUniform_PV[2], light[2].diffuse);
            gl.uniform3fv(LsUniform_PV[2], light[2].specular);
            gl.uniform4fv(lightPositionUniform_PV[2], light[2].position);
            gl.uniform3fv(KaUniform_PV, materialAmbient);
            gl.uniform3fv(KdUniform_PV, materialDiffuse);
            gl.uniform3fv(KsUniform_PV, materialSpecular);
            gl.uniform1f(materialShininessUniform_PV, materialShininiess);
            gl.uniform1i(LKeyPressUniform_PV, 1);
        } else {
            gl.uniform1i(LKeyPressUniform_PV, 0);
        }
    }

    sphere.draw();

    update();
    // double buffering using requestAnimationFrame
    requestAnimationFrame(display, canvas);
}

function update() {
    const RADIUS = 5.0;
    light[0].angle += 0.007;
    light[1].angle -= 0.007;
    light[2].angle += 0.007;

    // light 0 -> x-y plane
    var x = Math.cos(light[0].angle) * RADIUS;
    var y = Math.sin(light[0].angle) * RADIUS;
    light[0].position = [x, y, 0.0, 1.0];
    // light 1 -> x-z plane
    x = Math.cos(light[1].angle) * RADIUS;
    var z = Math.sin(light[1].angle) * RADIUS;
    light[1].position = [x, 0.0, z, 1.0];
    // light 2 -> y-z plane
    z = Math.cos(light[2].angle) * RADIUS;
    y = Math.sin(light[2].angle) * RADIUS;
    light[2].position = [0.0, y, z, 1.0];
}

function resize() {
    if (bFullScreen == true) {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
    } else {
        canvas.width = canvas_original_width;
        canvas.height = canvas_original_height;
    }

    // set the viewport to match the new canvas dimensions
    gl.viewport(0, 0, canvas.width, canvas.height);
    mat4.perspective(perspectiveProjectionMatrix, 45.0, parseFloat(canvas.width) / parseFloat(canvas.height), 0.1, 100.0);
}

function uninitialize() {
    if (bFullScreen == true)
        toggleFullScreen();
    if (vao) {
        gl.deleteVertexArray(vao);
        vao = null;
    }
    if (shaderProgramObject) {
        gl.useProgram(shaderProgramObject);
        var shaderObjects = gl.getAttachedShaders(shaderProgramObject);
        for (let i = 0; i < shaderObjects.length; i++) {
            gl.detachShader(shaderProgramObject, shaderObjects[i]);
            gl.deleteShader(shaderObjects[i]);
            shaderObjects[i] = null;
        }
        gl.useProgram(null);
        gl.deleteProgram(shaderProgramObject);
        shaderProgramObject = null;
    }
}

function keyDown(event) {
    switch (event.keyCode) {
        case 70:    // for 'F' or 'f'
        case 102:
            if (bFullScreen == false) {
                toggleFullScreen();
                bFullScreen = true;
            } else {
                toggleFullScreen();
                bFullScreen = false;
            }
            break;

        case 76:    // for 'L' (uppercase)
        case 108:
            bLight = !bLight;
        break;

        case 86:    // for 'V' or 'v'
        case 118:
            if (bPerVertex == false) {
                bPerVertex = true;
                bPerFragment = false;
            } else {
                bPerVertex = false;
                bPerFragment = true;
            }
            break;

        case 27:    // Escape
            uninitialize();
            window.close();
            break;
    }
}

function mouseDown() {}

function toggleFullScreen() {
    var fullscreen_element =    document.fullscreenElement ||
                                document.mozFullScreenElement ||
                                document.webkitFullscreenElement ||
                                document.msFullscreenElement ||
                                null;
    
    if (fullscreen_element == null) {
        if (canvas.requestFullscreen) canvas.requestFullscreen();
        else if (canvas.mozRequestFullScreen) canvas.mozRequestFullScreen();
        else if (canvas.webkitRequestFullscreen) canvas.webkitRequestFullscreen();
        else if (canvas.msRequestFullscreen) canvas.msRequestFullscreen();
    }
    else {
        if (document.exitFullscreen) document.exitFullscreen();
        else if (document.mozExitFullScreen) document.mozExitFullScreen();
        else if (document.webkitExitFullscreen) document.webkitExitFullscreen();
        else if (document.msExitFullscreen) document.msExitFullscreen();
    }
}
