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

  template<typename V> typename V::iterator list_begin(V& v, index_type head_index) { return tree_begin(v, head_index); }
  template<typename V> typename V::const_iterator list_begin(const V& v, index_type head_index) { return tree_begin(v, head_index); }
  template<typename V> typename V::const_iterator list_cbegin(const V& v, index_type head_index) { return tree_cbegin(v, head_index); }
  template<typename V> typename V::iterator list_end(V& v, index_type head_index) { return tree_end(v, head_index); }
  template<typename V> typename V::const_iterator list_end(const V& v, index_type head_index)  { return tree_end(v, head_index); }
  template<typename V> typename V::const_iterator list_cend(const V& v, index_type head_index) { return tree_cend(v, head_index); }
  template<typename V> typename V::reverse_iterator list_rbegin(V& v, index_type head_index) { return tree_rbegin(v, head_index); }
  template<typename V> typename V::const_reverse_iterator list_rbegin(const V& v, index_type head_index) { return tree_rbegin(v, head_index); }
  template<typename V> typename V::const_reverse_iterator list_crbegin(const V& v, index_type head_index) { return tree_crbegin(v, head_index); }
  template<typename V> typename V::reverse_iterator list_rend(V& v, index_type head_index) { return tree_rend(v, head_index); }
  template<typename V> typename V::const_reverse_iterator list_rend(const V& v, index_type head_index) { return tree_rend(v, head_index); }
  template<typename V> typename V::const_reverse_iterator list_crend(const V& v, index_type head_index) { return tree_crend(v, head_index); }
} // namespace rte

#endif // SPARSE_LIST_HPP
