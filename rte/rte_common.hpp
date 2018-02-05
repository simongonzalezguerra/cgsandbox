#ifndef RTE_COMMON_HPP
#define RTE_COMMON_HPP

#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <cassert>
#include <vector>

namespace rte
{
    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to represent the format of an image in memory.
    //-----------------------------------------------------------------------------------------------
    enum class image_format
    {
        none,
        rgb,
        rgba,
        bgr,
        bgra
    };

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a texture. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_texture_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a buffer. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_buffer_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a cubemap. Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_cubemap_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store a handle to a shader program (vertex and fragment shader). Non-zero.
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int gl_program_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Type used to store an id that can be set by the user. Non-zero, optional, but unique
    //-----------------------------------------------------------------------------------------------
    typedef unsigned int user_id;

    //-----------------------------------------------------------------------------------------------
    //! @brief Constant representing 'not a user id'. This is the default user id assiged to
    //!  objects that the user hasn't assigned an id to.
    //-----------------------------------------------------------------------------------------------
    constexpr user_id nuser_id = -1;

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

    template <typename C, bool isconst = false> 
    class sparse_vector_iterator {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename choose<isconst, const typename C::const_pointer, typename C::pointer>::type pointer;
        typedef typename choose<isconst, const typename C::const_reference, typename C::reference>::type reference;
        typedef typename C::difference_type difference_type;
        typedef typename C::value_type value_type;
        typedef typename C::size_type size_type;
    
        sparse_vector_iterator() {}

        sparse_vector_iterator(typename C::iterator begin, typename C::iterator current, typename C::iterator end)
            : m_begin(begin), m_current(current), m_end(end) {}

        sparse_vector_iterator(const sparse_vector_iterator<C, false>& spi) :
            m_begin(spi.m_begin), m_current(spi.m_current), m_end(spi.m_end) {} // TODO can we default this?

        ~sparse_vector_iterator() {}

        sparse_vector_iterator& operator=(const sparse_vector_iterator&) = default;

        bool operator==(const sparse_vector_iterator& spi) const
        {
            if (m_begin != spi.m_begin) return false;
            if (m_end != spi.m_end) return false;
            typename C::iterator tmp_current = m_current;
            while (tmp_current < m_end && !tmp_current->m_used) {
                tmp_current++;
            }
            typename C::iterator tmp_spi_current = spi.m_current;
            while (tmp_spi_current < spi.end && !tmp_spi_current->m_used) {
                tmp_spi_current++;
            }
            return (tmp_current == tmp_spi_current);
        }

        bool operator!=(const sparse_vector_iterator& spi) const { return !(*this == spi); }

        sparse_vector_iterator& operator++()
        {
            if (m_current < m_end) {
                do {
                    m_current++;
                } while (m_current < m_end && !m_current->m_used);
            }
            return (*this);
        }

        sparse_vector_iterator operator++(int)
        {
            sparse_vector_iterator tmp(*this);
            ++*this;
            return tmp;
        }

        sparse_vector_iterator& operator--() 
        {
            do {
                m_current--;
            } while (m_current >= m_begin && !m_current.m_used);

            return (*this);
        }

        sparse_vector_iterator& operator--(int)
        {
            sparse_vector_iterator tmp(*this);
            --*this;
            return tmp;
        }

        reference operator*() const
        {
            assert(m_current >= m_begin && m_current < m_end);
            if (!(m_current >= m_begin && m_current < m_end)) {
                throw std::domain_error("sparse_vector_iterator::operator*: invalid index");
            }

            return *m_current;
        }

        pointer operator->() const
        {
            assert(m_current >= m_begin && m_current < m_end);
            if (!(m_current >= m_begin && m_current < m_end)) {
                throw std::domain_error("sparse_vector_iterator::operator*: invalid index");
            }

            return m_current.operator->();
        }
    
    private:
        typename C::iterator  m_begin;
        typename C::iterator  m_current;
        typename C::iterator  m_end;
    };

    template <typename C>
    class sparse_vector {
    public:
        typedef typename C::allocator_type allocator_type;
        typedef typename C::value_type value_type; 
        typedef typename C::reference reference;
        typedef typename C::const_reference const_reference;
        typedef typename C::difference_type difference_type;
        typedef typename sparse_vector_iterator<C, false> iterator;
        typedef typename sparse_vector_iterator<C, true> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef std::size_t size_type;
    
        sparse_vector() : m_elems(), m_num_elems(0) {}
        sparse_vector(const sparse_vector&) = default;
        ~sparse_vector() {}
    
        sparse_vector& operator=(const sparse_vector&) = default;
        bool operator==(const sparse_vector& sv) const { return m_elems == sv.m_elems; }
        bool operator!=(const sparse_vector& sv) const { return !(m_elems == sv.m_elems); }

        iterator begin(); { return iterator(m_elems.begin(), m_elems.begin(), m_elems.end()); }
        const_iterator begin() const { return const_iterator(m_elems.begin(), m_elems.begin(), m_elems.end()); }
        const_iterator cbegin() const { return const_iterator(m_elems.begin(), m_elems.begin(), m_elems.end()); }
        iterator end() { return iterator(m_elems.begin(), m_elems.end(), m_elems.end()); }
        const_iterator end() const { return const_iterator(m_elems.begin(), m_elems.end(), m_elems.end()); }
        const_iterator cend() const { return const_iterator(m_elems.begin(), m_elems.end(), m_elems.end()); }

        reverse_iterator rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        reverse_iterator rend(); { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

        size_type insert(const value_type& value)
        {
            size_type new_index = std::find_if(m_elems.begin(), m_elems.end(), [](const value_type& elem) { return !elem.m_used; }) - m_elems.begin();
            if (new_index == m_elems.size()) {
                m_elems.push_back(value);
            } else {
                m_elems[new_index] = value;
                m_elems[new_index].m_used = true;
            }
            m_num_elems++;

            return new_index;
        }

        template<typename InputIterator >
        void insert(InputIterator first, InputIterator last)
        {
            for (InputIterator it = first; it != last; it++) {
                insert(*it);
            }
        }

        reference at(size_type index)
        {
            assert(index < m_elems.size());
            assert(m_elems[index].m_used);

            reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_vector::at invalid index, element has been erased");
            }

            return ret;
        }

        const_reference at(size_type) const
        {
            assert(index < m_elems.size());
            assert(m_elems[index].m_used);

            reference ret = m_elems.at(index);

            if (!ret.m_used) {
                throw std::domain_error("sparse_vector::at invalid index, element has been erased");
            }

            return ret;
        }

        iterator erase(const_iterator it)
        {
            reference elem = *it;
            assert(elem.m_used);
            if (!elem.m_used) {
                throw std::domain_error("sparse_vector::erase invalid index, element has already been erased");
            }
            elem.m_used = false;
            m_num_elems--;

            return ++it;
        }

        void clear()
        {
            m_elems.clear();
            m_num_elems = 0;
        }

        void swap(sparse_vector& sv)
        {
            swap(m_elems, sv.m_elems);
        }

        size_type size() const
        {
            return m_num_elems;
        }

        bool empty() const {return (m_num_elems == 0); }

        A get_allocator() const
        {
            return m_elems.get_allocator();
        }

    private:
        C          m_elems;
        size_type  m_num_elems;
    };
} // namespace rte

#endif // RTE_COMMON_HPP
