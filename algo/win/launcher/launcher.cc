#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

#define BUF_SIZE (1L << 16)

struct Defer {
  std::function<void()> action;
  Defer(std::function<void()> doLater) : action{doLater} {}
  ~Defer() {
    action();
  }
};

bool IsWinNT() {
    OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    return (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

std::string GetLastErrorAsString() {
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string();
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);

    return message;
}

void process_childs_stderr(HANDLE read_pipe) {
    int fd = _open_osfhandle((intptr_t)read_pipe, _O_TEXT | _O_RDONLY);
    if (fd == -1)
    {
        std::cerr << "_open_osfhandle has failed" << std::endl;
        return;
    }

    FILE* f = _fdopen(fd, "r");
    if (f == NULL)
    {
        std::cerr << "_fdopen has failed" << std::endl;
        return;
    }

    char buf[100];
    for (;;) {
        char* line = fgets(buf, 1L << 8, f);
        if (line) {
            std::cerr << line << std::endl;
        }
    }
}

int wmain(int argc, LPWSTR* argv) {
    Sleep(10 * 1000);
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    PROCESS_INFORMATION pi;
    HANDLE childs_stdin, childs_stdout, childs_stderr, read_stdout, write_stdin, read_stderr;

    if (IsWinNT()) {
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, true, NULL, false);
        sa.lpSecurityDescriptor = &sd;
    } else
        sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = true;

    if (!CreatePipe(&childs_stdin, &write_stdin, &sa, 0)) {
        std::cerr << "Can't create a pipe" << std::endl;
        return -1;
    }
    Defer child_stdin([&childs_stdin, &write_stdin]() {
        CloseHandle(childs_stdin);
        CloseHandle(write_stdin);
    });

    if (!CreatePipe(&read_stdout, &childs_stdout, &sa, 0)) {
        std::cerr << "Can't create a pipe" << std::endl;
        return -1;
    }
    Defer child_stdout([&childs_stdout, &read_stdout]() {
        CloseHandle(childs_stdout);
        CloseHandle(read_stdout);
    });

    if (!CreatePipe(&read_stderr, &childs_stderr, &sa, 0)) {
        std::cerr << "Can't create a pipe" << std::endl;
        return -1;
    }
    Defer child_stderr([&childs_stderr, &read_stderr]() {
        CloseHandle(childs_stderr);
        CloseHandle(read_stderr);
    });

    GetStartupInfo(&si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = childs_stdout;
    si.hStdInput = childs_stdin;
    si.hStdError = childs_stderr;

    wchar_t out_algo_buffer[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, out_algo_buffer, MAX_PATH)) {
        std::cerr << "Get module name has failed: " << GetLastErrorAsString() << std::endl;
        return -1;
    }
    const std::wstring out_algo = out_algo_buffer;
    const std::wstring host_base = out_algo.substr(0, out_algo.find_last_of(L"/\\"));
    const std::wstring host_path = host_base + std::wstring(L"\\algohost.exe");
    const wchar_t* app = host_path.c_str();

    LPWSTR cmd = NULL;
    if (argc > 2) {
        const auto cmd_str = std::wstring(argv[1]) + L' ' + std::wstring(argv[2]);
        cmd = const_cast<LPWSTR>(cmd_str.c_str());
    }
    SetHandleInformation(write_stdin, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(read_stdout, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(read_stderr, HANDLE_FLAG_INHERIT, 0);

    std::thread process_childs_stderr_thread(process_childs_stderr, read_stderr);

    if (!CreateProcess(app, cmd, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi)) {
        std::cerr << "Can't create a process: " << GetLastErrorAsString() << std::endl;
        return -1;
    }
    Defer create_proc([&pi]() {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    });

    const auto algo_base = std::string(host_base.begin(), host_base.end());
    const std::ifstream t(algo_base + std::string("\\task.json"));
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string content = buffer.str();
    std::cerr << "content length is " << content.size() << std::endl << std::endl;
    std::wstring json_str(content.begin(), content.end());
    std::replace(json_str.begin(), json_str.end(), '\r', ' ');
    std::replace(json_str.begin(), json_str.end(), '\n', ' ');

    unsigned long bytes_read;
    unsigned long bytes_written;

    if (!WriteFile(write_stdin, json_str.c_str(), json_str.size() * sizeof(wchar_t), &bytes_written, NULL)) {
        std::cerr << "Can't write to pipe: " << GetLastErrorAsString() << std::endl;
        return -1;
    }
    WriteFile(write_stdin, L"\r\n", 3, &bytes_read, NULL);
    assert(bytes_written > 0);

    std::replace(json_str.begin(), json_str.end(), 'B', 'F');
    std::replace(json_str.begin(), json_str.end(), 'D', 'B');
    WriteFile(write_stdin, json_str.c_str(), json_str.size() * sizeof(wchar_t), &bytes_written, NULL);
    WriteFile(write_stdin, L"\r\n", 3, &bytes_read, NULL);

    wchar_t buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    if (!ReadFile(read_stdout, buf, BUF_SIZE - 1, &bytes_read, NULL)) {
        std::cerr << "Can't read from pipe: " << GetLastErrorAsString() << std::endl;
        return -3;
    }
    std::cerr << "read " << bytes_read << " bytes!" << std::endl;
    std::wcout << buf << std::endl;

    memset(buf, 0, sizeof(buf));
    if (!ReadFile(read_stdout, buf, BUF_SIZE - 1, &bytes_read, NULL)) {
        std::cerr << "Can't read from pipe: " << GetLastErrorAsString() << std::endl;
        return -3;
    }
    std::cerr << "read " << bytes_read << " bytes!" << std::endl;
    std::wcout << buf << std::endl;

    std::cout << "Press Ctrl+C to finish launcher and broker processes" << std::endl;
    for (;;) {
        Sleep(1000);
    }

    return 0;
}
