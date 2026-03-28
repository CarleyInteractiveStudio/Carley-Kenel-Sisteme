#include <stdint.h>
#include <ipc.h>

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#include <string.h>

#define COMPOSER_DRAW_RECT     2
#define COMPOSER_DRAW_CHAR     3
#define COMPOSER_CREATE_WINDOW 10

#define SYS_AUDIO_PLAY 13
#define SYS_YIELD     0
#define SYS_IPC_SEND  1
#define SYS_IPC_RECV  2
#define SYS_SPAWN     10

extern "C" long syscall0(int num);
extern "C" long syscall1(int num, long arg1);
extern "C" long syscall3(int num, long arg1, long arg2, long arg3);

// App Definition
typedef struct {
    const char *name;
    const char *elf;
    uint32_t icon_color;
} app_t;

app_t apps[] = {
    {"Explorer", "explorer.elf", 0x3498DB},
    {"Terminal", "term.elf",     0x2ECC71},
    {"Settings", "settings.elf", 0x95A5A6},
    {"About",    "about.elf",    0xE74C3C},
    {"Snake",    "game.elf",     0xF1C40F},
    {"Editor",   "edit.elf",     0x8E44AD}
};

bool menu_open = false;
bool notifications_open = false;
bool settings_open = false;

char dynamic_notif[64] = "1. Welcome to Carley!";
char search_query[32] = "";
char active_app_name[32] = "Desktop";

void draw_dashboard() {
    ipc_msg_t msg;
    msg.sender = 1001;

    // 1. Draw Global Top Menu Bar
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 0; msg.data[1] = 0; msg.data[2] = 800; msg.data[3] = 30;
    msg.data[4] = 0xCC1A1A1A; // Translucent dark
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    msg.data[3] = 0xFFFFFF;

    // Carley Logo / Menu
    const char *logo = "CARLEY";
    msg.data[1] = 10; msg.data[2] = 10;
    for(int j=0; logo[j]; j++) {
        msg.data[0] = logo[j];
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
        msg.data[1] += 8;
    }

    // Battery Icon & Percentage (Top Right)
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 680; msg.data[1] = 8; msg.data[2] = 20; msg.data[3] = 12;
    msg.data[4] = 0xFF2ECC71; // Green
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    msg.type = COMPOSER_DRAW_CHAR;
    const char *bat_val = "85%";
    msg.data[1] = 705; msg.data[2] = 10;
    msg.data[3] = 0xFFFFFF;
    for(int j=0; bat_val[j]; j++) {
        msg.data[0] = bat_val[j];
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
        msg.data[1] += 8;
    }

    // Active App Name
    msg.data[1] = 80; msg.data[2] = 10;
    msg.data[3] = 0xAAAAAA; // Gray for app name
    for(int j=0; active_app_name[j]; j++) {
        msg.data[0] = active_app_name[j];
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
        msg.data[1] += 8;
    }

    // Right Side: Time & Status
    const char *time_str = "12:00 PM"; // Simulated for now
    msg.data[1] = 730; msg.data[2] = 10;
    for(int j=0; time_str[j]; j++) {
        msg.data[0] = time_str[j];
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
        msg.data[1] += 8;
    }

    // 2. Draw Desktop Shortcuts
    for (int i = 0; i < 3; i++) {
        msg.type = COMPOSER_DRAW_RECT;
        msg.data[0] = 20; msg.data[1] = 50 + i * 100; msg.data[2] = 60; msg.data[3] = 60;
        msg.data[4] = apps[i].icon_color;
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = 20; msg.data[2] = 115 + i * 100; msg.data[3] = 0xFFFFFF;
        for(int j=0; apps[i].name[j]; j++) {
            msg.data[0] = apps[i].name[j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
    }

    // Draw Bottom Dock Bubble
    msg.type = COMPOSER_DRAW_RECT;
    msg.data[0] = 250; msg.data[1] = 520; msg.data[2] = 300; msg.data[3] = 60;
    msg.data[4] = 0xAA222222; // Semi-transparent dark
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    // Draw Main Button (Center)
    msg.data[0] = 375; msg.data[1] = 525; msg.data[2] = 50; msg.data[3] = 50;
    msg.data[4] = 0xFFFFFFFF; // White circle/bubble
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    if (notifications_open) {
        // Left Pull-down (Notifications)
        msg.data[0] = 0; msg.data[1] = 0; msg.data[2] = 250; msg.data[3] = 400;
        msg.data[4] = 0xEE1A1A1A;
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = 20; msg.data[2] = 50; msg.data[3] = 0xFFFFFF;
        const char *notif_hdr = "NOTIFICATIONS\n-------------";
        for(int j=0; notif_hdr[j]; j++) {
            if (notif_hdr[j] == '\n') { msg.data[1] = 20; msg.data[2] += 20; continue; }
            msg.data[0] = notif_hdr[j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }

        msg.data[1] = 20; msg.data[2] += 20;
        for(int j=0; dynamic_notif[j]; j++) {
            msg.data[0] = dynamic_notif[j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
        msg.type = COMPOSER_DRAW_RECT;
    }

    if (settings_open) {
        // Right Pull-down (Quick Settings)
        msg.data[0] = 550; msg.data[1] = 0; msg.data[2] = 250; msg.data[3] = 400;
        msg.data[4] = 0xEE1A1A1A;
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = 570; msg.data[2] = 50; msg.data[3] = 0xFFFFFF;
        const char *quick = "QUICK SETTINGS\n--------------\nBattery: 100%\nPower: High Perf\nVolume: 80%\nWi-Fi: Connected";
        for(int j=0; quick[j]; j++) {
            if (quick[j] == '\n') { msg.data[1] = 570; msg.data[2] += 20; continue; }
            msg.data[0] = quick[j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
        msg.type = COMPOSER_DRAW_RECT;
    }

    if (menu_open) {
        // Draw Full Screen App Grid Background
        msg.data[0] = 50; msg.data[1] = 50; msg.data[2] = 700; msg.data[3] = 450;
        msg.data[4] = 0xEE111111;
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

        // Draw Search Bar
        msg.data[0] = 250; msg.data[1] = 70; msg.data[2] = 300; msg.data[3] = 30;
        msg.data[4] = 0xFF333333;
        syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

        msg.type = COMPOSER_DRAW_CHAR;
        msg.data[1] = 260; msg.data[2] = 78; msg.data[3] = 0xAAAAAA;
        const char *s_text = strlen(search_query) > 0 ? search_query : "Search apps...";
        for(int j=0; s_text[j]; j++) {
            msg.data[0] = s_text[j];
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
            msg.data[1] += 8;
        }
        msg.type = COMPOSER_DRAW_RECT;

        // Draw Apps
        int shown_count = 0;
        for (int i = 0; i < 6; i++) {
            if (strlen(search_query) > 0 && strstr(apps[i].name, search_query) == NULL) continue;

            int x = 100 + (shown_count % 3) * 200;
            int y = 100 + (i / 3) * 150;

            // Icon
            msg.data[0] = x; msg.data[1] = y; msg.data[2] = 80; msg.data[3] = 80;
            msg.data[4] = apps[i].icon_color;
            syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

            // Label
            msg.type = COMPOSER_DRAW_CHAR;
            msg.data[1] = x + 10; msg.data[2] = y + 90; msg.data[3] = 0xFFFFFF;
            for(int j=0; apps[i].name[j]; j++) {
                msg.data[0] = apps[i].name[j];
                syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);
                msg.data[1] += 8;
            }
            msg.type = COMPOSER_DRAW_RECT;
            shown_count++;
        }
    }
}

int main() {
    // Create Desktop Window (Invisible background to capture clicks)
    ipc_msg_t msg;
    msg.sender = 1001;
    msg.type = COMPOSER_CREATE_WINDOW;
    msg.data[0] = 0; msg.data[1] = 0; msg.data[2] = 800; msg.data[3] = 600;
    syscall3(SYS_IPC_SEND, 1, (long)&msg, 0);

    draw_dashboard();

    while(1) {
        ipc_msg_t rmsg;
        if (syscall3(SYS_IPC_RECV, 0, (long)&rmsg, 0) == 0) {
            if (rmsg.type == 22) { // KEYBOARD_EVENT
                if (menu_open) {
                    char c = (char)rmsg.data[0];
                    if (c == '\b') {
                        if (strlen(search_query) > 0) search_query[strlen(search_query)-1] = 0;
                    } else if (strlen(search_query) < 31 && c >= 32) {
                        int l = strlen(search_query);
                        search_query[l] = c;
                        search_query[l+1] = 0;
                    }
                    draw_dashboard();
                }
            } else if (rmsg.type == 50) { // NEW_NOTIFICATION
                strncpy(dynamic_notif, (char*)rmsg.data, 63);
                dynamic_notif[63] = 0;
                draw_dashboard();
            } else if (rmsg.type == 42) { // FOCUS_CHANGED
                // In a real system, we'd lookup the name by PID
                // For now, let's assume it's one of our known apps or just "App"
                strcpy(active_app_name, "Active App");
                draw_dashboard();
            } else if (rmsg.type == 21) { // WM_CLICK
                int cx = rmsg.data[0];
                int cy = rmsg.data[1];

                // Check pull-down triggers (Top corners)
                if (cy < 30) {
                    if (cx < 100) { notifications_open = !notifications_open; settings_open = false; menu_open = false; }
                    else if (cx > 700) { settings_open = !settings_open; notifications_open = false; menu_open = false; }
                    draw_dashboard();
                    continue;
                }

                // Main Button Toggle
                if (cx >= 375 && cx <= 425 && cy >= 525 && cy <= 575) {
                    menu_open = !menu_open;
                    // Play pop sound
                    uint8_t sound[128];
                    for(int s=0; s<128; s++) sound[s] = (s % 2) ? 0x80 : 0x00;
                    syscall3(SYS_AUDIO_PLAY, (long)sound, 128, 0);
                    notifications_open = false; settings_open = false;
                    draw_dashboard();
                } else if (!menu_open && cx >= 20 && cx <= 80) {
                    // Check desktop shortcuts
                    int shortcut_idx = (cy - 50) / 100;
                    if (shortcut_idx >= 0 && shortcut_idx < 3 && (cy % 100) >= 0 && (cy % 100) <= 60) {
                        syscall1(SYS_SPAWN, (long)apps[shortcut_idx].elf);
                    }
                } else if (menu_open) {
                    // Check app clicks
                    for (int i = 0; i < 6; i++) {
                        int ax = 100 + (i % 3) * 200;
                        int ay = 100 + (i / 3) * 150;
                        if (cx >= ax && cx <= ax+80 && cy >= ay && cy <= ay+80) {
                            syscall1(SYS_SPAWN, (long)apps[i].elf);
                            menu_open = false;
                            draw_dashboard();
                            break;
                        }
                    }
                }
            }
        }
        syscall0(SYS_YIELD);
    }
    return 0;
}
