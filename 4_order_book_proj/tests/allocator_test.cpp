#include "allocator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <random>
#include <set>
#include <string>
#include <vector>

// Tests the pool contract only (no internals):
//   - Size = number of T slots
//   - allocate() returns a distinct, aligned, non-overlapping slot, or nullptr
//     once all Size slots are live
//   - full() is true exactly when all slots are live
//   - deallocate() returns a slot for reuse

namespace {

struct Order {
  uint64_t id;
  int64_t price;
  uint32_t qty;
  uint32_t side;
};

struct Big {
  unsigned char bytes[200];
};

constexpr size_t N = 128;

template <typename T, size_t S> std::vector<T *> drain(Allocator<T, S> &a) {
  std::vector<T *> out;
  while (T *p = a.allocate()) {
    out.push_back(p);
    if (out.size() > S)
      break; // guard against a pool that never runs out
  }
  return out;
}

template <typename T>
void expect_distinct_aligned_non_overlapping(std::vector<T *> ptrs) {
  std::set<T *> uniq(ptrs.begin(), ptrs.end());
  ASSERT_EQ(uniq.size(), ptrs.size()) << "same slot handed out twice";

  for (T *p : ptrs)
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % alignof(T), 0u)
        << "misaligned slot " << p;

  std::sort(ptrs.begin(), ptrs.end());
  for (size_t i = 1; i < ptrs.size(); i++) {
    auto gap = reinterpret_cast<uintptr_t>(ptrs[i]) -
               reinterpret_cast<uintptr_t>(ptrs[i - 1]);
    EXPECT_GE(gap, sizeof(T)) << "slots overlap at index " << i;
  }
}

} // namespace

TEST(Allocator, FreshPoolIsNotFull) {
  Allocator<Order, N> a;
  EXPECT_FALSE(a.full());
}

TEST(Allocator, FirstAllocationSucceeds) {
  Allocator<Order, N> a;
  EXPECT_NE(a.allocate(), nullptr);
}

TEST(Allocator, HandsOutExactlySizeSlots) {
  Allocator<Order, N> a;
  auto ptrs = drain(a);
  EXPECT_EQ(ptrs.size(), N);
  EXPECT_EQ(a.allocate(), nullptr) << "allocation past capacity";
}

TEST(Allocator, FullTracksCapacity) {
  Allocator<Order, N> a;
  for (size_t i = 0; i < N; i++) {
    EXPECT_FALSE(a.full()) << "full after only " << i << " allocations";
    ASSERT_NE(a.allocate(), nullptr);
  }
  EXPECT_TRUE(a.full());
}

TEST(Allocator, SlotsAreDistinctAlignedNonOverlapping) {
  Allocator<Order, N> a;
  auto ptrs = drain(a);
  ASSERT_EQ(ptrs.size(), N);
  expect_distinct_aligned_non_overlapping(ptrs);
}

TEST(Allocator, LargeTypeSlotsDoNotOverlap) {
  Allocator<Big, N> a;
  auto ptrs = drain(a);
  ASSERT_EQ(ptrs.size(), N);
  expect_distinct_aligned_non_overlapping(ptrs);
}

// sizeof(T) < sizeof(void*): the free-list link must still fit in a slot.
TEST(Allocator, TypeSmallerThanPointer) {
  Allocator<uint16_t, N> a;
  auto ptrs = drain(a);
  ASSERT_EQ(ptrs.size(), N);
  expect_distinct_aligned_non_overlapping(ptrs);

  for (size_t i = 0; i < N; i++)
    *ptrs[i] = static_cast<uint16_t>(i);
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ(*ptrs[i], static_cast<uint16_t>(i));
}

TEST(Allocator, WritesDoNotCorruptOtherSlots) {
  Allocator<Order, N> a;
  auto ptrs = drain(a);
  ASSERT_EQ(ptrs.size(), N);

  for (size_t i = 0; i < N; i++)
    *ptrs[i] = Order{i, static_cast<int64_t>(i) * 100, 7, 1};
  for (size_t i = 0; i < N; i++) {
    EXPECT_EQ(ptrs[i]->id, i);
    EXPECT_EQ(ptrs[i]->price, static_cast<int64_t>(i) * 100);
  }
}

TEST(Allocator, DeallocateThenAllocateReusesSlot) {
  Allocator<Order, N> a;
  Order *p = a.allocate();
  ASSERT_NE(p, nullptr);
  a.deallocate(p);
  EXPECT_EQ(a.allocate(), p);
}

TEST(Allocator, DeallocateFromFullMakesRoom) {
  Allocator<Order, N> a;
  auto ptrs = drain(a);
  ASSERT_EQ(ptrs.size(), N);
  ASSERT_TRUE(a.full());

  a.deallocate(ptrs[N / 2]);
  EXPECT_FALSE(a.full());
  EXPECT_EQ(a.allocate(), ptrs[N / 2]);
  EXPECT_TRUE(a.full());
}

// User data overwrites the bytes the free list uses while the slot is live.
// After a full free + refill, the pool must still hand out every slot once.
TEST(Allocator, FullCycleAfterDirtyingSlots) {
  Allocator<Order, N> a;
  auto first = drain(a);
  ASSERT_EQ(first.size(), N);
  for (Order *p : first)
    std::memset(p, 0xAB, sizeof(Order));
  for (Order *p : first)
    a.deallocate(p);

  EXPECT_FALSE(a.full());
  auto second = drain(a);
  ASSERT_EQ(second.size(), N);
  expect_distinct_aligned_non_overlapping(second);

  std::sort(first.begin(), first.end());
  std::sort(second.begin(), second.end());
  EXPECT_EQ(first, second) << "refill returned slots outside the pool";
}

TEST(Allocator, RandomAllocFreeMatchesReference) {
  Allocator<Order, N> a;
  std::vector<Order *> live;
  std::set<Order *> seen;
  std::mt19937 rng(42);

  for (int step = 0; step < 100000; step++) {
    bool do_alloc = live.empty() || (live.size() < N && rng() % 2);
    if (do_alloc) {
      Order *p = a.allocate();
      ASSERT_NE(p, nullptr) << "step " << step << ", live " << live.size();
      ASSERT_EQ(std::count(live.begin(), live.end(), p), 0)
          << "live slot handed out again at step " << step;
      p->id = static_cast<uint64_t>(step);
      live.push_back(p);
      seen.insert(p);
    } else {
      size_t i = rng() % live.size();
      std::swap(live[i], live.back());
      a.deallocate(live.back());
      live.pop_back();
    }
    ASSERT_EQ(a.full(), live.size() == N) << "step " << step;
  }
  EXPECT_LE(seen.size(), N) << "more distinct slots than capacity";
}

TEST(Allocator, TwoPoolsDoNotOverlap) {
  Allocator<Order, N> a;
  Allocator<Order, N> b;
  auto pa = drain(a);
  auto pb = drain(b);
  ASSERT_EQ(pa.size(), N);
  ASSERT_EQ(pb.size(), N);

  std::vector<Order *> all(pa);
  all.insert(all.end(), pb.begin(), pb.end());
  expect_distinct_aligned_non_overlapping(all);
}

TEST(Allocator, ReconstructAfterDestroy) {
  for (int round = 0; round < 3; round++) {
    Allocator<Order, N> a;
    EXPECT_EQ(drain(a).size(), N) << "round " << round;
  }
}

// allocate() returns raw storage; objects with lifetimes need placement new.
TEST(Allocator, PlacementNewNonTrivialType) {
  Allocator<std::string, N> a;
  std::vector<std::string *> strs;
  for (size_t i = 0; i < N; i++) {
    void *mem = a.allocate();
    ASSERT_NE(mem, nullptr);
    strs.push_back(new (mem) std::string(64, static_cast<char>('a' + i % 26)));
  }
  for (size_t i = 0; i < N; i++)
    EXPECT_EQ((*strs[i])[0], static_cast<char>('a' + i % 26));
  for (std::string *s : strs) {
    s->~basic_string();
    a.deallocate(s);
  }
  EXPECT_FALSE(a.full());
}
