#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vector>
#include "renderer.h"
#include "resource.h"

namespace {
constexpr wchar_t WindowClass[]=L"ClassicFlipClock.ScreenSaver";
constexpr wchar_t SettingsKey[]=L"Software\\ClassicFlipClock";
constexpr UINT_PTR TimerId=1;
constexpr ULONGLONG FlipMs=560;
HINSTANCE instance;
flip::Options options;
std::vector<HWND> windows;
ULONGLONG started;
POINT mouseOrigin{};
bool fullscreen=false;

struct Window {
    bool preview=false;
    flip::Renderer renderer;
    SYSTEMTIME time{};
    std::array<std::wstring,3> previous,current;
    ULONGLONG flipStart=0;
    UINT interval=0;
};

flip::Options loadOptions() {
    DWORD flags=0,size=sizeof(flags);
    RegGetValueW(HKEY_CURRENT_USER,SettingsKey,L"Flags",RRF_RT_REG_DWORD,nullptr,&flags,&size);
    return {!(flags&1),bool(flags&2),bool(flags&4),!(flags&8)};
}

INT_PTR CALLBACK settingsProc(HWND dialog,UINT message,WPARAM wp,LPARAM) {
    if(message==WM_INITDIALOG) {
        CheckDlgButton(dialog,IDC_SECONDS,options.seconds?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(dialog,IDC_TWELVE,options.twelve?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(dialog,IDC_DATE,options.date?BST_CHECKED:BST_UNCHECKED);
        CheckDlgButton(dialog,IDC_DRIFT,options.drift?BST_CHECKED:BST_UNCHECKED);
        return TRUE;
    }
    if(message==WM_COMMAND) {
        if(LOWORD(wp)==IDOK) {
            DWORD flags=(IsDlgButtonChecked(dialog,IDC_SECONDS)==BST_CHECKED?0:1)
                |(IsDlgButtonChecked(dialog,IDC_TWELVE)==BST_CHECKED?2:0)
                |(IsDlgButtonChecked(dialog,IDC_DATE)==BST_CHECKED?4:0)
                |(IsDlgButtonChecked(dialog,IDC_DRIFT)==BST_CHECKED?0:8);
            HKEY key=nullptr;
            LSTATUS result=RegCreateKeyExW(HKEY_CURRENT_USER,SettingsKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);
            if(result==ERROR_SUCCESS) {
                result=RegSetValueExW(key,L"Flags",0,REG_DWORD,reinterpret_cast<BYTE*>(&flags),sizeof(flags));
                RegCloseKey(key);
            }
            if(result!=ERROR_SUCCESS) {
                MessageBoxW(dialog,L"Could not save settings for this Windows account.",L"Classic Flip Clock",MB_OK|MB_ICONERROR);
                return TRUE;
            }
            EndDialog(dialog,IDOK); return TRUE;
        }
        if(LOWORD(wp)==IDCANCEL) { EndDialog(dialog,IDCANCEL); return TRUE; }
    }
    if(message==WM_CLOSE) { EndDialog(dialog,IDCANCEL); return TRUE; }
    return FALSE;
}

void schedule(HWND hwnd,Window& state,UINT interval) {
    if(interval!=state.interval) {
        SetTimer(hwnd,TimerId,interval,nullptr);
        state.interval=interval;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    auto* state=reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    switch(message) {
    case WM_CREATE: {
        auto cs=reinterpret_cast<CREATESTRUCTW*>(lp);
        state=new Window;
        state->preview=cs->lpCreateParams!=nullptr;
        GetLocalTime(&state->time);
        state->current=flip::digits(state->time.wHour,state->time.wMinute,state->time.wSecond,options.twelve);
        state->previous=state->current;
        SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));
        schedule(hwnd,*state,100);
        return 0;
    }
    case WM_TIMER: {
        if(!state) return 0;
        auto now=GetTickCount64();
        if(state->preview) {
            HWND parent=GetParent(hwnd);
            if(!IsWindow(parent)) { DestroyWindow(hwnd); return 0; }
            RECT rect{},own{};
            GetClientRect(parent,&rect); GetClientRect(hwnd,&own);
            if(rect.right!=own.right || rect.bottom!=own.bottom)
                SetWindowPos(hwnd,nullptr,0,0,rect.right,rect.bottom,SWP_NOZORDER|SWP_NOACTIVATE);
        } else {
            POINT pos{};
            if(GetCursorPos(&pos)) {
                if(now-started<800) mouseOrigin=pos;
                else if(std::abs(pos.x-mouseOrigin.x)>6 || std::abs(pos.y-mouseOrigin.y)>6) { PostQuitMessage(0); return 0; }
            }
        }
        SYSTEMTIME time; GetLocalTime(&time);
        auto next=flip::digits(time.wHour,time.wMinute,time.wSecond,options.twelve);
        bool changed=false;
        for(int i=0;i<(options.seconds?3:2);++i) changed|=next[i]!=state->current[i];
        bool captionChanged=time.wDay!=state->time.wDay || time.wMonth!=state->time.wMonth || time.wYear!=state->time.wYear || time.wHour!=state->time.wHour;
        bool minuteChanged=time.wMinute!=state->time.wMinute;
        state->time=time;
        if(changed) { state->previous=state->current; state->current=next; state->flipStart=now; }
        bool active=state->previous!=state->current && now-state->flipStart<FlipMs;
        bool finishing=state->previous!=state->current && !active;
        if(finishing) state->previous=state->current;
        if(changed || active || finishing || captionChanged || minuteChanged) InvalidateRect(hwnd,nullptr,FALSE);
        // Poll input at 100 ms while idle; draw only when the displayed value changes.
        schedule(hwnd,*state,active?16:100);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);
        if(state) {
            RECT r; GetClientRect(hwnd,&r);
            float progress=std::min(1.0f,static_cast<float>(GetTickCount64()-state->flipStart)/FlipMs);
            state->renderer.paint(dc,r.right,r.bottom,state->time,options,state->previous,state->current,progress,state->preview);
        }
        EndPaint(hwnd,&ps); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_SIZE: InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_DPICHANGED:
        if(state && !state->preview) {
            auto r=reinterpret_cast<RECT*>(lp);
            SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
        }
        return 0;
    case WM_DISPLAYCHANGE:
        if(fullscreen) PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: case WM_XBUTTONDOWN: case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        if(state && !state->preview) PostQuitMessage(0);
        return 0;
    case WM_ACTIVATEAPP:
        if(fullscreen && !wp && GetTickCount64()-started>1000) PostQuitMessage(0);
        return 0;
    case WM_SETCURSOR:
        if(state && !state->preview) { SetCursor(nullptr); return TRUE; }
        break;
    case WM_SYSCOMMAND:
        if(fullscreen && (wp&0xFFF0)==SC_SCREENSAVE) return 0;
        break;
    case WM_DESTROY:
        KillTimer(hwnd,TimerId);
        PostQuitMessage(0); return 0;
    case WM_NCDESTROY:
        SetWindowLongPtrW(hwnd,GWLP_USERDATA,0); delete state; break;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}

BOOL CALLBACK createMonitorWindow(HMONITOR,HDC,LPRECT rect,LPARAM) {
    HWND window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,WindowClass,L"Classic Flip Clock",WS_POPUP,
        rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,nullptr,nullptr,instance,nullptr);
    if(!window) return FALSE;
    windows.push_back(window); return TRUE;
}
}

int WINAPI wWinMain(HINSTANCE app,HINSTANCE,PWSTR command,int) {
    instance=app;
    auto launch=flip::parse(command?command:L"");
    if(launch.mode==flip::Mode::Invalid) return 1;
    options=loadOptions();
    HWND parent=reinterpret_cast<HWND>(launch.parent);
    if(launch.mode==flip::Mode::Configure) {
        auto result=DialogBoxParamW(instance,MAKEINTRESOURCEW(IDD_SETTINGS),IsWindow(parent)?parent:nullptr,settingsProc,0);
        return result==-1?1:0;
    }
    bool preview=launch.mode==flip::Mode::Preview;
    if(preview && !IsWindow(parent)) return 1;
    Gdiplus::GdiplusStartupInput input;
    ULONG_PTR token=0;
    if(Gdiplus::GdiplusStartup(&token,&input,nullptr)!=Gdiplus::Ok) return 1;
    WNDCLASSW wc{};
    wc.lpfnWndProc=windowProc; wc.hInstance=instance; wc.lpszClassName=WindowClass;
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW); wc.hbrBackground=static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    if(!RegisterClassW(&wc)) { Gdiplus::GdiplusShutdown(token); return 1; }
    started=GetTickCount64(); GetCursorPos(&mouseOrigin); fullscreen=!preview;
    bool created=true;
    if(preview) {
        RECT r{}; GetClientRect(parent,&r);
        HWND window=CreateWindowExW(0,WindowClass,L"Classic Flip Clock Preview",WS_CHILD|WS_VISIBLE,
            0,0,r.right,r.bottom,parent,nullptr,instance,reinterpret_cast<void*>(1));
        if(window) windows.push_back(window); else created=false;
    } else {
        created=EnumDisplayMonitors(nullptr,nullptr,createMonitorWindow,0)!=FALSE;
        if(created) {
            for(auto window:windows) ShowWindow(window,SW_SHOWNA);
            if(!windows.empty()) { SetForegroundWindow(windows.front()); SetFocus(windows.front()); }
        }
    }
    int result=1;
    if(created && !windows.empty()) {
        MSG message{}; BOOL status;
        while((status=GetMessageW(&message,nullptr,0,0))>0) { TranslateMessage(&message); DispatchMessageW(&message); }
        result=status==-1?1:static_cast<int>(message.wParam);
    }
    for(auto window:windows) if(IsWindow(window)) DestroyWindow(window);
    windows.clear();
    UnregisterClassW(WindowClass,instance);
    Gdiplus::GdiplusShutdown(token);
    return result;
}
