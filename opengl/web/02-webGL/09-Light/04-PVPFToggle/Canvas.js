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
var LaUniform_PF = null;
var LdUniform_PF = null;
var LsUniform_PF = null;
var KaUniform_PF = null;
var KdUniform_PF = null;
var KsUniform_PF = null;
var materialShininessUniform_PF = null;
var lightPositionUniform_PF = null;
var LKeyPressUniform_PF = null;

// Uniforms for per vertex
var modelMatrixUniform_PV = null;
var viewMatrixUniform_PV = null;
var projectionMatrixUniform_PV = null;
var LaUniform_PV = null;
var LdUniform_PV = null;
var LsUniform_PV = null;
var KaUniform_PV = null;
var KdUniform_PV = null;
var KsUniform_PV = null;
var materialShininessUniform_PV = null;
var lightPositionUniform_PV = null;
var LKeyPressUniform_PV = null;

var perspectiveProjectionMatrix = null;

var lightAmbient = [0.0, 0.0, 0.0]; 
var lightDiffuse = [1.0, 1.0, 1.0]; 
var lightSpecular = [1.0, 1.0, 1.0];
var lightPosition = [100.0, 100.0, 100.0, 1.0];

var materialAmbient = [0.0, 0.0, 0.0]; 
var materialDiffuse = [0.5, 0.2, 0.7]; 
var materialSpecular = [0.7, 0.7, 0.7];
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
        "out vec3 out_lightDirection; \n"+
        "out vec3 out_viewerVector; \n"+
        "uniform mat4 uModelMatrix; \n"+
        "uniform mat4 uViewMatrix; \n"+
        "uniform mat4 uProjectionMatrix; \n"+
        "uniform vec4 uLightPosition; \n"+
        "uniform int uLKeyIsPressed; \n"+
        "void main(void) \n"+
        "{ \n"+
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
            "if (uLKeyIsPressed == 1) { \n"+
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                "out_transformedNormals = normalMatrix * aNormal; \n"+
                "out_lightDirection = vec3(uLightPosition - eyeCoordinates); \n"+
                "out_viewerVector = -eyeCoordinates.xyz; \n"+
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
            "in vec3 out_lightDirection; \n"+
            "in vec3 out_viewerVector; \n"+
            "out vec4 FragColor; \n"+
            "uniform vec3 uLa; \n"+
            "uniform vec3 uLd; \n"+
            "uniform vec3 uLs; \n"+
            "uniform vec3 uKa; \n"+
            "uniform vec3 uKd; \n"+
            "uniform vec3 uKs; \n"+
            "uniform float uMaterialSininess; \n"+
            "uniform int uLKeyIsPressed; \n"+
            "void main(void)\n"+
            "{\n"+
                "vec3 phong_ads_light; \n"+
                "if (uLKeyIsPressed == 1) { \n"+
                    "vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
                    "vec3 normalizedLightDirection = normalize(out_lightDirection); \n"+
                    "vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
                    "vec3 ambientLight = uLa * uKa * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
                    "vec3 diffuseLight = uLd * uKd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
                    "vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n"+
                    "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialSininess); \n"+
                    "phong_ads_light = ambientLight + diffuseLight + specularLight; \n"+
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

    // Get PF uniform locations
    modelMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uModelMatrix");
    viewMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uViewMatrix");
    projectionMatrixUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uProjectionMatrix");
    LaUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLa");
    LdUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLd");
    LsUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLs");
    lightPositionUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLightPosition");
    KaUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKa");
    KdUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKd");
    KsUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uKs");
    materialShininessUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uMaterialSininess");
    LKeyPressUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uLKeyIsPressed");


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
        "uniform vec3 uLa; \n"+
        "uniform vec3 uLd; \n"+
        "uniform vec3 uLs; \n"+
        "uniform vec4 uLightPosition; \n"+
        "uniform vec3 uKa; \n"+
        "uniform vec3 uKd; \n"+
        "uniform vec3 uKs; \n"+
        "uniform float uMaterialSininess; \n"+
        "uniform int uLKeyIsPressed; \n"+
        "void main(void) \n"+
        "{ \n"+
            "gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
            "if (uLKeyIsPressed == 1) { \n"+
                "vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
                "mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
                "vec3 transformedNormal = normalize(normalMatrix * aNormal); \n"+
                "vec3 lightDirection = normalize(vec3(uLightPosition - eyeCoordinates)); \n"+
                "vec3 ambientLight = uLa * uKa * max(dot(lightDirection, transformedNormal), 0.0f); \n"+
                "vec3 diffuseLight = uLd * uKd * max(dot(lightDirection, transformedNormal), 0.0f); \n"+
                "vec3 reflectionVector = reflect(-lightDirection, transformedNormal); \n"+
                "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n"+
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, viewerVector), 0.0f), uMaterialSininess); \n"+
                "out_phong_ads_light = ambientLight + diffuseLight + specularLight; \n"+
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

    // Get PV uniform locations
    modelMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uModelMatrix");
    viewMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uViewMatrix");
    projectionMatrixUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uProjectionMatrix");
    LaUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLa");
    LdUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLd");
    LsUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLs");
    lightPositionUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLightPosition");
    KaUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKa");
    KdUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKd");
    KsUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uKs");
    materialShininessUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uMaterialSininess");
    LKeyPressUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLKeyIsPressed");
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
            gl.uniform3fv(LaUniform_PF, lightAmbient);
            gl.uniform3fv(LdUniform_PF, lightDiffuse);
            gl.uniform3fv(LsUniform_PF, lightSpecular);
            gl.uniform4fv(lightPositionUniform_PF, lightPosition);
            gl.uniform3fv(KaUniform_PF, materialAmbient);
            gl.uniform3fv(KdUniform_PF, materialDiffuse);
            gl.uniform3fv(KsUniform_PF, materialSpecular);
            gl.uniform1f(materialShininessUniform_PF, materialShininiess);
            gl.uniform1i(LKeyPressUniform_PF, 1);
        } else {
            gl.uniform1i(LKeyPressUniform_PF, 0);
        }
    } else if (bPerVertex) {
        gl.useProgram(shaderProgramObject_PV);

        gl.uniformMatrix4fv(modelMatrixUniform_PV, false, modelMatrix);
        gl.uniformMatrix4fv(viewMatrixUniform_PV, false, viewMatrix);
        gl.uniformMatrix4fv(projectionMatrixUniform_PV, false, perspectiveProjectionMatrix);
        
        if (bLight == true) {
            gl.uniform3fv(LaUniform_PV, lightAmbient);
            gl.uniform3fv(LdUniform_PV, lightDiffuse);
            gl.uniform3fv(LsUniform_PV, lightSpecular);
            gl.uniform4fv(lightPositionUniform_PV, lightPosition);
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
