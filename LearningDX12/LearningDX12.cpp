// LearningDX12.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "LearningDX12.h"

#include <memory>



#include "DemoBoxGame.h"
#include "DX12App.h"
#include <shellapi.h>
#include "TexturedDemoBoxGame.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass( HINSTANCE hInstance );
HWND                InitInstance( HINSTANCE, int );
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK    About( HWND, UINT, WPARAM, LPARAM );

std::unique_ptr<Olex::DX12App> globalApplication;

int APIENTRY wWinMain( _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window
    // to achieve 100% scaling while still allowing non-client window content to
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );

    // Initialize global strings
    LoadStringW( hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
    LoadStringW( hInstance, IDC_LEARNINGDX12, szWindowClass, MAX_LOADSTRING );
    MyRegisterClass( hInstance );

    // Perform application initialization:
    globalApplication = std::make_unique<Olex::DX12App>();

    HWND window = InitInstance( hInstance, nCmdShow );
    if ( !window )
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_LEARNINGDX12 ) );

    // Choose a demo here
    {
        int argc;
        wchar_t** argv = ::CommandLineToArgvW( ::GetCommandLineW(), &argc );

        for ( size_t i = 0; i < argc; ++i )
        {
            if ( ::wcscmp( argv[i], L"-demo" ) == 0 || ::wcscmp( argv[i], L"--demo" ) == 0 )
            {
                const unsigned long choice = ::wcstoul(argv[++i], nullptr, 10);
                switch(choice)
                {
                case 0:
                    break;
                case 1:
                    globalApplication->SetGame( std::make_unique<Olex::DemoBoxGame>( *globalApplication ) );
                    break;
                case 2:
                    globalApplication->SetGame( std::make_unique<Olex::TexturedDemoBoxGame>( *globalApplication ) );
                    break;
                default:
                    break;
                }
                break;
            }
        }

        // Free memory allocated by CommandLineToArgvW
        ::LocalFree( argv );
    }

    MSG msg;
    // Main message loop:
    while ( GetMessage( &msg, nullptr, 0, 0 ) )
    {
        if ( !TranslateAccelerator( msg.hwnd, hAccelTable, &msg ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    globalApplication.reset();

    return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass( HINSTANCE hInstance )
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof( WNDCLASSEX );

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_LEARNINGDX12 ) );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = reinterpret_cast<HBRUSH>( ( COLOR_WINDOW + 1 ) );
    wcex.lpszMenuName = MAKEINTRESOURCEW( IDC_LEARNINGDX12 );
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_SMALL ) );

    return RegisterClassExW( &wcex );
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance( HINSTANCE hInstance, int nCmdShow )
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr );

    if ( !hWnd )
    {
        return hWnd;
    }

    globalApplication->Init( hWnd );

    ShowWindow( hWnd, nCmdShow );
    UpdateWindow( hWnd );

    return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if ( globalApplication )
    {
        if ( globalApplication->IsInitialized() )
        {
            switch ( message )
            {
            case WM_COMMAND:
            {
                int wmId = LOWORD( wParam );
                // Parse the menu selections:
                switch ( wmId )
                {
                case IDM_EXIT:
                    DestroyWindow( hWnd );
                    break;
                default:
                    return DefWindowProc( hWnd, message, wParam, lParam );
                }
            }
            break;
            case WM_PAINT:
            {
                globalApplication->OnPaintEvent();
                return 0;
            }
            break;
            case WM_DESTROY:
                PostQuitMessage( 0 );
                break;
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                globalApplication->OnKeyEvent( wParam );
                return DefWindowProc( hWnd, message, wParam, lParam );
            }
            break;
            // The default window procedure will play a system notification sound
            // when pressing the Alt+Enter keyboard combination if this message is
            // not handled.
            case WM_SYSCHAR:
                break;
            case WM_SIZE:
            {
                globalApplication->OnResize();
            }
            break;
            default:
                return DefWindowProc( hWnd, message, wParam, lParam );
            }
        }
        else
        {
            return DefWindowProc( hWnd, message, wParam, lParam );
        }
    }
    return 0;
}
