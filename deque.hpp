#pragma once

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

  Deque(size_t count);

  Deque(size_t count, const T& value);

  Deque(const Deque<T>& other);

  ~Deque();

  Deque<T>& operator=(const Deque& other);

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

  iterator end();
  const_iterator end() const;
  const_iterator cend() const;

  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  T& operator[](size_t idx);
  const T& operator[](size_t idx) const;

  T& at(size_t idx);
  const T& at(size_t idx) const;

  void push_back(const T& value);
  void push_front(const T& value);

  void pop_back();
  void pop_front();

  void clear();

  iterator insert(iterator pos, const T& value);

  iterator erase(iterator pos);

 private:
  void scale(size_t new_buckets_count);

  void set_null();

  void set_first(const T& value);

  static const size_t kBucketSize = 8;
  T** data_{nullptr};
  size_t size_{0};
  size_t buckets_{0};
  size_t first_{0};
  size_t last_{0};
  size_t front_{0};
  size_t back_{0};

  iterator begin_;
  iterator end_;
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

  Iterator& operator++();

  Iterator operator++(int);

  Iterator& operator--();

  Iterator operator--(int);

  Iterator operator+(difference_type value);

  Iterator operator+(difference_type value) const;

  Iterator operator-(difference_type value);

  Iterator operator-(difference_type value) const;

  Iterator& operator+=(difference_type value);

  Iterator& operator-=(difference_type value);

  bool operator==(const Iterator& other) const = default;
  auto operator<=>(const Iterator& other) const = default;

  difference_type operator-(const Iterator& other) const;

  reference operator*() const { return data_[bucket_][elem_]; }
  pointer operator->() const { return data_[bucket_] + elem_; }

  operator Iterator<true>() const {
    return Iterator<true>(data_, bucket_, elem_);
  }

 private:
  static const size_t kBucketSize = 8;
  storage_pointer data_;
  size_t bucket_;
  size_t elem_;
};

// Deque

template <typename T>
Deque<T>::Deque(size_t count) : Deque(count, T()) {}

template <typename T>
Deque<T>::Deque(size_t count, const T& value) {
  if (count == 0) {
    return;
  }
  size_t new_cap = ((count - 1) / kBucketSize) + 1;
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
  buckets_ = (count - 1) / kBucketSize + 1;
  back_ = (count - 1) % kBucketSize;
  last_ = buckets_ - 1;
  size_ = count;
}

template <typename T>
Deque<T>::Deque(const Deque<T>& other)
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

template <typename T>
Deque<T>::~Deque() {
  clear();
  delete[] data_;
}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& other) {
  if (&other == this) {
    return *this;
  }
  Deque<T> copy(other);
  std::swap(data_, copy.data_);
  std::swap(buckets_, copy.buckets_);
  std::swap(size_, copy.size_);
  std::swap(first_, copy.first_);
  std::swap(last_, copy.last_);
  std::swap(front_, copy.front_);
  std::swap(back_, copy.back_);
  return *this;
}

template <typename T>
Deque<T>::iterator Deque<T>::end() {
  if (data_ == nullptr) {
    return iterator(data_, 0, 0);
  }
  if (back_ + 1 == kBucketSize) {
    return iterator(data_, last_ + 1, 0);
  }
  return iterator(data_, last_, back_ + 1);
}

template <typename T>
Deque<T>::const_iterator Deque<T>::end() const {
  if (data_ == nullptr) {
    return const_iterator(data_, 0, 0);
  }
  if (back_ + 1 == kBucketSize) {
    return const_iterator(data_, last_ + 1, 0);
  }
  return const_iterator(data_, last_, back_ + 1);
}

template <typename T>
Deque<T>::const_iterator Deque<T>::cend() const {
  if (data_ == nullptr) {
    return const_iterator(data_, 0, 0);
  }
  if (back_ + 1 == kBucketSize) {
    return const_iterator(data_, last_ + 1, 0);
  }
  return const_iterator(data_, last_, back_ + 1);
}

template <typename T>
T& Deque<T>::operator[](size_t idx) {
  size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
  size_t element_idx = (front_ + idx) % kBucketSize;
  return data_[bucket_idx][element_idx];
}

template <typename T>
const T& Deque<T>::operator[](size_t idx) const {
  size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
  size_t element_idx = (front_ + idx) % kBucketSize;
  return data_[bucket_idx][element_idx];
}

template <typename T>
T& Deque<T>::at(size_t idx) {
  if (idx >= size_) {
    throw std::out_of_range("out of range");
  }
  size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
  size_t element_idx = (front_ + idx) % kBucketSize;
  return data_[bucket_idx][element_idx];
}

template <typename T>
const T& Deque<T>::at(size_t idx) const {
  if (idx >= size_) {
    throw std::out_of_range("out of range");
  }
  size_t bucket_idx = first_ + (front_ + idx) / kBucketSize;
  size_t element_idx = (front_ + idx) % kBucketSize;
  return data_[bucket_idx][element_idx];
}

template <typename T>
void Deque<T>::push_back(const T& value) {
  if (data_ == nullptr) {
    set_first(value);
    return;
  }
  ++back_;
  if (back_ == kBucketSize) {
    back_ = 0;
    ++last_;
  }
  if (last_ == buckets_) {
    size_t new_buckets_count = buckets_ * 2 + 1;
    scale(new_buckets_count);
    first_ += (new_buckets_count - buckets_) / 2;
    last_ += (new_buckets_count - buckets_) / 2;
    buckets_ = new_buckets_count;
  }
  try {
    data_[last_][back_] = T(value);
  } catch (...) {
    if (back_ == 0) {
      back_ = kBucketSize - 1;
      --last_;
    } else {
      --back_;
    }
    throw;
  }
  ++size_;
}

template <typename T>
void Deque<T>::push_front(const T& value) {
  if (data_ == nullptr) {
    set_first(value);
    return;
  }
  if (front_ == 0) {
    if (first_ == 0) {
      size_t new_buckets_count = buckets_ * 2 + 1;
      scale(new_buckets_count);
      first_ += (new_buckets_count - buckets_) / 2;
      last_ += first_;
      buckets_ = new_buckets_count;
    }
    --first_;
    front_ = kBucketSize - 1;
  } else {
    --front_;
  }
  try {
    data_[first_][front_] = T(value);
  } catch (...) {
    if (front_ == kBucketSize - 1) {
      front_ = 0;
      ++first_;
    } else {
      ++front_;
    }
    throw;
  }
  ++size_;
}

template <typename T>
void Deque<T>::pop_back() {
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

template <typename T>
void Deque<T>::pop_front() {
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

template <typename T>
void Deque<T>::clear() {
  if (data_ == nullptr) {
    return;
  }
  for (size_t idx = 0; idx < buckets_; ++idx) {
    delete[] data_[idx];
  }
  set_null();
}

template <typename T>
Deque<T>::iterator Deque<T>::insert(Deque::iterator pos, const T& value) {
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

template <typename T>
Deque<T>::iterator Deque<T>::erase(Deque::iterator pos) {
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

template <typename T>
void Deque<T>::scale(size_t new_buckets_count) {
  if (new_buckets_count < buckets_ + 1) {
    return;
  }
  T** new_data = new T*[new_buckets_count];
  try {
    for (size_t idx = 0; idx < (new_buckets_count - buckets_) / 2; ++idx) {
      new_data[idx] =
          reinterpret_cast<T*>(operator new[](kBucketSize * sizeof(T)));
    }
  } catch (...) {
    for (size_t idx = 0; idx < (new_buckets_count - buckets_) / 2; ++idx) {
      delete[] new_data[idx];
    }
    delete[] new_data;
    throw;
  }
  try {
    for (size_t jdx = 0; jdx < buckets_; ++jdx) {
      new_data[(new_buckets_count - buckets_) / 2 + jdx] = data_[jdx];
    }
  } catch (...) {
    for (size_t jdx = 0; jdx < buckets_; ++jdx) {
      delete[] new_data[(new_buckets_count - buckets_) / 2 + jdx];
    }
    delete[] new_data;
    throw;
  }
  try {
    for (size_t idx = (new_buckets_count - buckets_) / 2 + buckets_;
         idx < new_buckets_count; ++idx) {
      new_data[idx] =
          reinterpret_cast<T*>(operator new[](kBucketSize * sizeof(T)));
    }
  } catch (...) {
    for (size_t idx = (new_buckets_count - buckets_) / 2 + buckets_;
         idx < new_buckets_count; ++idx) {
      delete[] new_data[idx];
    }
    delete[] new_data;
    throw;
  }
  delete[] data_;
  data_ = new_data;
  begin_ = begin();
  end_ = end();
}

template <typename T>
void Deque<T>::set_null() {
  size_ = 0;
  buckets_ = 0;
  first_ = 0;
  last_ = 0;
  front_ = 0;
  back_ = 0;
}

template <typename T>
void Deque<T>::set_first(const T& value) {
  scale(3);
  data_[1][0] = value;
  buckets_ = 3;
  first_ = 1;
  last_ = 1;
  size_ = 1;
}

// Iterator

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>&
Deque<T>::Iterator<IsConst>::operator++() {
  ++elem_;
  if (elem_ == kBucketSize) {
    elem_ = 0;
    ++bucket_;
  }
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator++(int) {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>&
Deque<T>::Iterator<IsConst>::operator--() {
  if (elem_ == 0) {
    elem_ = kBucketSize - 1;
    --bucket_;
  } else {
    --elem_;
  }
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator--(int) {
  auto copy = *this;
  --(*this);
  return copy;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator+(
    Deque::Iterator<IsConst>::difference_type value) {
  Iterator temp = *this;
  temp += value;
  return temp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator+(
    Deque::Iterator<IsConst>::difference_type value) const {
  Iterator temp = *this;
  temp += value;
  return temp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator-(
    Deque::Iterator<IsConst>::difference_type value) {
  Iterator temp = *this;
  temp -= value;
  return temp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>
Deque<T>::Iterator<IsConst>::operator-(
    Deque::Iterator<IsConst>::difference_type value) const {
  Iterator temp = *this;
  temp -= value;
  return temp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>&
Deque<T>::Iterator<IsConst>::operator+=(
    Deque::Iterator<IsConst>::difference_type value) {
  if (value < 0) {
    return operator-=(-value);
  }
  bucket_ += (elem_ + value) / kBucketSize;
  elem_ = (elem_ + value) % kBucketSize;
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>&
Deque<T>::Iterator<IsConst>::operator-=(
    Deque::Iterator<IsConst>::difference_type value) {
  if (value < 0) {
    return operator+=(-value);
  }
  if (static_cast<size_t>(value) < elem_ + 1) {
    elem_ -= value;
  } else {
    value -= elem_ + 1;
    bucket_ -= 1 + value / kBucketSize;
    elem_ = kBucketSize - 1 - value % kBucketSize;
  }
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template Iterator<IsConst>::difference_type
Deque<T>::Iterator<IsConst>::operator-(const Iterator<IsConst>& other) const {
  if (bucket_ == other.bucket_) {
    return elem_ - other.elem_;
  }
  auto bucket_dif = bucket_ - other.bucket_ - 1;
  auto elem_dif = kBucketSize - other.elem_ + elem_;
  return bucket_dif * kBucketSize + elem_dif;
}