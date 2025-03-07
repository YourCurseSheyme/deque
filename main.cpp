//
// Created by sheyme on 26/02/25.
//

#include <algorithm>
#include <iostream>
#include <random>

#include "deque.hpp"

template <typename deque>
void Print(const deque& deq) {
  for (const auto& val : deq) {
    std::cout << val << ' ';
  }
}

int main() {
  Deque<int> first(10, 10);
  first.push_back(1);
  Print(first);
}