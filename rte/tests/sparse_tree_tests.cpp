#include "sparse_tree.hpp"
#include "gtest/gtest.h"

#include <iostream>
#include <vector>

typedef rte::sparse_tree<int> my_tree;

class sparse_tree_test : public ::testing::Test
{
protected:
    sparse_tree_test() {}
    virtual ~sparse_tree_test() {}
};

TEST_F(sparse_tree_test, default_construction) {
    my_tree st;
    auto& root = st.at(0);
    ASSERT_EQ(root.m_used, true);
    ASSERT_EQ(root.m_parent, my_tree::value_type::npos);
    ASSERT_EQ(root.m_first_child, my_tree::value_type::npos);
    ASSERT_EQ(root.m_last_child, my_tree::value_type::npos);
    ASSERT_EQ(root.m_next_sibling, my_tree::value_type::npos);
    ASSERT_EQ(root.m_previous_sibling, my_tree::value_type::npos);
}

TEST_F(sparse_tree_test, insert_one_node) {
    my_tree st;
    auto new_index = st.insert(1, 0);
    ASSERT_EQ(new_index, 1U);
}

bool find_value(int value, const my_tree& st, my_tree::size_type& index_out)
{
    std::vector<my_tree::size_type> pending_nodes;
    pending_nodes.push_back(0);
    bool found = false;
    while (!found && !pending_nodes.empty()) {
        auto node_index = pending_nodes.back();
        pending_nodes.pop_back();
        auto& node = st.at(node_index);
        if (node.m_elem == value) {
            found = true;
            index_out = node_index;
        }

        for (auto it = node.rbegin(); it != node.rend(); it++) {
            pending_nodes.push_back(index(it));
        }
    }

    return found;
}

void print_tree(const my_tree& st, my_tree::size_type root_index)
{
    struct context { my_tree::size_type node_index; unsigned int indentation; };
    std::vector<context> pending_nodes;
    pending_nodes.push_back({root_index, 0});
    while (!pending_nodes.empty()) {
        auto current = pending_nodes.back();
        pending_nodes.pop_back();

        for (unsigned int i = 0; i < current.indentation; i++) {
            std::cout << "    ";
        }
        auto& node = st.at(current.node_index);
        std::cout << node.m_elem;
        std::cout << "\n";

        for (auto it = node.rbegin(); it != node.rend(); ++it) {
            pending_nodes.push_back({index(it), current.indentation + 1});
        }
    }
}

// Test tree:
// 1
//   2
//     3
//     4
//   5
//   6
//     7
//     8
TEST_F(sparse_tree_test, insert_erase_search) {
    my_tree st;
    auto new_index = st.insert(1, 0);
    ASSERT_EQ(new_index, 1U);

    new_index = st.insert(2, 1);
    ASSERT_EQ(new_index, 2U);

    new_index = st.insert(3, 2);
    ASSERT_EQ(new_index, 3U);

    new_index = st.insert(4, 2);
    ASSERT_EQ(new_index, 4U);

    new_index = st.insert(5, 1);
    ASSERT_EQ(new_index, 5U);

    new_index = st.insert(6, 1);
    ASSERT_EQ(new_index, 6U);

    new_index = st.insert(7, 6);
    ASSERT_EQ(new_index, 7U);

    new_index = st.insert(8, 6);
    ASSERT_EQ(new_index, 8U);

    my_tree::size_type search_index = 0;
    bool search_result = false;
    search_result = find_value(7, st, search_index);
    ASSERT_EQ(search_result, true);
    ASSERT_EQ(search_index, 7U);

    std::cout << "tree before erasing node 7:\n";
    print_tree(st, 1);
    st.erase(7);
    std::cout << "tree after erasing node 7:\n";
    print_tree(st, 1);

    st.erase(2);
    std::cout << "tree after erasing node 2:\n";
    print_tree(st, 1);

    search_result = find_value(7, st, search_index);
    ASSERT_EQ(search_result, false);
}

// Initial tree:
// 1
//   2
//     3
//     4
//   5
//   6
//     7
//     8
// Tree to add:
// 9
//   10
//     11
//     12
//     13
//   14
//     15
//       16
TEST_F(sparse_tree_test, insert_tree) {
    // build initial tree
    my_tree ot;
    ot.insert(1, 0);
    ot.insert(2, 1);
    ot.insert(3, 2);
    ot.insert(4, 2);
    ot.insert(5, 1);
    ot.insert(6, 1);
    ot.insert(7, 6);
    ot.insert(8, 6);
    std::cout << "initial tree:\n";
    print_tree(ot, 1);

    // build tree to add
    my_tree it;
    it.insert(9, 0);
    it.insert(10, 1);
    it.insert(11, 2);
    it.insert(12, 2);
    it.insert(13, 2);
    it.insert(14, 1);
    it.insert(15, 6);
    it.insert(16, 7);
    std::cout << "tree to add:\n";
    print_tree(it, 1);

    ot.insert(it, 1, 4);

    std::cout << "tree after adding:\n";
    print_tree(ot, 1);

    bool search_result = false;
    my_tree::size_type search_index = my_tree::value_type::npos;
    search_result = find_value(9, ot, search_index);
    ASSERT_EQ(search_result, true);
    ASSERT_EQ(search_index, 9U);

    search_result = find_value(10, ot, search_index);
    ASSERT_EQ(search_result, true);
    ASSERT_EQ(search_index, 10U);

    search_result = find_value(16, ot, search_index);
    ASSERT_EQ(search_result, true);
    ASSERT_EQ(search_index, 16U);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
