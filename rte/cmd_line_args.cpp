#include "cmd_line_args.hpp"

#include <string>
#include <map>

namespace rte
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Internal types
        //---------------------------------------------------------------------------------------------
        typedef std::map<std::string, std::string>  string_map;
        typedef string_map::const_iterator          string_map_citerator;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        bool                initialized = false;
        string_map          options;
    } // anonymous namespace

    void cmd_line_args_initialize()
    {
        if (!initialized) {
            options = string_map();
            initialized = true;
        }
    }

    void cmd_line_args_set_args(unsigned int argc, char** argv)
    {
        if (!initialized) return;

        for (unsigned int i = 0; i < argc - 1; i++) {
          options[argv[i]] = argv[i + 1];
        }

        options[argv[argc - 1]] = "";
    }

    void cmd_line_args_finalize()
    {
        if (initialized) {
            options = string_map();
            initialized = false;
        }
    }

    bool cmd_line_args_has_option(const std::string& option)
    {
        return (initialized && options.find(option) != options.end());
    }

    std::string cmd_line_args_get_option_value(const std::string& option, const std::string& default_value)
    {
        string_map_citerator it = options.find(option);
        return (initialized && it != options.end() ? it->second : default_value);
    }
} // namespace rte