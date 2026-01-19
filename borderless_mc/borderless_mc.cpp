#include <windows.h>
#include <iostream>
#include <string>
#include <psapi.h>

std::string GetProcessName(DWORD pid) {
    char name[MAX_PATH] = "<unknown>";
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        GetModuleBaseNameA(hProcess, NULL, name, MAX_PATH);
        CloseHandle(hProcess);
    }
    return name;
}

void LogLastError(const std::string& step) {
    DWORD err = GetLastError();
    if (err == 0) return;

    LPVOID msg;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        err,
        0,
        (LPSTR)&msg,
        0,
        NULL
    );

    std::cout << "[MC-ERROR] " << step
        << " | Code: " << err
        << " | Msg: " << (char*)msg << "\n";

    LocalFree(msg);
}

HWND FindMinecraftWindow() {
    HWND hwnd = GetTopWindow(NULL);
    char title[256];

    while (hwnd) {
        GetWindowTextA(hwnd, title, sizeof(title));

        if (strstr(title, "Minecraft")) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            std::string exe = GetProcessName(pid);

            // Ignore launcher explicitly
            if (exe == "MinecraftLauncher.exe") {
                std::cout << "[MC-DEBUG] Ignored launcher window\n";
                hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
                continue;
            }

            // Accept only real game processes
            if (exe == "javaw.exe" || exe == "Minecraft.Windows.exe") {
                std::cout << "[MC-DEBUG] Minecraft game window detected\n";
                std::cout << "[MC-DEBUG] Title: \"" << title << "\"\n";
                std::cout << "[MC-DEBUG] Process: " << exe << "\n";
                return hwnd;
            }
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    return nullptr;
}

int main() {
    std::cout << "==== Borderless Minecraft ====\n";

    HWND hwnd = FindMinecraftWindow();
    if (!hwnd) {
        std::cout << "[MC-FATAL] Minecraft game window not found.\n";
        return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    std::cout << "[MC-DEBUG] Screen size: "
        << screenW << "x" << screenH << "\n";

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    style &= ~(WS_CAPTION | WS_THICKFRAME |
        WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

    exStyle &= ~(WS_EX_DLGMODALFRAME |
        WS_EX_CLIENTEDGE |
        WS_EX_STATICEDGE);

    if (!SetWindowLong(hwnd, GWL_STYLE, style))
        LogLastError("SetWindowLong STYLE");
    else
        std::cout << "[MC-DEBUG] STYLE stripped\n";

    if (!SetWindowLong(hwnd, GWL_EXSTYLE, exStyle))
        LogLastError("SetWindowLong EXSTYLE");
    else
        std::cout << "[MC-DEBUG] EXSTYLE stripped\n";

    if (!SetWindowPos(
        hwnd,
        HWND_TOP,
        0, 0,
        screenW,
        screenH,
        SWP_FRAMECHANGED | SWP_SHOWWINDOW
    )) {
        LogLastError("SetWindowPos");
    }
    else {
        std::cout << "[MC-DEBUG] Borderless fullscreen applied\n";
    }

    std::cout << "[MC-SUCCESS] Done.\n";
    std::cout << "===============================================\n";

    return 0;
}
