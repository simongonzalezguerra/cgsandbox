#include "cgs/log.hpp"

#include <iostream>
#include <fstream>
#include <cstring>
#include <set>

namespace cgs
{
    namespace
    {
        //---------------------------------------------------------------------------------------------
        // Constants
        //---------------------------------------------------------------------------------------------
        constexpr std::size_t BUFF_SIZE = 50;

        //---------------------------------------------------------------------------------------------
        // Internal data structures
        //---------------------------------------------------------------------------------------------
        struct entry
        {
            entry() : mmessage{0}, mlevel(LOG_LEVEL_DEBUG) {}

            char mmessage[MAX_MESSAGE_LENGTH + 1];
            log_level mlevel;
        };

        entry entries[BUFF_SIZE];                              //!< circular buffer of log entries
        int insert = 0;                                        //!< index of next insertion (points to an empty cell)
        int remove = 0;                                        //!< index of the next removal (if < insert then points to a cell with content)
        std::ofstream log_file;                                //!< stream object to write logs to a file
        std::set<logstream_callback> logstream_callbacks;      //!< collection of attached logstreams

        //---------------------------------------------------------------------------------------------
        // Helper functions
        //---------------------------------------------------------------------------------------------
        unsigned int size()
        {
            return (BUFF_SIZE + insert - remove) % BUFF_SIZE;
        }

        int increment(int index)
        {
            return (index + 1) % BUFF_SIZE;
        }
    }

    //-----------------------------------------------------------------------------------------------
    // Public functions
    //----------------------------------------------------------------------------------------------
    void log_init()
    {
        insert = remove = 0;
        log_file.close();   // will be opened the next time the default_logstream_file_callback is attached as logstream
        logstream_callbacks.clear();
    }

    void log(log_level level, const char* message)
    {
        for (auto c : logstream_callbacks) {
            c(level, message);
        }
    }

    void log(log_level level, const std::string& message)
    {
        log(level, message.c_str());
    }

    void attach_logstream(logstream_callback callback)
    {
        logstream_callbacks.insert(callback);
        if (callback == default_logstream_file_callback && !log_file.is_open()) {
            log_file.open(DEFAULT_LOGSTREAM_FILENAME, std::ios_base::out);
        }
    }

    void detach_logstream(logstream_callback callback)
    {
        if (callback == default_logstream_file_callback && log_file.is_open()) {
            log_file.close();
        }
        logstream_callbacks.erase(callback);
    }

    void detach_all_logstreams()
    {
        if (logstream_callbacks.count(default_logstream_file_callback)) {
            log_file.close();
        }
        logstream_callbacks.clear();
    }

    void default_logstream_tail_callback(log_level level, const char* message)
    {
        std::strncpy(entries[insert].mmessage, message, MAX_MESSAGE_LENGTH);
        entries[insert].mmessage[MAX_MESSAGE_LENGTH] = '\0';
        entries[insert].mlevel = level;
        remove = (size() < BUFF_SIZE - 1)? remove : increment(remove);
        insert = increment(insert);
    }

    bool default_logstream_tail_pop(char* message, log_level& level)
    {
        if (size() == 0) return false;
        std::strncpy(message, entries[remove].mmessage, MAX_MESSAGE_LENGTH);
        message[MAX_MESSAGE_LENGTH] = '\0';
        level = entries[remove].mlevel;
        remove = increment(remove);
        return true;
    }

    void default_logstream_tail_dump(log_level min_level)
    {
        char message[MAX_MESSAGE_LENGTH + 1];
        log_level level = LOG_LEVEL_DEBUG;
        while (default_logstream_tail_pop(message, level) && level >= min_level) {
            std::cout << message << "\n";
        }
    }

    void default_logstream_file_callback(log_level, const char* message)
    {
        log_file << message << "\n";
    }

    void default_logstream_stdout_callback(log_level /* level */, const char* message)
    {
        std::cout << message << "\n";
    }
} // namespace cgs
