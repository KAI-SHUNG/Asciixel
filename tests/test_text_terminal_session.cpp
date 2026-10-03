#include "asciixel/io/text_writer.hpp"
#include "asciixel/io/terminal_video_output.hpp"
#include "asciixel/io/terminal_progress.hpp"

#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include <cstdio>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// Test in an inactive native screen buffer, leaving the visible terminal intact.
class TestConsole {
public:
    TestConsole()
        : original_(GetStdHandle(STD_OUTPUT_HANDLE)), saved_fd_(_dup(_fileno(stdout)))
    {
        buffer_ = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
        require(buffer_ != INVALID_HANDLE_VALUE, "test requires a real console");
        SMALL_RECT window{0, 0, 39, 9};
        require(SetConsoleWindowInfo(buffer_, TRUE, &window), "cannot size test window");
        require(SetConsoleScreenBufferSize(buffer_, COORD{40, 10}), "cannot size test buffer");
        const int fd = _open_osfhandle(reinterpret_cast<std::intptr_t>(buffer_), _O_BINARY);
        require(fd != -1 && _dup2(fd, _fileno(stdout)) == 0, "cannot redirect console stdout");
        _close(fd);
        buffer_ = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stdout)));
        require(SetStdHandle(STD_OUTPUT_HANDLE, buffer_), "cannot set test stdout handle");
    }

    ~TestConsole()
    {
        std::cout.flush();
        _dup2(saved_fd_, _fileno(stdout));
        _close(saved_fd_);
        SetStdHandle(STD_OUTPUT_HANDLE, original_);
    }

    HANDLE handle() const { return buffer_; }

private:
    HANDLE original_;
    HANDLE buffer_ = INVALID_HANDLE_VALUE;
    int saved_fd_;
};

void verifiesRelativeOutputAndRestoration()
{
    TestConsole console;
    DWORD original_mode = 0;
    CONSOLE_CURSOR_INFO original_cursor{};
    require(GetConsoleMode(console.handle(), &original_mode), "cannot query original mode");
    require(GetConsoleCursorInfo(console.handle(), &original_cursor), "cannot query cursor");
    require(SetConsoleCursorPosition(console.handle(), COORD{5, 2}), "cannot set initial cursor");
    {
        asciixel::TextTerminalSession terminal;
        require(terminal.size().columns == 40 && terminal.size().rows == 10,
                "visible terminal capacity is incorrect");
        asciixel::AsciiFrame frame(3, 2);
        const std::string characters = "A  #. ";
        for (std::size_t i = 0; i < characters.size(); ++i) {
            frame.pixels[i].character = characters[i];
        }
        asciixel::writeAsciiFrame(frame);
        std::cout.flush();
        CONSOLE_SCREEN_BUFFER_INFO info{};
        require(GetConsoleScreenBufferInfo(console.handle(), &info), "cannot query position");
        require(info.dwCursorPosition.X == 0 && info.dwCursorPosition.Y == 4,
                "LF did not return to column one after each video row");

        frame.pixels.front().character = 'Z';
        asciixel::resetCursor(frame.height);
        asciixel::writeAsciiFrame(frame);
        std::cout.flush();
        char cells[3]{};
        DWORD count = 0;
        require(ReadConsoleOutputCharacterA(console.handle(), cells, 3, COORD{0, 2}, &count),
                "cannot read overwritten frame");
        require(count == 3 && std::string(cells, 3) == "Z  ",
                "second frame was not written over the first frame");
    }
    DWORD restored_mode = 0;
    CONSOLE_CURSOR_INFO restored_cursor{};
    require(GetConsoleMode(console.handle(), &restored_mode), "cannot query restored mode");
    require(GetConsoleCursorInfo(console.handle(), &restored_cursor), "cannot query restored cursor");
    require(restored_mode == original_mode &&
            restored_cursor.bVisible == original_cursor.bVisible,
            "terminal state was not restored on success");

    try {
        asciixel::TextTerminalSession terminal;
        throw std::runtime_error("simulate playback failure");
    }
    catch (const std::runtime_error&) {
    }
    require(GetConsoleMode(console.handle(), &restored_mode) &&
            restored_mode == original_mode, "exception did not restore terminal mode");
    require(GetConsoleCursorInfo(console.handle(), &restored_cursor) &&
            restored_cursor.bVisible == original_cursor.bVisible,
            "exception did not restore cursor visibility");
}

void verifiesTerminalVideoOutput()
{
    TestConsole console;
    DWORD original_mode = 0;
    CONSOLE_CURSOR_INFO original_cursor{};
    require(GetConsoleMode(console.handle(), &original_mode), "cannot query output mode");
    require(GetConsoleCursorInfo(console.handle(), &original_cursor), "cannot query output cursor");
    require(SetConsoleCursorPosition(console.handle(), COORD{5, 2}), "cannot position video output");
    {
        asciixel::TerminalVideoOutput output;
        {
            asciixel::TerminalProgress progress(std::cout, "Converting");
            progress.update(1, 2, "1 frame");
            progress.update(2, 2, "2 frames", true);
        }
        CONSOLE_SCREEN_BUFFER_INFO preparation_cursor{};
        require(GetConsoleScreenBufferInfo(console.handle(), &preparation_cursor) &&
                preparation_cursor.dwCursorPosition.Y == 2 && preparation_cursor.dwCursorPosition.X == 0,
                "progress cleanup must leave playback on its original row");
        asciixel::AsciiFrame frame(3, 2);
        const std::string characters = "A  #. ";
        for (std::size_t i = 0; i < characters.size(); ++i) frame.pixels[i].character = characters[i];
        output.writeFrame(frame);

        // Simulate preparation before the first scheduled wait. That elapsed time
        // must not consume the playback deadline.
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        const auto start = std::chrono::steady_clock::now();
        require(output.waitUntil(20000), "video output wait was interrupted");
        const auto waited = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start).count();
        require(waited >= 15000, "video output clock started during preparation");

        frame.pixels.front().character = 'Z';
        output.writeFrame(frame);
        char cells[3]{};
        DWORD count = 0;
        require(ReadConsoleOutputCharacterA(console.handle(), cells, 3, COORD{0, 2}, &count),
                "cannot read cached video output");
        require(count == 3 && std::string(cells, 3) == "Z  ",
                "terminal output must own the preceding frame cursor reset");

        SMALL_RECT window{0, 0, 1, 0};
        require(SetConsoleWindowInfo(console.handle(), TRUE, &window), "cannot resize output window");
        output.writeFrame(frame);
        require(static_cast<bool>(std::cout), "video output must continue after the window shrinks below the frame size");
    }
    DWORD restored_mode = 0;
    CONSOLE_CURSOR_INFO restored_cursor{};
    require(GetConsoleMode(console.handle(), &restored_mode) && restored_mode == original_mode,
            "video output did not restore terminal mode");
    require(GetConsoleCursorInfo(console.handle(), &restored_cursor) &&
            restored_cursor.bVisible == original_cursor.bVisible,
            "video output did not restore cursor visibility");
}

// Send Ctrl+Break only to the child process group, never to the user's shell.
void verifiesInterruptCleanup(const std::string& program, const std::string& video)
{
    TestConsole console;
    require(SetHandleInformation(console.handle(), HANDLE_FLAG_INHERIT,
                                 HANDLE_FLAG_INHERIT), "cannot inherit test output");
    DWORD original_mode = 0;
    CONSOLE_CURSOR_INFO original_cursor{};
    require(GetConsoleMode(console.handle(), &original_mode), "cannot read mode");
    require(GetConsoleCursorInfo(console.handle(), &original_cursor), "cannot read cursor");

    const auto executable = std::filesystem::absolute(std::filesystem::u8path(program)).wstring();
    const auto input = std::filesystem::absolute(std::filesystem::u8path(video)).wstring();
    std::wstring command = L"\"" + executable + L"\" \"" + input + L"\" --columns 8";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = console.handle();
    startup.hStdError = console.handle();
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    struct ChildProcess {
        PROCESS_INFORMATION info{};
        ~ChildProcess()
        {
            if (info.hProcess && WaitForSingleObject(info.hProcess, 0) == WAIT_TIMEOUT) {
                TerminateProcess(info.hProcess, 1);
            }
            if (info.hThread) CloseHandle(info.hThread);
            if (info.hProcess) CloseHandle(info.hProcess);
        }
    } child;
    require(CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                          CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startup, &child.info),
            "cannot start playback child");

    // Cursor hiding occurs after the interrupt handler is installed.
    CONSOLE_CURSOR_INFO cursor{};
    bool ready = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (GetConsoleCursorInfo(console.handle(), &cursor) && !cursor.bVisible) {
            ready = true;
            break;
        }
        Sleep(10);
    }
    require(ready, "playback child did not initialize its terminal");
    require(GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, child.info.dwProcessId),
            "cannot interrupt playback child");
    require(WaitForSingleObject(child.info.hProcess, 5000) == WAIT_OBJECT_0,
            "playback did not respond to interruption");
    DWORD exit_code = 0;
    require(GetExitCodeProcess(child.info.hProcess, &exit_code) && exit_code == 130,
            "interrupted playback must return 130");
    DWORD restored_mode = 0;
    require(GetConsoleMode(console.handle(), &restored_mode) && restored_mode == original_mode,
            "interrupted playback did not restore terminal mode");
    require(GetConsoleCursorInfo(console.handle(), &cursor) &&
            cursor.bVisible == original_cursor.bVisible,
            "interrupted playback did not restore cursor visibility");
}
}

int main(int argc, char** argv)
{
    try {
        verifiesRelativeOutputAndRestoration();
        verifiesTerminalVideoOutput();
        if (argc == 3) {
            verifiesInterruptCleanup(argv[1], argv[2]);
        }
        std::cout << "Native terminal output and restoration passed\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
