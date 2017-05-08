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
