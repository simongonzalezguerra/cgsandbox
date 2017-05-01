#include "cgs/log.hpp"
#include "gtest/gtest.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

const char* DUDE_1 = "That rug really tied the room together";

using namespace cgs;

class log_test : public ::testing::Test
{
protected:
  log_test()
  {
    custom_logstream_callback1_invocations = 0U;
    custom_logstream_callback2_invocations = 0U;
    std::remove(DEFAULT_LOGSTREAM_FILENAME);
    log_init();
  }

  static void custom_logstream_callback1(log_level, const char*)
  {
    custom_logstream_callback1_invocations++;
  }

  static void custom_logstream_callback2(log_level, const char*)
  {
    custom_logstream_callback2_invocations++;
  }
  virtual ~log_test() {}

  static unsigned int custom_logstream_callback1_invocations;
  static unsigned int custom_logstream_callback2_invocations;
};

unsigned int log_test::custom_logstream_callback1_invocations = 0U;
unsigned int log_test::custom_logstream_callback2_invocations = 0U;

TEST_F(log_test, log_positive_1) {
  attach_logstream(default_logstream_tail_callback);
  log(LOG_LEVEL_ERROR, DUDE_1);
  char message[MAX_MESSAGE_LENGTH + 1];
  log_level level = LOG_LEVEL_DEBUG;
  ASSERT_TRUE(default_logstream_tail_pop(message, level));
  ASSERT_EQ(std::strcmp(message, DUDE_1), 0);
  ASSERT_EQ(level, LOG_LEVEL_ERROR);
  ASSERT_FALSE(default_logstream_tail_pop(message, level));
}

TEST_F(log_test, log_positive_2) {
  attach_logstream(default_logstream_tail_callback);
  log(LOG_LEVEL_ERROR, std::string(DUDE_1));
  char message[MAX_MESSAGE_LENGTH + 1];
  log_level level = LOG_LEVEL_DEBUG;
  ASSERT_TRUE(default_logstream_tail_pop(message, level));
  ASSERT_EQ(std::strcmp(message, DUDE_1), 0);
  ASSERT_EQ(level, LOG_LEVEL_ERROR);
  ASSERT_FALSE(default_logstream_tail_pop(message, level));
}

TEST_F(log_test, log_negative1) {
  attach_logstream(default_logstream_tail_callback);
  constexpr unsigned int length = MAX_MESSAGE_LENGTH + 100;
  char very_long_string[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    very_long_string[i] = 'A';
  }
  very_long_string[length] = '\0';
  log(LOG_LEVEL_DEBUG, very_long_string);
  char message[MAX_MESSAGE_LENGTH + 1];
  log_level level = LOG_LEVEL_ERROR;
  ASSERT_TRUE(default_logstream_tail_pop(message, level));
  ASSERT_EQ(std::strncmp(message, very_long_string, MAX_MESSAGE_LENGTH), 0);
  ASSERT_EQ(level, LOG_LEVEL_DEBUG);
  ASSERT_EQ(message[MAX_MESSAGE_LENGTH], '\0');
  ASSERT_FALSE(default_logstream_tail_pop(message, level));
}

TEST_F(log_test, log_negative2) {
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 0U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 0U);
}

TEST_F(log_test, attach_logstream_positive1) {
  attach_logstream(log_test::custom_logstream_callback1);
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 0U);
  detach_logstream(log_test::custom_logstream_callback1);
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 0U);
  attach_logstream(log_test::custom_logstream_callback2);
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 1U);
  detach_logstream(log_test::custom_logstream_callback1);
  detach_logstream(log_test::custom_logstream_callback2);
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 1U);
}

TEST_F(log_test, attach_logstream_positive2) {
  std::ifstream ifs;
  ifs.open(DEFAULT_LOGSTREAM_FILENAME);
  ASSERT_TRUE(ifs.fail());
  attach_logstream(default_logstream_file_callback);
  ifs.open(DEFAULT_LOGSTREAM_FILENAME);
  ASSERT_FALSE(ifs.fail());
  ifs.close();
  attach_logstream(default_logstream_file_callback);
  ifs.open(DEFAULT_LOGSTREAM_FILENAME);
  ASSERT_FALSE(ifs.fail());
  ifs.close();
}

TEST_F(log_test, detach_all_logstreams_positive) {
  attach_logstream(log_test::custom_logstream_callback1);
  attach_logstream(log_test::custom_logstream_callback2);
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 1U);
  detach_all_logstreams();
  log(LOG_LEVEL_DEBUG, "A");
  ASSERT_EQ(log_test::custom_logstream_callback1_invocations, 1U);
  ASSERT_EQ(log_test::custom_logstream_callback2_invocations, 1U); 
}

TEST_F(log_test, default_logstream_tail_pop_positive) {
  attach_logstream(default_logstream_tail_callback);
  std::vector<std::string> messages;
  for (unsigned int i = 0; i < 200; i++) {
    std::ostringstream oss;
    oss << (i + 1);
    messages.push_back(oss.str());
    log(LOG_LEVEL_ERROR, oss.str());
  }
  char message[MAX_MESSAGE_LENGTH + 1];
  log_level level = LOG_LEVEL_DEBUG;
  default_logstream_tail_pop(message, level);
  auto it = std::find(messages.begin(), messages.end(), std::string(message));
  ASSERT_TRUE(it != messages.end());
  it++;

  while (default_logstream_tail_pop(message, level) && level >= LOG_LEVEL_ERROR) {
    ASSERT_EQ(level, LOG_LEVEL_ERROR);
    ASSERT_EQ(std::string(message), *it);
    std::cout << message << "\n";
    it++;
  }
}

TEST_F(log_test, default_logstream_tail_dump_positive) {
  attach_logstream(default_logstream_tail_callback);
  for (unsigned int i = 0; i < 16; i++) {
    std::ostringstream oss;
    oss << (i + 1);
    log(LOG_LEVEL_ERROR, oss.str());
  }
  default_logstream_tail_dump(LOG_LEVEL_ERROR);
}

TEST_F(log_test, default_logstream_file_callback_positive1) {
  attach_logstream(default_logstream_file_callback);
  log(LOG_LEVEL_DEBUG, "ABC");
  log(LOG_LEVEL_DEBUG, "DEF");
  log(LOG_LEVEL_DEBUG, "GHI");
  detach_logstream(default_logstream_file_callback);
  std::ifstream ifs(DEFAULT_LOGSTREAM_FILENAME);
  char line[10];
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "ABC"), 0);
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "DEF"), 0);
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "GHI"), 0);
  ifs.getline(line, 10);
  ASSERT_TRUE(ifs.fail());
}

TEST_F(log_test, default_logstream_file_callback_positive2) {
  // Exactly the same as default_logstream_file_callback_positive1, but we detach with detach_all_logstreams instead of detach_logstream
  attach_logstream(default_logstream_file_callback);
  log(LOG_LEVEL_DEBUG, "ABC");
  log(LOG_LEVEL_DEBUG, "DEF");
  log(LOG_LEVEL_DEBUG, "GHI");
  detach_all_logstreams();
  std::ifstream ifs(DEFAULT_LOGSTREAM_FILENAME);
  char line[10];
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "ABC"), 0);
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "DEF"), 0);
  ifs.getline(line, 10);
  ASSERT_EQ(strcmp(line, "GHI"), 0);
  ifs.getline(line, 10);
  ASSERT_TRUE(ifs.fail());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
