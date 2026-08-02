var canvas = null;
var context = null;
var bFullScreen = false;

function main() {
    // get canvas
    canvas = document.getElementById("lrc");
    if (canvas == null) console.log("Canvas element cannot be obtained\n");
    else console.log("Canvas element succesfully obtained\n");

    // get 2D context form canvas
    context = canvas.getContext("2d");
    if (context == null) console.log("Context element cannot be obtained\n");
    else console.log("Context element succesfully obtained\n");

    // tell the context to make canvas background color black
    context.fillStyle = "black";                // #000000
    context.fillRect(0, 0, canvas.width, canvas.height);

    // draw text
    drawText("Hello World!!!");

    // register our callback functions as event listeners
    window.addEventListener("keydown", keyDown, false);
    window.addEventListener("click", mouseDown, false);
}

function keyDown(event) {
    switch (event.keyCode) {
        case 70:
        case 102:
            if (bFullScreen == false) {
                toggleFullScreen();
                bFullScreen = true;
            } else {
                toggleFullScreen();
                bFullScreen = false;
            }
            drawText("Hello World!!!");
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

function drawText(text) {
    context.textAlign = "center";                   // horizontally
    context.textBaseline = "middle";                // text centered vertically
    context.font = "48px sans-serif";
    context.fillStyle = "lime";                      // state machine
    context.fillText(text, canvas.width/2, canvas.height/2);
}
