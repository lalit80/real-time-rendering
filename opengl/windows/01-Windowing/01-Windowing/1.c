#include<windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

/*
    hInstance, hPrevInstance:
        one program can have multiple instances/processes running at a time in memory
            hInstance refers to that particular process's identity
                ex: user can open notepad in 3 different window each having a different hInstance
                but text section for notepad is shared
    
    iShowCmd:
        how window should be displayed initially (maximised, minimised, normally) and where

    .lib(import libraries) contain info about where called functions are located in .dll at runtime
*/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, int iShowCmd)
{
    WNDCLASSEX wnd;
    MSG msg;
    HWND hwnd;
    TCHAR szAppName[] = TEXT("RTR6");

    wnd.cbSize = sizeof(WNDCLASSEX);
    wnd.cbClsExtra = 0;
    wnd.cbWndExtra = 0;
    wnd.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);        // BRUSH/PEN/FONT
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);                      // NULL cause os's stock image
    wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wnd.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wnd.hInstance = hInstance;
    wnd.lpfnWndProc = WndProc;
    wnd.lpszClassName = szAppName;
    wnd.lpszMenuName = NULL;
    wnd.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassEx(&wnd);

    hwnd = CreateWindow(szAppName, TEXT("Lalit Choudhary 1"),
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iShowCmd);
    UpdateWindow(hwnd);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return ((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch(iMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
