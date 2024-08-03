#include <iostream>
#include "Window/Window.h"

// the entry point for any Windows program
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd)
{

    Window wnd(L"Is Any Kernel Anti-Cheat Running", hInstance, 822, 600);
    while (true)
    {
        // Check to see if any messages are waiting in the queue
        if (PeekMessage(&wnd.msg(), NULL, 0, 0, PM_REMOVE))
        {
            // translate keystroke messages into the right format
            TranslateMessage(&wnd.msg());

            // send the message to the WindowProc function
            DispatchMessage(&wnd.msg());

            // check to see if it's time to quit
            if (wnd.msg().message == WM_QUIT)
                break;
        }
        else
        {
            wnd.Render();
        }
    }


    return 0;
}