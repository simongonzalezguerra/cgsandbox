#ifndef SPARSE_TREE_HPP
#define SPARSE_TREE_HPP

#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <cassert>
#include <vector>
#include <map>

namespace rte
{
    template <typename N, bool isconst = false>
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
        typedef typename choose<isconst, const N*, N*>::type pointer;
        typedef typename choose<isconst, const N&, N&>::type reference;
        typedef typename N::difference_type difference_type;
        typedef typename N::size_type size_type;
        typedef N value_type;
    
        tree_node_iterator() :
            m_begin(nullptr),
            m_previous(0U),
            m_current(0U),
            m_next(0U),
            m_size(0U) {}

        tree_node_iterator(pointer begin, size_type previous, size_type current, size_type next, size_type size) :
            m_begin(begin),
            m_previous(previous),
            m_current(current),
            m_next(next), m_size(size) {}

        tree_node_iterator(const tree_node_iterator<N, false>& spi) :
            m_begin(spi.get_begin()),
            m_previous(spi.get_previous()),
            m_current(spi.get_current()),
            m_next(spi.get_next()),
            m_size(spi.get_size()) {}

        ~tree_node_iterator() {}

        tree_node_iterator& operator=(const tree_node_iterator&) = default;

        bool operator==(const tree_node_iterator& spi) const
        {
            return (m_begin == spi.m_begin
                    && m_previous == spi.m_previous
                    && m_current == spi.m_current
                    && m_next == spi.m_next
                    && m_size == spi.m_size);
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

            assert(m_current->m_used);
            if (!m_current->m_used) {
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

            assert(m_current->m_used);
            if (!m_current->m_used) {
                throw std::domain_error("tree_node_iterator::operator->: entry has been erased");
            }

            return m_begin + m_current;
        }
    
        pointer get_begin() const
        {
            return m_begin;
        }

        size_type get_previous() const
        {
            return m_previous;
        }

        size_type get_current() const
        {
            return m_current;
        }

        size_type get_next() const
        {
            return m_next;
        }

        size_type get_size() const
        {
            return m_size;
        }

    private:
        // Note that in order for reverse iteration with std::reverse_iterator to work, the end()
        // iterator must be able to do it--. This is why we store prevous and next in the iterator,
        // otherwise end() wouldn't be able to go back.
        pointer        m_begin;
        size_type      m_previous;
        size_type      m_current;
        size_type      m_next;
        size_type      m_size;
    };

    template<typename N, bool isconst>
    typename N::size_type index(const tree_node_iterator<N, isconst>& it)
    {
        return it.get_current();
    }

    template<typename I>
    typename I::size_type index(const std::reverse_iterator<I>& it)
    {
        // std::reverse_iterator refers to the element one before the one refered by its internal iterator
        auto tmp = it.base();
        tmp--;
        return index(tmp);
    }

    template <typename T>
    class tree_node
    {
    public:
        typedef typename std::vector<tree_node<T>> container;
        typedef tree_node_iterator<tree_node<T>, false> iterator;
        typedef tree_node_iterator<tree_node<T>, true> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef typename container::difference_type difference_type;
        typedef typename container::size_type size_type;
        static size_type npos;
    
        tree_node(container& elems) :
            m_elem(),
            m_used(true),
            m_elems(elems),
            m_parent(npos),
            m_first_child(npos),
            m_last_child(npos),
            m_next_sibling(npos),
            m_previous_sibling(npos) {}

        tree_node(const tree_node&) = default;

        ~tree_node() { m_used = false; }
    
        tree_node& operator=(const tree_node& node)
        {
            if (&node != this) {
                m_elem = node.m_elem;
                m_parent = node.m_parent;
                m_first_child = node.m_first_child;
                m_last_child = node.m_last_child;
                m_next_sibling = node.m_next_sibling;
                m_previous_sibling = node.m_previous_sibling;
            }

            return *this;
        }

        iterator begin() { return iterator(&m_elems[0], npos, m_first_child, m_first_child != npos? m_elems.at(m_first_child).m_next_sibling : npos, m_elems.size()); }
        const_iterator begin() const { return const_iterator(&m_elems[0], npos, m_first_child, m_first_child != npos? m_elems.at(m_first_child).m_next_sibling : npos, m_elems.size()); }
        const_iterator cbegin() const { return const_iterator(&m_elems[0], npos, m_first_child, m_first_child != npos? m_elems.at(m_first_child).m_next_sibling : npos, m_elems.size()); }
        iterator end() { return iterator(&m_elems[0], m_last_child, npos, npos, m_elems.size()); }
        const_iterator end() const { return const_iterator(&m_elems[0], m_last_child, npos, npos, m_elems.size()); }
        const_iterator cend() const { return const_iterator(&m_elems[0], m_last_child, npos, npos, m_elems.size()); }
        reverse_iterator rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

        // Note the tree node needs to store a reference to the container (vector). A pointer to the
        // beginning of the array is not enough, because reallocation can change it.
        T               m_elem;
        bool            m_used;
        container&      m_elems;
        size_type       m_parent;
        size_type       m_first_child;
        size_type       m_last_child;
        size_type       m_next_sibling;
        size_type       m_previous_sibling;
    };

    template<typename T>
    typename tree_node<T>::size_type tree_node<T>::npos = -1;

    template <typename T>
    class sparse_tree
    {
    public:
        typedef tree_node<T> value_type;
        typedef T contained_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef std::vector<value_type> container;
        typedef typename container::difference_type difference_type;
        typedef typename container::allocator_type allocator_type;
        typedef typename container::size_type size_type;
        typedef tree_node_iterator<value_type, false> iterator;
        typedef tree_node_iterator<value_type, true> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    
        sparse_tree()
        {
            // This node acts as the common parent of all trees. This makes it possible to iterate over
            // all roots, by iterating over the children of node 0.
            m_elems.push_back(value_type(m_elems));
        }

        sparse_tree(const sparse_tree&) = default;

        ~sparse_tree() {}
    
        sparse_tree& operator=(const sparse_tree&) = default;

        // This class has no begin() and end() methods. This is intentional. In order to iterate
        // over the nodes, use method at() to access a tree_node by its index, and then call the
        // begin() method on the node_tree to iterate over its children (continue recursively). To
        // iterate over all nodes, use the at() method with index 0. This returns the root node
        // which is the parent of all trees.

        // Inserts a single node as the last child of an existing one
        size_type insert(const contained_type& t, size_type parent_index)
        {
            assert(parent_index < m_elems.size());
            if (!(parent_index < m_elems.size())) {
                throw std::out_of_range("sparse_tree::insert: invalid parent index");
            }
            assert(m_elems[parent_index].m_used);
            if (!(m_elems[parent_index].m_used)) {
                throw std::domain_error("sparse_tree::insert: parent has been erased");
            }

            size_type new_index = std::find_if(m_elems.begin(), m_elems.end(), [](const value_type& tn) {return !tn.m_used; }) - m_elems.begin();
            if (new_index == m_elems.size()) {
                m_elems.push_back(value_type(m_elems));
            }

            m_elems[new_index].m_elem = t;
            add_child(parent_index, new_index);

            return new_index;
        }

        // Inserts nodes by copying a subtree of another sparse_tree.
        // input_tree is the tree to read nodes from.
        // input_index is the index of the root of the subtree to read nodes from.
        // parent_index the index of the node in this sparse_tree to insert the subtree under.
        size_type insert(const sparse_tree<T>& input_tree, size_type input_index, size_type parent_index)
        {
            assert(parent_index < m_elems.size());
            if (!(parent_index < m_elems.size())) {
                throw std::out_of_range("sparse_tree::insert: invalid parent index");
            }
            assert(m_elems[parent_index].m_used);
            if (!(m_elems[parent_index].m_used)) {
                throw std::domain_error("sparse_tree::insert: parent has been erased");
            }

            // Insert all nodes
            std::map<size_type, size_type> new_indices;
            // The value_type::npos references should stay that way
            new_indices[value_type::npos] = value_type::npos;
            std::vector<size_type> pending_nodes;
            pending_nodes.push_back(input_index);
            while (!pending_nodes.empty()) {
                // If this node is the root of the input tree save its index for later
                size_type input_node_index = pending_nodes.back();
                pending_nodes.pop_back();

                // Allocate new index for the node and insert it
                auto nit = std::find_if(m_elems.begin(), m_elems.end(), [](const value_type& tn) {return !tn.m_used; });
                size_type new_index = nit - m_elems.begin();
                if (new_index == m_elems.size()) {
                    m_elems.push_back(value_type(m_elems));
                }
                new_indices[input_node_index] = new_index;

                // Insert the value in its new position
                const value_type& input_node = input_tree.at(input_node_index); 
                m_elems.at(new_index) = input_node;
                m_elems.at(new_index).m_used = true;

                for (auto it = input_node.rbegin(); it != input_node.rend(); ++it) {
                    pending_nodes.push_back(index(it));
                }
            }

            // Update the references in all inserted nodes
            // Remove the external index that may be present in the parent reference of the output root node
            // (we will set this later in method add_child())
            m_elems.at(new_indices.at(input_index)).m_parent = value_type::npos;
            for (auto& it : new_indices) {
                if (it.second != value_type::npos) {
                    value_type& new_node = m_elems.at(it.second);
                    new_node.m_parent = new_indices.at(new_node.m_parent);
                    new_node.m_first_child = new_indices.at(new_node.m_first_child);
                    new_node.m_last_child = new_indices.at(new_node.m_last_child);
                    new_node.m_next_sibling = new_indices.at(new_node.m_next_sibling);
                    new_node.m_previous_sibling = new_indices.at(new_node.m_previous_sibling);
                }
            }

            // Insert the root in the list of children of the parent
            size_type new_root_index = new_indices.at(input_index);
            add_child(parent_index, new_root_index);

            return new_root_index;
        }

        reference at(size_type index)
        {
            assert(index < m_elems.size());
            assert(m_elems[index].m_used);

            reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_tree::at invalid index, element has been erased");
            }

            return ret;
        }

        const_reference at(size_type index) const
        {
            assert(index < m_elems.size());
            assert(m_elems[index].m_used);

            const_reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_tree::at invalid index, element has been erased");
            }

            return ret;
        }

        void erase(size_type remove_index)
        {
            // Mark the node and all its descendants as not used, recursively
            std::vector<size_type> pending_nodes;
            pending_nodes.push_back(remove_index);
            while (!pending_nodes.empty()) {
                size_type pending_index = pending_nodes.back();
                pending_nodes.pop_back();

                value_type& node = m_elems.at(pending_index);
                node.m_used = false;

                for (auto it = node.rbegin(); it != node.rend(); ++it) {
                    pending_nodes.push_back(index(it));
                }
            }

            remove_child(m_elems.at(remove_index).m_parent, remove_index);
        }


        void erase(iterator it)
        {
            erase(index(it));
        }

        void erase(reverse_iterator it)
        {
            erase(index(it));
        }

        void clear()
        {
            m_elems.clear();
            // This node acts as the common parent of all trees. This makes it possible to iterate over
            // all roots, by iterating over the children of node 0.
            m_elems.push_back(value_type(m_elems));
        }

        void swap(sparse_tree& sf)
        {
            m_elems.swap(sf.m_elems);
        }

    private:
        void add_child(size_type parent_index, size_type new_index)
        {
            assert(parent_index < m_elems.size());
            assert(new_index < m_elems.size());

            size_type previous_sibling_index = m_elems.at(parent_index).m_last_child;
            if (previous_sibling_index != value_type::npos) {
                m_elems.at(previous_sibling_index).m_next_sibling = new_index;
            }

            value_type& new_node = m_elems.at(new_index);
            new_node.m_parent = parent_index;
            new_node.m_previous_sibling = previous_sibling_index;

            value_type& parent = m_elems.at(parent_index);
            parent.m_first_child = (parent.m_first_child == value_type::npos? new_index : parent.m_first_child);
            parent.m_last_child = new_index;
        }

        // Note: remove_index must still have valid references when this method is called
        void remove_child(size_type parent_index, size_type remove_index)
        {
            assert(parent_index < m_elems.size());
            assert(remove_index < m_elems.size());

            size_type previous_sibling_index = m_elems.at(remove_index).m_previous_sibling;
            if (previous_sibling_index < m_elems.size()) {
                value_type& previous_sibling = m_elems.at(previous_sibling_index);
                previous_sibling.m_next_sibling = m_elems.at(remove_index).m_next_sibling;
            }

            size_type next_sibling_index = m_elems.at(remove_index).m_next_sibling;
            if (next_sibling_index < m_elems.size()) {
                value_type& next_sibling = m_elems.at(next_sibling_index);
                next_sibling.m_previous_sibling = m_elems.at(remove_index).m_previous_sibling;
            }

            value_type& parent = m_elems.at(parent_index);
            parent.m_first_child = (parent.m_first_child == remove_index? m_elems.at(remove_index).m_next_sibling : parent.m_first_child);
            parent.m_last_child = (parent.m_last_child == remove_index? m_elems.at(remove_index).m_previous_sibling : parent.m_last_child);
        }

        container  m_elems;
    };
} // namespace rte

#endif // SPARSE_TREE_HPP
