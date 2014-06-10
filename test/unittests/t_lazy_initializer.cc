#include <gtest/gtest.h>
#include <pthread.h>

#include <tbb/tbb_thread.h>

#include "../../cvmfs/util.h"


int lazy_initializer_called = 0;
int lazy_void_initializer_called = 0;
int lazy_void_initializer_value = -1;
const int lazy_initializer_value = 1337;
void lazy_initializer(int * const &i) {
  *i = lazy_initializer_value;
  ++lazy_initializer_called;
}

void lazy_void_initializer() {
  ++lazy_void_initializer_called;
  lazy_void_initializer_value = lazy_initializer_value;
}

TEST(T_LazyInitializer, Simple) {
  ASSERT_EQ (0, lazy_initializer_called);
  LazyInitializer<int> i(LazyInitializer<int>::MakeCallback(&lazy_initializer));

  EXPECT_EQ (0, lazy_initializer_called);
  int i_res_1 = i.Get();
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_1);

  int i_res_2 = i.Get();
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_2);

  lazy_initializer_called = 0;
  ASSERT_EQ (0, lazy_initializer_called);
}

TEST(T_LazyInitializer, VoidCallback) {
  ASSERT_EQ (0, lazy_void_initializer_called);
  ASSERT_EQ (-1, lazy_void_initializer_value);

  typedef LazyInitializer<int*, void> VoidLI;

  VoidLI i(VoidLI::MakeCallback(&lazy_void_initializer));
  i = &lazy_void_initializer_value;

  EXPECT_EQ (0, lazy_void_initializer_called);
  int i_res_1 = *i.Get();
  EXPECT_EQ (1, lazy_void_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_1);
  int i_res_2 = *i.Get();
  EXPECT_EQ (1, lazy_void_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_2);


  lazy_void_initializer_called = 0;
  lazy_void_initializer_value  = -1;
  ASSERT_EQ (0, lazy_void_initializer_called);
  ASSERT_EQ (-1, lazy_void_initializer_value);
}

TEST(T_LazyInitializer, CastOperatorAccess) {
  ASSERT_EQ (0, lazy_initializer_called);
  LazyInitializer<int> i(LazyInitializer<int>::MakeCallback(&lazy_initializer));

  EXPECT_EQ (0, lazy_initializer_called);
  int i_res_1 = i;
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_1);

  int i_res_2 = i;
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_2);

  lazy_initializer_called = 0;
  ASSERT_EQ (0, lazy_initializer_called);
}

TEST(T_LazyInitializer, Reset) {
  ASSERT_EQ (0, lazy_initializer_called);
  LazyInitializer<int> i(LazyInitializer<int>::MakeCallback(&lazy_initializer));

  EXPECT_EQ (0, lazy_initializer_called);
  int i_res_1 = i;
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_1);

  i.Reset();

  int i_res_2 = i.Get();
  EXPECT_EQ (2, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_2);

  lazy_initializer_called = 0;
  ASSERT_EQ (0, lazy_initializer_called);
}

TEST(T_LazyInitializer, DirectAccess) {
  ASSERT_EQ (0, lazy_initializer_called);
  LazyInitializer<int> i(LazyInitializer<int>::MakeCallback(&lazy_initializer));

  EXPECT_EQ (0, lazy_initializer_called);
  i = lazy_initializer_value;
  EXPECT_EQ (0, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i.GetDirectly());

  i = 0;
  EXPECT_EQ (0, lazy_initializer_called);
  EXPECT_EQ (0, i.GetDirectly());

  const int i_res_1 = i;
  EXPECT_EQ (1, lazy_initializer_called);
  EXPECT_EQ (lazy_initializer_value, i_res_1);

  lazy_initializer_called = 0;
  ASSERT_EQ (0, lazy_initializer_called);
}
