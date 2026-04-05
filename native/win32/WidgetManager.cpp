#include "WidgetManager.h"
#include <windows.h>
#include <dwmapi.h>
#include <exdisp.h>
#include <mshtml.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>

// Minimal implementation for embedding IWebBrowser2
class SimpleClientSite : public IOleClientSite, public IOleInPlaceSite, public IOleInPlaceFrame {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_IOleClientSite) *ppv = (IOleClientSite*)this;
        else if (riid == IID_IOleInPlaceSite) *ppv = (IOleInPlaceSite*)this;
        else if (riid == IID_IOleInPlaceFrame || riid == IID_IOleInPlaceUIWindow) *ppv = (IOleInPlaceFrame*)this;
        else return (*ppv = NULL, E_NOINTERFACE);
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP SaveObject() { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker**) { return E_NOTIMPL; }
    STDMETHODIMP GetContainer(IOleContainer**) { return E_NOTIMPL; }
    STDMETHODIMP ShowObject() { return S_OK; }
    STDMETHODIMP OnShowWindow(BOOL) { return S_OK; }
    STDMETHODIMP RequestNewObjectLayout() { return E_NOTIMPL; }
    STDMETHODIMP GetWindow(HWND* phwnd) { *phwnd = m_hwnd; return S_OK; }
    STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP CanInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnUIActivate() { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) {
        *ppFrame = (IOleInPlaceFrame*)this;
        *ppDoc = NULL;
        GetClientRect(m_hwnd, lprcPosRect);
        GetClientRect(m_hwnd, lprcClipRect);
        lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = m_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE) { return E_NOTIMPL; }
    STDMETHODIMP OnUIDeactivate(BOOL) { return S_OK; }
    STDMETHODIMP OnInPlaceDeactivate() { return S_OK; }
    STDMETHODIMP DiscardUndoState() { return E_NOTIMPL; }
    STDMETHODIMP DeactivateAndUndo() { return E_NOTIMPL; }
    STDMETHODIMP OnPosRectChange(LPCRECT) { return S_OK; }
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND) { return E_NOTIMPL; }
    STDMETHODIMP RemoveMenus(HMENU) { return E_NOTIMPL; }
    STDMETHODIMP SetStatusText(LPCOLESTR) { return E_NOTIMPL; }
    STDMETHODIMP EnableModeless(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD) { return E_NOTIMPL; }
    STDMETHODIMP GetBorder(LPRECT) { return E_NOTIMPL; }
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject*, LPCOLESTR) { return E_NOTIMPL; }
    HWND m_hwnd;
};

struct Win32WidgetContext {
    HWND hwnd;
    IWebBrowser2* webBrowser;
    SimpleClientSite clientSite;
};

struct Win32Task {
    enum Type { CREATE, OPACITY, POSITION } type;
    const std::string* url;
    const WidgetOptions* options;
    void* handle;
    double opacity;
    int x, y;
    void* result;
    std::mutex mutex;
    std::condition_variable cv;
    bool done;
    Win32Task() : done(false), result(nullptr) {}
};

static std::queue<Win32Task*> task_queue;
static std::mutex queue_mutex;
static std::atomic<bool> loop_running{false};

static LRESULT CALLBACK WidgetWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SIZE) {
        Win32WidgetContext* ctx = (Win32WidgetContext*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (ctx && ctx->webBrowser) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            ctx->webBrowser->put_Width(rect.right);
            ctx->webBrowser->put_Height(rect.bottom);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void run_win32_loop() {
    std::cout << "[Native] Starting Win32 loop thread..." << std::endl;
    OleInitialize(NULL);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WidgetWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WidgetWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; // No background brush for true transparency
    RegisterClass(&wc);

    loop_running = true;
    while (true) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        std::vector<Win32Task*> local_tasks;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            while (!task_queue.empty()) {
                local_tasks.push_back(task_queue.front());
                task_queue.pop();
            }
        }

        for (auto* task : local_tasks) {
            if (task->type == Win32Task::CREATE) {
                std::cout << "[Native] Creating chromeless widget window (Cyber-Glow Mode)..." << std::endl;
                HWND hwnd = CreateWindowEx(
                    WS_EX_LAYERED | WS_EX_TOOLWINDOW | ((task->options->sticky && !task->options->interactive) ? WS_EX_TRANSPARENT : 0),
                    "WidgetWindowClass", "Widget",
                    WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,
                    task->options->x, task->options->y, task->options->width, task->options->height,
                    NULL, NULL, hInstance, NULL
                );
                
                if (!hwnd) {
                    std::cerr << "[Native] CreateWindowEx failed: " << GetLastError() << std::endl;
                } else {
                    // Set Color Key to RGB(1, 1, 1) for transparency
                    SetLayeredWindowAttributes(hwnd, RGB(1, 1, 1), 0, LWA_COLORKEY);
                    
                    if (task->options->blur) {
                        DWM_BLURBEHIND bb = {DWM_BB_ENABLE, TRUE, NULL, FALSE};
                        DwmEnableBlurBehindWindow(hwnd, &bb);
                    }
                    if (task->options->sticky) SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                    Win32WidgetContext* ctx = new Win32WidgetContext();
                    ctx->hwnd = hwnd;
                    ctx->clientSite.m_hwnd = hwnd;
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);

                    IOleObject* oleObject = NULL;
                    if (SUCCEEDED(CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&oleObject))) {
                        oleObject->SetClientSite(&ctx->clientSite);
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, &ctx->clientSite, 0, hwnd, &rect);
                        oleObject->QueryInterface(IID_IWebBrowser2, (void**)&ctx->webBrowser);
                        if (ctx->webBrowser) {
                            ctx->webBrowser->put_Visible(VARIANT_TRUE);
                            std::wstring wurl(task->url->begin(), task->url->end());
                            BSTR bstrURL = SysAllocString(wurl.c_str());
                            ctx->webBrowser->Navigate(bstrURL, NULL, NULL, NULL, NULL);
                            SysFreeString(bstrURL);
                            std::cout << "[Native] Browser navigated to: " << task->url->c_str() << std::endl;
                        }
                        oleObject->Release();
                    }
                    UpdateWindow(hwnd);
                    task->result = ctx;
                }
            } else if (task->type == Win32Task::OPACITY) {
                // Opacity handled via Color Key / Global Alpha mixture if needed, 
                // but for Color Key mode we focus on the key color.
                if (task->handle) {
                    Win32WidgetContext* ctx = (Win32WidgetContext*)task->handle;
                    SetLayeredWindowAttributes(ctx->hwnd, 0, (BYTE)(task->opacity * 255), LWA_ALPHA);
                }
            } else if (task->type == Win32Task::POSITION) {
                if (task->handle) {
                    Win32WidgetContext* ctx = (Win32WidgetContext*)task->handle;
                    SetWindowPos(ctx->hwnd, NULL, task->x, task->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
            {
                std::lock_guard<std::mutex> lock(task->mutex);
                task->done = true;
                task->cv.notify_one();
            }
        }
        Sleep(10);
    }
}

void* WidgetManager::CreateWidget(const std::string& url, const WidgetOptions& options) {
    static std::once_flag init_flag;
    std::call_once(init_flag, []() { std::thread(run_win32_loop).detach(); });
    while (!loop_running) Sleep(10);

    Win32Task task;
    task.type = Win32Task::CREATE;
    task.url = &url;
    task.options = &options;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        task_queue.push(&task);
    }
    std::unique_lock<std::mutex> lock(task.mutex);
    task.cv.wait(lock, [&]{ return task.done; });
    return task.result;
}

void WidgetManager::UpdateOpacity(void* handle, double opacity) {
    if (!handle) return;
    Win32Task task;
    task.type = Win32Task::OPACITY;
    task.handle = handle;
    task.opacity = opacity;
    { std::lock_guard<std::mutex> lock(queue_mutex); task_queue.push(&task); }
    std::unique_lock<std::mutex> lock(task.mutex);
    task.cv.wait(lock, [&]{ return task.done; });
}

void WidgetManager::UpdatePosition(void* handle, int x, int y) {
    if (!handle) return;
    Win32Task task;
    task.type = Win32Task::POSITION;
    task.handle = handle;
    task.x = x; task.y = y;
    { std::lock_guard<std::mutex> lock(queue_mutex); task_queue.push(&task); }
    std::unique_lock<std::mutex> lock(task.mutex);
    task.cv.wait(lock, [&]{ return task.done; });
}
