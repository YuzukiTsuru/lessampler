#ifndef LESSAMPLER_TESTPROCESS_H
#define LESSAMPLER_TESTPROCESS_H

#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace test {

    /*
 * Run a quoted command line and return the child exit code. On Windows
 * std::system routes through cmd.exe, which mishandles a command that starts
 * with a quoted executable path; CreateProcess parses the same line directly.
 */
    inline int run_command(std::string command) {
#ifdef _WIN32
        STARTUPINFOA startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE, 0,
                            nullptr, nullptr, &startup, &process)) {
            return -1;
        }
        WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exit_code = 1;
        GetExitCodeProcess(process.hProcess, &exit_code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return static_cast<int>(exit_code);
#else
        return std::system(command.c_str());
#endif
    }

}// namespace test

#endif//LESSAMPLER_TESTPROCESS_H
