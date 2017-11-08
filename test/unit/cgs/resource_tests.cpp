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
    num_meshes_out(2U),
    meshes_out(nullptr)
  {
    resource_database_init();
  }
  virtual ~resources_test() {}

  glm::mat4 local_transform_in;
  const float* local_transform_out;
  std::size_t num_meshes_in;
  std::size_t num_meshes_out;
  const mesh_id* meshes_out;
};

TEST_F(resources_test, add_resource_negative) {
  add_resource(nresource);
}

TEST_F(resources_test, add_resource_positive) {
  ASSERT_NE(add_resource(), nresource);
}

TEST_F(resources_test, get_first_child_resource_negative) {
  ASSERT_EQ(get_first_child_resource(nresource), nresource);
}

TEST_F(resources_test, get_first_child_resource_positive1) {
  resource_id r = add_resource();
  resource_id o = add_resource(r);
  ASSERT_EQ(get_first_child_resource(r), o);
}

TEST_F(resources_test, get_next_sibling_resource_negative) {
  ASSERT_EQ(get_next_sibling_resource(nresource), nresource);
}

TEST_F(resources_test, get_next_sibling_resource_positive) {
  resource_id r1 = add_resource();
  resource_id r2 = add_resource();
  resource_id r3 = add_resource();
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
