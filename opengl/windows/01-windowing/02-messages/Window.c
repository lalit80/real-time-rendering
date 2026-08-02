// Win32 headers
#include <windows.h>

// global function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// entry point function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // variable declarations
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("RTR6");

    // window class initialization
    // code
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName = NULL;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    // registration of window class
    RegisterClassEx(&wndclass);

    // create window
    hwnd = CreateWindow(szAppName, TEXT("Lalit Choudhary"),
                            WS_OVERLAPPEDWINDOW, 
                            CW_USEDEFAULT, CW_USEDEFAULT, 
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, hInstance, NULL);

    // show window
    ShowWindow(hwnd, iCmdShow);

    // paint background of window
    UpdateWindow(hwnd);

    // message loop
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return ((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // code
    switch(iMsg)
    {
        case WM_CREATE:
            MessageBox(hwnd, TEXT("This is the first message."), TEXT("WM_CREATE"), MB_OK);
            break;

        case WM_SIZE:
            MessageBox(hwnd, TEXT("Window is resized"), TEXT("WM_SIZE"), MB_OK);
            break;
        
        case WM_MOVE:
            MessageBox(hwnd, TEXT("Window is moved"), TEXT("WM_MOVE"), MB_OK);
            break;
        
        case WM_KEYDOWN:
            switch(wParam) {
                case VK_ESCAPE:
                    MessageBox(hwnd, TEXT("Escape key pressed"), TEXT("WM_KEYDOWN"), MB_OK);
                    break;
                default:
                    break;
            }
            break;
        
        case WM_CHAR:
            switch(wParam) {
                case 'F':
                    MessageBox(hwnd, TEXT("F key pressed"), TEXT("WM_CHAR"), MB_OK);
                    break;
                case 'f':
                    MessageBox(hwnd, TEXT("f key pressed"), TEXT("WM_CHAR"), MB_OK);
                    break;
                default:
                    break;
            }
            break;
        
        case WM_LBUTTONDOWN:
            MessageBox(hwnd, TEXT("Left mouse button clicked"), TEXT("WM_LBUTTONDOWN"), MB_OK);
            break;
        
        case WM_CLOSE:
            MessageBox(hwnd, TEXT("Window is closed"), TEXT("WM_CLOSE"), MB_OK);
            DestroyWindow(hwnd);
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    
    return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}
