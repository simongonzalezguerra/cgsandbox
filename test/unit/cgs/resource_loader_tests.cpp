#include "cgs/resource_database.hpp"
#include "cgs/resource_loader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "test/test_utils.hpp"
#include "cgs/log.hpp"
#include "gtest/gtest.h"
#include "glm/glm.hpp"

#include <cstddef> // for size_t

using namespace cgs;

const std::string RESOURCES_PATH   = "../../../../resources/";
const std::string SPONZA    = "sponza/sponza.obj";
const std::string THREE_LEVELS     = "f14_three_levels/F14_ThreeLevels.blend";
const std::string STANFORD_BUNNY   = "stanford-bunny/bun_zipper.ply";
const std::string BILLIARD_TABLE   = "billiard-table/Table_billiard_N150210.3DS";

class resources_loader_test : public ::testing::Test
{
protected:
  resources_loader_test()
  {
    resource_database_init();
  }

  virtual ~resources_loader_test()
  {
  }
};

TEST_F(resources_loader_test, load_resources_negative) {
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources("", &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  load_resources("some_file.3ds", nullptr, &num_materials_out, &meshes_out, &num_meshes_out);
  load_resources("some_file.3ds", &materials_out, nullptr, &meshes_out, &num_meshes_out);
  load_resources("some_file.3ds", &materials_out, &num_materials_out, nullptr, &num_meshes_out);
  load_resources("some_file.3ds", &materials_out, &num_materials_out, &meshes_out, nullptr);
}

TEST_F(resources_loader_test, load_resources_positive1) {
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  resource_id added_resource = load_resources(RESOURCES_PATH + SPONZA, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);

  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
  ASSERT_EQ(num_materials_out, 13U);
  glm::vec3 color_diffuse = get_material_diffuse_color(materials_out[0]);
  glm::vec3 color_spec = get_material_diffuse_color(materials_out[0]);
  float smoothness = get_material_smoothness(materials_out[0]);
  std::string texture_path = get_material_texture_path(materials_out[0]);

  float expected_color_diffuse[] = {0.75f, 0.71f, 0.67f};
  ASSERT_SEQUENCE_NEAR(glm::value_ptr(color_diffuse), expected_color_diffuse, 3, 0.01f);
  float expected_color_specular[] = {0.74f, 0.71f, 0.67f};
  ASSERT_SEQUENCE_NEAR(glm::value_ptr(color_spec), expected_color_specular, 3, 0.01f);
  ASSERT_EQ(smoothness, 200.0f);
  ASSERT_EQ(num_meshes_out, 36U);

  mesh_id m = meshes_out[0];
  std::vector<glm::vec3> vertices = get_mesh_vertices(m);
  std::vector<glm::vec2> texture_coords = get_mesh_texture_coords(m);
  std::vector<glm::vec3> normals = get_mesh_normals(m);
  std::vector<vindex> indices = get_mesh_indices(m);
  mat_id mat = get_mesh_material(m);

  ASSERT_EQ(vertices.size(), 150U);
  ASSERT_EQ(mat, materials_out[0]);
  ASSERT_NEAR(vertices[0][0], 10.93f, 0.1f);
  ASSERT_NEAR(vertices[0][1], 2.96f, 0.1f);
  ASSERT_NEAR(vertices[0][2], 2.37f, 0.1f);
  ASSERT_NEAR(vertices[vertices.size() - 1][0], -11.57f, 0.1f);
  ASSERT_NEAR(vertices[vertices.size() - 1][1],   2.69f, 0.1f);
  ASSERT_NEAR(vertices[vertices.size() - 1][2],  -2.16f, 0.1f);
  ASSERT_EQ(texture_coords.size(), 150U);
  ASSERT_NEAR(texture_coords[0][0], 0.04f, 0.1f);
  ASSERT_NEAR(texture_coords[0][1], 1.00f, 0.1f);
  ASSERT_NEAR(texture_coords[1][0], 0.08f, 0.1f);
  ASSERT_NEAR(texture_coords[texture_coords.size() - 2][0], 1.00f, 0.1f);
  ASSERT_NEAR(texture_coords[texture_coords.size() - 1][0], 0.91f, 0.1f);
  ASSERT_NEAR(texture_coords[texture_coords.size() - 1][1], 0.12f, 0.1f);
  ASSERT_EQ(indices[0], 0U);
  ASSERT_EQ(indices[1], 1U);
  ASSERT_EQ(indices[2], 2U);
  ASSERT_EQ(indices[indices.size() - 3], 78U);
  ASSERT_EQ(indices[indices.size() - 2], 146U);
  ASSERT_EQ(indices[indices.size() - 1], 79U);

  resource_id rid = get_first_child_resource(added_resource);
  std::vector<mesh_id> meshes = get_resource_meshes(rid);
  ASSERT_EQ(meshes.size(), 1U);
  ASSERT_EQ(meshes[0], 0U);

  ASSERT_NE(added_resource, nresource);
}

TEST_F(resources_loader_test, load_resources_three_levels) {
  // Load a scene with more than two levels
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + THREE_LEVELS, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);

  // Ugly hack: the test assumes the resources are created with a breadth-first search, so the node in the
  // third level is the last one to be created, with id 166.
  std::vector<mesh_id> meshes = get_resource_meshes(166U);
  glm::mat4 local_transform = get_resource_local_transform(166U);
  ASSERT_EQ(meshes.size(), 1U);
  ASSERT_EQ(meshes[0], 171U);
  const float expected_local_transform[] = { 0.95, -0.02, 0.32, 0.00, 0.00, 1.00, 0.07, 0.00, -0.32, -0.07, 0.95, 0.00, -5.16, 3.29, 2.77, 1.00 };
  ASSERT_SEQUENCE_NEAR(glm::value_ptr(local_transform), expected_local_transform, 16, 0.01f);
}

TEST_F(resources_loader_test, load_resources_no_texture_info) {
  // Load a scene with no texture info
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + STANFORD_BUNNY, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
}

TEST_F(resources_loader_test, load_resources_billiard_table) {
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + BILLIARD_TABLE, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
}

int main(int argc, char** argv)
{
  log_init();
  attach_logstream(default_logstream_file_callback);
  attach_logstream(default_logstream_tail_callback);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
