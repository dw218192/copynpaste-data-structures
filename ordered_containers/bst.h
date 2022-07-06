#pragma once
#include <vector>
#include <type_traits>
#include <algorithm>
#include <functional>

#define UNUSED(x) (void)(x)
using node_id_t = int;
struct null_t { };

template<typename Key, typename Mapped>
struct null_treedata {
    void init(Key const& key, Mapped const& val) { UNUSED(key),UNUSED(val); }
    void combine(null_treedata const* lhs, null_treedata const* rhs) { UNUSED(lhs),UNUSED(rhs); }
};

template<typename Key,
         typename Mapped = null_t,
         typename CmpFn = std::less<Key>,
         typename BalanceData = null_treedata<Key, Mapped>,
         typename MetaData = null_treedata<Key, Mapped>>
class bst {
public:
    using key_t = Key;
    using mapped_t = Mapped;
    using cmp_fn = CmpFn;
    using balancedata_t = BalanceData;
    using metadata_t = MetaData;

    static constexpr node_id_t null_id = 0;
    struct node {
        node() : p(null_id) { sons[0] = sons[1] = null_id; }
        node(key_t const& key, mapped_t const& data) {
            init(key, data);
        }
        void init(key_t const& key, mapped_t const& data) {
            p = sons[0] = sons[1] = null_id;
            this->key = key;
            this->data = data;
        }
        int p, sons[2]; // parent, children
        key_t key;
        mapped_t data;
        balancedata_t balance_data; // balance_data for various bst's
        metadata_t meta_data; // additional data to maintain
    };
protected:
    cmp_fn _cmp;
    size_t _size;
    std::vector<node> _nodes;     // node pool, 1-indexed
    std::vector<int>  _free_list; // free list for the node pool

    node_id_t _new_node(key_t const& key, mapped_t const& val) {
        if(_free_list.empty()) {
            _nodes.emplace_back(key, val);
            return _nodes.size() - 1;
        } else {
            int ret = _free_list.back();
            _free_list.pop_back();
            _nodes[ret].init(key, val);
            return ret;
        }
    }
    void _recycle(node_id_t id) {
        _free_list.push_back(id);
    }
#define N(x) _nodes[x]
    // forms a new link s.t. p.sons[dir] = x, x.p = p;
    void _relink(node_id_t p, bool dir, node_id_t x) {
        N(p).sons[dir] = x;
        if(x != null_id) N(x).p = p;
    }
    /** binary tree rotation, changes p-x to x-p
     *  precondition: p is not null, p.sons[!dir] is not null
     */
    void _rotate(node_id_t x) {
        node_id_t p = N(x).p, g = N(p).p;
        bool d1 = N(g).sons[1] == p, d2 = N(p).sons[1] == x;
        node_id_t y = N(x).sons[!d2];
        _relink(g, d1, x), _relink(p, d2, y), _relink(x, !d2, p);
        _pushup(p), _pushup(x);
    }
    /** swaps the data of node x and node y
     *  precondition: x and y are not null
     *  postcondition: y.sons, x.sons, y.p, x.p are not swapped
     */
    void _swap_data(node_id_t x, node_id_t y) {
        std::swap(N(x).key, N(y).key);
        std::swap(N(x).data, N(y).data);
        if constexpr (!std::is_same<balancedata_t, null_treedata<key_t, mapped_t>>::value)
            std::swap(N(x).balance_data, N(y).balance_data);
        if constexpr (!std::is_same<metadata_t, null_treedata<key_t, mapped_t>>::value)
            std::swap(N(x).meta_data, N(y).meta_data);
    }
    /** does BST search and stops at the parent if it exists
     *  fills p with the parent node, dir with the target node's dir if it exists
     *  returns 1 if the node exists, else 0
     */
    bool _find(key_t const& key, node_id_t& p, bool& dir) const {
        node_id_t u = root(); p = null_id; dir = 0;
        while (u != null_id) {
            bool b1 = _cmp(key, N(u).key), b2 = _cmp(N(u).key, key);
            if(b1 || b2)
                p = u, u = N(u).sons[b2], dir = b2;
            else return true;
        }
        return false;
    }
    /** find the successor or predecessor
     *  precondition: x must not be null
     */
    node_id_t _nxt(node_id_t x, bool dir) const {
        node_id_t u = N(x).sons[dir];
        if(u != null_id) {
            while(N(u).sons[!dir] != null_id)
                u = N(u).sons[!dir];
            return u;
        } else {
            u = x;
            node_id_t p = N(u).p;
            while(p != null_id) {
                if(N(p).sons[!dir] == u) return p;
                u = p, p = N(u).p;
            }
            return null_id;
        }
    }
    // metadata info maintainance
    void _pushup(node_id_t x) {
        node_id_t l = N(x).sons[0], r = N(x).sons[1];
        N(x).meta_data.init(N(x).key, N(x).data);
        N(x).balance_data.init(N(x).key, N(x).data);
        metadata_t const* lmt = nullptr, * rmt = nullptr;
        balancedata_t const* lbt = nullptr, * rbt = nullptr;
        if(l != null_id) lmt = &N(l).meta_data, lbt = &N(l).balance_data;
        if(r != null_id) rmt = &N(r).meta_data, rbt = &N(r).balance_data;
        N(x).meta_data.combine(lmt, rmt);
        N(x).balance_data.combine(lbt, rbt);
    }
    void _push_up_to_root(node_id_t x) {
        for(; x!=null_id; x=N(x).p)
            _pushup(x);
    }
    // called after insert x with parent p; x is null_id if it's already in the tree
    virtual void _post_insert(node_id_t p, node_id_t x) { UNUSED(p); 
        if(x != null_id) _push_up_to_root(x); 
    }
    // called after erase x with parent p; x is null_id if it's not found
    virtual void _post_erase(node_id_t p, node_id_t x) {
        if(x != null_id) _push_up_to_root(p);
    }
    // called after find x with parent p; x is null_id if it's not found
    virtual void _post_find(node_id_t p, node_id_t x) { UNUSED(p),UNUSED(x); }
public:
    template<typename T = cmp_fn, typename = std::enable_if_t<std::is_default_constructible<T>::value>>
    bst() : _size(0) { _nodes.emplace_back(); }
    bst(cmp_fn cmp) : _size(0), _cmp(cmp) { _nodes.emplace_back(); }
    node_id_t root() const { return _nodes[null_id].sons[0]; }
    node_id_t prev(node_id_t id) const { return _nxt(id,0); }
    node_id_t next(node_id_t id) const { return _nxt(id,1); }
    node const& get(node_id_t id) const { return _nodes[id]; }
    
    template<typename CallbackFn>
    void trav(CallbackFn&& f) {
        auto impl = [&](auto& self, node_id_t cur) {
            if(cur == null_id) return;
            self(self, N(cur).sons[0]), f(get(cur)), self(self, N(cur).sons[1]);
        };
        impl(impl, root());
    }
    /** inserts a key value pair into bst
     *  on success, returns the id of the node
     */
    virtual node_id_t insert(key_t const& key, mapped_t const& value = null_t()) {
        node_id_t p,x = null_id; bool dir;
        if(!_find(key, p, dir)) {
            x = _new_node(key, value);
            _relink(p, dir, x);
            ++ _size;
        }
        _post_insert(p,x);
        return x;
    }
    virtual void erase(key_t const& key) {
        node_id_t p, x; bool dir;
        if(_find(key, p, dir)) {
            x = N(p).sons[dir];
            if(N(x).sons[0] != null_id && N(x).sons[1] != null_id) {
                int u = _nxt(x, 1);
                _swap_data(x, u);
                x = u, p = N(x).p, dir = N(p).sons[1] == x;
            }
            _relink(p, dir, N(x).sons[N(x).sons[1] != null_id]);
            _recycle(x);
            -- _size;
        }
        _post_erase(p,x);
    }
    // key query functions
    node_id_t find(key_t const& key) { 
        node_id_t p; bool dir; 
        node_id_t x = _find(key, p, dir) ? N(p).sons[dir] : null_id;
        _post_find(p,x);
        return x;
    }
    node_id_t lower_bound(key_t const& key) {
        //todo
    }
    node_id_t upper_bound(key_t const& key) {
        //todo
    }
    bool has(key_t const& key) { return find(key) != null_id; }
    size_t size() const { return _size; }

    template<typename T = mapped_t, typename = std::enable_if_t<!std::is_same<T,null_t>::value>>
    mapped_t& operator[](key_t const& key) {
        node_id_t id = find(key);
        if(id == null_id) return N(insert(key, mapped_t())).data;
        else return N(id).data;
    }
#undef N
};


/***
 * update policy
 */
template<typename Key, typename Mapped>
struct size_metadata {
    size_t size;
    void init(Key const& key, Mapped const& val) {
        size = 1; (void)key, (void)val;
    }
    void combine(size_metadata const* lhs, size_metadata const* rhs) {
        size_t lsz = lhs ? lhs->size : 0, rsz = rhs ? rhs->size : 0;
        this->size = 1 + lsz + rsz;
    }
};

template<typename Key,
         typename Mapped = null_t,
         typename CmpFn = std::less<Key>,
         typename BalanceData = null_treedata<Key, Mapped>,
         typename MetaData = size_metadata<Key, Mapped>>
struct update_policy : public bst<Key, Mapped, CmpFn, BalanceData, MetaData> {
    using base = bst<Key, Mapped, CmpFn, BalanceData, MetaData>;
    using key_t = typename base::key_t;
    using mapped_t = typename base::mapped_t;
    using cmp_fn = typename base::cmp_fn;
    using metadata_t = typename base::metadata_t;
#define N(x) base::_nodes[x]
    template<typename T = metadata_t, typename = std::enable_if_t<std::is_same<T, size_metadata<key_t, mapped_t>>::value>>
    node_id_t at(size_t k){
        node_id_t u = this->root();
        while (u != base::null_id) {
            size_t lsz = 0;
            if(N(u).sons[0] != base::null_id) {
                lsz = N(N(u).sons[0]).meta_data.size;
            }
            if (lsz > k) u = N(u).sons[0];
            else if (lsz == k) return u;
            else k -= lsz + 1, u = N(u).sons[1];
        }
        return base::null_id;
    }
#undef N
};

/***
 * self-balancing BSTs
 */
template<typename Key, 
         typename Mapped = null_t,
         typename CmpFn = std::less<Key>>
class splaytree : public update_policy<Key, Mapped, CmpFn> {
    using base = update_policy<Key, Mapped, CmpFn>;
protected:
#define N(x) base::_nodes[x]
    void _splay(node_id_t x, node_id_t k) {
        while (N(x).p != k) {
            node_id_t p = N(x).p, g = N(p).p;
            if (g != k) {
                if ((N(p).sons[1] == x) ^ (N(g).sons[1] == p)) 
                    this->_rotate(x); // zig-zag
                else
                    this->_rotate(p); // zig-zig
            }
            this->_rotate(x);
        }
    }
    virtual void _post_insert(node_id_t p, node_id_t x) override {
        if(x != base::null_id) _splay(x, base::null_id);
        else if(p != base::null_id) _splay(p, base::null_id);
    }
    virtual void _post_erase(node_id_t p, node_id_t x) override {
        if(p != base::null_id) _splay(p, base::null_id), (void)x;
    }
    virtual void _post_find(node_id_t p, node_id_t x) override {
        if(x != base::null_id) _splay(x, base::null_id);
        else if(p != base::null_id) _splay(p, base::null_id);
    }
#undef N
};

template<typename Key, typename Mapped>
struct avl_data {
    int height = 0; // value for if node id == null_id
    void init(Key const& key, Mapped const& val) {
        height = 1; (void)key, (void)val;
    }
    void combine(avl_data const* lhs, avl_data const* rhs) {
        int lh = lhs ? lhs->height : 0, rh = rhs ? rhs->height : 0;
        this->height = 1 + std::max(lh, rh);
    }
};
template<typename Key, 
         typename Mapped = null_t,
         typename CmpFn = std::less<Key>>
class avltree : public update_policy<Key, Mapped, CmpFn, avl_data<Key, Mapped>> {
protected:
    using base = update_policy<Key, Mapped, CmpFn, avl_data<Key, Mapped>>;
#define N(x) base::_nodes[x]
    int _get_height(node_id_t x) __attribute__((always_inline)) {
    // no need to check for null_id because N(null_id)'s height is 0
        return N(x).balance_data.height;
    }
    bool _is_balanced(node_id_t x) {
        return std::abs(_get_height(N(x).sons[0])-_get_height(N(x).sons[1])) <= 1;
    }
    node_id_t _tall_child(node_id_t x) {
        return N(x).sons[_get_height(N(x).sons[1]) > _get_height(N(x).sons[0])];
    }
    void _fix(int x) {
        node_id_t p = N(x).p;
        if(p == base::null_id) return;
        node_id_t g = N(p).p;
        base::_pushup(x), base::_pushup(p);
        while(g != base::null_id) {
            if(!_is_balanced(g)) {
                p = _tall_child(g), x = _tall_child(p);
                if ((N(p).sons[1] == x) ^ (N(g).sons[1] == p)) 
                    this->_rotate(x), this->_rotate(x); // zig-zag
                else
                    this->_rotate(p); // zig-zig
            } else {
                base::_pushup(g);
            }
            g = N(g).p;
        }
    }
    virtual void _post_insert(node_id_t p, node_id_t x) override { UNUSED(p);
        if(x != base::null_id) _fix(x);
    }
    virtual void _post_erase(node_id_t p, node_id_t x) override { UNUSED(x);
        if(p != base::null_id) _fix(p);
    }
#undef N
};

template<typename Key, typename Mapped>
struct rb_data {
    enum color_t : bool { B=0,R=1 };
    color_t color = color_t::B; // value for color if node_id == null_id
    void init(Key const& key, Mapped const& val) { (void)key,(void)val; }
    void combine(rb_data const* lhs, rb_data const* rhs) { (void)lhs,(void)rhs; }
};
template<typename Key, 
         typename Mapped = null_t,
         typename CmpFn = std::less<Key>>
class rbtree : public update_policy<Key, Mapped, CmpFn, rb_data<Key, Mapped>> {
protected:
    using base = update_policy<Key, Mapped, CmpFn, rb_data<Key, Mapped>>;
    using color_t = typename rb_data<Key, Mapped>::color_t;
    using key_t = typename base::key_t;
    using mapped_t = typename base::mapped_t;
#define N(x) base::_nodes[x]
    void _flip_color(node_id_t x) { N(x).balance_data.color = (color_t)!(bool)N(x).balance_data.color; }
    color_t _get_color(node_id_t x) __attribute__((always_inline)) {
    // no need to check for null_id because N(null_id)'s color is black
        return N(x).balance_data.color;
    }
    void _set_color(node_id_t x, color_t c) { N(x).balance_data.color = c; }
    bool _is_two_node(node_id_t x) {
        return _get_color(x) == color_t::B && _get_color(N(x).sons[0]) == color_t::B && _get_color(N(x).sons[1]) == color_t::B;
    }
    bool _is_three_node(node_id_t x) {
        return _get_color(x) == color_t::B && ((_get_color(N(x).sons[0]) == color_t::R) ^ (_get_color(N(x).sons[1]) == color_t::R));
    }
    bool _is_four_node(node_id_t x) {
        return _get_color(x) == color_t::B && _get_color(N(x).sons[0]) == color_t::R && _get_color(N(x).sons[1]) == color_t::R;
    }
    // make x,p,g a four-node
    int _fix_four_node(node_id_t x) {
        int p = N(x).p, g = N(p).p;
        int r;
        if ((N(p).sons[1] == x) ^ (N(g).sons[1] == p)) {
            this->_rotate(x), this->_rotate(x); // zig-zag 
            r = x;
        } else {
            this->_rotate(p); // zig-zig
            r = p;
        }
        _set_color(r, color_t::B), _set_color(N(r).sons[0], color_t::R), _set_color(N(r).sons[1], color_t::R);
        return r;
    }
public:
    /* since I implement top-down insert/erase, rbtree needs to override insert and erase directly */
    virtual node_id_t insert(key_t const& key, mapped_t const& value = null_t()) override {
        node_id_t x = this->root(), p = base::null_id;
        bool dir = false;
        while(x != base::null_id) {
            dir = this->_cmp(N(x).key, key);
            if(this->_cmp(key, N(x).key) || dir) {
                if(_is_four_node(x)) {
                    _flip_color(N(x).sons[0]), _flip_color(N(x).sons[1]), _flip_color(x);
                    if(_get_color(p) == color_t::R)
                        _fix_four_node(x);
                }
            } else {
                return base::null_id;
            }
            p = x, x = N(x).sons[dir];
        }
        x = base::_new_node(key, value), _set_color(x, color_t::R);
        base::_relink(p, dir, x);
        if(_get_color(p) == color_t::R)
            _fix_four_node(x);
        _set_color(this->root(), color_t::B);
        ++ this->_size;
        base::_post_insert(p,x);
        return x;
    }
    virtual void erase(key_t const& key) override {
        node_id_t x, p = base::null_id, f = base::null_id, sibling = base::null_id;
        x = N(this->root()).p; // N(null_id).sons[0] is the root
        
        bool dir = false;
        while(N(x).sons[dir] != base::null_id) {
            p = x;
            sibling = N(x).sons[!dir];
            x = N(x).sons[dir];
            
            dir = this->_cmp(N(x).key, key);
            if(!this->_cmp(key, N(x).key) && !dir)
                f = x;

            if(_is_two_node(x)) {
                if(sibling != base::null_id) {
                    if(_is_two_node(sibling)) {
                        _set_color(x, color_t::R), _set_color(sibling, color_t::R), _set_color(p, color_t::B);
                    } else {
                        node_id_t red_child = N(sibling).sons[_get_color(N(sibling).sons[1]) == color_t::R];
                        node_id_t g = _fix_four_node(red_child);
                        _set_color(g, color_t::R), _set_color(N(g).sons[0], color_t::B),
                        _set_color(N(g).sons[1], color_t::B), _set_color(x, color_t::R);
                    }
                }
            } else if(_is_three_node(x) && _get_color(N(x).sons[dir]) == color_t::B) {
                node_id_t red_child = N(x).sons[!dir];
                this->_rotate(red_child);
                _set_color(red_child, color_t::B), _set_color(x, color_t::R);
                p = N(x).p;
            }
        }
        _set_color(this->root(), color_t::B);
        
        if(f != base::null_id) {
            if(f != x) {
                base::_swap_data(f,x);
                // swap color back, overriding default behavior
                std::swap(N(f).balance_data.color, N(x).balance_data.color);
            }
            base::_relink(p, N(p).sons[1]==x, N(x).sons[N(x).sons[1] != base::null_id]);
            base::_recycle(x);
            -- this->_size;
        } else {
            p = base::null_id, x = base::null_id;
        }
        base::_post_erase(p,x);
    }
#undef N
};