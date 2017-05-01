#include "cgs/resource_database.hpp"
#include "gtest/gtest.h"

#include <cstddef> // for size_t
#include <vector>

using namespace cgs;

class materials_test : public ::testing::Test
{
protected:
  materials_test()
  {
    resource_database_init();
  }
  virtual ~materials_test() {}
};

TEST_F(materials_test, add_material) {
  ASSERT_NE(add_material(), nmat);
}

TEST_F(materials_test, add_get_material_properties_negative) {
  float color_diffuse[] = {0.0f, 0.0f, 0.0f};
  float color_spec[] = {0.0f, 0.0f, 0.0f};
  float smoothness = 0.0f;
  const char* texture_path = nullptr;
  get_material_properties(0, color_diffuse, color_spec, smoothness, &texture_path);
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_diffuse[i], 0.0f);
  }
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_spec[i], 0.0f);
  }
  ASSERT_EQ(smoothness, 0.0f);
}

TEST_F(materials_test, add_get_material_properties_positive) {
  mat_id m = add_material();
  float color_diffuse[] = {0.0f, 0.0f, 0.0f};
  float color_spec[] = {0.0f, 0.0f, 0.0f};
  float smoothness = 0.0f;
  const char* texture_path = nullptr;
  get_material_properties(m, color_diffuse, color_spec, smoothness, &texture_path);
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_diffuse[i], 1.0f);
  }
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_spec[i], 1.0f);
  }
  ASSERT_EQ(smoothness, 1.0f);
}

TEST_F(materials_test, add_set_material_properties_negative) {
  set_material_properties(0, NULL, NULL, 0.0f, nullptr);
}

TEST_F(materials_test, add_set_material_properties_positive) {
  mat_id m = add_material();
  float color_diffuse_in[] = {0.3f, 0.3f, 0.3f};
  float color_spec_in[] = {0.2f, 0.2f, 0.2f};
  float smoothness_in = 0.1f;
  set_material_properties(m, color_diffuse_in, color_spec_in, smoothness_in, nullptr);
  float color_diffuse_out[] = {0.0f, 0.0f, 0.0f};
  float color_spec_out[] = {0.0f, 0.0f, 0.0f};
  float smoothness_out = 0.0f;
  const char* texture_path = nullptr;
  get_material_properties(m, color_diffuse_out, color_spec_out, smoothness_out, &texture_path);
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_diffuse_out[i], 0.3f);
  }
  for (std::size_t i = 0; i < 3; i++) {
    ASSERT_EQ(color_spec_out[i], 0.2f);
  }
  ASSERT_EQ(smoothness_out, 0.1f); 
}

TEST_F(materials_test, get_first_material_negative) {
  ASSERT_EQ(get_first_material(), nmat);
}

TEST_F(materials_test, get_first_material_positive) {
  mat_id m = add_material();
  ASSERT_EQ(get_first_material(), m);
}

TEST_F(materials_test, get_next_material_negative) {
  ASSERT_EQ(get_next_material(0), nmat);
}

TEST_F(materials_test, get_next_material_positive) {
  std::vector<mat_id> materials;
  for (std::size_t i = 0; i < 10; i++) {
    materials.push_back(add_material());
  }
  mat_id m = nmat;
  size_t i = 0;
  for (m = get_first_material(), i = 0; m != nmat; m = get_next_material(m), i++) {
    ASSERT_EQ(m, materials[i]);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
