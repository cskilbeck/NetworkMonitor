//////////////////////////////////////////////////////////////////////
// TODO (chs): logging sinks
// TODO (chs): filter sinks by log_level (? and regex?)
// 

#pragma once

//////////////////////////////////////////////////////////////////////

// clang-format off
enum LogLevel
{
    max_log_level = 4,
    Debug = 4,
    Verbose = 3,
    Info = 2,
    Warn = 1,
    Error = 0,
    min_log_level = 0
};
// clang-format on

//////////////////////////////////////////////////////////////////////

#if !defined(DISABLE_LOGGING)

void Log_SetOutputFiles(FILE *log_file, FILE *error_log_file);
void Log_SetLevel(int level);

namespace Logging
{
    extern int log_level;
    extern FILE *log_file;
    extern FILE *error_log_file;

} // namespace Logging

//////////////////////////////////////////////////////////////////////

#define __LOG_WIDEN2(x) L##x
#define __LOG_WIDEN(X) __LOG_WIDEN2(X)

namespace
{
    template <typename T> void Log(int level, T const *context, T const *msg, ...);

    struct __loginfo_t
    {
        char const *__log_contextA;
        wchar const *__log_contextW;
    };

    //////////////////////////////////////////////////////////////////////

    template <typename T> struct _log
    {
        static void DebugWrite(T const *s) = delete;
        static void FileWrite(FILE *f, T const *s) = delete;
        static constexpr T const *log_format_string = null;
        static constexpr char const *LOG_Names[] = {};
    };

    //////////////////////////////////////////////////////////////////////

    template <> struct _log<char>
    {
        static char const *get_context(__loginfo_t const &info)
        {
            return info.__log_contextA;
        }

        static void DebugWrite(char const *s)
        {
            OutputDebugStringA(s);
        }
        static void FileWrite(FILE *f, char const *s)
        {
            fputs(s, f);
        }
        static void StdoutWrite(char const *s)
        {
            fputs(s, stdout);
        }
        static void StderrWrite(char const *s)
        {
            fputs(s, stderr);
        }
        static constexpr char const *log_format = "%s %-20.20s %s%s\n";
        static constexpr char const *ansi_stop = ansi_off;
        static constexpr char const *empty_str = "";

        // clang-format off
	    static constexpr char const *LOG_Names[] = {
		    ansi_bold ansi_red      "Error  " ansi_off,
		    ansi_bold ansi_yellow   "Warn   " ansi_off,
		    ansi_bold ansi_green    "Info   " ansi_off,
		    ansi_bold ansi_magenta  "Verbose" ansi_off,
            ansi_bold ansi_cyan     "Debug  " ansi_off
	    };

        static constexpr char const *LOG_BareNames[] = {
		    "Error  ",
		    "Warn   ",
		    "Info   ",
		    "Debug  ",
		    "Verbose"
	    };
        // clang-format on
    };

    //////////////////////////////////////////////////////////////////////

    template <> struct _log<wchar>
    {
        static wchar const *get_context(__loginfo_t const &info)
        {
            return info.__log_contextW;
        }

        static void DebugWrite(wchar const *s)
        {
            OutputDebugStringW(s);
        }
        static void FileWrite(FILE *f, wchar const *s)
        {
            fputws(s, f);
        }
        static void StdoutWrite(wchar const *s)
        {
            fputws(s, stdout);
        }
        static void StderrWrite(wchar const *s)
        {
            fputws(s, stderr);
        }
        static constexpr wchar const *log_format = L"%s %-20.20s %s%s\n";
        static constexpr wchar const *ansi_stop = ansi_offW;
        static constexpr wchar const *empty_str = L"";

        // clang-format off
	static constexpr wchar const *LOG_Names[] = {
		ansi_boldW ansi_redW        L"Error  " ansi_offW,
		ansi_boldW ansi_yellowW     L"Warn   " ansi_offW,
		ansi_boldW ansi_greenW      L"Info   " ansi_offW,
		ansi_boldW ansi_magentaW    L"Verbose" ansi_offW,
        ansi_boldW ansi_cyanW       L"Debug  " ansi_offW
    };
    static constexpr wchar const *LOG_BareNames[] = {
		L"Error  ",
		L"Warn   ",
		L"Info   ",
		L"Debug  ",
		L"Verbose"
	};
        // clang-format on
    };

    //////////////////////////////////////////////////////////////////////

    template <typename T> void Log(int level, __loginfo_t const &info, T const *msg, ...)
    {
        using L = _log<T>;
        using S = str_base<T>;

        if(level > Logging::log_level) {
            return;
        }
        if(level < min_log_level) {
            level = (int)min_log_level;
        }
        if(level > max_log_level) {
            level = (int)max_log_level;
        }
        va_list v;
        va_start(v, msg);

        auto name = L::LOG_Names[level];
        auto bare = L::LOG_BareNames[level];

        // bare (no ansi colors)
        S s = $(L::log_format, bare, L::get_context(info), str_base<T>::format(msg, v).c_str(), L::empty_str);

        // with ansi colors
        S p = $(L::log_format, name, L::get_context(info), str_base<T>::format(msg, v).c_str(), L::ansi_stop);

        L::DebugWrite(s.c_str());

        if(level == Error) {
            L::StderrWrite(s);
            if(Logging::error_log_file != null) {
                L::FileWrite(Logging::error_log_file, s);
            }
        }
        else {
            L::StdoutWrite(s);
            if(Logging::log_file != null) {
                L::FileWrite(Logging::log_file, s);
            }
        }
    }

} // namespace

//////////////////////////////////////////////////////////////////////

#define LOG_Context(x)                     \
    static constexpr __loginfo_t __loginfo \
    {                                      \
        x, __LOG_WIDEN(x)                  \
    }
#define LOG_Print(level, msg, ...) Log(level, __loginfo, msg, __VA_ARGS__)
#define LOG_Verbose(msg, ...) Log(Debug, __loginfo, msg, __VA_ARGS__)
#define LOG_Debug(msg, ...) Log(Verbose, __loginfo, msg, __VA_ARGS__)
#define LOG_Info(msg, ...) Log(Info, __loginfo, msg, __VA_ARGS__)
#define LOG_Warn(msg, ...) Log(Warn, __loginfo, msg, __VA_ARGS__)
#define LOG_Error(msg, ...) Log(Error, __loginfo, msg, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////

#else

#define LOG_Context(c)

inline void Log_SetOutputFiles(FILE *log_file, FILE *error_log_file)
{
}

inline void LOG_SetLevel(int x)
{
}

#define LOG_Print(level, msg, ...) \
    if(false) {                    \
    }                              \
    else
#define LOG_Debug(msg, ...) \
    if(false) {             \
    }                       \
    else
#define LOG_Verbose(msg, ...) \
    if(false) {               \
    }                         \
    else
#define LOG_Info(msg, ...) \
    if(false) {            \
    }                      \
    else
#define LOG_Warn(msg, ...) \
    if(false) {            \
    }                      \
    else
#define LOG_Error(msg, ...) \
    if(false) {             \
    }                       \
    else

//////////////////////////////////////////////////////////////////////

#endif
