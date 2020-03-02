#include "generaldomo/logging.hpp"
#include <cstdio>

using namespace generaldomo;


void console_log::debug(const std::string& msg)
{
    if (level > log_level::debug) { return; }
    always(msg);
}

void console_log::info(const std::string& msg)
{
    if (level > log_level::info) { return; }
    always(msg);
}


void console_log::error(const std::string& msg)
{
    if (level > log_level::error) { return; }
    always(msg);
}

void console_log::always(const std::string& msg)
{
    time_t curtime = time (NULL);
    struct tm *loctime = localtime (&curtime);
    char formatted[20] = {0};
    strftime (formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    printf ("%s", formatted);
    const char* code=" DIE";
    printf("%c: %s", code[(int)level], msg.c_str());
}
