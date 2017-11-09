#include "glm/gtc/matrix_transform.hpp"
#include "cgs/scenegraph.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "test/test_utils.hpp"
#include "gtest/gtest.h"
#include "glm/glm.hpp"

#include <cstddef> // for size_t

using namespace cgs;

class nodes_test : public ::testing::Test
{
protected:
  nodes_test() :
    v(0),
    l(nlayer)
  {
    scenegraph_init();
    resource_database_init();
    v = add_view();
    l = add_layer(v);
  }

  virtual ~nodes_test() {}

  view_id v;
  layer_id l;
};

TEST_F(nodes_test, add_node_negative1) {
  add_node(nlayer, nnode);
}

TEST_F(nodes_test, add_node_negative2) {
  auto n = add_node(l);
  remove_node(l, n);
  n = add_node(l);
}

TEST_F(nodes_test, add_node_positive) {
  auto n1 = add_node(l);
  auto n2 = add_node(l);
  auto n3 = add_node(l);
  ASSERT_EQ(get_next_sibling_node(l, n1), n2);
  ASSERT_EQ(get_next_sibling_node(l, n2), n3);
  ASSERT_EQ(get_next_sibling_node(l, n3), nnode);
}

TEST_F(nodes_test, remove_node_negative1) {
  remove_node(nlayer, nnode);
  remove_node(l, nnode);
}

TEST_F(nodes_test, remove_node_positive1) {
  auto n1 = add_node(l);
  remove_node(l, n1);
}

TEST_F(nodes_test, remove_node_positive2) {
  auto n1 = add_node(l);
  auto n2 = add_node(l);
  auto n3 = add_node(l);
  remove_node(l, n2);
  ASSERT_EQ(get_next_sibling_node(l, n1), n3);
}

TEST_F(nodes_test, set_node_transform_negative1) {
  glm::mat4 local_transform{1.0f};
  set_node_transform(nlayer, nnode, local_transform);
  set_node_transform(l, nnode, local_transform);
}

TEST_F(nodes_test, set_node_transform_negative2) {
  auto n = add_node(l);
  remove_node(l, n);
  glm::mat4 local_transform{1.0f};
  set_node_transform(l, n, local_transform);
}

TEST_F(nodes_test, set_node_transform_positive1) {
  add_node(l);
  auto n2 = add_node(l);
  add_node(l);
  glm::mat4 local_transform_in = 3.0f * glm::mat4{1.0f};
  set_node_transform(l, n2, local_transform_in);
  glm::mat4 a_local_transform_out;
  glm::mat4 a_accum_transform_out;
  get_node_transform(l, n2, &a_local_transform_out, &a_accum_transform_out);
  ASSERT_EQ(a_local_transform_out, local_transform_in);
  ASSERT_EQ(a_accum_transform_out, local_transform_in);
}

TEST_F(nodes_test, set_node_transform_positive2) {
  // Create the following test tree:
  //   root
  //     n11
  //     n12
  //       n121
  //         n1211
  //           n12111
  //           n12112
  //           n12113
  //           n12114
  //           n12115
  //       n122
  //       n123
  //       n124
  //     n13
  float a_data[] = {2.0f, 1.0f, 3.0f, 4.8f, -1.5f, 0.7f, 1.0f, -10.4f, 0.4f, 1.3f, 4.5f, 0.4f, 8.9f, 3.4f, 7.2f, 12.1f};
  glm::mat4 a = glm::make_mat4(a_data);
  node_id root_node = add_node(l);
  set_node_transform(l, root_node, a);
  auto n11 = add_node(l, root_node);
  set_node_transform(l, n11, a);
  auto n12 = add_node(l, root_node);
  set_node_transform(l, n12, a);
  auto n13 = add_node(l, root_node);
  set_node_transform(l, n13, a);
  auto n121 = add_node(l, n12);
  set_node_transform(l, n121, a);
  auto n122 = add_node(l, n12);
  set_node_transform(l, n122, a);
  auto n123 = add_node(l, n12);
  set_node_transform(l, n123, a);
  auto n124 = add_node(l, n12);
  set_node_transform(l, n124, a);
  auto n1211 = add_node(l, n121);
  set_node_transform(l, n1211, a);
  auto n12111 = add_node(l, n1211);
  set_node_transform(l, n12111, a);
  auto n12112 = add_node(l, n1211);
  set_node_transform(l, n12112, a);
  auto n12113 = add_node(l, n1211);
  set_node_transform(l, n12113, a);
  auto n12114 = add_node(l, n1211);
  set_node_transform(l, n12114, a);
  auto n12115 = add_node(l, n1211);
  set_node_transform(l, n12115, a);

  // Update the local transform in node n121
  glm::mat4 mult{1.0f, 3.0f, -4.0f, 10.8f, -3.5f, 12.7f, 1.0f, -4.4f, 13.4f, 1.3f, -4.5f, -5.4f, 2.9f, 3.4f, 11.2f, 17.1f};
  set_node_transform(l, n121, mult * a);

  // Check that the accumulated transforms in all its descendants change accordingly, and the other nodes stay the same
  glm::mat4 local_transform_out;
  glm::mat4 accum_transform_out;
  get_node_transform(l, root_node, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.5f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a, 0.5f);

  get_node_transform(l, n11, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a, 0.1f);

  get_node_transform(l, n12, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a, 0.1f);

  get_node_transform(l, n13, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a, 0.1f);

  get_node_transform(l, n121, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, mult * a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a, 0.1f);

  get_node_transform(l, n122, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * a, 0.1f);

  get_node_transform(l, n123, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * a, 0.1f);

  get_node_transform(l, n124, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * a, 0.1f);

  get_node_transform(l, n1211, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a, 0.1f);

  get_node_transform(l, n12111, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a * a, 1.0f);

  get_node_transform(l, n12112, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a * a, 1.0f);

  get_node_transform(l, n12113, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a * a, 1.0f);

  get_node_transform(l, n12114, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a * a, 1.0f);

  get_node_transform(l, n12115, &local_transform_out, &accum_transform_out);
  ASSERT_MATRIX_NEAR(local_transform_out, a, 0.1f);
  ASSERT_MATRIX_NEAR(accum_transform_out, a * a * mult * a * a * a, 1.0f);
}

TEST_F(nodes_test, get_node_transform_negative) {
  glm::mat4 local_transform_out;
  glm::mat4 accum_transform_out;
  get_node_transform(l, nnode, &local_transform_out, &accum_transform_out);
  node_id n = add_node(l);
  remove_node(l, n);
  get_node_transform(l, n, &local_transform_out, &accum_transform_out);
  get_node_transform(l, n, &local_transform_out, nullptr);
  get_node_transform(l, n, nullptr, nullptr);
}

TEST_F(nodes_test, get_node_transform_positive) {
  // This case is also implicitly tested in set_node_transform_positive2. We keep it here for completeness
  glm::mat4 local_transform_out;
  glm::mat4 accum_transform_out;
  node_id n = add_node(l);
  get_node_transform(l, n, &local_transform_out, &accum_transform_out);
  ASSERT_EQ(glm::mat4{1.0f}, local_transform_out);
  ASSERT_EQ(glm::mat4{1.0f}, accum_transform_out);
}

TEST_F(nodes_test, get_node_meshes_positive) {
  // Already tested implicitly in set_node_meshes_positive
}

TEST_F(nodes_test, set_node_enabled_negative) {
  set_node_enabled(l, nnode, false);
  node_id n = add_node(l);
  remove_node(l, n);
  set_node_enabled(l, n, false);
}

TEST_F(nodes_test, is_node_enabled_negative) {
  node_id n = add_node(l);
  remove_node(l, n);
  ASSERT_EQ(is_node_enabled(l, n), false);
}

TEST_F(nodes_test, is_node_enabled_positive) {
  // Already tested implicitly in set_node_enabled_positive
}

TEST_F(nodes_test, get_next_sibling_node_negative) {
  ASSERT_EQ(get_next_sibling_node(l, nnode), nnode);
  node_id n = add_node(l);
  remove_node(l, n);
  ASSERT_EQ(get_next_sibling_node(l, n), nnode);
}

TEST_F(nodes_test, get_next_sibling_node_positive) {
  node_id n1 = add_node(l);
  node_id n2 = add_node(l);
  node_id n3 = add_node(l);
  ASSERT_EQ(get_next_sibling_node(l, n1), n2);
  ASSERT_EQ(get_next_sibling_node(l, n2), n3);
  ASSERT_EQ(get_next_sibling_node(l, n3), nnode);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
