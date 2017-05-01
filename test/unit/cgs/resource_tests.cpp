#include "cgs/resource_database.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gtest/gtest.h"
#include "glm/glm.hpp"

#include <cstddef> // for size_t

using namespace cgs;

class resources_test : public ::testing::Test
{
protected:
  resources_test() :
    local_transform_in{1.0f},
    local_transform_out(nullptr),
    num_meshes_in(0U),
    meshes_in{},
    num_meshes_out(2U),
    meshes_out(nullptr)
  {
    resource_database_init();
  }
  virtual ~resources_test() {}

  glm::mat4 local_transform_in;
  const float* local_transform_out;
  std::size_t num_meshes_in;
  mesh_id meshes_in[max_meshes_by_resource];
  std::size_t num_meshes_out;
  const mesh_id* meshes_out;
};

TEST_F(resources_test, resource_database_init) {
  get_resource_properties(root_resource, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_NE(meshes_out, nullptr);
  ASSERT_NE(local_transform_out, nullptr);
  ASSERT_EQ(num_meshes_out, 0U);
  ASSERT_EQ(glm::make_mat4(local_transform_out), glm::mat4{1.0f});
}

TEST_F(resources_test, add_resource_negative) {
  add_resource(nresource);
}

TEST_F(resources_test, add_resource_positive) {
  ASSERT_NE(add_resource(root_resource), nresource);
}

TEST_F(resources_test, get_resource_properties_negative) {
  resource_id r = add_resource(root_resource);
  get_resource_properties(nresource, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_EQ(meshes_out, nullptr);
  ASSERT_EQ(num_meshes_out, 2U);
  ASSERT_EQ(local_transform_out, nullptr);
  ASSERT_EQ(local_transform_out, nullptr);
  get_resource_properties(r, nullptr, &num_meshes_out, &local_transform_out);
  ASSERT_EQ(meshes_out, nullptr);
  ASSERT_EQ(num_meshes_out, 2U);
  ASSERT_EQ(local_transform_out, nullptr);
  ASSERT_EQ(local_transform_out, nullptr);
  get_resource_properties(r, &meshes_out, nullptr, &local_transform_out);
  ASSERT_EQ(meshes_out, nullptr);
  ASSERT_EQ(num_meshes_out, 2U);
  ASSERT_EQ(local_transform_out, nullptr);
  ASSERT_EQ(local_transform_out, nullptr);
  get_resource_properties(r, &meshes_out, &num_meshes_out, nullptr);
  ASSERT_EQ(meshes_out, nullptr);
  ASSERT_EQ(num_meshes_out, 2U);
  ASSERT_EQ(local_transform_out, nullptr);
  ASSERT_EQ(local_transform_out, nullptr);
}

TEST_F(resources_test, get_resource_properties_positive) {
  meshes_out = nullptr;
  num_meshes_out = 2U;
  local_transform_out = nullptr;
  get_resource_properties(add_resource(root_resource), &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_NE(meshes_out, nullptr);
  ASSERT_NE(local_transform_out, nullptr);
  ASSERT_EQ(num_meshes_out, 0U);
  ASSERT_EQ(glm::make_mat4(local_transform_out), glm::mat4{1.0f});
}

TEST_F(resources_test, set_resource_properties_negative1) {
  set_resource_properties(nresource, meshes_in, num_meshes_in, glm::value_ptr(local_transform_in));
}

TEST_F(resources_test, set_resource_properties_negative2) {
  resource_id r = add_resource(root_resource);
  set_resource_properties(r, meshes_in, num_meshes_in, nullptr);
  get_resource_properties(r, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_NE(meshes_out, nullptr);
  ASSERT_NE(local_transform_out, nullptr);
  ASSERT_EQ(num_meshes_out, 0U);
  ASSERT_EQ(glm::make_mat4(local_transform_out), glm::mat4{1.0f});
}

TEST_F(resources_test, set_resource_properties_negative3) {
  resource_id r = add_resource(root_resource);
  meshes_in[0] = 4U;
  meshes_in[1] = 7U;
  meshes_in[2] = 9U;
  num_meshes_in = 3;
  set_resource_properties(r, meshes_in, num_meshes_in, glm::value_ptr(local_transform_in));
  get_resource_properties(r, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_NE(meshes_out, nullptr);
  ASSERT_NE(local_transform_out, nullptr);
  ASSERT_EQ(num_meshes_out, 0U);
  ASSERT_EQ(glm::make_mat4(local_transform_out), glm::mat4{1.0f});
}

TEST_F(resources_test, set_resource_properties_negative4) {
  resource_id r = add_resource(root_resource);
  set_resource_properties(r, nullptr, 5U, glm::value_ptr(local_transform_in));
}

TEST_F(resources_test, set_resource_properties_negative5) {
  resource_id r = add_resource(root_resource);
  set_resource_properties(r, meshes_in, num_meshes_in, nullptr);
}

TEST_F(resources_test, set_resource_properties_positive1) {
  resource_id r = add_resource(root_resource);
  meshes_in[0] = add_mesh();
  meshes_in[1] = add_mesh();
  meshes_in[2] = add_mesh();
  num_meshes_in = 3;
  glm::rotate(local_transform_in, 45.0f, glm::vec3(0.23f, 0.1f, 0.89f));
  set_resource_properties(r, meshes_in, num_meshes_in, glm::value_ptr(local_transform_in));
  get_resource_properties(r, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_EQ(num_meshes_out, 3U);
  ASSERT_NE(meshes_out, nullptr);
  ASSERT_EQ(meshes_out[0], meshes_in[0]);
  ASSERT_EQ(meshes_out[1], meshes_in[1]);
  ASSERT_EQ(meshes_out[2], meshes_in[2]);
  ASSERT_NE(local_transform_out, nullptr);
  ASSERT_EQ(glm::make_mat4(local_transform_out), local_transform_in);
}

TEST_F(resources_test, get_first_child_resource_negative) {
  ASSERT_EQ(get_first_child_resource(nresource), nresource);
}

TEST_F(resources_test, get_first_child_resource_positive1) {
  resource_id r = add_resource(root_resource);
  resource_id o = add_resource(r);
  ASSERT_EQ(get_first_child_resource(root_resource), r);
  ASSERT_EQ(get_first_child_resource(r), o);
}

TEST_F(resources_test, get_next_sibling_resource_negative) {
  ASSERT_EQ(get_next_sibling_resource(nresource), nresource);
}

TEST_F(resources_test, get_next_sibling_resource_positive) {
  resource_id r1 = add_resource(root_resource);
  resource_id r2 = add_resource(root_resource);
  resource_id r3 = add_resource(root_resource);
  ASSERT_EQ(get_first_child_resource(root_resource), r1);
  ASSERT_EQ(get_next_sibling_resource(root_resource), nresource);
  ASSERT_EQ(get_first_child_resource(r1), nresource);
  ASSERT_EQ(get_next_sibling_resource(r1), r2);
  ASSERT_EQ(get_first_child_resource(r2), nresource);
  ASSERT_EQ(get_next_sibling_resource(r2), r3);
  ASSERT_EQ(get_first_child_resource(r3), nresource);
  ASSERT_EQ(get_next_sibling_resource(r3), nresource);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
