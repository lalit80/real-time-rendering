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
var vbo_cube = null;
var cube_angle = 0.0;

var modelMatrixUniform = null;
var viewMatrixUniform = null;
var projectionMatrixUniform = null;
var LaUniform = null;
var LdUniform = null;
var LsUniform = null;
var KaUniform = null;
var KdUniform = null;
var KsUniform = null;
var materialShininessUniform = null;
var lightPositionUniform = null;
var LKeyPressUniform = null;

var textureSamplerUniform = null;
var texture_marble = null;

var perspectiveProjectionMatrix = null;

var lightAmbient = [0.0, 0.0, 0.0];
var lightDiffuse = [1.0, 1.0, 1.0];
var lightSpecular = [1.0, 1.0, 1.0];
var lightPosition = [100.0, 100.0, 100.0, 1.0];

var materialAmbient = [0.25, 0.25, 0.25];
var materialDiffuse = [1.0, 1.0, 1.0];
var materialSpecular = [1.0, 1.0, 1.0];
var materialShininess = 128.0;
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
            "in vec4 aColor; \n"+
            "in vec2 aTexCoord; \n"+
            "out vec4 out_color; \n"+
            "out vec2 out_texcoord; \n"+
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
            "   gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * aPosition; \n"+
            "   if (uLKeyIsPressed == 1) { \n"+
            "       vec4 eyeCoordinates = uViewMatrix * uModelMatrix * aPosition; \n"+
            "       mat3 normalMatrix = mat3(uViewMatrix * uModelMatrix); \n"+
            "       out_transformedNormals = normalMatrix * aNormal; \n"+
            "       out_lightDirection = vec3(uLightPosition - eyeCoordinates); \n"+
            "       out_viewerVector = -eyeCoordinates.xyz; \n"+
            "   } \n"+
            "   out_color = aColor; \n"+
            "   out_texcoord = aTexCoord; \n"+
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
            "in vec4 out_color; \n"+
            "in vec2 out_texcoord; \n"+
            "uniform sampler2D uTextureSampler; \n"+
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
            "   vec3 phong_ads_light; \n"+
            "   if (uLKeyIsPressed == 1) { \n"+
            "       vec3 normalizedTransformedNormals = normalize(out_transformedNormals); \n"+
            "       vec3 normalizedLightDirection = normalize(out_lightDirection); \n"+
            "       vec3 normalizedViewerVector = normalize(out_viewerVector); \n"+
            "       vec3 ambientLight = uLa * uKa * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
            "       vec3 diffuseLight = uLd * uKd * max(dot(normalizedLightDirection, normalizedTransformedNormals), 0.0f); \n"+
            "       vec3 reflectionVector = reflect(-normalizedLightDirection, normalizedTransformedNormals); \n"+
            "       vec3 specularLight = uLs * uKs * pow(max(dot(reflectionVector, normalizedViewerVector), 0.0f), uMaterialShininess); \n"+
            "       phong_ads_light = ambientLight + diffuseLight + specularLight; \n"+
            "   } \n"+
            "   else { \n"+
            "       phong_ads_light = vec3(1.0, 1.0, 1.0); \n"+
            "   } \n"+
            "   vec4 tex = texture(uTextureSampler, out_texcoord); \n"+
            "   vec4 texColor = out_color * tex; \n"+
            "   FragColor = vec4(phong_ads_light, 1.0) * texColor; \n"+
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
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_COLOR, "aColor");
    gl.bindAttribLocation(shaderProgramObject, MyAttributes.LRC_ATTRIBUTE_TEXCOORD, "aTexCoord");
    
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
    LaUniform = gl.getUniformLocation(shaderProgramObject, "uLa");
    LdUniform = gl.getUniformLocation(shaderProgramObject, "uLd");
    LsUniform = gl.getUniformLocation(shaderProgramObject, "uLs");
    lightPositionUniform = gl.getUniformLocation(shaderProgramObject, "uLightPosition");
    KaUniform = gl.getUniformLocation(shaderProgramObject, "uKa");
    KdUniform = gl.getUniformLocation(shaderProgramObject, "uKd");
    KsUniform = gl.getUniformLocation(shaderProgramObject, "uKs");
    materialShininessUniform = gl.getUniformLocation(shaderProgramObject, "uMaterialShininess");
    LKeyPressUniform = gl.getUniformLocation(shaderProgramObject, "uLKeyIsPressed");
    textureSamplerUniform = gl.getUniformLocation(shaderProgramObject, "uTextureSampler");

    const cube_PCNT = new Float32Array([
        // front
        // position             // color            // normals              // texcoords
         1.0,  1.0,  1.0,    1.0, 0.0, 0.0,     0.0,  0.0,  1.0,    1.0, 1.0,
        -1.0,  1.0,  1.0,    1.0, 0.0, 0.0,     0.0,  0.0,  1.0,    0.0, 1.0,
        -1.0, -1.0,  1.0,    1.0, 0.0, 0.0,     0.0,  0.0,  1.0,    0.0, 0.0,
         1.0, -1.0,  1.0,    1.0, 0.0, 0.0,     0.0,  0.0,  1.0,    1.0, 0.0,
                         
        // right
         1.0,  1.0, -1.0,    0.0, 0.0, 1.0,     1.0,  0.0,  0.0,    1.0, 1.0,
         1.0,  1.0,  1.0,    0.0, 0.0, 1.0,     1.0,  0.0,  0.0,    0.0, 1.0,
         1.0, -1.0,  1.0,    0.0, 0.0, 1.0,     1.0,  0.0,  0.0,    0.0, 0.0,
         1.0, -1.0, -1.0,    0.0, 0.0, 1.0,     1.0,  0.0,  0.0,    1.0, 0.0,
                         
        // back
         1.0,  1.0, -1.0,    1.0, 1.0, 0.0,     0.0,  0.0, -1.0,    1.0, 1.0,
        -1.0,  1.0, -1.0,    1.0, 1.0, 0.0,     0.0,  0.0, -1.0,    0.0, 1.0,
        -1.0, -1.0, -1.0,    1.0, 1.0, 0.0,     0.0,  0.0, -1.0,    0.0, 0.0,
         1.0, -1.0, -1.0,    1.0, 1.0, 0.0,     0.0,  0.0, -1.0,    1.0, 0.0,
                         
        // left
        -1.0,  1.0,  1.0,    1.0, 0.0, 1.0,    -1.0,  0.0,  0.0,    1.0, 1.0,
        -1.0,  1.0, -1.0,    1.0, 0.0, 1.0,    -1.0,  0.0,  0.0,    0.0, 1.0,
        -1.0, -1.0, -1.0,    1.0, 0.0, 1.0,    -1.0,  0.0,  0.0,    0.0, 0.0,
        -1.0, -1.0,  1.0,    1.0, 0.0, 1.0,    -1.0,  0.0,  0.0,    1.0, 0.0,
                            
        // top
         1.0,  1.0, -1.0,    0.0, 1.0, 0.0,     0.0,  1.0,  0.0,    1.0, 1.0,
        -1.0,  1.0, -1.0,    0.0, 1.0, 0.0,     0.0,  1.0,  0.0,    0.0, 1.0,
        -1.0,  1.0,  1.0,    0.0, 1.0, 0.0,     0.0,  1.0,  0.0,    0.0, 0.0,
         1.0,  1.0,  1.0,    0.0, 1.0, 0.0,     0.0,  1.0,  0.0,    1.0, 0.0,
                         
        // bottom
         1.0, -1.0,  1.0,    1.0, 0.5, 0.0,     0.0, -1.0,  0.0,    1.0, 1.0,
        -1.0, -1.0,  1.0,    1.0, 0.5, 0.0,     0.0, -1.0,  0.0,    0.0, 1.0,
        -1.0, -1.0, -1.0,    1.0, 0.5, 0.0,     0.0, -1.0,  0.0,    0.0, 0.0,
         1.0, -1.0, -1.0,    1.0, 0.5, 0.0,     0.0, -1.0,  0.0,    1.0, 0.0,
    ]);
    
    vao_cube = gl.createVertexArray();
    gl.bindVertexArray(vao_cube);

    vbo_cube = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vbo_cube);
    gl.bufferData(gl.ARRAY_BUFFER, cube_PCNT, gl.STATIC_DRAW);
    
    // Position (3 floats at offset 0)
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_POSITION, 3, gl.FLOAT, false, 11 * 4, 0);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_POSITION);

    // Color (3 floats at offset 3*4)
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_COLOR, 3, gl.FLOAT, false, 11 * 4, 3 * 4);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_COLOR);
    
    // Normal (3 floats at offset 6*4)
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_NORMAL, 3, gl.FLOAT, false, 11 * 4, 6 * 4);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_NORMAL);

    // Texture (2 floats at offset 9*4)
    gl.vertexAttribPointer(MyAttributes.LRC_ATTRIBUTE_TEXCOORD, 2, gl.FLOAT, false, 11 * 4, 9 * 4);
    gl.enableVertexAttribArray(MyAttributes.LRC_ATTRIBUTE_TEXCOORD);

    gl.bindBuffer(gl.ARRAY_BUFFER, null);

    gl.bindVertexArray(null);

    // enable depth
    gl.clearDepth(1.0);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);

    perspectiveProjectionMatrix = mat4.create();

    texture_marble = loadGLTexture("marble.png");

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
    var rotationMatrix = mat4.create();
    var rotationX = mat4.create();
    var rotationY = mat4.create();
    var rotationZ = mat4.create();
    var scaleMatrix = mat4.create();

    mat4.scale(scaleMatrix, scaleMatrix, [0.75, 0.75, 0.75]);
    mat4.translate(translationMatrix, translationMatrix, [0.0, 0.0, -5.0]);
    mat4.rotateX(rotationX, rotationX, glMatrix.toRadian(cube_angle));
    mat4.rotateY(rotationY, rotationY, glMatrix.toRadian(cube_angle));
    mat4.rotateZ(rotationZ, rotationZ, glMatrix.toRadian(cube_angle));
    
    mat4.multiply(rotationMatrix, rotationX, rotationY);
    mat4.multiply(rotationMatrix, rotationMatrix, rotationZ);
    
    mat4.identity(modelMatrix);
    mat4.multiply(modelMatrix, modelMatrix, translationMatrix); // M = T
    mat4.multiply(modelMatrix, modelMatrix, scaleMatrix);       // M = T * S
    mat4.multiply(modelMatrix, modelMatrix, rotationMatrix);    // M = T * S * R

    // send matrices
    gl.uniformMatrix4fv(modelMatrixUniform, false, modelMatrix);
    gl.uniformMatrix4fv(viewMatrixUniform, false, viewMatrix);
    gl.uniformMatrix4fv(projectionMatrixUniform, false, perspectiveProjectionMatrix);

    if (bLight == true) {
        gl.uniform3fv(LaUniform, lightAmbient);
        gl.uniform3fv(LdUniform, lightDiffuse);
        gl.uniform3fv(LsUniform, lightSpecular);
        gl.uniform4fv(lightPositionUniform, lightPosition);
        gl.uniform3fv(KaUniform, materialAmbient);
        gl.uniform3fv(KdUniform, materialDiffuse);
        gl.uniform3fv(KsUniform, materialSpecular);
        gl.uniform1f(materialShininessUniform, materialShininess);
        gl.uniform1i(LKeyPressUniform, 1);
    } else {
        gl.uniform1i(LKeyPressUniform, 0);
    }

    // Texture Bindings
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, texture_marble);
    gl.uniform1i(textureSamplerUniform, 0);
    
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

function loadGLTexture(imageFileName) {
    var tex = gl.createTexture();
    tex.image = new Image();
    tex.image.src = imageFileName;
    tex.image.onload = function () {
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
        gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);                      // gl.nearest
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, tex.image);
        gl.generateMipmap(gl.TEXTURE_2D);
        gl.bindTexture(gl.TEXTURE_2D, null);
    }
    return tex;
}
