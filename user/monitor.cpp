#include <gui.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

extern "C" long syscall1(int num, long arg1);

int main() {
    CarleyWindow win("Activity Monitor", 200, 100, 400, 500);

    win.draw_rect(0, 0, 400, 500, 0x1E1E1E);
    win.draw_text(10, 10, "PID   TASK NAME      STATUS", 0xFFFFFF);
    win.draw_rect(10, 25, 380, 2, 0x555555);

    // Simulated task list (until sys_get_tasks is implemented)
    struct {
        int pid;
        const char *name;
        const char *status;
    } tasks[] = {
        {1, "Composer", "Running"},
        {2, "InputSrv", "Running"},
        {102, "ShellGUI", "Active"},
        {205, "Explorer", "Sleeping"},
        {310, "Terminal", "Running"}
    };

    for (int i = 0; i < 5; i++) {
        char buf[64];
        sprintf(buf, "%d    %-14s %s", tasks[i].pid, tasks[i].name, tasks[i].status);
        win.draw_text(10, 40 + i * 30, buf, 0xAAAAAA);
        win.draw_button(320, 35 + i * 30, 60, 25, "Kill", 0xC0392B);
    }

    gui_event_t ev;
    while (1) {
        if (win.poll_event(&ev)) {
            if (ev.type == GUI_EVENT_CLICK) {
                printf("Monitor: Requested kill at %d, %d\n", ev.x, ev.y);
            }
        }
    }

    return 0;
}
