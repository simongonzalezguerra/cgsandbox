#include "glsandbox/resource_database.hpp"
#include "glsandbox/resource_loader.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "test/test_utils.hpp"
#include "glsandbox/log.hpp"
#include "gtest/gtest.h"
#include "glm/glm.hpp"

#include <cstddef> // for size_t

using namespace glsandbox;

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

  default_logstream_tail_dump(glsandbox::LOG_LEVEL_DEBUG);
  ASSERT_EQ(num_materials_out, 13U);
  float color_diffuse[] = {0.0f, 0.0f, 0.0f};
  float color_spec[] = {0.0f, 0.0f, 0.0f};
  float smoothness = 0.0f;
  const char* texture_path = nullptr;
  get_material_properties(materials_out[0], color_diffuse, color_spec, smoothness, &texture_path);
  float expected_color_diffuse[] = { 0.75f, 0.71f, 0.67f };
  ASSERT_SEQUENCE_NEAR(color_diffuse, expected_color_diffuse, 3, 0.01f);
  float expected_color_specular[] = { 0.0f, 0.0f, 0.0f };
  ASSERT_SEQUENCE_NEAR(color_spec, expected_color_specular, 3, 0.01f);
  ASSERT_EQ(smoothness, 200.0f);

  ASSERT_EQ(num_meshes_out, 36U);
  const float* vertex_base_out;
  std::size_t vertex_stride_out;
  const float* texture_coords_out;
  std::size_t texture_coords_stride_out;
  std::size_t num_vertices_out;
  const vindex* faces_out;
  std::size_t faces_stride_out;
  std::size_t num_faces_out;
  const float* normals_out = nullptr;
  mat_id material_out;
  get_mesh_properties(meshes_out[0],
                      &vertex_base_out,
                      &vertex_stride_out,
                      &texture_coords_out,
                      &texture_coords_stride_out,
                      &num_vertices_out,
                      &faces_out,
                      &faces_stride_out,
                      &num_faces_out,
                      &normals_out,
                      &material_out);
  ASSERT_EQ(num_vertices_out, 150U);
  ASSERT_EQ(num_faces_out, 192U);
  ASSERT_EQ(faces_stride_out, 3U);
  ASSERT_EQ(material_out, materials_out[0]);
  ASSERT_NEAR(vertex_base_out[0], 10.93f, 0.1f);
  ASSERT_NEAR(vertex_base_out[1], 2.96f, 0.1f);
  ASSERT_NEAR(vertex_base_out[2], 2.37f, 0.1f);
  ASSERT_NEAR(vertex_base_out[447], -11.57f, 0.1f);
  ASSERT_NEAR(vertex_base_out[448],   2.69f, 0.1f);
  ASSERT_NEAR(vertex_base_out[449],  -2.16f, 0.1f);
  ASSERT_EQ(vertex_stride_out, 3U);
  ASSERT_NE(texture_coords_out, nullptr);
  ASSERT_EQ(texture_coords_stride_out, 2U);
  ASSERT_NEAR(texture_coords_out[0], 0.04f, 0.1f);
  ASSERT_NEAR(texture_coords_out[1], 1.00f, 0.1f);
  ASSERT_NEAR(texture_coords_out[2], 0.08f, 0.1f);
  ASSERT_NEAR(texture_coords_out[297], 1.00f, 0.1f);
  ASSERT_NEAR(texture_coords_out[298], 0.91f, 0.1f);
  ASSERT_NEAR(texture_coords_out[299], 0.12f, 0.1f);
  ASSERT_EQ(faces_out[0], 0U);
  ASSERT_EQ(faces_out[1], 1U);
  ASSERT_EQ(faces_out[2], 2U);
  ASSERT_EQ(faces_out[573], 78U);
  ASSERT_EQ(faces_out[574], 146U);
  ASSERT_EQ(faces_out[575], 79U);

  const float* local_transform_out = nullptr;
  get_resource_properties(get_first_child_resource(added_resource), &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_EQ(num_meshes_out, 1U);
  ASSERT_EQ(meshes_out[0], 0U);

  ASSERT_NE(added_resource, nresource);
}

TEST_F(resources_loader_test, load_resources_three_levels) {
  // Load a scene with more than two levels
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + THREE_LEVELS, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(glsandbox::LOG_LEVEL_DEBUG);

  // Ugly hack: the test assumes the resources are created with a breadth-first search, so the node in the
  // third level is the last one to be created, with id 166.
  const float* local_transform_out = nullptr;
  get_resource_properties(166U, &meshes_out, &num_meshes_out, &local_transform_out);
  ASSERT_EQ(num_meshes_out, 1U);
  ASSERT_EQ(meshes_out[0], 171U);
  const float expected_local_transform[] = { 0.95, -0.02, 0.32, 0.00, 0.00, 1.00, 0.07, 0.00, -0.32, -0.07, 0.95, 0.00, -5.16, 3.29, 2.77, 1.00 };
  ASSERT_SEQUENCE_NEAR(local_transform_out, expected_local_transform, 16, 0.01f);
}

TEST_F(resources_loader_test, load_resources_no_texture_info) {
  // Load a scene with no texture info
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + STANFORD_BUNNY, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(glsandbox::LOG_LEVEL_DEBUG);

  const float* vertex_base_out;
  std::size_t vertex_stride_out;
  const float* texture_coords_out;
  std::size_t texture_coords_stride_out;
  std::size_t num_vertices_out;
  const vindex* faces_out;
  std::size_t faces_stride_out;
  std::size_t num_faces_out;
  const float* normals_out = nullptr;
  mat_id material_out;
  get_mesh_properties(0U,
                      &vertex_base_out,
                      &vertex_stride_out,
                      &texture_coords_out,
                      &texture_coords_stride_out,
                      &num_vertices_out,
                      &faces_out,
                      &faces_stride_out,
                      &num_faces_out,
                      &normals_out,
                      &material_out);
  ASSERT_EQ(texture_coords_out, nullptr);
}

TEST_F(resources_loader_test, load_resources_billiard_table) {
  const mat_id* materials_out = nullptr;
  std::size_t num_materials_out = 0U;
  const mesh_id* meshes_out = nullptr;
  std::size_t num_meshes_out = 0U;
  load_resources(RESOURCES_PATH + BILLIARD_TABLE, &materials_out, &num_materials_out, &meshes_out, &num_meshes_out);
  default_logstream_tail_dump(glsandbox::LOG_LEVEL_DEBUG);
}

int main(int argc, char** argv)
{
  log_init();
  attach_logstream(default_logstream_file_callback);
  attach_logstream(default_logstream_tail_callback);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
