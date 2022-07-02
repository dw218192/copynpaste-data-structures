#pragma once

template<typename DataType>
struct circular_array
{
    DataType* _arr;
    size_t _size;
    explicit circular_array(size_t size) : _size(size) {
        _arr = new DataType[size];
    }
    circular_array(circular_array const& other) noexcept
        : _arr(new DataType[other._size]), _size(other._size) {
        memcpy(_arr, other._arr, _size * sizeof(DataType));
    }
    circular_array(circular_array&& other) noexcept
        : _arr(other._arr), _size(other._size) {
        other._arr = nullptr;
        other._size = 0;
    }
    circular_array& operator=(circular_array const& other) noexcept {
        if(this == &other)
            return *this;
        _arr = new DataType[other._size];
        _size = other._size;
        memcpy(_arr, other._arr, _size * sizeof(DataType));
    }
    circular_array& operator=(circular_array&& other) noexcept {
        if(this == &other)
            return *this;
        _arr = other._arr;
        _size = other._size;
        other._arr = nullptr;
        other._size = 0;
    }

    ~circular_array() {
        delete[] _arr;
    }
    DataType const& operator[](int index) const {
        if(index < 0)
            return _arr[(index % _size + _size) % _size];
        else if(static_cast<size_t>(index) >= _size)
            return _arr[index % _size];
        else
            return _arr[index];
    }
    DataType& operator[](int index) {
        return const_cast<DataType&>((*const_cast<circular_array const*>(this))[index]);
    }
};

template<typename DataType>
struct static_deque
{
	circular_array<DataType> arr;
	int head=0, tail=0;
	
	explicit static_deque(size_t size) : arr(size) { }

	size_t size() const { return static_cast<size_t>(tail-head); }
	bool empty() const { return head==tail; }
	DataType& back() { return arr[tail-1]; }
    DataType const& back() const { return arr[tail-1]; }
    DataType& front() { return arr[head]; }
    DataType const& front() const { return arr[head]; }
    DataType& operator[](int index) { return arr[head+index]; }
    DataType const& operator[](int index) const { return arr[head+index]; }
	void pop_front() { ++head; }
	void pop_back() { --tail; }
	void push_back(DataType const& val) { arr[tail++] = val; }
};