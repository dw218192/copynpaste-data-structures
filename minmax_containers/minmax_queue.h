#pragma once
#include "static_deque.h"
#include "tags.h"
#include <type_traits>

/**
 * @brief FIFO queue with O(1) min and max operations
 * 
 * @tparam DataType   type of data, must implement operator<,==,>
 * @tparam Tag        min_tag: min() only; max_tag: max() only; minmax_tag: both are available
 * @tparam FixedSize  sets a maximum size for the queue, making it more efficient and enabling eviction
 */
template <typename DataType, typename Tag = min_tag, bool FixedSize = false>
class minmax_queue {
    struct dummy { dummy() { } dummy(int) { } };

    template<typename T>
    using use_min_queue = std::disjunction<std::is_same<T, min_tag>, std::is_same<T, minmax_tag>>;
    template<typename T>
    using use_max_queue = std::disjunction<std::is_same<T, max_tag>, std::is_same<T, minmax_tag>>;
    using deque_type = std::conditional_t<FixedSize, static_deque<DataType>, std::deque<DataType>>;
public:
    template<bool Enable = FixedSize, typename Require = std::enable_if_t<Enable>>
    explicit minmax_queue(size_t n) : _size(n), _elements(n), _minq(n), _maxq(n) { }
    template<bool Enable = !FixedSize, typename Require = std::enable_if_t<Enable>>
    minmax_queue() { }
    
    void push(DataType const& val) {
        // eviction if fixed size
        if constexpr (FixedSize) {
            if(!_size) {
                return;
            } else if(_elements.size() == _size) {
                pop();
            }
        }

		// the _maxq is monotonically non-increasing
        if constexpr (use_max_queue<Tag>::value) {
            while(!_maxq.empty() && _maxq.back() < val)
                _maxq.pop_back();
            _maxq.push_back(val);
        }
        if constexpr (use_min_queue<Tag>::value) {
            // the min_q is monotonically non-decreasing
            while(!_minq.empty() && _minq.back() > val)
                _minq.pop_back();
            _minq.push_back(val);
        }

        _elements.push_back(val);
	}
    void pop() {
        if constexpr (use_max_queue<Tag>::value) {
            if(!_maxq.empty() && _maxq.front() == _elements.front())
                _maxq.pop_front();
        }
        if constexpr (use_min_queue<Tag>::value) {
            if(!_minq.empty() && _minq.front() == _elements.front())
                _minq.pop_front();
        }
        _elements.pop_front();
    }
    template<typename T = Tag, typename Require = std::enable_if_t<use_max_queue<T>::value>>
    DataType const& max() const {
		return _maxq.front();
	}
    template<typename T = Tag, typename Require = std::enable_if_t<use_min_queue<T>::value>>
	DataType const& min() const {
		return _minq.front();
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
    deque_type _elements;
    std::conditional_t<use_min_queue<Tag>::value, deque_type, dummy> _minq;
    std::conditional_t<use_max_queue<Tag>::value, deque_type, dummy> _maxq;
};