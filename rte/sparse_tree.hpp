#ifndef SPARSE_TREE_HPP
#define SPARSE_TREE_HPP

#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <cassert>
#include <vector>
#include <set>
#include <map>

namespace rte
{
    typedef std::size_t index_type;
    constexpr index_type npos = -1;

    class tree_node
    {
    public:
        tree_node() :
            m_used(false),
            m_parent(npos),
            m_first_child(npos),
            m_last_child(npos),
            m_next_sibling(npos),
            m_previous_sibling(npos) {}

        tree_node(const tree_node&) = default;

        ~tree_node() { m_used = false; }
    
        tree_node& operator=(const tree_node& node)
        {
            // Member m_used is not touched, this is only set explicitly
            if (&node != this) {
                m_parent = node.m_parent;
                m_first_child = node.m_first_child;
                m_last_child = node.m_last_child;
                m_next_sibling = node.m_next_sibling;
                m_previous_sibling = node.m_previous_sibling;
            }

            return *this;
        }

        bool            m_used;
        index_type      m_parent;
        index_type      m_first_child;
        index_type      m_last_child;
        index_type      m_next_sibling;
        index_type      m_previous_sibling;
    };

    template <typename T, bool isconst = false>
    class tree_node_iterator
    {
    public:
        template <bool flag, typename IsTrue, typename IsFalse>
        struct choose;
        
        template <typename IsTrue, typename IsFalse>
        struct choose<true, IsTrue, IsFalse> {
           typedef IsTrue type;
        };
        
        template <typename IsTrue, typename IsFalse>
        struct choose<false, IsTrue, IsFalse> {
           typedef IsFalse type;
        };

        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename choose<isconst, const T*, T*>::type pointer;
        typedef typename choose<isconst, const T&, T&>::type reference;
        typedef typename std::ptrdiff_t difference_type;
        typedef T value_type;
    
        tree_node_iterator() :
            m_begin(nullptr),
            m_previous(0U),
            m_current(0U),
            m_next(0U),
            m_size(0U) {}

        tree_node_iterator(pointer begin,
                index_type previous,
                index_type current,
                index_type next,
                index_type size) :
            m_begin(begin),
            m_previous(previous),
            m_current(current),
            m_next(next),
            m_size(size) {}

        tree_node_iterator(const tree_node_iterator<T, false>& spi) :
            m_begin(spi.get_begin()),
            m_previous(spi.get_previous()),
            m_current(spi.get_current()),
            m_next(spi.get_next()),
            m_size(spi.get_size()) {}

        ~tree_node_iterator() {}

        tree_node_iterator& operator=(const tree_node_iterator&) = default;

        bool operator==(const tree_node_iterator& spi) const
        {
            return (m_begin == spi.m_begin && m_current == spi.m_current);
        }

        bool operator!=(const tree_node_iterator& spi) const { return !(*this == spi); }

        tree_node_iterator& operator++()
        {
            m_current = m_next;
            m_previous = m_begin[m_current].m_previous_sibling;
            m_next = m_begin[m_current].m_next_sibling;
            return (*this);
        }

        tree_node_iterator operator++(int)
        {
            tree_node_iterator tmp(*this);
            ++*this;
            return tmp;
        }

        tree_node_iterator& operator--()
        {
            m_current = m_previous;
            m_previous = m_begin[m_current].m_previous_sibling;
            m_next = m_begin[m_current].m_next_sibling;
            return (*this);
        }

        tree_node_iterator operator--(int)
        {
            tree_node_iterator tmp(*this);
            --*this;
            return tmp;
        }

        reference operator*() const
        {
            assert(m_current < m_size);
            if (!(m_current < m_size)) {
                throw std::out_of_range("tree_node_iterator::operator*: invalid index");
            }

            assert(m_begin[m_current].m_used);
            if (!m_begin[m_current].m_used) {
                throw std::domain_error("tree_node_iterator::operator*: entry has been erased");
            }

            return m_begin[m_current];
        }

        pointer operator->() const
        {
            assert(m_current < m_size);
            if (!(m_current < m_size)) {
                throw std::out_of_range("tree_node_iterator::operator->: invalid index");
            }

            assert(m_begin[m_current].m_used);
            if (!m_begin[m_current].m_used) {
                throw std::domain_error("tree_node_iterator::operator->: entry has been erased");
            }

            return m_begin + m_current;
        }
    
        pointer get_begin() const { return m_begin; }
        index_type get_previous() const { return m_previous; }
        index_type get_current() const { return m_current; }
        index_type get_next() const { return m_next; }
        index_type get_size() const { return m_size; }

    private:
        // Note that in order for reverse iteration with std::reverse_iterator to work, the end()
        // iterator must be able to do it--. This is why we store previous and next in the iterator,
        // otherwise end() wouldn't be able to go back.
        pointer         m_begin;
        index_type      m_previous;
        index_type      m_current;
        index_type      m_next;
        index_type      m_size;
    };

    template<typename T, bool isconst>
    typename T::index_type index(const tree_node_iterator<T, isconst>& it)
    {
        return it.get_current();
    }

    template<typename I>
    typename I::index_type index(const std::reverse_iterator<I>& it)
    {
        // std::reverse_iterator refers to the element one before the one refered by its internal iterator
        auto tmp = it.base();
        tmp--;
        return index(tmp);
    }

    template <typename T>
    class sparse_vector
    {
    public:
        typedef T value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef std::vector<value_type> container;
        typedef typename container::difference_type difference_type;
        typedef typename container::allocator_type allocator_type;
        typedef tree_node_iterator<value_type, false> iterator;
        typedef tree_node_iterator<value_type, true> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    
        sparse_vector() {}

        sparse_vector(const sparse_vector& st) = default;

        sparse_vector(sparse_vector&& st) :
            m_elems(std::move(st.m_elems)) {}

        ~sparse_vector() {}
    
        sparse_vector& operator=(const sparse_vector& st) = default;

        sparse_vector& operator=(sparse_vector&& st)
        {
            if (&st != this) {
                m_elems = std::move(st.m_elems);            
            }

            return *this;
        }

        // Inserts a single element, but doesn't mark it as used yet for exception safety in the calling function.
        index_type insert(const T& t)
        {
            auto nit = std::find_if(m_elems.begin(), m_elems.end(), [](const value_type& tn) {
                return !tn.m_used;
            }); 
            index_type new_index = nit - m_elems.begin();
            if (new_index == m_elems.size()) {
                m_elems.push_back(value_type());
            }
            m_elems.at(new_index) = t;

            return new_index;
        }

        reference at(index_type index)
        {
            assert(index < m_elems.size());
            assert(m_elems.at(index).m_used);

            reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_vector::at invalid index, element has been erased");
            }

            return ret;
        }

        const_reference at(index_type index) const
        {
            assert(index < m_elems.size());
            assert(m_elems.at(index).m_used);

            const_reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_vector::at invalid index, element has been erased");
            }

            return ret;
        }

        void erase(const std::set<index_type>& to_delete)
        {
            for (auto it = to_delete.begin(); it != to_delete.end(); ++it) {
                m_elems.at(*it).m_used = false;
            }
        }

        void set_used(index_type index_to_set)
        {
            m_elems.at(index_to_set).m_used = true;
        }

        void clear_used(index_type index_to_clear)
        {
            m_elems.at(index_to_clear).m_used = false;
        }

        // Returs physical number of elements. Elements that have been erased are counted too.
        index_type size() const { return m_elems.size(); }
        // Does a physical push_back on the underlying vector, but doesn't mark the new element as used
        // for exception safety of the calling function.
        void push_back(const value_type& t) { m_elems.push_back(t); }
        void clear() { m_elems.clear(); }
        void swap(sparse_vector& sf) { m_elems.swap(sf.m_elems); }

    private:
        container      m_elems;
    };

    template<typename V>
    index_type tree_allocate_node(V& tree, const std::set<index_type>& previously_allocated_indexes)
    {
        index_type ret = tree.size();
        for (index_type i = 0; i < tree.size(); i++) {
            if (!tree.at(i).m_used && previously_allocated_indexes.count(i) == 0) {
                ret = i;
                break;
            }
        }
        if ((!ret < tree.size())) {
            tree.push_back(V::value_type());
        }

        return ret;
    }

    // parent_index must be the index of a valid, used element.
    template<typename V>
    void tree_add_child(V& tree, index_type parent_index, index_type new_index)
    {
        assert(parent_index < tree.size());
        assert(new_index < tree.size());
        auto& parent = tree.at(parent_index);
        auto& new_node = tree.at(new_index);

        if (parent.m_last_child < tree.size()) {
            tree.at(parent.m_last_child).m_next_sibling = new_index;
        }

        new_node.m_parent = parent_index;
        new_node.m_previous_sibling = parent.m_last_child;

        parent.m_first_child = (parent.m_first_child == npos? new_index : parent.m_first_child);
        parent.m_last_child = new_index;
    }

    // Note: remove_index must still have valid references when this method is called
    template<typename V>
    void tree_remove_child(V& tree, index_type parent_index, index_type remove_index)
    {
        assert(parent_index < tree.size());
        assert(remove_index < tree.size());

        index_type previous_sibling_index = tree.at(remove_index).m_previous_sibling;
        if (previous_sibling_index < tree.size()) {
            auto& previous_sibling = tree.at(previous_sibling_index);
            previous_sibling.m_next_sibling = tree.at(remove_index).m_next_sibling;
        }

        index_type next_sibling_index = tree.at(remove_index).m_next_sibling;
        if (next_sibling_index < tree.size()) {
            auto& next_sibling = tree.at(next_sibling_index);
            next_sibling.m_previous_sibling = tree.at(remove_index).m_previous_sibling;
        }

        if (parent_index < tree.size()) {
            auto& parent = tree.at(parent_index);
            parent.m_first_child = (parent.m_first_child == remove_index? tree.at(remove_index).m_next_sibling : parent.m_first_child);
            parent.m_last_child = (parent.m_last_child == remove_index? tree.at(remove_index).m_previous_sibling : parent.m_last_child);
        }
    }

    // Inserts a single node as the last child of an existing one
    template<typename V>
    index_type tree_insert(V& tree, const typename V::value_type t, index_type parent_index = npos)
    {
        assert(parent_index == npos || parent_index < tree.size());
        if (!(parent_index == npos || parent_index < tree.size())) {
            throw std::domain_error("tree_insert: invalid parent index");
        }
        if (!(parent_index == npos || tree.at(parent_index).m_used)) {
            throw std::domain_error("tree_insert: parent has been erased");
        }

        index_type new_index = tree.insert(t);
    
        // Exception safety: now that we have passed all the throw points, impact the changes in the structure
        tree.set_used(new_index);
        if (parent_index < tree.size() && tree.at(parent_index).m_used) {
            tree_add_child(tree, parent_index, new_index);
        }

        return new_index;
    }

    // Inserts nodes by copying a subtree of another sparse_vector.
    // input_tree is the tree to read nodes from.
    // input_index is the index of the root of the subtree to read nodes from.
    // output_parent_index the index of the node in this sparse_vector to insert the subtree under.
    template<typename V>
    index_type tree_insert(const V& input_tree, index_type input_index, V& output_tree, index_type output_parent_index)    
    {
        assert(output_parent_index == npos || output_parent_index < output_tree.size());
        if (!(output_parent_index == npos || output_parent_index < output_tree.size())) {
            throw std::domain_error("tree_insert: invalid parent index");
        }
        if (!(output_parent_index == npos || output_tree.at(output_parent_index).m_used)) {
            throw std::domain_error("tree_insert: parent has been erased");
        }
        assert(input_index < input_tree.output_tree.size());
        if (!(input_index < input_tree.output_tree.size())) {
            throw std::out_of_range("tree_insert: invalid input index");
        }
        assert(input_tree.output_tree.at(input_index).m_used);
        if (!(input_tree.output_tree.at(input_index).m_used)) {
            throw std::out_of_range("tree_insert: input index has been erased");
        }

        // Insert all nodes
        std::map<index_type, index_type> new_index_map;
        // The npos references should stay that way
        new_index_map[npos] = npos;
        std::set<index_type> new_indices;
        std::vector<index_type> pending_nodes;
        pending_nodes.push_back(input_index);
        while (!pending_nodes.empty()) {
            // If this node is the root of the input tree save its index for later
            index_type input_node_index = pending_nodes.back();
            pending_nodes.pop_back();

            index_type new_index = tree_allocate_node(output_tree, new_indices);
            new_indices.insert(new_index);
            new_index_map[input_node_index] = new_index;

            // Insert the value in its new position
            const auto& input_node = input_tree.at(input_node_index);
            output_tree.at(new_index) = input_node;

            for (auto it = input_node.rbegin(); it != input_node.rend(); ++it) {
                pending_nodes.push_back(index(it));
            }
        }

        // Update the references in all inserted nodes
        // Remove the external index that may be present in the parent reference of the output root node
        // (we will set this later in function tree_add_child())
        output_tree.at(new_index_map.at(input_index)).m_parent = npos;
        for (auto& it : new_index_map) {
            if (it.second != npos) {
                auto& new_node = output_tree.at(it.second);
                new_node.m_parent = new_index_map.at(new_node.m_parent);
                new_node.m_first_child = new_index_map.at(new_node.m_first_child);
                new_node.m_last_child = new_index_map.at(new_node.m_last_child);
                new_node.m_next_sibling = new_index_map.at(new_node.m_next_sibling);
                new_node.m_previous_sibling = new_index_map.at(new_node.m_previous_sibling);
            }
        }

        // Exception safety: now that we have passed all the throw points, impact the changes in the structure

        // Insert the root in the list of children of the parent
        index_type new_root_index = new_index_map.at(input_index);
        if (output_parent_index < output_tree.size() && output_tree.at(output_parent_index).m_used) {
            tree_add_child(output_tree, output_parent_index, new_root_index);        
        }

        // Mark all the new nodes as used to make them visible
        for (auto& it : new_index_map) {
            if (it.second != npos) {
                output_tree.set_used(it.second);
            }
        }

        return new_root_index;
    }

    template<typename V>
    index_type tree_erase(V& tree, index_type erase_index)
    {
        assert(erase_index < tree.size());
        if (!(erase_index < tree.size())) {
            throw std::out_of_range("sparse_vector::erase: invalid remove index");
        }
        assert(tree.at(erase_index).m_used);
        if (!(tree.at(erase_index).m_used)) {
            throw std::domain_error("sparse_vector::erase: remove index has already been erased");
        }
        assert(erase_index > 0);
        if (!(erase_index > 0)) {
            throw std::domain_error("sparse_vector:erase: attempted to remove root node (index 0)");
        }
        assert(tree.at(erase_index).m_parent < tree.size());
        if (!(tree.at(erase_index).m_parent < tree.size())) {
            throw std::domain_error("sparse_vector:erase: remove index has no valid parent");
        }

        // Mark the node and all its descendants as not used, recursively
        std::set<index_type> to_delete;
        std::vector<index_type> pending_nodes;
        pending_nodes.push_back(erase_index);
        while (!pending_nodes.empty()) {
            index_type pending_index = pending_nodes.back();
            pending_nodes.pop_back();

            auto& node = tree.at(pending_index);
            to_delete.insert(pending_index);

            for (auto it = node.rbegin(); it != node.rend(); ++it) {
                pending_nodes.push_back(index(it));
            }
        }

        // Exception safety: now that we have passed all the throw points, impact the changes in the structure
        for (auto it = to_delete.begin(); it != to_delete.end(); ++it) {
            tree.clear_used(*it);
        }
        tree_remove_child(tree, tree.at(erase_index).m_parent, erase_index);
    }

    //template<typename V> typename V::iterator tree_begin(V& v, index_type i) { auto& elem = v.at(i); return V::iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).elem.m_next_sibling : npos, v.size()); }
    //template<typename V> typename V::const_iterator tree_begin(const V& v, index_type i) { auto& elem = v.at(i); return V::const_iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).elem.m_next_sibling : npos, v.size()); }
    //template<typename V> typename V::const_iterator tree_cbegin(const V& v, index_type i) { auto& elem = v.at(i); return V::const_iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).elem.m_next_sibling : npos, v.size()); }
    template<typename V> typename V::iterator tree_end(V& v, index_type i) { auto& elem = v.at(i); return V::iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    template<typename V> typename  V::const_iterator tree_end(const V& v, index_type i) { auto& elem = v.at(i); return V::const_iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    //template<typename V> typename  V::const_iterator tree_cend(const V& v, index_type i) { auto& elem = v.at(i); return V::const_iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    template<typename V> typename  V::reverse_iterator tree_rbegin(V& v, index_type i) { return V::reverse_iterator(tree_end(v, i)); }
    template<typename V> typename  V::const_reverse_iterator tree_rbegin(const V& v, index_type i) { return V::const_reverse_iterator(tree_end(v, i)); }
    //template<typename V> typename V::const_reverse_iterator tree_crbegin(const V&, index_type i) { return V::const_reverse_iterator(tree_end(v, i)); }
    template<typename V> typename V::reverse_iterator tree_rend(V& v, index_type i) { return V::reverse_iterator(tree_begin(v, i)); }
    template<typename V> typename V::const_reverse_iterator tree_rend(const V& v, index_type i) { return V::const_reverse_iterator(tree_begin(v, i)); }
    //template<typename V> typename V::const_reverse_iterator tree_crend(const V& v, index_type i) { return V::const_reverse_iterator(tree_begin(v, i)); }
} // namespace rte

#endif // SPARSE_TREE_HPP
