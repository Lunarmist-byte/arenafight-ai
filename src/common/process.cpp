#include "arenafight/common/process.hpp"
#include <iostream>
#include <algorithm>
#include <regex>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

namespace arenafight {

bool ProcessExecutor::isWindows() {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

bool ProcessExecutor::isDestructive(const std::string& command) {
    std::string lower = command;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    static const std::vector<std::string> dangerousPatterns = {
        "git reset --hard",
        "git clean -fd",
        "git clean -f",
        "rm -rf /",
        "rm -rf ~",
        "rmdir /s /q c:\\",
        "del /f /s /q c:\\",
        "format ",
        "mkfs",
        "drop database",
        ":(){ :|:& };:"
    };

    for (const auto& pattern : dangerousPatterns) {
        if (lower.find(pattern) != std::string::npos) {
            return true;
        }
    }

    // Regex check for broad recursive deletes
    static const std::regex rmAllRegex("rm\\s+-rf?\\s+([/\\*]+|\\.\\.)", std::regex::icase);
    if (std::regex_search(lower, rmAllRegex)) {
        return true;
    }

    return false;
}

#if defined(_WIN32)

namespace {

std::wstring toWide(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

[[maybe_unused]] std::string toUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

ProcessResult runProcessWin32(const std::wstring& cmdLine, const std::wstring& cwd, int timeoutMs) {
    ProcessResult result;
    auto startTime = std::chrono::steady_clock::now();

    HANDLE hStdOutRead = NULL;
    HANDLE hStdOutWrite = NULL;
    HANDLE hStdErrRead = NULL;
    HANDLE hStdErrWrite = NULL;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) return result;
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0)) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return result;
    }
    SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hStdErrWrite;
    si.hStdOutput = hStdOutWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::vector<wchar_t> cmdLineMutable(cmdLine.begin(), cmdLine.end());
    cmdLineMutable.push_back(L'\0');

    const wchar_t* pCwd = cwd.empty() ? NULL : cwd.c_str();

    BOOL success = CreateProcessW(
        NULL,
        cmdLineMutable.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        pCwd,
        &si,
        &pi
    );

    CloseHandle(hStdOutWrite);
    CloseHandle(hStdErrWrite);

    if (!success) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);
        result.exitCode = -1;
        result.stdErr = "CreateProcess failed with error " + std::to_string(GetLastError());
        return result;
    }

    // Asynchronously read stdout and stderr
    std::string outStr, errStr;

    auto readPipe = [](HANDLE hPipe, std::string& buffer) {
        char buf[4096];
        DWORD bytesRead = 0;
        while (ReadFile(hPipe, buf, sizeof(buf), &bytesRead, NULL) && bytesRead > 0) {
            buffer.append(buf, bytesRead);
        }
    };

    std::thread outThread(readPipe, hStdOutRead, std::ref(outStr));
    std::thread errThread(readPipe, hStdErrRead, std::ref(errStr));

    DWORD waitRes = WaitForSingleObject(pi.hProcess, timeoutMs > 0 ? timeoutMs : INFINITE);
    if (waitRes == WAIT_TIMEOUT) {
        result.timedOut = true;
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000);
    }

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = static_cast<int>(exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (outThread.joinable()) outThread.join();
    if (errThread.joinable()) errThread.join();

    CloseHandle(hStdOutRead);
    CloseHandle(hStdErrRead);

    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    result.stdOut = outStr;
    result.stdErr = errStr;

    return result;
}

} // namespace

#endif

ProcessResult ProcessExecutor::executeShell(
    const std::string& command,
    const std::string& workingDir,
    int timeoutMs
) {
#if defined(_WIN32)
    // On Windows, run in powershell.exe
    std::string fullCmd = "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"" + command + "\"";
    return runProcessWin32(toWide(fullCmd), toWide(workingDir), timeoutMs);
#else
    // On Linux/POSIX, run in bash -c
    auto startTime = std::chrono::steady_clock::now();
    ProcessResult result;

    int outPipe[2];
    int errPipe[2];
    if (pipe(outPipe) != 0 || pipe(errPipe) != 0) {
        result.exitCode = -1;
        result.stdErr = "pipe() failed";
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(outPipe[0]);
        close(outPipe[1]);
        close(errPipe[0]);
        close(errPipe[1]);
        result.exitCode = -1;
        result.stdErr = "fork() failed";
        return result;
    }

    if (pid == 0) {
        // Child process
        close(outPipe[0]);
        close(errPipe[0]);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);
        close(outPipe[1]);
        close(errPipe[1]);

        if (!workingDir.empty()) {
            if (chdir(workingDir.c_str()) != 0) {
                // Ignore chdir failure or let shell handle
            }
        }

        execl("/bin/bash", "bash", "-c", command.c_str(), (char*)NULL);
        _exit(127);
    }

    // Parent process: close write ends
    close(outPipe[1]);
    close(errPipe[1]);

    std::string outStr, errStr;

    // Concurrent pipe reading to prevent deadlock when pipe buffers fill up
    auto readPipeFd = [](int fd, std::string& buffer) {
        char buf[4096];
        ssize_t bytesRead = 0;
        while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
            buffer.append(buf, bytesRead);
        }
        close(fd);
    };

    std::thread outThread(readPipeFd, outPipe[0], std::ref(outStr));
    std::thread errThread(readPipeFd, errPipe[0], std::ref(errStr));

    // Wait for child process with timeout support
    int status = 0;
    bool finished = false;
    auto timeoutDuration = std::chrono::milliseconds(timeoutMs > 0 ? timeoutMs : 3600000);
    auto pollInterval = std::chrono::milliseconds(10);

    while (true) {
        pid_t waitRes = waitpid(pid, &status, WNOHANG);
        if (waitRes == pid) {
            finished = true;
            break;
        } else if (waitRes < 0) {
            break;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime
        );
        if (elapsed > timeoutDuration) {
            result.timedOut = true;
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            finished = true;
            break;
        }

        std::this_thread::sleep_for(pollInterval);
    }

    if (outThread.joinable()) outThread.join();
    if (errThread.joinable()) errThread.join();

    auto endTime = std::chrono::steady_clock::now();
    result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    result.exitCode = (finished && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    result.stdOut = outStr;
    result.stdErr = errStr;
    return result;
#endif
}

ProcessResult ProcessExecutor::executeProgram(
    const std::string& program,
    const std::vector<std::string>& args,
    const std::string& workingDir,
    int timeoutMs
) {
#if defined(_WIN32)
    std::string cmdLine = "\"" + program + "\"";
    for (const auto& arg : args) {
        cmdLine += " \"" + arg + "\"";
    }
    return runProcessWin32(toWide(cmdLine), toWide(workingDir), timeoutMs);
#else
    std::string cmd = program;
    for (const auto& arg : args) {
        cmd += " \"" + arg + "\"";
    }
    return executeShell(cmd, workingDir, timeoutMs);
#endif
}

} // namespace arenafight
