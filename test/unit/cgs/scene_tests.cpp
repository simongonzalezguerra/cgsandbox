#include "cgs/scenegraph.hpp"
#include "gtest/gtest.h"

using namespace cgs;

class scenes_test : public ::testing::Test
{
protected:
  scenes_test()
  {
    scenegraph_init();
  }
  virtual ~scenes_test() {}
};

TEST_F(scenes_test, add_view) {
  view_id v = add_view();
  ASSERT_TRUE(is_view_enabled(v));
}

TEST_F(scenes_test, set_view_enabled) {
  view_id v = add_view();
  set_view_enabled(v, false);
  ASSERT_FALSE(is_view_enabled(v));
}

TEST_F(scenes_test, add_scene_negative) {
  ASSERT_EQ(add_scene(0), nscene);
}

TEST_F(scenes_test, add_scene_positive) {
  view_id v = add_view();
  ASSERT_NE(add_scene(v), nscene);
}

TEST_F(scenes_test, is_scene_enabled_negative) {
  ASSERT_FALSE(is_scene_enabled(nscene));
}

TEST_F(scenes_test, is_scene_enabled_positive) {
  view_id v = add_view();
  scene_id l = add_scene(v);
  ASSERT_TRUE(is_scene_enabled(l));
}

TEST_F(scenes_test, set_scene_enabled_negative) {
  set_scene_enabled(nscene, false);
}

TEST_F(scenes_test, set_scene_enabled_positive) {
  view_id v = add_view();
  scene_id l = add_scene(v);
  set_scene_enabled(l, false);
  ASSERT_FALSE(is_scene_enabled(l));
  set_scene_enabled(l, true);
  ASSERT_TRUE(is_scene_enabled(l));
}

TEST_F(scenes_test, get_first_scene_negative1) {
  ASSERT_EQ(get_first_scene(0), nscene);
}

TEST_F(scenes_test, get_first_scene_negative2) {
  view_id v = add_view();
  scene_id l = get_first_scene(v);
  ASSERT_EQ(l, nscene);
}

TEST_F(scenes_test, get_first_scene_positive) {
  view_id v = add_view();
  scene_id l1 = add_scene(v);
  scene_id l2 = get_first_scene(v);
  ASSERT_NE(l1, nscene);
  ASSERT_NE(l2, nscene);
  ASSERT_EQ(l1, l2);
}

TEST_F(scenes_test, get_next_scene_negative) {
  ASSERT_EQ(get_next_scene(add_scene(add_view())), nscene);
}

TEST_F(scenes_test, get_next_scene_positive) {
  view_id v1 = add_view();
  for (unsigned int i = 0; i < 10; i++) {
    add_scene(v1);
  }

  view_id v2 = add_view();
  std::vector<scene_id> scenes;
  for (unsigned int i = 0; i < 10; i++) {
    scenes.push_back(add_scene(v2));
  }
  unsigned int i = 0;
  scene_id l = nscene;
  for (i = 0, l = get_first_scene(v2); l != nscene; l = get_next_scene(l), i++) {
    ASSERT_EQ(l, scenes[i]);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
