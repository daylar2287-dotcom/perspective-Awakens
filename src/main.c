#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

PSP_MODULE_INFO("Perspective Awakens", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main(void)
{
    SceCtrlData pad, oldpad;
    int x = 10, y = 15;
    int scale = 1;
    int help = 1;
    int solved = 0;
    int room = 1;

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    pspDebugScreenInit();
    sceCtrlReadBufferPositive(&oldpad, 1);

    for (;;)
    {
        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_UP) y--;
        if (pad.Buttons & PSP_CTRL_DOWN) y++;
        if (pad.Buttons & PSP_CTRL_LEFT) x--;
        if (pad.Buttons & PSP_CTRL_RIGHT) x++;

        if ((pad.Buttons & PSP_CTRL_CROSS) &&
            !(oldpad.Buttons & PSP_CTRL_CROSS))
        {
            if (solved)
            {
                room = room % 6 + 1;
                x = 10;
                y = 15;
                scale = 1;
                solved = 0;
            }
            else
            {
                scale = scale % 4 + 1;

                if (scale >= 3 &&
                    x > 25 && x < 45 &&
                    y > 10 && y < 23)
                    solved = 1;
            }
        }

        if ((pad.Buttons & PSP_CTRL_START) &&
            !(oldpad.Buttons & PSP_CTRL_START))
        {
            x = 10;
            y = 15;
            scale = 1;
            solved = 0;
        }

        if ((pad.Buttons & PSP_CTRL_TRIANGLE) &&
            !(oldpad.Buttons & PSP_CTRL_TRIANGLE))
            help = !help;

        if (x < 1) x = 1;
        if (x > 58) x = 58;
        if (y < 4) y = 4;
        if (y > 30) y = 30;

        pspDebugScreenSetXY(0, 0);
        pspDebugScreenClear();

        pspDebugScreenPrintf("PERSPECTIVE AWAKENS | ROOM %d / 6\n", room);
        pspDebugScreenPrintf("============================================\n\n");
        pspDebugScreenPrintf("A strange room bends around one object.\n");
        pspDebugScreenPrintf("Change its scale and reach the goal zone.\n\n");
        pspDebugScreenPrintf("       +--------------------------+\n");
        pspDebugScreenPrintf("       |                          |\n");
        pspDebugScreenPrintf("       |        GOAL ZONE         |\n");
        pspDebugScreenPrintf("       |                          |\n");
        pspDebugScreenPrintf("       +--------------------------+\n\n");
        pspDebugScreenPrintf("Object scale: %d\n", scale);

        pspDebugScreenSetXY(x, y);
        pspDebugScreenPrintf("@");

        pspDebugScreenSetXY(0, 27);

        if (solved)
            pspDebugScreenPrintf("PUZZLE SOLVED! Press CROSS for next room.\n");
        else if (help)
            pspDebugScreenPrintf("D-pad: Move | CROSS: Scale | START: Reset\n");
        else
            pspDebugScreenPrintf("TRIANGLE: Show controls\n");

        oldpad = pad;
        sceDisplayWaitVblankStart();
    }

    return 0;
}
