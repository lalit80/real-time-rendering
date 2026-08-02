var canvas = null;
var gl = null;
var bFullScreen = false;
var canvas_original_width = 0;
var canvas_original_height = 0;
var requestAnimationFrame = window.requestAnimationFrame || window.webkitRequestAnimationFrame || window.mozRequestAnimationFrame || window.oRequestAnimationFrame || window.msRequestAnimationFrame;

const WIN_WIDTH = 800;
const WIN_HEIGHT = 600;
const FBO_WIDTH = 512;
const FBO_HEIGHT = 512;

// webGL related variables
const MyAttributes = {
    LRC_ATTRIBUTE_POSITION: 0,
    LRC_ATTRIBUTE_COLOR: 1,
    LRC_ATTRIBUTE_NORMAL: 2,
    LRC_ATTRIBUTE_TEXCOORD: 3,
};

// shader Objects
var shaderProgramObject = null;
var sphereShaderProgramObject = null;

var vao_cube = null;
var vbo_position_cube = null;
var vbo_texcoord_cube = null;
var cube_angle = 0.0;
var mvpUniform = null;
var textureSamplerUniform_cube = null;

var sphere = null;

var modelMatrixUniform_sphere = null;
var viewMatrixUniform_sphere = null;
var projectionMatrixUniform_sphere = null;

var laUniform = null;
var ldUniform = null;
var lsUniform = null;
var lightPositionUniform = null;
var kaUniform = null;
var kdUniform = null;
var ksUniform = null;
var materialShininessUniform = null;
var lightingEnabledUniform = null;
var bLight = false;

// FBO Variables
var fbo = null;
var rbo = null;
var fbo_texture = null;
var fbo_result = false;

var perspectiveProjectionMatrix = null;
var perspectiveProjectionMatrix_sphere = null;

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

    // shaders
    var vertexShaderSourceCode = 
            "#version 300 es \n"+    
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec4 aPosition; \n"+
            "in vec2 aTexCoord; \n"+
            "out vec2 out_texcoord; \n"+
            "uniform mat4 uMVPMatrix; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   gl_Position = uMVPMatrix * aPosition; \n"+
            "   out_texcoord = aTexCoord; \n"+
            "} \n";

    var vertexShaderObject = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertexShaderObject, vertexShaderSourceCode);
    gl.compileShader(vertexShaderObject);
    if (gl.getShaderParameter(vertexShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(vertexShaderObject);
        if (error.length > 0) {
            alert("Cube Vertex Shader Compilation Error: " + error);
            uninitialize();
        }
    }

    var fragmentShaderSourceCode = 
            "#version 300 es \n"+    
            "precision highp float; \n"+
            "precision highp int; \n"+
            "in vec2 out_texcoord; \n"+
            "uniform sampler2D uTextureSampler; \n"+
            "out vec4 fragColor; \n"+
            "void main(void) \n"+
            "{ \n"+
            "   fragColor = texture(uTextureSampler, out_texcoord); \n"+
            "} \n";

    var fragmentShaderObject = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragmentShaderObject, fragmentShaderSourceCode);
    gl.compileShader(fragmentShaderObject);
    if (gl.getShaderParameter(fragmentShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(fragmentShaderObject);
        if (error.length > 0) {
            alert("Cube Fragment Shader Compilation Error: " + error);
            uninitialize();
        }
    } else {
        console.log("Cube Fragment Shader Compilation Successful.\n");
    }

    shaderProgramObject = gl.createProgram();
    gl.attachShader(shaderProgramObject, vertexShaderObject);
    gl.attachShader(shaderProgramObject, fragmentShaderObject);
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_POSITION, "aPosition");
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_TEXCOORD, "aTexCoord");
    gl.linkProgram(shaderProgramObject);
    if (gl.getProgramParameter(shaderProgramObject, gl.LINK_STATUS) == false) {
        var error = gl.getProgramInfoLog(shaderProgramObject);
        if (error.length > 0) {
            alert("Cube Shader Program Linking Error: " + error);
            uninitialize();
        }
    } else {
        console.log("Cube Shader Program Linking Successful.\n");
    }

    mvpUniform = gl.getUniformLocation(shaderProgramObject, "uMVPMatrix");
    textureSamplerUniform_cube = gl.getUniformLocation(shaderProgramObject, "uTextureSampler");

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

    var cube_texcoord = new Float32Array([
        // front
        1.0, 1.0, 
        0.0, 1.0,
        0.0, 0.0, 
        1.0, 0.0,
        // right
        1.0, 1.0, 
        0.0, 1.0, 
        0.0, 0.0, 
        1.0, 0.0,
        // back
        1.0, 1.0, 
        0.0, 1.0, 
        0.0, 0.0, 
        1.0, 0.0,
        // left
        1.0, 1.0, 
        0.0, 1.0, 
        0.0, 0.0, 
        1.0, 0.0,
        // top
        1.0, 1.0, 
        0.0, 1.0, 
        0.0, 0.0, 
        1.0, 0.0,
        // bottom
        1.0, 1.0, 
        0.0, 1.0, 
        0.0, 0.0, 
        1.0, 0.0,
    ]);
    
    vao_cube = gl.createVertexArray();
    gl.bindVertexArray(vao_cube);

    vbo_position_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_position_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_position, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    vbo_texcoord_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_texcoord_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_texcoord, gl.STATIC_DRAW);
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_TEXCOORD, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_TEXCOORD);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    
    gl.bindVertexArray(null);

    // enable depth
    gl.clearDepth(1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);

    perspectiveProjectionMatrix = mat4.create();

    // set clear color
    gl.clearColor(0.0, 0.0, 0.0, 1.0);

    if(createFBO(FBO_WIDTH, FBO_HEIGHT) == true) {
        console.log("FBO Created Successfully\n");
        // Initialize sphere shader and data
        fbo_result = initialize_sphere();
        if (fbo_result == true) {
            console.log("FBO initialize_sphere successfull\n");
        } else {
            console.log("FBO initialize_sphere failed\n");
        }
    } else {
        console.log("FBO Creation Failed\n");
        fbo_result = false;
        uninitialize();
        window.close();
    }
}

function initialize_sphere() {
    var vertexShaderSourceCode = 
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
        "uniform int uLightingEnabled; \n"+
        "void main(void) \n"+
        "{ \n"+
        "   gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
        "   if (uLightingEnabled == 1) { \n"+
        "       vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
        "       mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
        "       out_transformedNormals = normalMatrix * aNormal; \n"+
        "       out_lightDirection = vec3(uLightPosition - eyeCoordinates); \n"+
        "       out_viewerVector = -eyeCoordinates.xyz; \n"+
        "   } \n"+
        "} \n";

    var vertexShaderObject = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertexShaderObject, vertexShaderSourceCode);
    gl.compileShader(vertexShaderObject);
    if (gl.getShaderParameter(vertexShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(vertexShaderObject);
        if (error.length > 0) {
            alert("Sphere Vertex Shader Compilation Error: " + error);
            uninitialize();
        }
    }

    var fragmentShaderSourceCode = 
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
        "uniform int uLightingEnabled; \n"+
        "void main(void) \n"+
        "{ \n"+
        "   vec3 phong_ads_light; \n"+
        "   if (uLightingEnabled == 1) { \n"+
        "       vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
        "       vec3 normalizedLightDirection = normalize(out_lightDirection); \n"+
        "       vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
        "       vec3 ambientLight = uLa * uKa; \n"+
        "       vec3 diffuseLight = uLd * uKd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0); \n"+
        "       vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n"+
        "       vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0), uMaterialShininess); \n"+
        "       phong_ads_light = ambientLight + diffuseLight + specularLight; \n"+
        "   } else { \n"+
        "       phong_ads_light = vec3(1.0, 1.0, 1.0); \n"+
        "   } \n"+
        "   FragColor = vec4(phong_ads_light, 1.0); \n"+
        "} \n";

    var fragmentShaderObject = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragmentShaderObject, fragmentShaderSourceCode);
    gl.compileShader(fragmentShaderObject);
    if (gl.getShaderParameter(fragmentShaderObject, gl.COMPILE_STATUS) == false) {
        var error = gl.getShaderInfoLog(fragmentShaderObject);
        if (error.length > 0) {
            alert("Sphere Fragment Shader Compilation Error: " + error);
            uninitialize();
        }
    } else {
        console.log("Sphere Fragment Shader Compilation Successful.\n");
    }

    sphereShaderProgramObject = gl.createProgram();
    gl.attachShader(sphereShaderProgramObject, vertexShaderObject);
    gl.attachShader(sphereShaderProgramObject, fragmentShaderObject);
    
    // Bind attributes to match Mesh.js
    gl.bindAttribLocation(sphereShaderProgramObject, MyAttributes.LRC_ATTRIBUTE_POSITION, "aPosition");
    gl.bindAttribLocation(sphereShaderProgramObject, MyAttributes.LRC_ATTRIBUTE_NORMAL, "aNormal");
    
    gl.linkProgram(sphereShaderProgramObject);
    if (gl.getProgramParameter(sphereShaderProgramObject, gl.LINK_STATUS) == false) {
        var error = gl.getProgramInfoLog(sphereShaderProgramObject);
        if (error.length > 0) {
            alert("Sphere Shader Program Linking Error: " + error);
            uninitialize();
        }
    } else {
        console.log("Sphere Shader Program Linking Successful.\n");
    }

    modelMatrixUniform_sphere = gl.getUniformLocation(sphereShaderProgramObject, "uModelMatrix");
    viewMatrixUniform_sphere = gl.getUniformLocation(sphereShaderProgramObject, "uViewMatrix");
    projectionMatrixUniform_sphere = gl.getUniformLocation(sphereShaderProgramObject, "uProjectionMatrix");
    
    laUniform = gl.getUniformLocation(sphereShaderProgramObject, "uLa");
    ldUniform = gl.getUniformLocation(sphereShaderProgramObject, "uLd");
    lsUniform = gl.getUniformLocation(sphereShaderProgramObject, "uLs");
    lightPositionUniform = gl.getUniformLocation(sphereShaderProgramObject, "uLightPosition");
    kaUniform = gl.getUniformLocation(sphereShaderProgramObject, "uKa");
    kdUniform = gl.getUniformLocation(sphereShaderProgramObject, "uKd");
    ksUniform = gl.getUniformLocation(sphereShaderProgramObject, "uKs");
    materialShininessUniform = gl.getUniformLocation(sphereShaderProgramObject, "uMaterialShininess");
    lightingEnabledUniform = gl.getUniformLocation(sphereShaderProgramObject, "uLightingEnabled");

    sphere = new Mesh();
	makeSphere(sphere, 1.0, 30, 30);

    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    
    perspectiveProjectionMatrix_sphere = mat4.create();
    mat4.perspective(perspectiveProjectionMatrix_sphere, 
                    45.0 * Math.PI / 180.0,
                    parseFloat(FBO_WIDTH) / parseFloat(FBO_HEIGHT), 
                    0.1, 
                    100.0);

    return true;
}

function display() {
    // render sphere to FBO
    if(fbo_result == true) {
        displaySphere();
    }

    gl.viewport(0, 0, canvas.width, canvas.height);

    // clear the color buffer
    gl.clearColor(1.0, 1.0, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.useProgram(shaderProgramObject);

    var modelViewMatrix = mat4.create();
    var translationMatrix = mat4.create();
    var modelViewProjectionMatrix = mat4.create();
    var rotationMatrix = mat4.create();

    mat4.rotateX(rotationMatrix, rotationMatrix, glMatrix.toRadian(cube_angle)); // Rotate X
    mat4.rotateY(rotationMatrix, rotationMatrix, glMatrix.toRadian(cube_angle)); // Rotate Y
    mat4.rotateZ(rotationMatrix, rotationMatrix, glMatrix.toRadian(cube_angle)); // Rotate Z
    
    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -6.0]);
    mat4.multiply(modelViewMatrix, translationMatrix, rotationMatrix);
    mat4.multiply(modelViewProjectionMatrix, perspectiveProjectionMatrix, modelViewMatrix);
    gl.uniformMatrix4fv(mvpUniform, false, modelViewProjectionMatrix);

    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, fbo_texture);
    gl.uniform1i(textureSamplerUniform_cube, 0);
    
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

function displaySphere() {
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    
    gl.viewport(0, 0, FBO_WIDTH, FBO_HEIGHT);
    
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.useProgram(sphereShaderProgramObject);

    var modelMatrix = mat4.create();
    var viewMatrix = mat4.create();
    var translationMatrix = mat4.create();

    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -4.0]);
    mat4.multiply(modelMatrix, modelMatrix, translationMatrix);

    gl.uniformMatrix4fv(modelMatrixUniform_sphere, false, modelMatrix);
    gl.uniformMatrix4fv(viewMatrixUniform_sphere, false, viewMatrix);
    gl.uniformMatrix4fv(projectionMatrixUniform_sphere, false, perspectiveProjectionMatrix_sphere);

    // lighting uniforms
    if (bLight) {
        gl.uniform1i(lightingEnabledUniform, 1);
        gl.uniform3fv(laUniform, [0.1, 0.1, 0.1]);
        gl.uniform3fv(ldUniform, [1.0, 1.0, 1.0]);
        gl.uniform3fv(lsUniform, [1.0, 1.0, 1.0]);
        gl.uniform4fv(lightPositionUniform, [100.0, 100.0, 100.0, 1.0]);
        
        gl.uniform3fv(kaUniform, [0.0, 0.0, 0.0]);
        gl.uniform3fv(kdUniform, [0.5, 0.2, 0.7]);
        gl.uniform3fv(ksUniform, [0.7, 0.7, 0.7]);
        gl.uniform1f(materialShininessUniform, 128.0);
    } else {
        gl.uniform1i(lightingEnabledUniform, 0);
    }

    sphere.draw();

    gl.useProgram(null);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
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
    mat4.perspective(perspectiveProjectionMatrix, 45.0 * Math.PI / 180.0, parseFloat(canvas.width) / parseFloat(canvas.height), 0.1, 100.0);
}

function uninitialize() {
    if (bFullScreen == true)
        toggleFullScreen();
    if (vbo_position_cube) {
        gl.deleteBuffer(vbo_position_cube);
        vbo_position_cube = null;
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

        case 76: // 'L'
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

function createFBO(width, height) {
    fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);

    fbo_texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, fbo_texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, fbo_texture, 0);

    rbo = gl.createRenderbuffer();
    gl.bindRenderbuffer(gl.RENDERBUFFER, rbo);
    gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, width, height);
    gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, rbo);

    var status = gl.checkFramebufferStatus(gl.FRAMEBUFFER);
    if (status != gl.FRAMEBUFFER_COMPLETE) {
        return false;
    }
    
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    return true;
}
