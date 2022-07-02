#pragma once
#include <vector>
#include <functional>
#include <algorithm>
using std::vector;

struct no_lazy_prop_tag { };
/**
 * @brief a segment tree
 * 
 * @tparam DataType: data type stored in the array, must overload operator = and +=
 * @tparam CombineFn: a binary function that takes two DataType and returns a combined DataType  
 * @tparam ResolveFn: (only in lazy propagation) a function that is equivalent to applying CombineFn over a range of DataType in [l,r]
 * @tparam CumulativeUpdate: whether update overwrites or adds to the data
 */
template<
    typename DataType,
    typename CombineFn,
    typename ResolveFn = no_lazy_prop_tag,
    bool     CumulativeUpdate = false
>
class segtree {
    struct base_node {
        base_node(int left, int right) :
            val(), left(left), right(right) { }
        base_node(int left, int right, DataType const& val) : 
            val(val), left(left), right(right) { }
        DataType val;
        int left, right;
    };
    struct node : public base_node {
        node(int left, int right) :
            base_node(left, right), pending(), lazy(0) { }
        node(int left, int right, DataType const& val) : 
            base_node(left, right, val), pending(), lazy(0) { }
        DataType pending;
        bool lazy : 1;
    };

    using no_lazy_prop = std::is_same<ResolveFn,no_lazy_prop_tag>;
    template <typename F, typename G>
    using default_construct = std::conjunction<std::is_default_constructible<F>, std::is_default_constructible<G>>;
    template <typename It>
    using require_input_iterator = std::is_base_of<std::input_iterator_tag, typename std::iterator_traits<It>::iterator_category>;
    using node_type = std::conditional_t<no_lazy_prop::value, base_node, node>;

public:
    template<typename F = CombineFn, typename G = ResolveFn, 
             typename Require = typename std::enable_if_t<default_construct<F,G>::value>>
    segtree(size_t n)
        : _max_index(n-1), _combineFn(), _resolvefn() {
        _new_node();
    }
    segtree(size_t n, CombineFn combinefn, ResolveFn resolvefn = no_lazy_prop_tag())
        : _max_index(n-1), _combineFn(combinefn), _resolvefn(resolvefn) {
        _new_node();
    }
    template<typename It, typename F = CombineFn, typename G = ResolveFn,
             typename Require = typename std::enable_if_t<std::conjunction<default_construct<F,G>, require_input_iterator<It>>::value>>
    segtree(It first, It last)
        : _combineFn(), _resolvefn() {
        _max_index = std::distance(first, last) - 1;
        _new_node();
        _init_range(first);
    }
    template<typename It, 
             typename Require = typename std::enable_if_t<require_input_iterator<It>::value>>
    segtree(It first, It last, CombineFn combinefn, ResolveFn resolvefn = no_lazy_prop_tag())
        : _combineFn(combinefn), _resolvefn(resolvefn) {
        _max_index = std::distance(first, last) - 1;
        _new_node();
        _init_range(first);
    }
    template<typename F = CombineFn, typename G = ResolveFn, 
             typename Require = typename std::enable_if_t<default_construct<F,G>::value>>
    segtree(size_t n, DataType const& val)
        : _max_index(n-1), _combineFn(), _resolvefn() {
        _new_node();
        _init_fill(val);
    }
    segtree(size_t n, DataType const& val, CombineFn combinefn, ResolveFn resolvefn = no_lazy_prop_tag())
        : _max_index(n-1), _combineFn(combinefn), _resolvefn(resolvefn) {
        _new_node();
        _init_fill(val);
    }
    void update(int i, DataType const& val) {
        if constexpr (!no_lazy_prop::value) {
            _update(i, i, val, 0, 0, _max_index);
        } else {
            _update(i, val, 0, 0, _max_index);
        }
    }
    
    template<typename Require = typename std::enable_if<!no_lazy_prop::value>>
    void update(int l, int r, DataType const& val){
        _update(l, r, val, 0, 0, _max_index);
    }

    DataType queryall() {
        DataType ret;
        _query(ret, 0, _max_index, 0, 0, _max_index);
        return ret;
    }

    DataType query(int l, int r) {
        DataType ret;
        _query(ret, l, r, 0, 0, _max_index);
        return ret;
    }

    DataType operator[](int index) {
        DataType ret;
        _query(ret, index, index, 0, 0, _max_index);
        return ret;
    }

private:
    vector<node_type> _nodes;
    int _max_index;
    CombineFn _combineFn;
    ResolveFn _resolvefn;

    int _new_node() {
        _nodes.emplace_back(-1,-1);
        return _nodes.size()-1;
    }
    int _left(int i) {
        int ret = _nodes[i].left;
        if(ret == -1)
            ret = _nodes[i].left = _new_node();
        return ret;
    }
    int _right(int i) {
        int ret = _nodes[i].right;
        if(ret == -1)
            ret = _nodes[i].right = _new_node();
        return ret;
    }

    #define N(x) _nodes[x]
    #define L(x) _nodes[_left(x)]
    #define R(x) _nodes[_right(x)]

    template<typename It>
    void _init_range(It first) {
        auto access = [first](int index)->DataType {
            return *(first + index);
        };
        _build<decltype(access)>(0, 0, _max_index, access);
    }

    // fills the segtree with an init value val
    void _init_fill(DataType const& val) {
        if constexpr (no_lazy_prop::value) {
            auto access = [&val]([[maybe_unused]]int index)->DataType {
                return val;
            };
            _build<decltype(access)>(0, 0, _max_index, access);
        } else {
            update(0, _max_index, val);
        }
    }

    template<typename GetValueFn>
    void _build(int i, int l, int r, GetValueFn const& get) {
        if (l >= r) { 
            N(i).val = get(l);
            return; 
        }
        int m = (l + r)/2;
        _build(_left(i),l,m,get);
        _build(_right(i),m+1,r,get);
        N(i).val = _combineFn(L(i).val, R(i).val);
    }

    template<typename = typename std::enable_if<no_lazy_prop::value>>
    void _update(int idx, DataType const& val, int i, int l, int r) {
        if (l >= r) {
            if constexpr (CumulativeUpdate)
                N(i).val += val;
            else
                N(i).val = val;

            return;
        }
        
        int m = l + (r-l)/2;
        if(idx <= m)
            _update(idx,val,_left(i),l,m);
        else
            _update(idx,val,_right(i),m+1,r);
        
        N(i).val = _combineFn(L(i).val, R(i).val);
    }

    template<typename = typename std::enable_if<!no_lazy_prop::value>>
    void _update(int ql, int qr, DataType const& val, int i, int l, int r) {
        _resolve_update(i,l,r);

        if(ql > r || qr < l)
            return;
        else if(l >= ql && r <= qr) {
            _apply(i,l,r,val);
            return;
        }

        int m = l + (r-l)/2;
        _update(ql,qr,val,_left(i),l,m);
        _update(ql,qr,val,_right(i),m+1,r);
        
        N(i).val = _combineFn(L(i).val, R(i).val);
    }

    bool _query(DataType& ret, int ql, int qr, int i, int l, int r) {
        if constexpr (!no_lazy_prop::value)
            _resolve_update(i,l,r);

        if(ql > r || qr < l)
            return false;
        if(l >= ql && r <= qr)
            return ret = N(i).val, true;
        
        int m = l + (r-l)/2;

        DataType ltmp, rtmp;
        bool lsub = _query(ltmp, ql,qr,_left(i),l,m);
        bool rsub = _query(rtmp, ql,qr,_right(i),m+1,r);
        
        if(lsub && rsub)
            return ret = _combineFn(ltmp, rtmp), true;
        else if(lsub)
            return ret = ltmp, true;
        else if(rsub)
            return ret = rtmp, true;
        else
            return false;
    }

    // resolves update at node i
    template<typename = typename std::enable_if<!no_lazy_prop::value>>
    void _resolve_update(int i, int l, int r)
    {
        if(N(i).lazy)
        {
            _apply(i,l,r,N(i).pending);
            N(i).lazy = 0;
            N(i).pending = 0;
        }
    }
    template<typename = typename std::enable_if<!no_lazy_prop::value>>
    void _apply(int i, int l, int r, int val)
    {
        if constexpr (CumulativeUpdate)
            N(i).val += _resolvefn(l, r, val);
        else
            N(i).val = _resolvefn(l, r, val);

        if(l<r)
        {
            L(i).lazy = 1;
            R(i).lazy = 1;

            if constexpr (CumulativeUpdate) {
                L(i).pending += val;
                R(i).pending += val;
            } else {
                L(i).pending = val;
                R(i).pending = val;
            }
        }
    }
    #undef N
    #undef L
    #undef R
};

template<typename DataType>
struct sum_resolve {
    DataType operator()(int l,
                        int r,
                        DataType const& data) {
        return (r-l+1) * data;
    }
};
template<typename DataType>
struct replace_resolve {
    DataType operator()([[maybe_unused]] int l, 
                        [[maybe_unused]] int r, 
                        DataType const& data) {
        return data;
    }
};
template<typename DataType>
struct min_compose {
    DataType operator()(DataType const& data1, DataType const& data2) {
        return std::min(data1, data2);
    }
};
template<typename DataType>
struct max_compose {
    DataType operator()(DataType const& data1, DataType const& data2) {
        return std::max(data1, data2);
    }
};
template<typename DataType>
using min_lazysegtree = segtree<DataType, min_compose<DataType>, replace_resolve<DataType>>;
template<typename DataType>
using max_lazysegtree = segtree<DataType, max_compose<DataType>, replace_resolve<DataType>>;
template<typename DataType>
using sum_lazysegtree = segtree<DataType, std::plus<DataType>, sum_resolve<DataType>>;
template<typename DataType>
using min_segtree = segtree<DataType, min_compose<DataType>>;
template<typename DataType>
using max_segtree = segtree<DataType, max_compose<DataType>>;
template<typename DataType>
using sum_segtree = segtree<DataType, std::plus<DataType>>;