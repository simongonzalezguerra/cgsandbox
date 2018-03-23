#include "sparse_list.hpp"
#include "gtest/gtest.h"

#include <iostream>
#include <vector>

using namespace rte;

struct my_struct : public sparse_node
{
    my_struct() : m_val(0) {}
    my_struct(int val) : m_val(val) {}

    int m_val;
};

std::ostream& operator<<(std::ostream& os, const my_struct ms)
{
    os << ms.m_val;
    return os;
}

typedef sparse_vector<my_struct> my_vector;

class sparse_list_test : public ::testing::Test
{
protected:
    sparse_list_test() {}
    virtual ~sparse_list_test() {}
};

TEST_F(sparse_list_test, init) {
    my_vector sl;
    list_init(sl);
}

TEST_F(sparse_list_test, empty_list) {
    my_vector sl;
    list_init(sl);
    auto list_head = list_empty_list(sl);
    ASSERT_EQ(list_head, 0U);
}

unsigned int get_number_of_elements(const my_vector& v, index_type head_index)
{
    unsigned int ret = 0U;
    // TODO implemen list_begin and list_end methods
    for (auto it = list_begin(v, head_index); it != list_end(v, head_index); ++it) {
        ret++;
    }

    return ret;
}

TEST_F(sparse_list_test, insert) {
    my_vector sl;
    list_init(sl);
    auto list_head = list_empty_list(sl);
    list_insert(sl, list_head, my_struct(1));
    list_insert(sl, list_head, my_struct(2));
    list_insert(sl, list_head, my_struct(3));
    list_insert(sl, list_head, my_struct(4));
    list_insert(sl, list_head, my_struct(5));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
