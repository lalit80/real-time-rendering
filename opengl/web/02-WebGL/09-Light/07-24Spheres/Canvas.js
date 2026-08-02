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

var light = [
    {
        ambient: [0.0, 0.0, 0.0],
        diffuse: [1.0, 1.0, 1.0],
        specular: [1.0, 1.0, 1.0],
        position: [0.0, 0.0, 0.0, 1.0],
        angle: 0.0 
    }
];

var material = new Array(24);
for (let i = 0; i < 24; ++i) 
    material[i] = { ambient: [0,0,0],
                    diffuse: [0,0,0], 
                    specular: [0,0,0], 
                    shininess: 0.0 
    };

var bLight = false;
var bPerFragment = true;
var bPerVertex = false;
var keyPressed = 1;

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
            "uniform float uMaterialShininess; \n"+
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
                    "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialShininess); \n"+
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
    materialShininessUniform_PF = gl.getUniformLocation(shaderProgramObject_PF, "uMaterialShininess");
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
        "uniform float uMaterialShininess; \n"+
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
                "vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, viewerVector), 0.0f), uMaterialShininess); \n"+
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
    materialShininessUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uMaterialShininess");
    LKeyPressUniform_PV = gl.getUniformLocation(shaderProgramObject_PV, "uLKeyIsPressed");
    sphere = new Mesh();
	makeSphere(sphere, 0.5, 30, 30);

    // enable depth
    gl.clearDepth(1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);

    perspectiveProjectionMatrix = mat4.create();

    // set clear color
    gl.clearColor(0.75, 0.75, 0.75, 1.0);

    fillMaterialProperties();
}

function display() {
    // clear the color buffer
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    if (bPerFragment == true) {
        gl.useProgram(shaderProgramObject_PF);

        // transformations
        var viewMatrix = mat4.create();
        var modelMatrix = mat4.create();

        gl.uniformMatrix4fv(viewMatrixUniform_PF, false, viewMatrix);
        gl.uniformMatrix4fv(projectionMatrixUniform_PF, false, perspectiveProjectionMatrix);
        if (bLight == true) {
            gl.uniform3fv(LaUniform_PF, light[0].ambient);
            gl.uniform3fv(LdUniform_PF, light[0].diffuse);
            gl.uniform3fv(LsUniform_PF, light[0].specular);
            gl.uniform4fv(lightPositionUniform_PF, light[0].position);
            gl.uniform1i(LKeyPressUniform_PF, 1);
        } else {
            gl.uniform1i(LKeyPressUniform_PF, 0);
        }

        // *** For 24 spheres ***
        var yTranslate = 0.0;
        var xTranslate = -2.5;
        var i = 0;
        var k = 0;

        for (i = 0; i < 4; ++i) { 
            yTranslate = 3.0;
            for (var j = 0; j < 6; ++j) {
                mat4.identity(modelMatrix);
                mat4.translate(modelMatrix, modelMatrix, [xTranslate, yTranslate, -8.5]);
                gl.uniform3fv(KaUniform_PF, material[k].ambient);
                gl.uniform3fv(KdUniform_PF, material[k].diffuse);
                gl.uniform3fv(KsUniform_PF, material[k].specular);
                gl.uniform1f(materialShininessUniform_PF, material[k].shininess);
                gl.uniformMatrix4fv(modelMatrixUniform_PF, false, modelMatrix);

                sphere.draw();

                yTranslate -= 1.2;
                ++k;
            }
            xTranslate += 1.5;
        }
    } else if (bPerVertex == true) {
        gl.useProgram(shaderProgramObject_PV);

        // transformations
        var viewMatrix = mat4.identity(mat4.create());
        var modelMatrix = mat4.create();

        // send this matrix to vertex shader in uniform
        gl.uniformMatrix4fv(viewMatrixUniform_PV, false, viewMatrix);
        gl.uniformMatrix4fv(projectionMatrixUniform_PV, false, perspectiveProjectionMatrix);

        // send light properties
        if (bLight == true) {
            gl.uniform1i(LKeyPressUniform_PV, 1);
            gl.uniform3fv(LaUniform_PV, light[0].ambient);
            gl.uniform3fv(LdUniform_PV, light[0].diffuse);
            gl.uniform3fv(LsUniform_PV, light[0].specular);
            gl.uniform4fv(lightPositionUniform_PV, light[0].position);
        } else {
            gl.uniform1i(LKeyPressUniform_PV, 0);
        }

        // *** For 24 spheres ***
        var yTranslate = 0.0;
        var xTranslate = -2.5;
        var i = 0;
        var k = 0;

        for (i = 0; i < 4; ++i) { 
            yTranslate = 3.0;
            for (var j = 0; j < 6; ++j) {
                mat4.identity(modelMatrix);
                mat4.translate(modelMatrix, modelMatrix, [xTranslate, yTranslate, -8.5]);
                gl.uniform3fv(KaUniform_PV, material[k].ambient);
                gl.uniform3fv(KdUniform_PV, material[k].diffuse);
                gl.uniform3fv(KsUniform_PV, material[k].specular);
                gl.uniform1f(materialShininessUniform_PV, material[k].shininess);
                gl.uniformMatrix4fv(modelMatrixUniform_PV, false, modelMatrix);

                sphere.draw();

                yTranslate -= 1.2;
                ++k;
            }
            xTranslate += 1.5;
        }

        gl.bindVertexArray(null);
    }

    update();
    // double buffering using requestAnimationFrame
    requestAnimationFrame(display, canvas);
}

function update() {
    var RADIUS = 20.0;
    var x = 0.0;
    var y = 0.0;
    var z = 0.0;

    light[0].angle += + 0.005;
    var angleRad = light[0].angle * Math.PI / 180.0;
    if (keyPressed == 1) {
        x = Math.cos(light[0].angle) * RADIUS;
        y = Math.sin(light[0].angle) * RADIUS;
        light[0].position = [x, y, 0.0, 1.0];
    }
    else if (keyPressed == 2) {
        // update position of light 0 (x-z plane)
        x = Math.cos(light[0].angle) * RADIUS;
        z = Math.sin(light[0].angle) * RADIUS;
        light[0].position = [x, 0.0, z, 1.0];
    }
    else if (keyPressed == 3) {
        // update position of light 0 (x-y plane)
        x = Math.cos(light[0].angle) * RADIUS;
        y = Math.sin(light[0].angle) * RADIUS;
        light[0].position = [x, y, 0.0, 1.0];
    }
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
    if (shaderProgramObject_PF) {
        gl.useProgram(shaderProgramObject_PF);
        var shaderObjects = gl.getAttachedShaders(shaderProgramObject_PF);
        for (let i = 0; i < shaderObjects.length; i++) {
            gl.detachShader(shaderProgramObject_PF, shaderObjects[i]);
            gl.deleteShader(shaderObjects[i]);
            shaderObjects[i] = null;
        }
        gl.useProgram(null);
        gl.deleteProgram(shaderProgramObject_PF);
        shaderProgramObject_PF = null;
    }
    if (shaderProgramObject_PV) {
        gl.useProgram(shaderProgramObject_PF);
        var shaderObjects = gl.getAttachedShaders(shaderProgramObject_PV);
        for (let i = 0; i < shaderObjects.length; i++) {
            gl.detachShader(shaderProgramObject_PV, shaderObjects[i]);
            gl.deleteShader(shaderObjects[i]);
            shaderObjects[i] = null;
        }
        gl.useProgram(null);
        gl.deleteProgram(shaderProgramObject_PV);
        shaderProgramObject_PV = null;
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

        case 88: // 'X'
        case 120:
            keyPressed = 1;
            break;

        case 89: // 'Y'
        case 121:
            keyPressed = 2;
            break;

        case 90: // 'Z'
        case 122:
            keyPressed = 3;
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

function fillMaterialProperties() {
    material[0].ambient = [0.0215, 0.1745, 0.0215];
    material[0].diffuse = [0.07568, 0.61424, 0.07568];
    material[0].specular = [0.633, 0.727811, 0.633];
    material[0].shininess = 0.6 * 128;

    material[1].ambient = [0.135, 0.2225, 0.1575];
    material[1].diffuse = [0.54, 0.89, 0.63];
    material[1].specular = [0.316228, 0.316228, 0.316228];
    material[1].shininess = 0.1 * 128;

    material[2].ambient = [0.05375, 0.05, 0.06625];
    material[2].diffuse = [0.18275, 0.17, 0.22525];
    material[2].specular = [0.332741, 0.328634, 0.346435];
    material[2].shininess = 0.3 * 128;

    material[3].ambient = [0.25, 0.20725, 0.20725];
    material[3].diffuse = [1.0, 0.829, 0.829];
    material[3].specular = [0.296648, 0.296648, 0.296648];
    material[3].shininess = 0.088 * 128;

    material[4].ambient = [0.1745, 0.01175, 0.01175];
    material[4].diffuse = [0.61424, 0.04136, 0.04136];
    material[4].specular = [0.727811, 0.626959, 0.626959];
    material[4].shininess = 0.6 * 128;

    material[5].ambient = [0.1, 0.18725, 0.1745];
    material[5].diffuse = [0.396, 0.396, 0.69102];
    material[5].specular = [0.297254, 0.30829, 0.306678];
    material[5].shininess = 0.1 * 128;

    material[6].ambient = [0.329412, 0.223529, 0.027451];
    material[6].diffuse = [0.780392, 0.568627, 0.113725];
    material[6].specular = [0.992157, 0.941176, 0.807843];
    material[6].shininess = 0.21794872 * 128;

    material[7].ambient = [0.2125, 0.1275, 0.054];
    material[7].diffuse = [0.714, 0.4284, 0.18144];
    material[7].specular = [0.393548, 0.271906, 0.166721];
    material[7].shininess = 0.2 * 128;

    material[8].ambient = [0.25, 0.25, 0.25];
    material[8].diffuse = [0.4, 0.4, 0.4];
    material[8].specular = [0.774597, 0.774597, 0.774597];
    material[8].shininess = 0.6 * 128;

    material[9].ambient = [0.19125, 0.0735, 0.0225];
    material[9].diffuse = [0.7038, 0.27048, 0.0828];
    material[9].specular = [0.256777, 0.137622, 0.086014];
    material[9].shininess = 0.1 * 128;

    material[10].ambient = [0.24725, 0.1995, 0.0745];
    material[10].diffuse = [0.75164, 0.60648, 0.22648];
    material[10].specular = [0.628281, 0.555802, 0.366065];
    material[10].shininess = 0.4 * 128;

    material[11].ambient = [0.19225, 0.19225, 0.19225];
    material[11].diffuse = [0.50754, 0.50754, 0.50754];
    material[11].specular = [0.508273, 0.508273, 0.508273];
    material[11].shininess = 0.4 * 128;

    material[12].ambient = [0.0, 0.0, 0.0];
    material[12].diffuse = [0.01, 0.01, 0.01];
    material[12].specular = [0.5, 0.5, 0.5];
    material[12].shininess = 0.25 * 128;

    material[13].ambient = [0.0, 0.1, 0.06];
    material[13].diffuse = [0.0, 0.50980392, 0.50980392];
    material[13].specular = [0.50196078, 0.50196078, 0.50196078];
    material[13].shininess = 0.25 * 128;

    material[14].ambient = [0.0, 0.0, 0.0];
    material[14].diffuse = [0.1, 0.35, 0.1];
    material[14].specular = [0.45, 0.55, 0.45];
    material[14].shininess = 0.25 * 128;

    material[15].ambient = [0.0, 0.0, 0.0];
    material[15].diffuse = [0.5, 0.0, 0.0];
    material[15].specular = [0.7, 0.6, 0.6];
    material[15].shininess = 0.25 * 128;

    material[16].ambient = [0.0, 0.0, 0.0];
    material[16].diffuse = [0.55, 0.55, 0.55];
    material[16].specular = [0.7, 0.7, 0.7];
    material[16].shininess = 0.25 * 128;

    material[17].ambient = [0.0, 0.0, 0.0];
    material[17].diffuse = [0.5, 0.5, 0.0];
    material[17].specular = [0.6, 0.6, 0.5];
    material[17].shininess = 0.25 * 128;

    material[18].ambient = [0.02, 0.02, 0.02];
    material[18].diffuse = [0.01, 0.01, 0.01];
    material[18].specular = [0.4, 0.4, 0.4];
    material[18].shininess = 0.078125 * 128;

    material[19].ambient = [0.0, 0.05, 0.05];
    material[19].diffuse = [0.4, 0.5, 0.5];
    material[19].specular = [0.04, 0.7, 0.7];
    material[19].shininess = 0.078125 * 128;

    material[20].ambient = [0.0, 0.05, 0.0];
    material[20].diffuse = [0.4, 0.5, 0.4];
    material[20].specular = [0.04, 0.7, 0.04];
    material[20].shininess = 0.078125 * 128;

    material[21].ambient = [0.05, 0.0, 0.0];
    material[21].diffuse = [0.5, 0.4, 0.4];
    material[21].specular = [0.7, 0.04, 0.04];
    material[21].shininess = 0.078125 * 128;

    material[22].ambient = [0.05, 0.05, 0.05];
    material[22].diffuse = [0.5, 0.5, 0.5];
    material[22].specular = [0.7, 0.7, 0.7];
    material[22].shininess = 0.078125 * 128;

    material[23].ambient = [0.05, 0.05, 0.0];
    material[23].diffuse = [0.5, 0.5, 0.4];
    material[23].specular = [0.7, 0.7, 0.04];
    material[23].shininess = 0.078125 * 128;
}
