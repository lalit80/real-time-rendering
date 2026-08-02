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
var shaderProgramObject = null;
var vao = null;
var vbo_position = null;
var vbo_normal = null;
var cube_angle = 0.0;

var modelMatrixUniform = null;
var viewMatrixUniform = null;
var projectionMatrixUniform = null;
var LaUniforms = new Array(2);                    
var LdUniforms = new Array(2);                    
var LsUniforms = new Array(2);                    
var KaUniform = null;                             
var KdUniform = null;                             
var KsUniform = null;                             
var materialShininessUniform = null;
var lightPositionUniforms = new Array(2);         
var LKeyPressUniform = null;

var perspectiveProjectionMatrix = null;

var materialAmbient = [0.0, 0.0, 0.0]; 
var materialDiffuse = [0.5, 0.5, 0.5];
var materialSpecular = [1.0, 1.0, 1.0];
var materialShininiess = 50.0;
var bLight = false;
var bAnimation = false;

// somewhat similar to c structs
var lightData = [
    {
        ambient: [0.0, 0.0, 0.0],
        diffuse: [1.0, 0.0, 0.0],
        specular: [1.0, 0.0, 0.0],
        position: [-2.0, 0.0, 0.0, 1.0],
    },
    {
        ambient: [0.0, 0.0, 0.0],
        diffuse: [0.0, 0.0, 1.0],
        specular: [0.0, 0.0, 1.0],
        position: [2.0, 0.0, 0.0, 1.0],
    }
];

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

    // vertex shader
    var vertexShaderObject = gl.createShader(gl.VERTEX_SHADER);
    var vertexShaderSourceCode = 
            "#version 300 es \n"+
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec4 aPosition; \n"+
        "in vec3 aNormal; \n"+
        "out vec3 out_phong_ads_light; \n"+
        "uniform mat4 uModelMatrix; \n"+
        "uniform mat4 uViewMatrix; \n"+
        "uniform mat4 uProjectionMatrix; \n"+
        "uniform vec3 uLa[2]; \n"+
        "uniform vec3 uLd[2]; \n"+
        "uniform vec3 uLs[2]; \n"+
        "uniform vec4 uLightPosition[2]; \n"+
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
                "vec3 viewerVector = normalize(-eyeCoordinates.xyz); \n"+
                "vec3 lightDirection[2]; \n"+
                "vec3 ambientLight[2]; \n"+
                "vec3 diffuseLight[2]; \n"+
                "vec3 reflectionVector[2]; \n"+
                "vec3 specularLight[2]; \n"+
                "out_phong_ads_light = vec3(0.0f, 0.0f, 0.0f); \n"+
                "for (int i = 0; i < 2; ++i) { \n"+
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
        "} \n";
    gl.shaderSource(vertexShaderObject, vertexShaderSourceCode);
    gl.compileShader(vertexShaderObject);
    if (gl.getShaderParameter(vertexShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(vertexShaderObject);
        if (error.length > 0) {
            alert("Vertex Shader Compilation Error: " + error);
            uninitialize();
        }
    }

    // fragment shader
    var fragmentShaderObject = gl.createShader(gl.FRAGMENT_SHADER);
    var fragmentShaderSourceCode = 
            "#version 300 es \n"+
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec3 out_phong_ads_light; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
            "   FragColor = vec4(out_phong_ads_light, 1.0f); \n"+
            "}\n";
    gl.shaderSource(fragmentShaderObject, fragmentShaderSourceCode);
    gl.compileShader(fragmentShaderObject);
    if (gl.getShaderParameter(fragmentShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(fragmentShaderObject);
        if (error.length > 0) {
            alert("Fragment Shader Compilation Error: " + error);
            uninitialize();
        }   
    } else {
        console.log("Fragment Shader Compilation Successful.\n");
    }

    // shader program
    shaderProgramObject = gl.createProgram();
    gl.attachShader(shaderProgramObject, vertexShaderObject);
    gl.attachShader(shaderProgramObject, fragmentShaderObject);
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_POSITION, "aPosition");
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_NORMAL, "aNormal");
    gl.linkProgram(shaderProgramObject);
    if (gl.getProgramParameter(shaderProgramObject, gl.LINK_STATUS) == false) {
        var error = gl.getProgramInfoLog(shaderProgramObject);  
        if (error.length > 0) {
            alert("Shader Program Linking Error: " + error);
            uninitialize();
        }
    } else {
        console.log("Shader Program Linking Successful.\n");
    }

    // get uniform locations
    modelMatrixUniform = gl.getUniformLocation(shaderProgramObject, "uModelMatrix");
    viewMatrixUniform = gl.getUniformLocation(shaderProgramObject, "uViewMatrix");
    projectionMatrixUniform = gl.getUniformLocation(shaderProgramObject, "uProjectionMatrix");
    
    // Light and Material uniforms
    KaUniform = gl.getUniformLocation(shaderProgramObject, "uKa");
    KdUniform = gl.getUniformLocation(shaderProgramObject, "uKd");
    KsUniform = gl.getUniformLocation(shaderProgramObject, "uKs");
    materialShininessUniform = gl.getUniformLocation(shaderProgramObject, "uMaterialSininess");
    LKeyPressUniform = gl.getUniformLocation(shaderProgramObject, "uLKeyIsPressed");

    LaUniforms[0] = gl.getUniformLocation(shaderProgramObject, "uLa[0]");
    LdUniforms[0] = gl.getUniformLocation(shaderProgramObject, "uLd[0]");
    LsUniforms[0] = gl.getUniformLocation(shaderProgramObject, "uLs[0]");
    lightPositionUniforms[0] = gl.getUniformLocation(shaderProgramObject, "uLightPosition[0]");
    LaUniforms[1] = gl.getUniformLocation(shaderProgramObject, "uLa[1]");
    LdUniforms[1] = gl.getUniformLocation(shaderProgramObject, "uLd[1]");
    LsUniforms[1] = gl.getUniformLocation(shaderProgramObject, "uLs[1]");
    lightPositionUniforms[1] = gl.getUniformLocation(shaderProgramObject, "uLightPosition[1]");

    // cube position, color, vao, vbo
    var pyramid_position = new Float32Array([
        // front
        0.0,  1.0,  0.0, // front-top
        -1.0, -1.0,  1.0, // front-left
        1.0, -1.0,  1.0, // front-right
        
        // right
        0.0,  1.0,  0.0, // right-top
        1.0, -1.0,  1.0, // right-left
        1.0, -1.0, -1.0, // right-right

        // back
        0.0,  1.0,  0.0, // back-top
        1.0, -1.0, -1.0, // back-left
        -1.0, -1.0, -1.0, // back-right

        // left
        0.0,  1.0,  0.0, // left-top
        -1.0, -1.0, -1.0, // left-left
        -1.0, -1.0,  1.0, // left-right
    ]);

    var pyramid_normal = new Float32Array([
        // front
        0.000000, 0.447214,  0.894427, // front-top
        0.000000, 0.447214,  0.894427, // front-left
        0.000000, 0.447214,  0.894427, // front-right
                                
        // right			    
        0.894427, 0.447214,  0.0, // right-top
        0.894427, 0.447214,  0.0, // right-left
        0.894427, 0.447214,  0.0, // right-right

        // back
        0.000000, 0.447214, -0.894427, // back-top
        0.0, 0.447214, -0.894427, // back-left
        0.0, 0.447214, -0.894427, // back-right

        // left
        -0.894427, 0.447214,  0.0, // left-top
        -0.894427, 0.447214,  0.0, // left-left
        -0.894427, 0.447214,  0.0, // left-right
    ]);
    
    vao = gl.createVertexArray();
    gl.bindVertexArray(vao);

    vbo_position = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_position);
    gl.bufferData(gl.ARRAY_BUFFER, pyramid_position, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    vbo_normal = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_normal);
    gl.bufferData(gl.ARRAY_BUFFER, pyramid_normal, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_NORMAL, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_NORMAL);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    
    gl.bindVertexArray(null);

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
    gl.useProgram(shaderProgramObject);

    var modelMatrix = mat4.create();
    var viewMatrix = mat4.create();
    var translationMatrix = mat4.create();
    var scaleMatrix = mat4.create();
    var rotationMatrix = mat4.create();

    mat4.rotateY(rotationMatrix, rotationMatrix, glMatrix.toRadian(cube_angle));
    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -4.0]);
    mat4.multiply(modelMatrix, translationMatrix, rotationMatrix);

    gl.uniformMatrix4fv(modelMatrixUniform, false, modelMatrix);
    gl.uniformMatrix4fv(viewMatrixUniform, false, viewMatrix);
    gl.uniformMatrix4fv(projectionMatrixUniform, false, perspectiveProjectionMatrix);

    if (bLight == true) {
        gl.uniform3fv(LaUniforms[0], lightData[0].ambient);
        gl.uniform3fv(LdUniforms[0], lightData[0].diffuse);
        gl.uniform3fv(LsUniforms[0], lightData[0].specular);
        gl.uniform4fv(lightPositionUniforms[0], lightData[0].position);
        gl.uniform3fv(LaUniforms[1], lightData[1].ambient);
        gl.uniform3fv(LdUniforms[1], lightData[1].diffuse);
        gl.uniform3fv(LsUniforms[1], lightData[1].specular);
        gl.uniform4fv(lightPositionUniforms[1], lightData[1].position);
        gl.uniform3fv(KaUniform, materialAmbient);
        gl.uniform3fv(KdUniform, materialDiffuse);
        gl.uniform3fv(KsUniform, materialSpecular);
        gl.uniform1f(materialShininessUniform, materialShininiess);
        gl.uniform1i(LKeyPressUniform, 1);
    } else {
        gl.uniform1i(LKeyPressUniform, 0);
    }
    
    gl.bindVertexArray(vao);
    gl.drawArrays(gl.TRIANGLES, 0, 12);
    gl.bindVertexArray(null);
    gl.useProgram(null);

    update();
    // double buffering using requestAnimationFrame
    requestAnimationFrame(display, canvas);
}

function update() {
    cube_angle += 0.5;
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
    if (vbo_position) {
        gl.deleteBuffer(vbo_position);
        vbo_position = null;
    }
    if (vbo_normal) {
        gl.deleteBuffer(vbo_normal);
        vbo_normal = null;
    }
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
