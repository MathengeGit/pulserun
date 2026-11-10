#pragma once

// Input for one display frame, filled by Window::poll_events().
// "Held" flags describe the current state; "pressed" flags are edges that are
// true only for the frame the key or button went down (key repeat is ignored).
struct Input {
    bool jumpHeld = false;     // Space, Up, W, left mouse button or a touch is down
    bool jumpPressed = false;  // one of those went down this frame

    bool escPressed = false;     // Esc or P: pause / back
    bool enterPressed = false;   // Enter: activate the focused button
    bool upPressed = false, downPressed = false, leftPressed = false, rightPressed = false;
    bool restartPressed = false;  // R
    bool hitboxPressed = false;   // H: toggle the collision overlay
    bool focusLost = false;       // window lost focus (the game auto-pauses)

    bool clicked = false;           // pointer went down this frame
    int pointerX = -1, pointerY = -1;  // in framebuffer pixels (-1 when unknown)

    // Clear the one-frame edges (called after the game has consumed them).
    void clearEdges() {
        jumpPressed = escPressed = enterPressed = false;
        upPressed = downPressed = leftPressed = rightPressed = false;
        restartPressed = hitboxPressed = focusLost = clicked = false;
    }

    // Take the newest held state and pointer from `src`, and OR in its edges, so an edge
    // that arrives between two fixed updates is kept until an update consumes it.
    void merge(const Input& src) {
        jumpHeld = src.jumpHeld;
        jumpPressed |= src.jumpPressed;
        escPressed |= src.escPressed;
        enterPressed |= src.enterPressed;
        upPressed |= src.upPressed;
        downPressed |= src.downPressed;
        leftPressed |= src.leftPressed;
        rightPressed |= src.rightPressed;
        restartPressed |= src.restartPressed;
        hitboxPressed |= src.hitboxPressed;
        focusLost |= src.focusLost;
        if (src.clicked) clicked = true;
        if (src.pointerX >= 0) {
            pointerX = src.pointerX;
            pointerY = src.pointerY;
        }
    }
};
