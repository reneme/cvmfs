#include <gtest/gtest.h>
#include <pthread.h>

#include <tbb/tbb_thread.h>

#include "../../cvmfs/util.h"

//
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//


class ThreadDummy {
 public:
  ThreadDummy(int canary_value) : result_value(0), value_(canary_value) {}

  void OtherThread() {
    result_value = value_;
  }

  int result_value;

 private:
  const int value_;
};


TEST(T_Util, ThreadProxy) {
  const int canary = 1337;

  ThreadDummy dummy(canary);
  tbb::tbb_thread thread(&ThreadProxy<ThreadDummy>,
                         &dummy,
                         &ThreadDummy::OtherThread);
  thread.join();

  EXPECT_EQ (canary, dummy.result_value);
}


TEST(T_Util, IsAbsolutePath) {
  const bool empty = IsAbsolutePath("");
  EXPECT_FALSE (empty) << "empty path string treated as absolute";

  const bool relative = IsAbsolutePath("foo.bar");
  EXPECT_FALSE (relative) << "relative path treated as absolute";
  const bool absolute = IsAbsolutePath("/tmp/foo.bar");
  EXPECT_TRUE (absolute) << "absolute path not recognized";
}


//
// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
//


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

TEST(T_Util, LazyInitializer) {
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

TEST(T_Util, LazyInitializerVoidCallback) {
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

TEST(T_Util, LazyInitializerCastOperator) {
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

TEST(T_Util, LazyInitializerReset) {
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

TEST(T_Util, LazyInitializerDirectAccess) {
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
