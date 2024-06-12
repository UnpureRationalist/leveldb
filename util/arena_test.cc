// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/arena.h"

#include <cstddef>
#include <thread>
#include <unordered_set>

#include "util/random.h"

#include "gtest/gtest.h"

namespace leveldb {

TEST(ArenaTest, Empty) { Arena arena; }

TEST(ArenaTest, Simple) {
  std::vector<std::pair<size_t, char*>> allocated;
  Arena arena;
  const int N = 100000;
  size_t bytes = 0;
  Random rnd(301);
  for (int i = 0; i < N; i++) {
    size_t s;
    if (i % (N / 10) == 0) {
      s = i;
    } else {
      s = rnd.OneIn(4000)
              ? rnd.Uniform(6000)
              : (rnd.OneIn(10) ? rnd.Uniform(100) : rnd.Uniform(20));
    }
    if (s == 0) {
      // Our arena disallows size 0 allocations.
      s = 1;
    }
    char* r;
    if (rnd.OneIn(10)) {
      r = arena.AllocateAligned(s);
    } else {
      r = arena.Allocate(s);
    }

    for (size_t b = 0; b < s; b++) {
      // Fill the "i"th allocation with a known bit pattern
      r[b] = i % 256;
    }
    bytes += s;
    allocated.push_back(std::make_pair(s, r));
    ASSERT_GE(arena.MemoryUsage(), bytes);
    if (i > N / 10) {
      ASSERT_LE(arena.MemoryUsage(), bytes * 1.10);
    }
  }
  for (size_t i = 0; i < allocated.size(); i++) {
    size_t num_bytes = allocated[i].first;
    const char* p = allocated[i].second;
    for (size_t b = 0; b < num_bytes; b++) {
      // Check the "i"th allocation for the known bit pattern
      ASSERT_EQ(int(p[b]) & 0xff, i % 256);
    }
  }
}

void ConcurrencyTest(bool align) {
  Arena arena;
  std::unordered_set<char*> thread1_mem;
  std::unordered_set<char*> thread2_mem;
  constexpr int allocate_times = 1000000;
  size_t bytes = 100;
  std::thread t1([&] {
    for (int i = 0; i < allocate_times; ++i) {
      thread1_mem.insert(align ? arena.AllocateAligned(bytes)
                               : arena.Allocate(bytes));
    }
  });

  std::thread t2([&] {
    for (int i = 0; i < allocate_times; ++i) {
      thread2_mem.insert(align ? arena.AllocateAligned(bytes)
                               : arena.Allocate(bytes));
    }
  });

  t1.join();
  t2.join();

  for (auto& ptr : thread2_mem) {
    ASSERT_EQ(thread1_mem.count(ptr), 0);
  }
}

TEST(ArenaTest, ConcurrencyAllocate) {
  constexpr int test_times = 10;

  for (int i = 0; i < test_times; ++i) {
    ConcurrencyTest(false);
  }
}

TEST(ArenaTest, ConcurrencyAllocateAligned) {
  constexpr int test_times = 10;

  for (int i = 0; i < test_times; ++i) {
    ConcurrencyTest(true);
  }
}

}  // namespace leveldb
