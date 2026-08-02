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
var vao_cube = null;
var vbo_position_cube = null;
var vbo_normal = null;
var cube_angle = 0.0;

var modelViewMatrixUniform = null;
var projectionMatrixUniform = null;
var LdUniform = null;
var KdUniform = null;
var lightPositionUniform = null;
var LKeyPressUniform = null;

var perspectiveProjectionMatrix = null;

var lightDiffuse = [1.0, 1.0, 1.0];
var materialDiffuse = [0.4, 0.4, 0.4];
var lightPosition = [0, 0, 2.0, 1.0];
var bLight = false;

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
            "out vec3 outDiffuseLight; \n"+
            "uniform mat4 uModelViewMatrix; \n"+
            "uniform mat4 uProjectionMatrix; \n"+
            "uniform vec3 uLd; \n"+
            "uniform vec3 uKd; \n"+
            "uniform vec4 uLightPosition; \n"+
            "uniform int uLKeyIsPressed; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   gl_Position = uProjectionMatrix * uModelViewMatrix * aPosition; \n"+
            "   if (uLKeyIsPressed == 1) { \n"+
            "       vec4 eyeCoordinates = uModelViewMatrix * aPosition; \n"+
            "       mat3 normalMatrix = mat3(transpose(inverse(uModelViewMatrix))); \n"+
            "       vec3 transformedNormal = normalize(normalMatrix * aNormal); \n"+
            "       vec3 lightSource = vec3(uLightPosition - eyeCoordinates); \n"+
            "       outDiffuseLight = uLd * uKd * max(dot(lightSource, transformedNormal), 0.0); \n"+
            "        \n"+
            "   } \n"+
            "   else { \n"+
            "       outDiffuseLight = vec3(1.0, 1.0, 1.0); \n"+
            "   } \n"+
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
            "in vec3 outDiffuseLight; \n"+
            "out vec4 FragColor; \n"+
            "void main(void)\n"+
            "{\n"+
            "   FragColor = vec4(outDiffuseLight, 1.0); \n"+
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
    modelViewMatrixUniform = gl.getUniformLocation(shaderProgramObject, "uModelViewMatrix");
    projectionMatrixUniform = gl.getUniformLocation(shaderProgramObject, "uProjectionMatrix");
    LdUniform = gl.getUniformLocation(shaderProgramObject, "uLd");
    KdUniform = gl.getUniformLocation(shaderProgramObject, "uKd");
    lightPositionUniform = gl.getUniformLocation(shaderProgramObject, "uLightPosition");
    LKeyPressUniform = gl.getUniformLocation(shaderProgramObject, "uLKeyIsPressed");


    // cube position, color, vao_cube, vbo
    var cube_position = new Float32Array([
        // front
        1.0,  1.0,  1.0, // top-right of front
        -1.0,  1.0,  1.0, // top-left of front
        -1.0, -1.0,  1.0, // bottom-left of front
        1.0, -1.0,  1.0, // bottom-right of front

        // right
        1.0,  1.0, -1.0, // top-right of right
        1.0,  1.0,  1.0, // top-left of right
        1.0, -1.0,  1.0, // bottom-left of right
        1.0, -1.0, -1.0, // bottom-right of right

        // back
        1.0,  1.0, -1.0, // top-right of back
        -1.0,  1.0, -1.0, // top-left of back
        -1.0, -1.0, -1.0, // bottom-left of back
        1.0, -1.0, -1.0, // bottom-right of back

        // left
        -1.0,  1.0,  1.0, // top-right of left
        -1.0,  1.0, -1.0, // top-left of left
        -1.0, -1.0, -1.0, // bottom-left of left
        -1.0, -1.0,  1.0, // bottom-right of left

        // top
        1.0,  1.0, -1.0, // top-right of top
        -1.0,  1.0, -1.0, // top-left of top
        -1.0,  1.0,  1.0, // bottom-left of top
        1.0,  1.0,  1.0, // bottom-right of top

        // bottom
        1.0, -1.0,  1.0, // top-right of bottom
        -1.0, -1.0,  1.0, // top-left of bottom
        -1.0, -1.0, -1.0, // bottom-left of bottom
        1.0, -1.0, -1.0, // bottom-right of bottom
    ]);

    var cube_normal = new Float32Array([
        // front surface
        0.0,  0.0,  1.0, // top-right of front
        0.0,  0.0,  1.0, // top-left of front
        0.0,  0.0,  1.0, // bottom-left of front
        0.0,  0.0,  1.0, // bottom-right of front

        // right surface
        1.0,  0.0,  0.0, // top-right of right
        1.0,  0.0,  0.0, // top-left of right
        1.0,  0.0,  0.0, // bottom-left of right
        1.0,  0.0,  0.0, // bottom-right of right

        // back surface
        0.0,  0.0, -1.0, // top-right of back
        0.0,  0.0, -1.0, // top-left of back
        0.0,  0.0, -1.0, // bottom-left of back
        0.0,  0.0, -1.0, // bottom-right of back

        // left surface
        -1.0,  0.0,  0.0, // top-right of left
        -1.0,  0.0,  0.0, // top-left of left
        -1.0,  0.0,  0.0, // bottom-left of left
        -1.0,  0.0,  0.0, // bottom-right of left

        // top surface
        0.0,  1.0,  0.0, // top-right of top
        0.0,  1.0,  0.0, // top-left of top
        0.0,  1.0,  0.0, // bottom-left of top
        0.0,  1.0,  0.0, // bottom-right of top

        // bottom surface
        0.0, -1.0,  0.0, // top-right of bottom
        0.0, -1.0,  0.0, // top-left of bottom
        0.0, -1.0,  0.0, // bottom-left of bottom
        0.0, -1.0,  0.0, // bottom-right of bottom
    ]);
    
    vao_cube = gl.createVertexArray();
    gl.bindVertexArray(vao_cube);

    vbo_position_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_position_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_position, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    vbo_normal = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_normal);
    gl.bufferData(gl.ARRAY_BUFFER, cube_normal, gl.STATIC_DRAW);
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

    var modelViewMatrix = mat4.create();
    var translationMatrix = mat4.create();
    var modelViewProjectionMatrix = mat4.create();
    var rotationMatrix = mat4.create();

    mat4.rotateY(rotationMatrix, rotationMatrix, glMatrix.toRadian(cube_angle));
    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -4.0]);
    mat4.multiply(modelViewMatrix, translationMatrix, rotationMatrix);
    mat4.multiply(modelViewProjectionMatrix, perspectiveProjectionMatrix, modelViewMatrix);


    gl.uniformMatrix4fv(modelViewMatrixUniform, false, modelViewMatrix);
    gl.uniformMatrix4fv(projectionMatrixUniform, false, perspectiveProjectionMatrix);

    if (bLight == true) {
        gl.uniform3fv(LdUniform, lightDiffuse);
        gl.uniform3fv(KdUniform, materialDiffuse);
        gl.uniform4fv(lightPositionUniform, lightPosition);
        gl.uniform1i(LKeyPressUniform, 1);
    } else {
        gl.uniform1i(LKeyPressUniform, 0);
    }
    
    gl.bindVertexArray(vao_cube);
    gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 4, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 8, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 12, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 16, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 20, 4);
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
    if (vbo_position_cube) {
        gl.deleteBuffer(vbo_position_cube);
        vbo_position_cube = null;
    }
    if (vbo_normal) {
        gl.deleteBuffer(vbo_normal);
        vbo_normal = null;
    }
    if (vao_cube) {
        gl.deleteVertexArray(vao_cube);
        vao_cube = null;
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
