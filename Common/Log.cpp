//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

namespace Logging
{
    int log_level = Error;
    FILE *log_file = null;
    FILE *error_log_file = null;
} // namespace Log

//////////////////////////////////////////////////////////////////////

void Log_SetOutputFiles(FILE *log_file, FILE *error_log_file)
{
    Logging::log_file = log_file;
    Logging::error_log_file = error_log_file;
}

//////////////////////////////////////////////////////////////////////

void Log_SetLevel(int level)
{
    if(level < min_log_level) {
        level = (int)min_log_level;
    }
    if(level > max_log_level) {
        level = (int)max_log_level;
    }
    Logging::log_level = level;
}
