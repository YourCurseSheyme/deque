//
// Created by sheyme on 26/02/25.
//

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <tuple>
#include <chrono>

#include "deque.hpp"

template <typename Cont>
void Print(const Cont& deque) {
  for (auto iter = deque.begin(); iter != deque.end(); ++iter) {
    std::cout << iter->value << ' ';
  }
}

struct NotDefaultConstructible {
  NotDefaultConstructible() = delete;
  NotDefaultConstructible(int data) : data(data) {}
  int data;

  auto operator<=>(const NotDefaultConstructible&) const = default;
};

struct ThrowStruct {
  ThrowStruct(int value, bool throw_in_assignment, bool throw_in_copy) :
                                                                         value(value),
                                                                         throw_in_assignment(throw_in_assignment),
                                                                         throw_in_copy(throw_in_copy) {}

  ThrowStruct(const ThrowStruct& s) {
    value = s.value;
    throw_in_assignment = s.throw_in_assignment;
    throw_in_copy = s.throw_in_copy;

    if (throw_in_copy) {
      throw 1;
    }
  }

  ThrowStruct& operator=(const ThrowStruct& s) {
    if (throw_in_assignment) {
      throw 1;
    }

    value = s.value;
    throw_in_assignment = s.throw_in_assignment;
    throw_in_copy = s.throw_in_copy;

    return *this;
  }

  auto operator<=>(const ThrowStruct&) const = default;

  int value;
  bool throw_in_assignment;
  bool throw_in_copy;
};

template <class Stack1, class Stack2>
bool CompareStacks(const Stack1& s1, const Stack2& s2) {
  for (size_t i = 0; i < s1.size(); ++i) {
    if (s1[i] != s2[i]) {
      return false;
    }
  }

  return true;
}

struct MemoryManager {
  static size_t type_new_allocated;
  static size_t type_new_deleted;
  static size_t allocator_allocated;
  static size_t allocator_deallocated;

  static size_t allocator_constructed;
  static size_t allocator_destroyed;

  static void TypeNewAllocate(size_t n) {
    type_new_allocated += n;
  }

  static void TypeNewDelete(size_t n) {
    type_new_deleted += n;
  }

  static void AllocatorAllocate(size_t n) {
    allocator_allocated += n;
  }

  static void AllocatorDeallocate(size_t n) {
    allocator_deallocated += n;
  }

  static void AllocatorConstruct(size_t n) {
    allocator_constructed += n;
  }

  static void AllocatorDestroy(size_t n) {
    allocator_destroyed += n;
  }
};

struct TypeWithFancyNewDeleteOperators {
  TypeWithFancyNewDeleteOperators() = default;
  explicit TypeWithFancyNewDeleteOperators(int value): value(value) {}

  static void* operator new(size_t n) {
    MemoryManager::TypeNewAllocate(n);
    return ::operator new(n);
  }

  static void operator delete(void* ptr, size_t n) {
    MemoryManager::TypeNewDelete(n);
    ::operator delete(ptr);
  }

  int value = 0;
};

size_t MemoryManager::type_new_allocated = 0;
size_t MemoryManager::type_new_deleted = 0;
size_t MemoryManager::allocator_allocated = 0;
size_t MemoryManager::allocator_deallocated = 0;
size_t MemoryManager::allocator_constructed = 0;
size_t MemoryManager::allocator_destroyed = 0;

void SetupTest() {
  MemoryManager::type_new_allocated = 0;
  MemoryManager::type_new_deleted = 0;
  MemoryManager::allocator_allocated = 0;
  MemoryManager::allocator_deallocated = 0;
  MemoryManager::allocator_constructed = 0;
  MemoryManager::allocator_destroyed = 0;
}

void* operator new(size_t n, bool from_my_allocator ) {
  (void)from_my_allocator;
  return malloc(n);
}

void operator delete(void* ptr, size_t n, bool from_my_allocator) noexcept {
  (void)n;
  (void)from_my_allocator;
  free(ptr);
}

template <typename T>
struct AllocatorWithCount {
  using value_type = T;

  AllocatorWithCount() = default;

  template <typename U>
  AllocatorWithCount(const AllocatorWithCount<U>& other) {
    std::ignore = other;
  }

  T* allocate(size_t n) {
    MemoryManager::AllocatorAllocate(n * sizeof(T));
    allocator_allocated += n * sizeof(T);
    return reinterpret_cast<T*>(operator new(n * sizeof(T), true));
  }

  void deallocate(T* ptr, size_t n) {
    MemoryManager::AllocatorDeallocate(n * sizeof(T));
    allocator_deallocated += n * sizeof(T);
    operator delete(ptr, n, true);
  }

  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    MemoryManager::AllocatorConstruct(1);
    allocator_constructed += 1;
    ::new(ptr) U(std::forward<Args>(args)...);
  }

  template <typename U>
  void destroy(U* ptr) noexcept {
    MemoryManager::AllocatorDestroy(1);
    allocator_destroyed += 1;
    ptr->~U();
  }

  size_t allocator_allocated = 0;
  size_t allocator_deallocated = 0;
  size_t allocator_constructed = 0;
  size_t allocator_destroyed = 0;
};

template <typename T>
bool operator==(const AllocatorWithCount<T>& lhs, const AllocatorWithCount<T>& rhs) {
  return lhs.allocator_allocated == rhs.allocator_allocated &&
         lhs.allocator_deallocated == rhs.allocator_deallocated &&
         lhs.allocator_constructed == rhs.allocator_constructed &&
         lhs.allocator_destroyed == rhs.allocator_destroyed;
}

struct TypeWithCounts: public TypeWithFancyNewDeleteOperators {
  using smart_counter = std::shared_ptr<size_t>;

  TypeWithCounts(int v) {
    value = v;
    *int_c += 1;
  }

  TypeWithCounts() {
    value = 0;
    *default_c += 1;
  }

  TypeWithCounts(const TypeWithCounts& other): TypeWithFancyNewDeleteOperators(other.value) {
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *copy_c += 1;
  }

  TypeWithCounts(TypeWithCounts&& other) {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *move_c += 1;
  }

  TypeWithCounts& operator=(const TypeWithCounts& other) {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *ass_copy += 1;
    return *this;
  }

  TypeWithCounts& operator=(TypeWithCounts&& other)  noexcept {
    value = other.value;
    default_c = other.default_c;
    copy_c = other.copy_c;
    move_c = other.move_c;
    int_c = other.int_c;
    ass_copy = other.ass_copy;
    ass_move = other.ass_move;
    *ass_move += 1;
    return *this;
  }

  smart_counter default_c = std::make_shared<size_t>(0);
  smart_counter copy_c = std::make_shared<size_t>(0);
  smart_counter move_c = std::make_shared<size_t>(0);
  smart_counter int_c = std::make_shared<size_t>(0);
  smart_counter ass_copy = std::make_shared<size_t>(0);
  smart_counter ass_move = std::make_shared<size_t>(0);
};

bool operator==(const TypeWithCounts& lhs, const TypeWithCounts& rhs) {
  return lhs.default_c == rhs.default_c &&
         lhs.copy_c == rhs.copy_c &&
         lhs.move_c == rhs.move_c &&
         lhs.int_c == rhs.int_c &&
         lhs.ass_copy == rhs.ass_copy &&
         lhs.ass_move == rhs.ass_move;
}

bool operator!=(const TypeWithCounts& lhs, const TypeWithCounts& rhs) {
  return !(lhs == rhs);
}

template <typename T, bool PropagateOnConstruct, bool PropagateOnAssign>
struct WhimsicalAllocator : public std::allocator<T> {
  std::shared_ptr<int> number;

  auto select_on_container_copy_construction() const {
    return PropagateOnConstruct
               ? WhimsicalAllocator<T, PropagateOnConstruct, PropagateOnAssign>()
               : *this;
  }

  struct propagate_on_container_copy_assignment
      : std::conditional_t<PropagateOnAssign, std::true_type, std::false_type>
  {};

  template <typename U>
  struct rebind {
    using other = WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>;
  };

  WhimsicalAllocator(): number(std::make_shared<int>(counter)) {
    ++counter;
  }

  template <typename U>
  WhimsicalAllocator(const WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>& another)
      : number(another.number)
  {}

  template <typename U>
  auto& operator=(const WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>& another) {
    number = another.number;
    return *this;
  }

  template <typename U>
  bool operator==(const WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>& another) const {
    return *number == *another.number;
  }

  template <typename U>
  bool operator!=(const WhimsicalAllocator<U, PropagateOnConstruct, PropagateOnAssign>& another) const {
    return *number != *another.number;
  }

  static size_t counter;
};

template <typename T, bool PropagateOnConstruct, bool PropagateOnAssign>
size_t WhimsicalAllocator<T, PropagateOnConstruct, PropagateOnAssign>::counter = 0;

struct OnlyMovable: public TypeWithFancyNewDeleteOperators {
  OnlyMovable(int a) {std::ignore = a; }
  OnlyMovable() = delete;
  OnlyMovable(const OnlyMovable&) = delete;
  OnlyMovable(OnlyMovable&& other) noexcept { std::ignore = other; }
};

struct Accountant {
  // Some field of strange size
  char arr[40];

  static size_t ctor_calls;
  static size_t dtor_calls;

  // NO LINT
  static void reset() {
    ctor_calls = 0;
    dtor_calls = 0;
  }

  Accountant() {
    ++ctor_calls;
  }

  // NO LINT
  Accountant(const Accountant&) {
    ++ctor_calls;
  }

  // NO LINT
  Accountant& operator=(const Accountant&) {
    // Actually, when it comes to assign one list to another,
    // list can use element-wise assignment instead of destroying nodes and creating new ones
    ++ctor_calls;
    ++dtor_calls;
    return *this;
  }

  Accountant(Accountant&&) = default;
  Accountant& operator=(Accountant&&) = default;

  ~Accountant() {
    ++dtor_calls;
  }
};

size_t Accountant::ctor_calls = 0;
size_t Accountant::dtor_calls = 0;

struct ThrowingAccountant: public Accountant {
  static bool need_throw;

  int value = 0;

  // NO LINT
  ThrowingAccountant(int value = 0): Accountant(), value(value) {
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
  }

  // NO LINT
  ThrowingAccountant(const ThrowingAccountant& other): Accountant(), value(other.value) {
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
  }

  // NO LINT
  ThrowingAccountant& operator=(const ThrowingAccountant& other) {
    value = other.value;
    ++ctor_calls;
    ++dtor_calls;
    if (need_throw && ctor_calls % 5 == 4)
      throw std::string("Ahahahaha you have been cocknut");
    return *this;
  }

};

bool ThrowingAccountant::need_throw = false;

void FillVectorWithRandomNumbers(std::vector<size_t>& v,
                                 size_t numbers_count,
                                 size_t begin,
                                 size_t end) {
  v.resize(numbers_count);

  std::random_device rnd_device;
  std::mt19937 mersenne_engine{rnd_device()};
  std::uniform_int_distribution<size_t> dist{begin, end};

  auto gen = [&dist, &mersenne_engine]() {
    return dist(mersenne_engine);
  };

  std::generate(v.begin(), v.end(), gen);
}

void TestFunction(const std::vector<size_t>& test_vector) {
  Deque<size_t> d;

  for (const auto& number: test_vector) {
    d.push_back(number);
  }

  for (const auto& number: test_vector) {
    d.push_front(number);
  }

  while (!d.empty()) {
    d.pop_back();
  }
}

static constexpr size_t kTestSize = 10000000;
static constexpr size_t kDistrBegin = 1;
static constexpr size_t kDistrEnd = 100;
static constexpr long long kNormalDuration = 5;

int RunTest() {
  std::vector<size_t> vector_with_random_numbers;
  FillVectorWithRandomNumbers(vector_with_random_numbers,
                              kTestSize,
                              kDistrBegin,
                              kDistrEnd);

  auto start = std::chrono::high_resolution_clock::now();
  TestFunction(vector_with_random_numbers);
  auto stop = std::chrono::high_resolution_clock::now();

  auto duration_in_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();

  std::cout << "Stress test took " << duration_in_seconds << " seconds" << std::endl;

  return duration_in_seconds > kNormalDuration ? 1 : 0;
}

int main() {
//  std::cout << RunTest();
  Deque<NotDefaultConstructible> d(10000, {1});
  auto start_size = d.size();

  d.insert(d.begin() + static_cast<int>(start_size) / 2,
           NotDefaultConstructible{2});
//  std::cout << (d.begin() + static_cast<int>(start_size) / 2 - 1)->data << '\n';
  std::cout << (d.size() == start_size + 1) << '\n';
  d.erase(d.begin() + static_cast<int>(start_size) / 2 - 1);
  std::cout << (d.size() == start_size) << '\n';

  std::cout << "count: " << std::count(d.begin(), d.end(), NotDefaultConstructible{1}) << '\n';
  std::cout << (size_t(std::count(d.begin(), d.end(), NotDefaultConstructible{1}))
              == start_size - 1) << '\n';
//  std::cout << (std::count(d.begin(), d.end(), NotDefaultConstructible{2}) == 1) << '\n';
//
//  Deque<NotDefaultConstructible> copy;
//  for (const auto& item: d) {
//    copy.insert(copy.end(), item);
//  }
//
//  std::cout << (d.size() == copy.size()) << '\n';
//  std::cout << (std::equal(d.begin(), d.end(), copy.begin())) << '\n';
}