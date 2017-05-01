#include "cgs/resource_database.hpp"
#include "gtest/gtest.h"

#include <cstddef> // for size_t
#include <vector>

using namespace cgs;

struct state
{
  state() :
    vertex_base_out(nullptr),
    vertex_stride_out(0U),
    texture_coords_out(nullptr),
    texture_coords_stride_out(0U),
    num_vertices_out(0U),
    faces_out(nullptr),
    faces_stride_out(0U),
    num_faces_out(0U),
    normals_out(nullptr),
    material_out(nmat),
    vertex_base_in{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f},
    vertex_stride_in(3),
    texture_coords_in{0.5f, 0.1f, 0.5f, 0.1f, 0.5f, 0.1f, 0.5f, 0.1f},
    texture_coords_stride_in(2),
    num_vertices_in(4),
    faces_in{0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2},
    faces_stride_in(3),
    num_faces_in(4),
    normals_in{2.3f, -1.3f, 4.5f, 7.4f, 0.5f, 9.3f, 0.7f, 2.3f, -1.7f, 1.0f, 8.1f, 4.7f},
    material_in(nmat) {}

  const float* vertex_base_out;
  std::size_t vertex_stride_out;
  const float* texture_coords_out;
  std::size_t texture_coords_stride_out;
  std::size_t num_vertices_out;
  const vindex* faces_out;
  std::size_t faces_stride_out;
  std::size_t num_faces_out;
  const float* normals_out;
  mat_id material_out;
  float vertex_base_in[12];
  std::size_t vertex_stride_in;
  float texture_coords_in[8];
  std::size_t texture_coords_stride_in;
  std::size_t num_vertices_in;
  vindex faces_in[12];
  std::size_t faces_stride_in;
  std::size_t num_faces_in;
  float normals_in[12];
  mat_id material_in;
};

class meshes_test : public ::testing::Test
{
protected:
  meshes_test() : s{}
  {
    resource_database_init();
  }
  virtual ~meshes_test() {}

  state s;
};

#define check_state(actual, expected) \
do { \
  ASSERT_EQ(actual.vertex_base_out, expected.vertex_base_out); \
  ASSERT_EQ(actual.vertex_stride_out, expected.vertex_stride_out); \
  ASSERT_EQ(actual.texture_coords_out, expected.texture_coords_out); \
  ASSERT_EQ(actual.texture_coords_stride_out, expected.texture_coords_stride_out); \
  ASSERT_EQ(actual.num_vertices_out, expected.num_vertices_out); \
  ASSERT_EQ(actual.faces_out, expected.faces_out); \
  ASSERT_EQ(actual.faces_stride_out, expected.faces_stride_out); \
  ASSERT_EQ(actual.num_faces_out, expected.num_faces_out); \
  ASSERT_EQ(actual.material_out, expected.material_out); \
} while (0)

state fresh_mesh()
{
  state expected{};
  expected.vertex_stride_out = 3U;
  expected.texture_coords_out = nullptr;;
  expected.texture_coords_stride_out = 2U;
  expected.num_vertices_out = 0U;
  expected.faces_stride_out = 3U;
  expected.num_faces_out = 0U;
  expected.material_out = nmat;
  return expected;
}

TEST_F(meshes_test, add_material) {
  ASSERT_NE(add_mesh(), nmesh);
}

TEST_F(meshes_test, get_mesh_properties_negative1) {
  state expected = s;
  get_mesh_properties(nmesh, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), nullptr, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, nullptr, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, nullptr, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, nullptr, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, nullptr, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, nullptr, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, nullptr, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, nullptr, &s.normals_out, &s.material_out);
  check_state(s, expected);
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, nullptr);
  check_state(s, expected);
}

TEST_F(meshes_test, get_mesh_properties_positive) {
  state expected = fresh_mesh();
  get_mesh_properties(add_mesh(), &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
}

TEST_F(meshes_test, set_mesh_properties_negative) {
  set_mesh_properties(nmesh, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, s.material_in);

  state expected = fresh_mesh();
  mesh_id m = add_mesh();
  set_mesh_properties(m, nullptr, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  set_mesh_properties(m, s.vertex_base_in, 0U, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, 0U, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, nullptr, s.faces_stride_in, s.num_faces_in, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, 0U, s.num_faces_in, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, s.faces_stride_in, 0U, s.normals_in, s.material_in);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  check_state(s, expected);
}

TEST_F(meshes_test, set_mesh_properties_positive1) {
  mat_id mat = add_material();
  mesh_id m = add_mesh();
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, s.texture_coords_in, s.texture_coords_stride_in, s.num_vertices_in, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, mat);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  for (auto i = 0; i < 12; i++) {
    ASSERT_EQ(s.vertex_base_out[i], s.vertex_base_in[i]);
  }
  ASSERT_EQ(s.vertex_stride_out, s.vertex_stride_in);
  for (auto i = 0; i < 8; i++) {
    ASSERT_EQ(s.texture_coords_out[i], s.texture_coords_in[i]);
  }
  ASSERT_EQ(s.texture_coords_stride_out, s.texture_coords_stride_in);
  ASSERT_EQ(s.num_vertices_out, 4U);
  for (auto i = 0; i < 12; i++) {
    ASSERT_EQ(s.faces_out[i], s.faces_in[i]);
  }
  ASSERT_EQ(s.faces_stride_out, 3U);
  ASSERT_EQ(s.num_faces_out, 4U);
  ASSERT_EQ(s.material_out, mat);
}

TEST_F(meshes_test, set_mesh_properties_positive2) {
  mat_id mat = add_material();
  mesh_id m = add_mesh();
  set_mesh_properties(m, s.vertex_base_in, s.vertex_stride_in, nullptr, 5U, s.num_vertices_in, s.faces_in, s.faces_stride_in, s.num_faces_in, s.normals_in, mat);
  get_mesh_properties(m, &s.vertex_base_out, &s.vertex_stride_out, &s.texture_coords_out, &s.texture_coords_stride_out, &s.num_vertices_out, &s.faces_out, &s.faces_stride_out, &s.num_faces_out, &s.normals_out, &s.material_out);
  ASSERT_EQ(s.texture_coords_out, nullptr);
  ASSERT_EQ(s.texture_coords_stride_out, 2U);
}

TEST_F(meshes_test, get_first_mesh_negative) {
  ASSERT_EQ(get_first_mesh(), nmesh);
}

TEST_F(meshes_test, get_first_mesh_positive) {
  mesh_id m = add_mesh();
  ASSERT_EQ(get_first_mesh(), m);
}

TEST_F(meshes_test, get_next_mesh_negative) {
  ASSERT_EQ(get_next_mesh(nmesh), nmesh);
}

TEST_F(meshes_test, get_next_mesh_positive1) {
  ASSERT_EQ(get_next_mesh(add_mesh()), nmesh);
}

TEST_F(meshes_test, get_next_mesh_positive2) {
  std::vector<mesh_id> meshes;
  for (std::size_t i = 0; i < 10; i++) {
    meshes.push_back(add_mesh());
  }
  mesh_id m = nmesh;
  size_t i = 0;
  for (m = get_first_mesh(), i = 0; m != nmesh; m = get_next_mesh(m), i++) {
    ASSERT_EQ(m, meshes[i]);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
