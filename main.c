#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <stdio.h>

PSP_MODULE_INFO("Perspective Awakens", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main(void) {
    SceCtrlData pad, oldpad;
    int x = 10, y = 15, scale = 1, help = 1, solved = 0, room = 1;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    pspDebugScreenInit();
    sceCtrlReadBufferPositive(&oldpad, 1);

    for (;;) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_UP) y--;
        if (pad.Buttons & PSP_CTRL_DOWN) y++;
        if (pad.Buttons & PSP_CTRL_LEFT) x--;
        if (pad.Buttons & PSP_CTRL_RIGHT) x++;
        if ((pad.Buttons & PSP_CTRL_CROSS) && !(oldpad.Buttons & PSP_CTRL_CROSS)) {
            if (solved) { room = room % 6 + 1; x = 10; y = 15; scale = 1; solved = 0; }
            else { scale = scale % 4 + 1; if (scale >= 3 && x > 25 && x < 45 && y > 10 && y < 23) solved = 1; }
        }
        if ((pad.Buttons & PSP_CTRL_START) && !(oldpad.Buttons & PSP_CTRL_START)) {
            x = 10; y = 15; scale = 1; solved = 0;
        }
        if ((pad.Buttons & PSP_CTRL_TRIANGLE) && !(oldpad.Buttons & PSP_CTRL_TRIANGLE)) help = !help;
        if (x < 1) x = 1; if (x > 58) x = 58;
        if (y < 4) y = 4; if (y > 30) y = 30;

        pspDebugScreenSetXY(0, 0);
        pspDebugScreenClear();
        printf("PERSPECTIVE AWAKENS   |   ROOM %d / 6\n", room);
        printf("============================================================\n");
        printf("\nA strange room bends around one simple object.\n");
        printf("Change its scale, then move into the goal zone.\n\n");
        printf("   +------------------------------------------+\n");
        printf("   |                                          |\n");
        printf("   |          GOAL ZONE                       |\n");
        printf("   |                                          |\n");
        printf("   +------------------------------------------+\n\n");
        printf("Object scale: %d    Player: @\n", scale);
        pspDebugScreenSetXY(x, y);
        printf("@");
        pspDebugScreenSetXY(0, 27);
        if (solved) printf("PUZZLE SOLVED! Press CROSS for the next room.\n");
        else if (help) printf("D-pad: move   CROSS: change scale   START: reset   TRIANGLE: help\n");
        else printf("TRIANGLE: show controls\n");
        oldpad = pad;
        sceDisplayWaitVblankStart();
    }
    return 0;
}
