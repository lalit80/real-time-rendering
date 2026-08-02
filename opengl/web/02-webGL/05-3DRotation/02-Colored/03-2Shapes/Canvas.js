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
var vbo_color_cube = null;
var vao = null;
var vbo_position = null;
var vbo_color = null;
var cube_angle = 0.0;
var triangle_angle = 0.0;
var mvpUniform = null;
var perspectiveProjectionMatrix = null;

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
            "in vec4 aPosition; \n"+
            "in vec4 aColor; \n"+
            "out vec4 out_color; \n"+
            "uniform mat4 uMVPMatrix; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   gl_Position = uMVPMatrix * aPosition; \n"+
            "   out_color = aColor; \n"+
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
            "in vec4 out_color; \n"+
            "out vec4 fragColor; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   fragColor = out_color; \n"+
            "} \n";
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
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_COLOR, "aColor");
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
    mvpUniform = gl.getUniformLocation(shaderProgramObject, "uMVPMatrix");

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

    var cube_color = new Float32Array([
        // front
        1.0, 0.0, 0.0, // top-right of front
        1.0, 0.0, 0.0, // top-left of front
        1.0, 0.0, 0.0, // bottom-left of front
        1.0, 0.0, 0.0, // bottom-right of front

        // right
        0.0, 0.0, 1.0, // top-right of right
        0.0, 0.0, 1.0, // top-left of right
        0.0, 0.0, 1.0, // bottom-left of right
        0.0, 0.0, 1.0, // bottom-right of right

        // back
        1.0, 1.0, 0.0, // top-right of back
        1.0, 1.0, 0.0, // top-left of back
        1.0, 1.0, 0.0, // bottom-left of back
        1.0, 1.0, 0.0, // bottom-right of back

        // left
        1.0, 0.0, 1.0, // top-right of left
        1.0, 0.0, 1.0, // top-left of left
        1.0, 0.0, 1.0, // bottom-left of left
        1.0, 0.0, 1.0, // bottom-right of left

        // top
        0.0, 1.0, 0.0, // top-right of top
        0.0, 1.0, 0.0, // top-left of top
        0.0, 1.0, 0.0, // bottom-left of top
        0.0, 1.0, 0.0, // bottom-right of top

        // bottom
        1.0, 0.5, 0.0, // top-right of bottom
        1.0, 0.5, 0.0, // top-left of bottom
        1.0, 0.5, 0.0, // bottom-left of bottom
        1.0, 0.5, 0.0, // bottom-right of bottom
    ]);

    // triangle position, color, vao, vbo
    var traingle_position = new Float32Array([
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
    var traingle_color = new Float32Array([
        // front
        1.0, 0.0, 0.0, // front-top
        0.0, 1.0, 0.0, // front-left
        0.0, 0.0, 1.0, // front-right
        
        // right
        1.0, 0.0, 0.0, // right-top
        0.0, 0.0, 1.0, // right-left
        0.0, 1.0, 0.0, // right-right
        
        // back
        1.0, 0.0, 0.0, // back-top
        0.0, 1.0, 0.0, // back-left
        0.0, 0.0, 1.0, // back-right
        
        // left
        1.0, 0.0, 0.0, // left-top
        0.0, 0.0, 1.0, // left-left
        0.0, 1.0, 0.0, // left-right
    ]);
    
    vao_cube = gl.createVertexArray();
    gl.bindVertexArray(vao_cube);

    vbo_position_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_position_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_position, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    vbo_color_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_color_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_color, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_COLOR, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_COLOR);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    
    gl.bindVertexArray(null);

    vao = gl.createVertexArray();
    gl.bindVertexArray(vao);

    vbo_position = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_position);
    gl.bufferData(gl.ARRAY_BUFFER, traingle_position, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    vbo_color = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_color);
    gl.bufferData(gl.ARRAY_BUFFER, traingle_color, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_COLOR, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_COLOR);
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
    mat4.translate(translationMatrix, translationMatrix, [1.5, 0.0, -6.0]);
    mat4.multiply(modelViewMatrix, translationMatrix, rotationMatrix);
    mat4.multiply(modelViewProjectionMatrix, perspectiveProjectionMatrix, modelViewMatrix);
    gl.uniformMatrix4fv(mvpUniform, false, modelViewProjectionMatrix);
    
    gl.bindVertexArray(vao_cube);
    gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 4, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 8, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 12, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 16, 4);
    gl.drawArrays(gl.TRIANGLE_FAN, 20, 4);
    gl.bindVertexArray(null);

    var modelViewMatrix = mat4.create();
    var translationMatrix = mat4.create();
    var modelViewProjectionMatrix = mat4.create();
    var rotationMatrix = mat4.create();
    mat4.rotateY(rotationMatrix, rotationMatrix, glMatrix.toRadian(triangle_angle));
    mat4.translate(translationMatrix, translationMatrix, [-1.5, 0.0, -6.0]);
    mat4.multiply(modelViewMatrix, translationMatrix, rotationMatrix);
    mat4.multiply(modelViewProjectionMatrix, perspectiveProjectionMatrix, modelViewMatrix);
    gl.uniformMatrix4fv(mvpUniform, false, modelViewProjectionMatrix);
    
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
    triangle_angle += 0.5;
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
    if (vbo_color_cube) {
        gl.deleteBuffer(vbo_color_cube);
        vbo_color_cube = null;
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
