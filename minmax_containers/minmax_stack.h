#pragma once
#include "static_deque.h"
#include "tags.h"
#include <type_traits>

/**
 * @brief FILO satck with O(1) min and max operations
 * 
 * @tparam DataType   type of data, must implement operator<,>
 * @tparam Tag        min_tag: min() only; max_tag: max() only; minmax_tag: both are available
 * @tparam FixedSize  sets a maximum size for the queue, making it more efficient and enabling eviction
 */
template<typename DataType, typename Tag = min_tag, bool FixedSize = false>
class minmax_stack {
    struct dummy { dummy() { } dummy(int) { } };
    
    template<typename T>
    using use_min_stack = std::disjunction<std::is_same<T, min_tag>, std::is_same<T, minmax_tag>>;
    template<typename T>
    using use_max_stack = std::disjunction<std::is_same<T, max_tag>, std::is_same<T, minmax_tag>>;
    using stack_type = std::conditional_t<FixedSize, static_deque<DataType>, std::vector<DataType>>;
    using mono_stack_type = std::conditional_t<FixedSize, static_deque<size_t>, std::vector<size_t>>;
public:
    template<bool Enable = FixedSize, typename Require = std::enable_if_t<Enable>>
    explicit minmax_stack(size_t n) : _size(n), _elements(n), _minst(n), _maxst(n) { }
    template<bool Enable = !FixedSize, typename Require = std::enable_if_t<Enable>>
    minmax_stack() { }

    void push(DataType const& val) {
        _elements.push_back(val);
        if constexpr (use_min_stack<Tag>::value) {
            if(_minst.empty() || !(_elements[_minst.back()] < val))
                _minst.push_back(_elements.size()-1);
        }
        if constexpr (use_max_stack<Tag>::value) {
            if(_maxst.empty() || !(_elements[_maxst.back()] > val))
                _maxst.push_back(_elements.size()-1);
        }
    }
    void pop() {
        _elements.pop_back();
        if constexpr (use_min_stack<Tag>::value) {
            if(_minst.back() == _elements.size())
                _minst.pop_back();
        }
        if constexpr (use_max_stack<Tag>::value) {
            if(_maxst.back() == _elements.size())
                _maxst.pop_back();
        }
    }
    template<typename T = Tag, typename Require = std::enable_if_t<use_max_stack<T>::value>>
    DataType const& max() const {
        return _elements[_maxst.back()];
    }
    template<typename T = Tag, typename Require = std::enable_if_t<use_min_stack<T>::value>>
    DataType const& min() const {
        return _elements[_minst.back()];
    }
    size_t size() const {
        return _elements.size();
    }
    DataType const& front() const {
        return _elements.front();
    }
    DataType const& back() const {
        return _elements.back();
    }
    DataType const& operator[](int index) const {
        return _elements[index];
    }
private:
    std::conditional_t<FixedSize, size_t, dummy> _size;
    stack_type _elements;
    std::conditional_t<use_min_stack<Tag>::value, mono_stack_type, dummy> _minst;
    std::conditional_t<use_max_stack<Tag>::value, mono_stack_type, dummy> _maxst;
};