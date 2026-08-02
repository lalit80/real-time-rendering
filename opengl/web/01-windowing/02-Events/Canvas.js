function main() {
    // get canvas
    var canvas = document.getElementById("lrc");
    if (canvas == null) console.log("Canvas element cannot be obtained\n");
    else console.log("Canvas element succesfully obtained\n");

    // get 2D context form canvas
    var context = canvas.getContext("2d");
    if (context == null) console.log("Context element cannot be obtained\n");
    else console.log("Context element succesfully obtained\n");

    // tell the context to make canvas background color black
    context.fillStyle = "black";                // #000000
    context.fillRect(0, 0, canvas.width, canvas.height);

    // draw text
    context.textAlign = "center";                   // horizontally
    context.textBaseline = "middle";                // text centered vertically
    context.font = "48px sans-serif";
    context.fillStyle = "lime";                      // state machine
    var str = "Hello World!!!";
    context.fillText(str, canvas.width/2, canvas.height/2);

    // register our callback functions as event listeners
    window.addEventListener("keydown", keyDown, false);
    window.addEventListener("click", mouseDown, false);
}

function keyDown(event) {
    alert("key is pressed");
}

function mouseDown() {
    alert("mouse is clicked");
}
