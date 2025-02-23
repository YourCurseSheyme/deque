//
// Created by sheyme on 22/02/25.
//
#include <iterator>

template <typename T>
class Deque {
 private:
  template <bool IsConst>
  class Iterator;

 public:
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  Deque() = default;

  Deque(size_t count) {
    if (count == 0) {
      return;
    }
    scale((count - 1) / kBucketSize + 1);
    buckets_ = (count - 1) / kBucketSize + 1;
    back_ = (count - 1) % kBucketSize;
    last_ = buckets_ - 1;
    size_ = count;
    size_t jdx = front_;
    size_t number = 0;
    try {
      for (size_t idx = first_; idx < last_ + 1; ++idx) {
        while (jdx != kBucketSize) {
          if (number == size_) {
            return;
          }
          data_[idx][jdx] = T();
          ++jdx;
        }
        jdx = 0;
      }
    } catch (...) {
      clear();
      delete[] data_;
      throw;
    }
  }

  Deque(size_t count, const T& value) {
    if (count == 0) {
      return;
    }
    init((count - 1) / kBucketSize + 1, value);
    buckets_ = (count - 1) / kBucketSize + 1;
    back_ = (count - 1) % kBucketSize;
    last_ = buckets_ - 1;
    size_ = count;
  }

  Deque(const Deque<T>& other)
      : first_(other.first_),
        last_(other.last_),
        front_(other.front_),
        back_(other.back_),
        size_(other.size_) {
    if (other.data_ == nullptr) {
      return;
    }
    scale(other.buckets_);
    buckets_ = other.buckets_;
    size_t jdx = front_;
    try {
      for (size_t idx = first_; idx < last_ + 1; ++idx) {
        while (jdx != kBucketSize) {
          data_[idx][jdx] = other.data_[idx][jdx];
          ++jdx;
        }
        jdx = 0;
      }
    } catch (...) {
      clear();
      delete[] data_;
      throw;
    }
  }

  ~Deque() {
    clear();
    delete[] data_;
  }

  Deque<T>& operator=(const Deque& other) {
    if (&other == this) {
      return *this;
    }
    clear();
    scale(other.buckets_);
    size_t jdx = other.front_;
    try {
      for (size_t idx = other.first_; idx < other.last_ + 1; ++idx) {
        while (jdx != kBucketSize) {
          data_[idx][jdx] = other.data_[idx][jdx];
          ++jdx;
        }
        jdx = 0;
      }
    } catch (...) {
      clear();
      delete[] data_;
      throw;
    }
    buckets_ = other.buckets_;
    size_ = other.size_;
    first_ = other.first_;
    last_ = other.last_;
    front_ = other.front_;
    back_ = other.back_;
    return *this;
  }

  iterator begin() { return iterator(data_, first_, front_); }
  const_iterator begin() const { return const_iterator(data_, first_, front_); }
  const_iterator cbegin() const {
    return const_iterator(data_, first_, front_);
  }

  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return std::make_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }

  iterator end() {
    if (data_ == nullptr) {
      return iterator(data_, 0, 0);
    }
    if (back_ + 1 == kBucketSize) {
      return iterator(data_, last_ + 1, 0);
    }
    return iterator(data_, last_, back_ + 1);
  }
  const_iterator end() const {
    if (data_ == nullptr) {
      return const_iterator(data_, 0, 0);
    }
    if (back_ + 1 == kBucketSize) {
      return const_iterator(data_, last_ + 1, 0);
    }
    return const_iterator(data_, last_, back_ + 1);
  }
  const_iterator cend() const {
    if (data_ == nullptr) {
      return const_iterator(data_, 0, 0);
    }
    if (back_ + 1 == kBucketSize) {
      return const_iterator(data_, last_ + 1, 0);
    }
    return const_iterator(data_, last_, back_ + 1);
  }

  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  T& operator[](size_t idx) {
    size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
    size_t element_idx = (front_ + idx) % kBucketSize;
    return data_[bucket_idx][element_idx];
  }
  const T& operator[](size_t idx) const {
    size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
    size_t element_idx = (front_ + idx) % kBucketSize;
    return data_[bucket_idx][element_idx];
  }

  T& at(size_t idx) {
    if (idx >= size_) {
      throw std::out_of_range("out of range");
    }
    size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
    size_t element_idx = (front_ + idx) % kBucketSize;
    return data_[bucket_idx][element_idx];
  }
  const T& at(size_t idx) const {
    if (idx >= size_) {
      throw std::out_of_range("out of range");
    }
    size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
    size_t element_idx = (front_ + idx) % kBucketSize;
    return data_[bucket_idx][element_idx];
  }

  void push_back(const T& value) {
    if (data_ == nullptr) {
      set_first(value);
      return;
    }
    ++size_;
    ++back_;
    if (back_ == kBucketSize) {
      back_ = 0;
      ++last_;
    }
    if (last_ == buckets_) {
      size_t new_cap = buckets_ * 2 + 1;
      scale(new_cap);
      first_ += (new_cap - buckets_) / 2;
      last_ += (new_cap - buckets_) / 2;
      buckets_ = new_cap;
    }
    try {
      data_[last_][back_] = T(value);
    } catch (...) {
      --size_;
      if (back_ == 0) {
        back_ = kBucketSize - 1;
        --last_;
      } else {
        --back_;
      }
      throw;
    }
  }
  void push_front(const T& value) {
    if (data_ == nullptr) {
      set_first(value);
      return;
    }
    ++size_;
    if (front_ == 0) {
      if (first_ == 0) {
        size_t new_cap = buckets_ * 2 + 1;
        scale(new_cap);
        first_ += (new_cap - buckets_) / 2;
        last_ += first_;
        buckets_ = new_cap;
      }
      --first_;
      front_ = kBucketSize - 1;
    } else {
      --front_;
    }
    try {
      data_[first_][front_] = T(value);
    } catch (...) {
      --size_;
      if (front_ == kBucketSize - 1) {
        front_ = 0;
        ++first_;
      } else {
        ++front_;
      }
      throw;
    }
  }

  void pop_back() {
    if (size_ == 0) {
      return;
    }
    --size_;
    if (back_ == 0) {
      back_ = kBucketSize - 1;
      --last_;
    } else {
      --back_;
    }
  }
  void pop_front() {
    if (size_ == 0) {
      return;
    }
    --size_;
    ++front_;
    if (front_ == kBucketSize) {
      front_ = 0;
      ++first_;
    }
  }

  void clear() {
    if (data_ == nullptr) {
      return;
    }
    for (size_t idx = 0; idx < buckets_; ++idx) {
      delete[] data_[idx];
    }
    set_null();
  }

  iterator insert(iterator pos, const T& value) {
    if (pos == begin()) {
      push_front(value);
      return begin();
    }
    if (pos == end()) {
      push_back(value);
      return end() - 1;
    }
    size_t edge = pos - begin();
    push_back(value);
    for (size_t idx = size_ - 1; idx > edge; --idx) {
      std::swap(operator[](idx), operator[](idx - 1));
    }
    return pos;
  }

  iterator erase(iterator pos) {
    if (pos == end()) {
      throw;
    }
    auto next = pos + 1;
    for (auto iter = pos; iter != end() - 1; ++iter) {
      std::swap(*iter, *(iter + 1));
    }
    pop_back();
    return next;
  }

 private:
  void scale(size_t new_cap) {
    if (new_cap < buckets_ + 1) {
      return;
    }
    T** new_data = new T*[new_cap];
    try {
      for (size_t idx = 0; idx < (new_cap - buckets_) / 2; ++idx) {
        new_data[idx] =
            static_cast<T*>(operator new[](kBucketSize * sizeof(T)));
      }
    } catch (...) {
      for (size_t idx = 0; idx < (new_cap - buckets_) / 2; ++idx) {
        delete[] new_data[idx];
      }
      delete[] new_data;
      throw;
    }
    try {
      for (size_t jdx = 0; jdx < buckets_; ++jdx) {
        new_data[(new_cap - buckets_) / 2 + jdx] = data_[jdx];
      }
    } catch (...) {
      for (size_t jdx = 0; jdx < buckets_; ++jdx) {
        delete[] new_data[(new_cap - buckets_) / 2 + jdx];
      }
      delete[] new_data;
      throw;
    }
    try {
      for (size_t idx = (new_cap - buckets_) / 2 + buckets_; idx < new_cap;
           ++idx) {
        new_data[idx] =
            static_cast<T*>(operator new[](kBucketSize * sizeof(T)));
      }
    } catch (...) {
      for (size_t idx = (new_cap - buckets_) / 2 + buckets_; idx < new_cap;
           ++idx) {
        delete[] new_data[idx];
      }
      delete[] new_data;
      throw;
    }
    delete[] data_;
    data_ = new_data;
  }

  void init(size_t new_cap, const T& value) {
    scale(new_cap);
    auto cleanup = [this, new_cap]() {
      for (size_t idx = 0; idx < new_cap; ++idx) {
        if (data_[idx]) {
          for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
            data_[idx][kdx].~T();
          }
          operator delete[](data_[idx]);
        }
      }
      delete[] data_;
    };
    try {
      for (size_t idx = 0; idx < (new_cap - buckets_) / 2; ++idx) {
        for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
          new (&data_[idx][kdx]) T(value);
        }
      }
    } catch (...) {
      cleanup();
      throw;
    }
    try {
      for (size_t idx = (new_cap - buckets_) / 2 + buckets_; idx < new_cap;
           ++idx) {
        for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
          new (&data_[idx][kdx]) T(value);
        }
      }
    } catch (...) {
      cleanup();
      throw;
    }
  }

  void set_null() {
    size_ = 0;
    buckets_ = 0;
    first_ = 0;
    last_ = 0;
    front_ = 0;
    back_ = 0;
  }

  void set_first(const T& value) {
    scale(3);
    data_[1][0] = value;
    buckets_ = 3;
    first_ = 1;
    last_ = 1;
    size_ = 1;
  }

  static const size_t kBucketSize = 8;
  T** data_{nullptr};
  size_t size_{0};
  size_t buckets_{0};
  size_t first_{0};
  size_t last_{0};
  size_t front_{0};
  size_t back_{0};
};

template <typename T>
template <bool IsConst>
class Deque<T>::Iterator {
 public:
  using value_type = std::conditional_t<IsConst, const T, T>;
  using storage_pointer =
      std::conditional_t<IsConst, const value_type* const*, value_type**>;
  using pointer = value_type*;
  using reference = value_type&;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::contiguous_iterator_tag;

  Iterator(storage_pointer data, size_t bucket, size_t elem)
      : data_(data), bucket_(bucket), elem_(elem) {}

  Iterator& operator++() {
    ++elem_;
    if (elem_ == kBucketSize) {
      elem_ = 0;
      ++bucket_;
    }
    return *this;
  }

  Iterator operator++(int) {
    auto copy = *this;
    ++elem_;
    if (elem_ == kBucketSize) {
      elem_ = 0;
      ++bucket_;
    }
    return copy;
  }

  Iterator& operator--() {
    if (elem_ == 0) {
      elem_ = kBucketSize - 1;
      --bucket_;
    } else {
      --elem_;
    }
    return *this;
  }

  Iterator operator--(int) {
    auto copy = *this;
    if (elem_ == 0) {
      elem_ = kBucketSize - 1;
      --bucket_;
    } else {
      --elem_;
    }
    return copy;
  }

  Iterator operator+(difference_type value) {
    Iterator temp = *this;
    temp += value;
    return temp;
  }

  Iterator operator+(difference_type value) const {
    Iterator temp = *this;
    temp += value;
    return temp;
  }

  Iterator operator-(difference_type value) {
    Iterator temp = *this;
    temp -= value;
    return temp;
  }

  Iterator operator-(difference_type value) const {
    Iterator temp = *this;
    temp -= value;
    return temp;
  }

  Iterator& operator+=(difference_type value) {
    while (value != 0) {
      ++(*this);
      --value;
    }
    return *this;
  }

  Iterator& operator-=(difference_type value) {
    while (value != 0) {
      --(*this);
      --value;
    }
    return *this;
  }

  bool operator==(const Iterator& other) const = default;
  auto operator<=>(const Iterator& other) const = default;

  difference_type operator-(const Iterator& other) const {
    if (bucket_ == other.bucket_) {
      return elem_ - other.elem_;
    }
    auto bucket_dif = bucket_ - other.bucket_ - 1;
    auto elem_dif = kBucketSize - other.elem_ + elem_;
    return bucket_dif * kBucketSize + elem_dif;
  }

  const value_type& operator*() const { return (*(data_ + bucket_))[elem_]; }

  reference operator*() { return (*(data_ + bucket_))[elem_]; }

  pointer operator->() { return &(*(data_ + bucket_))[elem_]; }

  const value_type* operator->() const { return &(*(data_ + bucket_))[elem_]; }

  operator Iterator<true>() const {
    return Iterator<true>(data_, bucket_, elem_);
  }

 private:
  static const size_t kBucketSize = 8;
  storage_pointer data_;
  size_t bucket_;
  size_t elem_;
};
