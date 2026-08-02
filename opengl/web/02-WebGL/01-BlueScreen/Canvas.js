var canvas = null;
var gl = null;
var bFullScreen = false;
var canvas_original_width = 0;
var canvas_original_height = 0;
var requestAnimationFrame = window.requestAnimationFrame || window.webkitRequestAnimationFrame || window.mozRequestAnimationFrame || window.oRequestAnimationFrame || window.msRequestAnimationFrame;

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

    // set clear color
    gl.clearColor(0.0, 0.0, 1.0, 1.0);
}

function display() {
    // clear the color buffer
    gl.clear(gl.COLOR_BUFFER_BIT);

    update();
    // double buffering using requestAnimationFrame
    requestAnimationFrame(display, canvas);
}

function update() {}

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
}

function uninitialize() {}

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
