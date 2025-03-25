#pragma once

#include <cstring>
#include <iterator>

template <typename T, typename Allocator = std::allocator<T>>
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

  Deque(const Allocator& alloc);

  Deque(size_t count, const Allocator& alloc = Allocator());
  Deque(size_t count, const T& value, const Allocator& alloc = Allocator());

  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator());

  Deque(const Deque& other);
  Deque(Deque&& other) noexcept;

  ~Deque();

  Deque& operator=(const Deque& other);
  Deque& operator=(Deque<T, Allocator>&& other) noexcept;

  iterator begin() { return begin_; }
  const_iterator begin() const { return begin_; }
  const_iterator cbegin() const { return begin_; }

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

  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] bool empty() const { return size_ == 0; }
  [[nodiscard]] Allocator get_allocator() const { return alloc_; }

  T& operator[](size_t idx);
  const T& operator[](size_t idx) const;

  T& at(size_t idx);
  const T& at(size_t idx) const;

  template <typename... Args>
  void emplace_back(Args&&... args);

  template <typename... Args>
  void emplace_front(Args&&... args);

  void push_back(const T& value);
  void push_back(T&& value);
  void push_front(const T& value);
  void push_front(T&& value);

  void pop_back();
  void pop_front();

  void clear();

  iterator insert(iterator pos, const T& value);

  iterator erase(iterator pos);

 private:
  using alloc = Allocator;
  using alloc_traits = std::allocator_traits<Allocator>;
  using bucket_alloc = typename alloc_traits::template rebind_alloc<T*>;
  using bucket_alloc_traits = typename alloc_traits::template rebind_traits<T*>;

  void scale(size_t new_buckets_count);
  template <typename... Args>
  void init(size_t count, Args&&... args);
  template <typename... Args>
  void init_particularly(size_t count, size_t start, size_t end,
                         Args&&... args);
  void clear_particularly(size_t count, size_t start, size_t end);

  void set_null();

  template <typename... Args>
  void set_first(Args&&... value);

  static const size_t kBucketSize = 8;
  T** data_{nullptr};
  size_t size_{0};
  size_t buckets_{0};
  iterator begin_ = Iterator<false>(data_, 0, 0);
  iterator end_ = Iterator<false>(data_, 0, 0);

  [[no_unique_address]] alloc alloc_;
  [[no_unique_address]] bucket_alloc bucket_alloc_;
};

template <typename T, typename Allocator>
template <bool IsConst>
class Deque<T, Allocator>::Iterator {
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
  friend Deque<T, Allocator>;
  void fix(storage_pointer new_data, size_t diff) {
    data_ = new_data;
    bucket_ += diff;
  }

  static const size_t kBucketSize = 8;
  storage_pointer data_;
  size_t bucket_;
  size_t elem_;
};

// Deque

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Allocator& alloc) : alloc_(alloc) {}

template <typename T, typename Allocator>
void Deque<T, Allocator>::clear_particularly(size_t count, size_t start,
                                             size_t end) {
  size_t new_cap = ((count - 1) / kBucketSize) + 1;
  size_t deleted_num = 0;
  for (size_t idx = 0; idx < new_cap; ++idx) {
    if (idx >= start && idx < end) {
      for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
        if (deleted_num == size_) {
          break;
        }
        alloc_traits::destroy(alloc_, &data_[idx][kdx]);
        ++deleted_num;
      }
    }
    alloc_traits::deallocate(alloc_, data_[idx], kBucketSize);
  }
  bucket_alloc_traits::deallocate(bucket_alloc_, data_, new_cap);
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::init_particularly(size_t count, size_t start,
                                            size_t end, Args&&... args) {
  size_t new_cap = ((count - 1) / kBucketSize) + 1;
  try {
    for (size_t idx = start; idx < end; ++idx) {
      for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
        if (size_ == count) {
          break;
        }
        alloc_traits::construct(alloc_, &data_[idx][kdx],
                                std::forward<Args>(args)...);
        ++size_;
      }
    }
  } catch (...) {
    clear_particularly(count, start, end);
    throw;
  }
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::init(size_t count, Args&&... args) {
  if (count == 0) {
    return;
  }
  size_t new_cap = ((count - 1) / kBucketSize) + 1;
  scale(new_cap);
  init_particularly(count, 0, (new_cap - buckets_) / 2,
                    std::forward<Args>(args)...);
  size_t constructed_num = 0;
  try {
    for (size_t idx = ((new_cap - buckets_) / 2) + buckets_; idx < new_cap;
         ++idx) {
      for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
        if (size_ == count) {
          break;
        }
        alloc_traits::construct(alloc_, &data_[idx][kdx],
                                std::forward<Args>(args)...);
        ++size_;
        ++constructed_num;
      }
    }
  } catch (...) {
    size_t deleted_num = 0;
    for (size_t idx = ((new_cap - buckets_) / 2) + buckets_; idx < new_cap;
         ++idx) {
      for (size_t kdx = 0; kdx < kBucketSize; ++kdx) {
        if (deleted_num == constructed_num) {
          break;
        }
        alloc_traits::destroy(alloc_, &data_[idx][kdx]);
        ++deleted_num;
      }
    }
    clear_particularly(count, 0, (new_cap - buckets_) / 2);
    throw;
  }
  buckets_ = (count - 1) / kBucketSize + 1;
  begin_ = Iterator<false>(data_, 0, 0);
  end_ = Iterator<false>(data_, buckets_ - 1, (count - 1) % kBucketSize) + 1;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const Allocator& alloc)
    : alloc_(alloc) {
  init(count);
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(size_t count, const T& value, const Allocator& alloc)
    : alloc_(alloc) {
  init(count, value);
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(const Deque& other) : size_(other.size_) {
  if (other.data_ == nullptr) {
    return;
  }
  alloc_ = alloc_traits::select_on_container_copy_construction(other.alloc_);
  bucket_alloc_ = bucket_alloc_traits::select_on_container_copy_construction(
      other.bucket_alloc_);
  scale(other.buckets_);
  buckets_ = other.buckets_;
  begin_ = Iterator<false>(data_, 0, 0);
  end_ = begin_;
  try {
    for (auto iter = other.begin_; iter != other.end_; ++iter) {
      alloc_traits::construct(alloc_, &*end_, *iter);
      ++end_;
    }
  } catch (...) {
    for (auto iter = begin_; iter != end_; ++iter) {
      alloc_traits::destroy(alloc_, &*iter);
    }
    for (size_t kdx = 0; kdx < buckets_; ++kdx) {
      alloc_traits::deallocate(alloc_, data_[kdx], kBucketSize);
    }
    bucket_alloc_traits::deallocate(bucket_alloc_, data_, buckets_);
    throw;
  }
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(Deque&& other) noexcept
    : data_(other.data_),
      size_(other.size_),
      buckets_(other.buckets_),
      begin_(other.begin_),
      end_(other.end_) {
  other.set_null();
  other.data_ = nullptr;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::Deque(std::initializer_list<T> init,
                           const Allocator& alloc)
    : alloc_(alloc), size_(init.size()) {
  if (init.size() == 0) {
    return;
  }
  scale(((init.size() - 1) / kBucketSize) + 1);
  buckets_ = ((init.size() - 1) / kBucketSize) + 1;
  begin_ = Iterator<false>(data_, 0, 0);
  auto iter = begin_;
  try {
    for (auto init_iter = init.begin(); init_iter != init.end(); ++init_iter) {
      alloc_traits::construct(alloc_, &*iter, std::move(*init_iter));
      ++iter;
    }
  } catch (...) {
    clear();
    bucket_alloc_traits::deallocate(bucket_alloc_, data_, buckets_);
    throw;
  }
  end_ = iter;
}

template <typename T, typename Allocator>
Deque<T, Allocator>::~Deque() {
  clear();
  bucket_alloc_traits::deallocate(bucket_alloc_, data_, buckets_);
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(const Deque& other) {
  if (&other == this) {
    return *this;
  }
  if (alloc_traits::propagate_on_container_copy_assignment::value) {
    alloc_ = other.alloc_;
    bucket_alloc_ = other.bucket_alloc_;
  }
  Deque<T, Allocator> copy(other);
  std::swap(data_, copy.data_);
  std::swap(buckets_, copy.buckets_);
  std::swap(size_, copy.size_);
  std::swap(begin_, copy.begin_);
  std::swap(end_, copy.end_);
  return *this;
}

template <typename T, typename Allocator>
Deque<T, Allocator>& Deque<T, Allocator>::operator=(
    Deque<T, Allocator>&& other) noexcept {
  if (&other == this) {
    return *this;
  }
  alloc new_alloc = alloc_;
  if (alloc_traits::propagate_on_container_copy_assignment::value) {
    new_alloc = other.alloc_;
  }
  bucket_alloc new_bucket_alloc = bucket_alloc_;
  if (bucket_alloc_traits::propagate_on_container_copy_assignment::value) {
    new_bucket_alloc = other.bucket_alloc_;
  }
  clear();
  bucket_alloc_traits::deallocate(bucket_alloc_, data_, buckets_);
  data_ = std::move(other.data_);
  alloc_ = new_alloc;
  bucket_alloc_ = new_bucket_alloc;
  size_ = other.size_;
  buckets_ = other.buckets_;
  begin_ = other.begin_;
  end_ = other.end_;
  other.set_null();
  return *this;
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::end() {
  return end_;
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::end() const {
  return end_;
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::const_iterator Deque<T, Allocator>::cend() const {
  return end_;
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::operator[](size_t idx) {
  return *(begin_ + idx);
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::operator[](size_t idx) const {
  return *(begin_ + idx);
}

template <typename T, typename Allocator>
T& Deque<T, Allocator>::at(size_t idx) {
  if (idx >= size_) {
    throw std::out_of_range("out of range");
  }
  return *(begin_ + idx);
}

template <typename T, typename Allocator>
const T& Deque<T, Allocator>::at(size_t idx) const {
  if (idx >= size_) {
    throw std::out_of_range("out of range");
  }
  return *(begin_ + idx);
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_back(Args&&... args) {
  if (data_ == nullptr) {
    set_first(std::forward<Args>(args)...);
    return;
  }
  if (&*(end_ - 1) == &data_[buckets_ - 1][kBucketSize - 1]) {
    size_t new_buckets_count = (buckets_ * 2) + 1;
    scale(new_buckets_count);
    begin_.fix(data_, (buckets_ + 1) / 2);
    end_.fix(data_, (buckets_ + 1) / 2);
    buckets_ = new_buckets_count;
  }
  alloc_traits::construct(alloc_, &*end_, std::forward<Args>(args)...);
  ++size_;
  ++end_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(const T& value) {
  emplace_back(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_back(T&& value) {
  emplace_back(std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace_front(Args&&... args) {
  if (data_ == nullptr) {
    set_first(std::forward<Args>(args)...);
    return;
  }
  if (&*begin_ == &data_[0][0]) {
    size_t new_buckets_count = (buckets_ * 2) + 1;
    scale(new_buckets_count);
    begin_.fix(data_, (new_buckets_count - buckets_) / 2);
    end_.fix(data_, (new_buckets_count - buckets_) / 2);
    buckets_ = new_buckets_count;
  }
  alloc_traits::construct(alloc_, &*(begin_ - 1), std::forward<Args>(args)...);
  ++size_;
  --begin_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(const T& value) {
  emplace_front(value);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::push_front(T&& value) {
  emplace_front(std::move(value));
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_back() {
  if (size_ == 0) {
    return;
  }
  --size_;
  alloc_traits::destroy(alloc_, &*(end_ - 1));
  --end_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::pop_front() {
  if (size_ == 0) {
    return;
  }
  --size_;
  alloc_traits::destroy(alloc_, &*begin_);
  ++begin_;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::clear() {
  if (data_ == nullptr) {
    return;
  }
  for (auto iter = begin_; iter != end_; ++iter) {
    alloc_traits::destroy(alloc_, &*iter);
  }
  for (size_t idx = 0; idx < buckets_; ++idx) {
    alloc_traits::deallocate(alloc_, data_[idx], kBucketSize);
  }
  set_null();
}

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::insert(
    Deque::iterator pos, const T& value) {
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

template <typename T, typename Allocator>
typename Deque<T, Allocator>::iterator Deque<T, Allocator>::erase(
    Deque::iterator pos) {
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

template <typename T, typename Allocator>
void Deque<T, Allocator>::scale(size_t new_buckets_count) {
  if (new_buckets_count < buckets_ + 1) {
    return;
  }
  T** new_data =
      bucket_alloc_traits::allocate(bucket_alloc_, new_buckets_count);
  size_t allocated_num = 0;
  try {
    for (size_t idx = 0; idx < (new_buckets_count - buckets_) / 2; ++idx) {
      new_data[idx] = alloc_traits::allocate(alloc_, kBucketSize);
      ++allocated_num;
    }
  } catch (...) {
    for (size_t idx = 0; idx < (new_buckets_count - buckets_) / 2; ++idx) {
      if (idx == allocated_num) {
        break;
      }
      alloc_traits::deallocate(alloc_, new_data[idx], kBucketSize);
    }
    bucket_alloc_traits::deallocate(bucket_alloc_, new_data, new_buckets_count);
    throw;
  }
  if (data_ != nullptr) {
    std::memcpy(&new_data[(new_buckets_count - buckets_) / 2], data_,
                buckets_ * sizeof(T*));
  }
  try {
    for (size_t idx = ((new_buckets_count - buckets_) / 2) + buckets_;
         idx < new_buckets_count; ++idx) {
      new_data[idx] = alloc_traits::allocate(alloc_, kBucketSize);
      ++allocated_num;
    }
  } catch (...) {
    for (size_t idx = 0; idx < new_buckets_count; ++idx) {
      if (idx == allocated_num) {
        break;
      }
      alloc_traits::deallocate(alloc_, new_data[idx], kBucketSize);
    }
    bucket_alloc_traits::deallocate(bucket_alloc_, new_data, new_buckets_count);
    throw;
  }
  bucket_alloc_traits::deallocate(bucket_alloc_, data_, buckets_);
  data_ = new_data;
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::set_null() {
  size_ = 0;
  buckets_ = 0;
  begin_ = Iterator<false>(data_, 0, 0);
  end_ = Iterator<false>(data_, 0, 0);
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::set_first(Args&&... value) {
  scale(1);
  try {
    alloc_traits::construct(alloc_, &data_[0][kBucketSize / 2],
                            std::forward<Args>(value)...);
  } catch (...) {
    alloc_traits::deallocate(alloc_, data_[0], kBucketSize);
    bucket_alloc_traits::deallocate(bucket_alloc_, data_, 1);
  }
  buckets_ = 1;
  size_ = 1;
  begin_ = Iterator<false>(data_, 0, kBucketSize / 2);
  end_ = Iterator<false>(data_, 0, (kBucketSize / 2) + 1);
}

// Iterator

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator++() {
  ++elem_;
  if (elem_ == kBucketSize) {
    elem_ = 0;
    ++bucket_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator++(int) {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator--() {
  if (elem_ == 0) {
    elem_ = kBucketSize - 1;
    --bucket_;
  } else {
    --elem_;
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator--(int) {
  auto copy = *this;
  --(*this);
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator+(
    Deque::Iterator<IsConst>::difference_type value) {
  Iterator temp = *this;
  temp += value;
  return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator+(
    Deque::Iterator<IsConst>::difference_type value) const {
  Iterator temp = *this;
  temp += value;
  return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator-(
    Deque::Iterator<IsConst>::difference_type value) {
  Iterator temp = *this;
  temp -= value;
  return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>
Deque<T, Allocator>::Iterator<IsConst>::operator-(
    Deque::Iterator<IsConst>::difference_type value) const {
  Iterator temp = *this;
  temp -= value;
  return temp;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator+=(
    Deque::Iterator<IsConst>::difference_type value) {
  if (value < 0) {
    return operator-=(-value);
  }
  bucket_ += (elem_ + value) / kBucketSize;
  elem_ = (elem_ + value) % kBucketSize;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>&
Deque<T, Allocator>::Iterator<IsConst>::operator-=(
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

template <typename T, typename Allocator>
template <bool IsConst>
typename Deque<T, Allocator>::template Iterator<IsConst>::difference_type
Deque<T, Allocator>::Iterator<IsConst>::operator-(
    const Iterator<IsConst>& other) const {
  if (bucket_ == other.bucket_) {
    return elem_ - other.elem_;
  }
  auto bucket_dif = bucket_ - other.bucket_ - 1;
  auto elem_dif = kBucketSize - other.elem_ + elem_;
  return (bucket_dif * kBucketSize) + elem_dif;
}