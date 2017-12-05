#ifndef CMD_LINE_ARGS_HPP
#define CMD_LINE_ARGS_HPP

#include <string>
#include <map>

namespace rte
{
    void cmd_line_args_initialize();
    void cmd_line_args_set_args(unsigned int argc, char** argv);
    void cmd_line_args_finalize();
    bool cmd_line_args_has_option(const std::string& option);
    std::string cmd_line_args_get_option_value(const std::string& option, const std::string& default_value);
} // namespace rte

#endif
