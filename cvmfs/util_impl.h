/**
 * This file is part of the CernVM File System.
 */


#ifdef CVMFS_NAMESPACE_GUARD
namespace CVMFS_NAMESPACE_GUARD {
#endif


template <class T>
class LazyInitializer<T>::LazyInitializ() {
  // Thread Safety Note:
  //   Double Checked Locking with atomics!
  //   Simply double checking registered_plugins_.empty() is _not_ thread safe
  //   since a second thread might find a registered_plugins_ list that is
  //   currently under construction and therefore _not_ empty but also _not_
  //   fully initialized!
  // See StackOverflow: http://stackoverflow.com/questions/8097439/
  //                    lazy-initialized-caching-how-do-i-make-it-thread-safe
  if (atomic_read32(&needs_init_)) {
    pthread_mutex_lock(&init_mutex_);
    if (atomic_read32(&needs_init_)) {
      assert (initializer_ != NULL);
      (*initializer_)(&data_);
      delete initializer_;
      initializer_ = NULL;
      atomic_dec32(&needs_init_);
    }
    pthread_mutex_unlock(&init_mutex_);
  }
}


#ifdef CVMFS_NAMESPACE_GUARD
}
#endif
