#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "nlohmann/json.hpp"
#include "glsandbox/log.hpp"
#include "gtest/gtest.h"

struct video_mode
{
  video_mode() : m_width(0), m_height(0), m_fullscreen(false) {}

  unsigned int m_width;
  unsigned int m_height;
  bool m_fullscreen;
};

struct resource_def
{
  resource_def() : m_res_id(0), m_path() {}

  unsigned int m_res_id;
  std::string m_path;
};

struct configuration
{
  configuration() {}

  video_mode m_video_mode;
  std::vector<resource_def> m_resource_defs;
};

configuration make_config(const nlohmann::json& json)
{
  configuration config;
  config.m_video_mode.m_width = json["video_mode"]["width"];
  config.m_video_mode.m_height = json["video_mode"]["height"];
  config.m_video_mode.m_fullscreen = json["video_mode"]["fullscreen"];
  auto resource_defs = json["resource_defs"];
  for (nlohmann::json::iterator it = resource_defs.begin(); it != resource_defs.end(); ++it) {
    resource_def spec;
    spec.m_res_id = (*it)["id"];
    spec.m_path = (*it)["path"];
    config.m_resource_defs.push_back(spec);
  }

  return config;
}

class config_test : public ::testing::Test
{
protected:
  config_test() {}
  virtual ~config_test() {}
};

TEST_F(config_test, load_file) {
  std::ifstream ifs("../../../../test/unit/json/example.json");
  nlohmann::json j;
  ifs >> j;
}

TEST_F(config_test, load_config) {
  std::ifstream ifs("../../../../test/unit/json/config.json");
  nlohmann::json j;
  ifs >> j;

  std::cout << "Read json value: " << std::endl << std::setw(4) << j << std::endl;
  configuration config = make_config(j);
  std::cout << "Read config" << std::endl;
  ASSERT_EQ(config.m_video_mode.m_width, 1920U);
  ASSERT_EQ(config.m_video_mode.m_height, 1080U);
  ASSERT_EQ(config.m_video_mode.m_fullscreen, false);
  ASSERT_EQ(config.m_resource_defs.size(), 3U);
  ASSERT_EQ(config.m_resource_defs.at(0).m_res_id, 1U);
  ASSERT_EQ(config.m_resource_defs.at(0).m_path, "resource1");
  ASSERT_EQ(config.m_resource_defs.at(1).m_res_id, 2U);
  ASSERT_EQ(config.m_resource_defs.at(1).m_path, "resource2");
  ASSERT_EQ(config.m_resource_defs.at(2).m_res_id, 3U);
  ASSERT_EQ(config.m_resource_defs.at(2).m_path, "resource3");
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
