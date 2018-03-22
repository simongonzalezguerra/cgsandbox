#ifndef SPARSE_TREE_HPP
#define SPARSE_TREE_HPP

#include "sparse_vector.hpp"

namespace rte
{
    template<typename V>
    void tree_init(V& tree)
    {
        tree.clear();
    }

    template<typename V>
    index_type tree_allocate_node(V& tree, const std::set<index_type>& previously_allocated_indexes)
    {
        index_type ret = tree.size();
        for (index_type i = 0; i < tree.size(); i++) {
            if (!tree.physical_at(i).m_used && previously_allocated_indexes.count(i) == 0) {
                ret = i;
                break;
            }
        }
        if ((!ret < tree.size())) {
            tree.push_back(typename V::value_type());
        }

        return ret;
    }

    // parent_index must be the index of a valid, used element.
    // new_index can be not marked as used yet
    template<typename V>
    void tree_add_child(V& tree, index_type parent_index, index_type new_index)
    {
        assert(parent_index < tree.size());
        assert(new_index < tree.size());
        auto& parent = tree.at(parent_index);
        auto& new_node = tree.physical_at(new_index);

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
        assert(input_index < input_tree.size());
        if (!(input_index < input_tree.size())) {
            throw std::out_of_range("tree_insert: invalid input index");
        }
        assert(input_tree.at(input_index).m_used);
        if (!(input_tree.at(input_index).m_used)) {
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

            // Insert the value in its new position. The new entries are not yet marked as used, so we
            // can't use at() because it would fail the assertion on m_used. We use utility method physical_at
            // which has the same behavior as at() but doesn't check for m_used
            const auto& input_node = input_tree.at(input_node_index);
            output_tree.physical_at(new_index) = input_node;

            for (auto it = tree_rbegin(input_tree, input_node_index); it != tree_rend(input_tree, input_node_index); ++it) {
                pending_nodes.push_back(index(it));
            }
        }

        // Update the references in all inserted nodes
        // Remove the external index that may be present in the parent reference of the output root node
        // (we will set this later in function tree_add_child())
        output_tree.physical_at(new_index_map.at(input_index)).m_parent = npos;
        for (auto& it : new_index_map) {
            if (it.second != npos) {
                auto& new_node = output_tree.physical_at(it.second);
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
        if (output_parent_index < output_tree.size() && output_tree.physical_at(output_parent_index).m_used) {
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
    void tree_erase(V& tree, index_type erase_index)
    {
        assert(erase_index < tree.size());
        if (!(erase_index < tree.size())) {
            throw std::out_of_range("sparse_vector::erase: invalid remove index");
        }
        assert(tree.at(erase_index).m_used);
        if (!(tree.at(erase_index).m_used)) {
            throw std::domain_error("sparse_vector::erase: remove index has already been erased");
        }
        // Mark the node and all its descendants as not used, recursively
        std::set<index_type> to_delete;
        std::vector<index_type> pending_nodes;
        pending_nodes.push_back(erase_index);
        while (!pending_nodes.empty()) {
            index_type pending_index = pending_nodes.back();
            pending_nodes.pop_back();

            to_delete.insert(pending_index);

            for (auto it = tree_rbegin(tree, pending_index); it != tree_rend(tree, pending_index); ++it) {
                pending_nodes.push_back(index(it));
            }
        }

        // Exception safety: now that we have passed all the throw points, impact the changes in the structure
        if (tree.at(erase_index).m_parent < tree.size()) {
            tree_remove_child(tree, tree.at(erase_index).m_parent, erase_index);
        }
        for (auto it = to_delete.begin(); it != to_delete.end(); ++it) {
            tree.clear_used(*it);
        }
    }

    template<typename V> typename V::iterator tree_begin(V& v, index_type i) { auto& elem = v.at(i); return typename V::iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).m_next_sibling : npos, v.size()); }
    template<typename V> typename V::const_iterator tree_begin(const V& v, index_type i) { auto& elem = v.at(i); return typename V::const_iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).m_next_sibling : npos, v.size()); }
    template<typename V> typename V::const_iterator tree_cbegin(const V& v, index_type i) { auto& elem = v.at(i); return typename V::const_iterator(&v.at(0), npos, elem.m_first_child, elem.m_first_child != npos? v.at(elem.m_first_child).m_next_sibling : npos, v.size()); }
    template<typename V> typename V::iterator tree_end(V& v, index_type i) { auto& elem = v.at(i); return typename V::iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    template<typename V> typename V::const_iterator tree_end(const V& v, index_type i)  { auto& elem = v.at(i); return typename V::const_iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    template<typename V> typename V::const_iterator tree_cend(const V& v, index_type i) { auto& elem = v.at(i); return typename V::const_iterator(&v.at(0), elem.m_last_child, npos, npos, v.size()); }
    template<typename V> typename V::reverse_iterator tree_rbegin(V& v, index_type i) { return typename V::reverse_iterator(tree_end(v, i)); }
    template<typename V> typename V::const_reverse_iterator tree_rbegin(const V& v, index_type i) { return typename V::const_reverse_iterator(tree_end(v, i)); }
    template<typename V> typename V::const_reverse_iterator tree_crbegin(const V& v, index_type i) { return typename V::const_reverse_iterator(tree_end(v, i)); }
    template<typename V> typename V::reverse_iterator tree_rend(V& v, index_type i) { return typename V::reverse_iterator(tree_begin(v, i)); }
    template<typename V> typename V::const_reverse_iterator tree_rend(const V& v, index_type i) { return typename V::const_reverse_iterator(tree_begin(v, i)); }
    template<typename V> typename V::const_reverse_iterator tree_crend(const V& v, index_type i) { return typename V::const_reverse_iterator(tree_begin(v, i)); }
} // namespace rte

#endif // SPARSE_TREE_HPP
