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
const int lazy_initializer_value = 1337;
void lazy_initializer(int * const &i) {
  *i = lazy_initializer_value;
  ++lazy_initializer_called;
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
}
