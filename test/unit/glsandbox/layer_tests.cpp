#include "glsandbox/scenegraph.hpp"
#include "gtest/gtest.h"

using namespace glsandbox;

class layers_test : public ::testing::Test
{
protected:
  layers_test()
  {
    scenegraph_init();
  }
  virtual ~layers_test() {}
};

TEST_F(layers_test, add_view) {
  view_id v = add_view();
  ASSERT_TRUE(is_view_enabled(v));
}

TEST_F(layers_test, set_view_enabled) {
  view_id v = add_view();
  set_view_enabled(v, false);
  ASSERT_FALSE(is_view_enabled(v));
}

TEST_F(layers_test, add_layer_negative) {
  ASSERT_EQ(add_layer(0), nlayer);
}

TEST_F(layers_test, add_layer_positive) {
  view_id v = add_view();
  ASSERT_NE(add_layer(v), nlayer);
}

TEST_F(layers_test, is_layer_enabled_negative) {
  ASSERT_FALSE(is_layer_enabled(nlayer));
}

TEST_F(layers_test, is_layer_enabled_positive) {
  view_id v = add_view();
  layer_id l = add_layer(v);
  ASSERT_TRUE(is_layer_enabled(l));
}

TEST_F(layers_test, set_layer_enabled_negative) {
  set_layer_enabled(nlayer, false);
}

TEST_F(layers_test, set_layer_enabled_positive) {
  view_id v = add_view();
  layer_id l = add_layer(v);
  set_layer_enabled(l, false);
  ASSERT_FALSE(is_layer_enabled(l));
  set_layer_enabled(l, true);
  ASSERT_TRUE(is_layer_enabled(l));
}

TEST_F(layers_test, get_first_layer_negative1) {
  ASSERT_EQ(get_first_layer(0), nlayer);
}

TEST_F(layers_test, get_first_layer_negative2) {
  view_id v = add_view();
  layer_id l = get_first_layer(v);
  ASSERT_EQ(l, nlayer);
}

TEST_F(layers_test, get_first_layer_positive) {
  view_id v = add_view();
  layer_id l1 = add_layer(v);
  layer_id l2 = get_first_layer(v);
  ASSERT_NE(l1, nlayer);
  ASSERT_NE(l2, nlayer);
  ASSERT_EQ(l1, l2);
}

TEST_F(layers_test, get_next_layer_negative) {
  ASSERT_EQ(get_next_layer(add_layer(add_view())), nlayer);
}

TEST_F(layers_test, get_next_layer_positive) {
  view_id v1 = add_view();
  for (unsigned int i = 0; i < 10; i++) {
    add_layer(v1);
  }

  view_id v2 = add_view();
  std::vector<layer_id> layers;
  for (unsigned int i = 0; i < 10; i++) {
    layers.push_back(add_layer(v2));
  }
  unsigned int i = 0;
  layer_id l = nlayer;
  for (i = 0, l = get_first_layer(v2); l != nlayer; l = get_next_layer(l), i++) {
    ASSERT_EQ(l, layers[i]);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
