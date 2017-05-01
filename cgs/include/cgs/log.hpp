#ifndef LOG_HPP
#define LOG_HPP

#include <cstddef>
#include <string>

namespace cgs
{
  //-----------------------------------------------------------------------------------------------
  // Types
  //-----------------------------------------------------------------------------------------------
  typedef unsigned int log_level;
  typedef void (*logstream_callback)(log_level level, const char * message);

  //-----------------------------------------------------------------------------------------------
  // Constants
  //-----------------------------------------------------------------------------------------------
  constexpr const char* const DEFAULT_LOGSTREAM_FILENAME = "cgs.log";
  constexpr std::size_t MAX_MESSAGE_LENGTH = 2048;
  constexpr log_level LOG_LEVEL_DEBUG = 0;
  constexpr log_level LOG_LEVEL_ERROR = 1;

  //-----------------------------------------------------------------------------------------------
  //! @brief Initializes the logging system.
  //! @remarks This function can be called several times during program execution. After a call to
  //!  this function it is guaranteed that all logstreams have been detached and the log file of the
  //!  default_logstream_file_callback has been closed.
  //----------------------------------------------------------------------------------------------
  void log_init();

  //-----------------------------------------------------------------------------------------------
  //! @brief Logs a message.
  //! @param level The level of the message, to be stored together with the message.
  //! @param message The message to log.
  //! @remarks This function broadcasts the message to all attached logstreams. If no
  //!  logstreams are attached, it does nothing.
  //----------------------------------------------------------------------------------------------
  void log(log_level level, const char* message);

  //-----------------------------------------------------------------------------------------------
  //! @brief Convenience overload for std::string messages. See the other taking a C-string.
  //----------------------------------------------------------------------------------------------
  void log(log_level level, const std::string& message);

  //-----------------------------------------------------------------------------------------------
  //! @brief Attaches a logstream callback.
  //! @param callback The callback to invoke.
  //! @remarks The attached callback will be called every time one of the log functions is
  //!  called, passing the message and its level.
  //! @remarks If the callback being attached is default_logstream_file_callback, this function
  //!  opens the log file.
  //----------------------------------------------------------------------------------------------
  void attach_logstream(logstream_callback callback);

  //-----------------------------------------------------------------------------------------------
  //! @brief Detaches a logstream.
  //! @param callback The logstream callback to detach.
  //! @remarks After a call to this function, the given callback will not be called anymore
  //!  when messages are logged.
  //! @remarks If the callback being detached is default_logstream_file_callback, this function
  //!  closes the log file. This is useful to force the shuffling of the output buffer without
  //!  having to wait for the automatic shuffle at program termination.
  //----------------------------------------------------------------------------------------------
  void detach_logstream(logstream_callback callback);

  //-----------------------------------------------------------------------------------------------
  //! @brief Detaches all attached logstreams.
  //! @remarks If one of the attached callbacks is default_logstream_file_callback, this function
  //!  closes the log file. This is useful to force the shuffling of the output buffer without
  //!  having to wait for the automatic shuffle at program termination.
  //----------------------------------------------------------------------------------------------
  void detach_all_logstreams();

  //-----------------------------------------------------------------------------------------------
  //! @brief Logstream callback that saves the messages to an internal limited-size circular queue.
  //! @param level The message level.
  //! @param message The message to log.
  //! @remarks After the internal queue becomes full, new messages will rotate it. Attach this
  //!  logstream if you want to be able to access the most recent messages without storing the
  //!  whole history. If you need the full history, see default_logstream_file_callback.
  //! @remarks Use function default_logstream_tail_pop to retrieve the saved messages.
  //----------------------------------------------------------------------------------------------
  void default_logstream_tail_callback(log_level level, const char* message);

  //-----------------------------------------------------------------------------------------------
  //! @brief Extracts one message from the default_logstream_tail_callback internal queue and
  //!  saves it in message.
  //! @param message Buffer to store the message. The maximum message length (without
  //!  counting the terminating null character) is MAX_MESSAGE_LENGTH, so you can use this as
  //!  buffer size.
  //! @param level The message level.
  //----------------------------------------------------------------------------------------------
  bool default_logstream_tail_pop(char* message, log_level& level);

  //-----------------------------------------------------------------------------------------------
  //! @brief Convenience function to extract all messages stored in the
  //!  default_logstream_file_callback internal queue and print them to standard output.
  //! @param min_level The minimum level to print. Only messages with level equal or higher than
  //!  min_level will be printed.
  //----------------------------------------------------------------------------------------------
  void default_logstream_tail_dump(log_level min_level);

  //-----------------------------------------------------------------------------------------------
  //! @brief Logstream callback that saves all messages to file DEFAULT_LOGSTREAM_FILENAME in
  //!  the current directory (see constant definition above for the actual file name).
  //! @param level The message level.
  //! @param message The message to log.
  //! @remarks Attach this logstream if you want to keep the whole log history.
  //----------------------------------------------------------------------------------------------
  void default_logstream_file_callback(log_level level, const char* message);

  //-----------------------------------------------------------------------------------------------
  //! @brief Logstream callback that prints messages to standard output.
  //! @param level The message level.
  //! @param message The message to log.
  //----------------------------------------------------------------------------------------------
  void default_logstream_stdout_callback(log_level level, const char* message);
} // namespace cgs

#endif // LOG_HPP
