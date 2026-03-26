#include <gui.h>
#include <stdio.h>

int main() {
    CarleyWindow win("Calculator", 150, 150, 300, 400);

    // Background
    win.draw_rect(0, 0, 300, 400, 0x1A1A1A);

    // Display
    win.draw_rect(10, 10, 280, 50, 0x333333);

    // Buttons
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            char label[2];
            label[0] = '0' + (i * 4 + j); label[1] = 0;
            win.draw_button(10 + j * 70, 80 + i * 75, 60, 60, label, 0x444444);
        }
    }

    gui_event_t ev;
    while (1) {
        if (win.poll_event(&ev)) {
            if (ev.type == GUI_EVENT_CLICK) {
                // Logic for calculation
                printf("Calc: Click at %d, %d\n", ev.x, ev.y);
                // Feedback: re-draw the display area
                win.draw_rect(10, 10, 280, 50, 0x44CC44);
            }
        }
        // Yield is handled internally or implicitly
    }

    return 0;
}
