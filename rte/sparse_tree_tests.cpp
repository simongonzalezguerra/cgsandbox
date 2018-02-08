#include "gtest/gtest.h"

#include "rte_common.hpp"

struct my_struct
{
    int     m_a;
    int     m_b;
    float   m_c;
};

typedef rte::sparse_tree<my_struct> my_tree;

class sparse_tree_test : public ::testing::Test
{
protected:
    sparse_tree_test() {}
    virtual ~sparse_tree_test() {}
};

TEST_F(sparse_tree_test, default_construction) {
    my_tree sf;
    auto& root = sf.at(0);
    ASSERT_EQ(root.m_used, true);
    ASSERT_EQ(root.m_parent, rte::npos);
    ASSERT_EQ(root.m_first_child, rte::npos);
    ASSERT_EQ(root.m_last_child, rte::npos);
    ASSERT_EQ(root.m_next_sibling, rte::npos);
    ASSERT_EQ(root.m_previous_sibling, rte::npos);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}