#ifndef SPARSE_VECTOR_HPP
#define SPARSE_VECTOR_HPP

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

    class sparse_node
    {
    public:
        sparse_node() :
            m_used(false),
            m_parent(npos),
            m_first_child(npos),
            m_last_child(npos),
            m_next_sibling(npos),
            m_previous_sibling(npos) {}

        sparse_node(const sparse_node&) = default;

        ~sparse_node() { m_used = false; }
    
        sparse_node& operator=(const sparse_node& node)
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
    class sparse_node_iterator
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
    
        sparse_node_iterator() :
            m_begin(nullptr),
            m_previous(0U),
            m_current(0U),
            m_next(0U),
            m_size(0U) {}

        sparse_node_iterator(pointer begin,
                index_type previous,
                index_type current,
                index_type next,
                index_type size) :
            m_begin(begin),
            m_previous(previous),
            m_current(current),
            m_next(next),
            m_size(size) {}

        sparse_node_iterator(const sparse_node_iterator<T, false>& spi) :
            m_begin(spi.get_begin()),
            m_previous(spi.get_previous()),
            m_current(spi.get_current()),
            m_next(spi.get_next()),
            m_size(spi.get_size()) {}

        ~sparse_node_iterator() {}

        sparse_node_iterator& operator=(const sparse_node_iterator&) = default;

        bool operator==(const sparse_node_iterator& spi) const
        {
            return (m_begin == spi.m_begin && m_current == spi.m_current);
        }

        bool operator!=(const sparse_node_iterator& spi) const { return !(*this == spi); }

        sparse_node_iterator& operator++()
        {
            m_current = m_next;
            m_previous = m_begin[m_current].m_previous_sibling;
            m_next = m_begin[m_current].m_next_sibling;
            return (*this);
        }

        sparse_node_iterator operator++(int)
        {
            sparse_node_iterator tmp(*this);
            ++*this;
            return tmp;
        }

        sparse_node_iterator& operator--()
        {
            m_current = m_previous;
            m_previous = m_begin[m_current].m_previous_sibling;
            m_next = m_begin[m_current].m_next_sibling;
            return (*this);
        }

        sparse_node_iterator operator--(int)
        {
            sparse_node_iterator tmp(*this);
            --*this;
            return tmp;
        }

        reference operator*() const
        {
            assert(m_current < m_size);
            if (!(m_current < m_size)) {
                throw std::out_of_range("sparse_node_iterator::operator*: invalid index");
            }

            assert(m_begin[m_current].m_used);
            if (!m_begin[m_current].m_used) {
                throw std::domain_error("sparse_node_iterator::operator*: entry has been erased");
            }

            return m_begin[m_current];
        }

        pointer operator->() const
        {
            assert(m_current < m_size);
            if (!(m_current < m_size)) {
                throw std::out_of_range("sparse_node_iterator::operator->: invalid index");
            }

            assert(m_begin[m_current].m_used);
            if (!m_begin[m_current].m_used) {
                throw std::domain_error("sparse_node_iterator::operator->: entry has been erased");
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
    index_type index(const sparse_node_iterator<T, isconst>& it)
    {
        return it.get_current();
    }

    template<typename I>
    index_type index(const std::reverse_iterator<I>& it)
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
        typedef sparse_node_iterator<value_type, false> iterator;
        typedef sparse_node_iterator<value_type, true> const_iterator;
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

        // This method has the same behavior as at() but doesn't check for m_used. This is meant to be used
        // in contexts where entries that have not yet been marked as used need to be manipulated
        reference physical_at(index_type index)
        {
            assert(index < m_elems.size());
            reference ret = m_elems.at(index);

            return ret;
        }

        const_reference physical_at(index_type index) const
        {
            assert(index < m_elems.size());
            const_reference ret = m_elems.at(index);

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
} // namespace rte

#endif // SPARSE_VECTOR_HPP
