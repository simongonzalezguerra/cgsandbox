#ifndef SPARSE_LIST_HPP
#define SPARSE_LIST_HPP

#include "sparse_tree.hpp"

namespace rte
{

// Erases all list nodes in a sparse_vector
template<typename V>
void list_init(V& v)
{
    v.clear();
}

template<typename V>
index_type list_empty_list(V& v)
{
    return tree_insert(v, typename V::value_type());
}

template<typename V>
index_type list_insert(V& v, index_type head_index, const typename V::value_type& t)
{
    return tree_insert(v, t, head_index);
}

template<typename V>
void list_erase(V& v, index_type node_index)
{
    tree_erase(v, node_index);
}
} // namespace rte

#endif // SPARSE_LIST_HPP
