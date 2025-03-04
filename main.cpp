//
// Created by sheyme on 26/02/25.
//

#include <algorithm>
#include <iostream>
#include <random>

#include "deque.hpp"

template <typename Deque>
void Print(Deque& deq) {
  for (auto value : deq) {
    std::cout << value << ' ';
  }
}

template <class Stack1, class Stack2>
bool CompareStacks(const Stack1& s1, const Stack2& s2) {
  for (size_t i = 0; i < s1.size(); ++i) {
    if (s1[i] != s2[i]) {
      return false;
    }
  }

  return true;
}

int main() {
  Deque<int> first(10, 10);
  Deque<int> second(9, 9);
  first = second;

  std::cout << (first.size() == 9) << '\n';
  std::cout << (first.size() == second.size()) << '\n';
  std::cout << (CompareStacks(first, second)) << '\n';
}