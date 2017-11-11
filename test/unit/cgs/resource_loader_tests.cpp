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
  material_vector materials_out;
  mesh_vector meshes_out;
  resource_vector resources_out;
  resource_id resource_out = nresource;
  ASSERT_ANY_THROW(load_resources("", &resource_out, &resources_out, &materials_out, &meshes_out));
  ASSERT_ANY_THROW(load_resources("some_file.3ds", &resource_out, &resources_out, nullptr, &meshes_out));
  ASSERT_ANY_THROW(load_resources("some_file.3ds", &resource_out, &resources_out, &materials_out, &meshes_out));
  ASSERT_ANY_THROW(load_resources("some_file.3ds", &resource_out, &resources_out, &materials_out, nullptr));
  ASSERT_ANY_THROW(load_resources("some_file.3ds", &resource_out, &resources_out, &materials_out, &meshes_out));
}

TEST_F(resources_loader_test, load_resources_positive1) {
  material_vector materials_out;
  mesh_vector meshes_out;
  resource_vector resources_out;
  resource_id resource_out = nresource;
  load_resources(RESOURCES_PATH + SPONZA, &resource_out, &resources_out, &materials_out, &meshes_out);

  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
  glm::vec3 color_diffuse = get_material_diffuse_color(materials_out[0].get());
  glm::vec3 color_spec = get_material_diffuse_color(materials_out[0].get());
  float smoothness = get_material_smoothness(materials_out[0].get());
  std::string texture_path = get_material_texture_path(materials_out[0].get());

  float expected_color_diffuse[] = {0.75f, 0.71f, 0.67f};
  ASSERT_SEQUENCE_NEAR(glm::value_ptr(color_diffuse), expected_color_diffuse, 3, 0.01f);
  float expected_color_specular[] = {0.74f, 0.71f, 0.67f};
  ASSERT_SEQUENCE_NEAR(glm::value_ptr(color_spec), expected_color_specular, 3, 0.01f);
  ASSERT_EQ(smoothness, 200.0f);

  auto& m = meshes_out[0];
  std::vector<glm::vec3> vertices = get_mesh_vertices(m.get());
  std::vector<glm::vec2> texture_coords = get_mesh_texture_coords(m.get());
  std::vector<glm::vec3> normals = get_mesh_normals(m.get());
  std::vector<vindex> indices = get_mesh_indices(m.get());

  ASSERT_EQ(vertices.size(), 150U);
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

  ASSERT_NE(resource_out, nresource);
}

TEST_F(resources_loader_test, load_resources_three_levels) {
  // Load a scene with more than two levels
  material_vector materials_out;
  mesh_vector meshes_out;
  resource_id resource_out = nresource;
  resource_vector resources_out;
  load_resources(RESOURCES_PATH + THREE_LEVELS, &resource_out, &resources_out, &materials_out, &meshes_out);
  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
}

TEST_F(resources_loader_test, load_resources_no_texture_info) {
  // Load a scene with no texture info
  material_vector materials_out;
  mesh_vector meshes_out;
  resource_id resource_out = nresource;
  resource_vector resources_out;
  load_resources(RESOURCES_PATH + STANFORD_BUNNY, &resource_out, &resources_out, &materials_out, &meshes_out);
  default_logstream_tail_dump(cgs::LOG_LEVEL_DEBUG);
}

TEST_F(resources_loader_test, load_resources_billiard_table) {
  material_vector materials_out;
  mesh_vector meshes_out;
  resource_id resource_out = nresource;
  resource_vector resources_out;
  load_resources(RESOURCES_PATH + BILLIARD_TABLE, &resource_out, &resources_out, &materials_out, &meshes_out);
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
