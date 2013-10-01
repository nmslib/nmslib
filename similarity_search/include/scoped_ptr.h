#ifndef _SCOPED_PRT_H_
#define _SCOPED_PRT_H_

#include <assert.h>
#include <stddef.h>

template <class T>
class scoped_ptr {
 public:
  explicit scoped_ptr(T* ptr) : ptr_(ptr) {}

  ~scoped_ptr() {
    enum { type_must_be_complete = sizeof(T) };
    delete ptr_;
  }

  T* get() const { return ptr_; }

  T* operator->() const {
    assert(ptr_ != NULL);
    return ptr_;
  }

  T& operator*() const {
    assert(ptr_ != NULL);
    return *ptr_;
  }

  bool operator==(T* ptr) const { return ptr_ == ptr; }
  bool operator!=(T* ptr) const { return ptr_ != ptr; }

 private:
  T* ptr_;

  // disable copy and assign
  scoped_ptr(const scoped_ptr&);
  scoped_ptr& operator=(const scoped_ptr&);

  // disable comparisons
  template <class U> bool operator==(scoped_ptr<U>&) const;
  template <class U> bool operator!=(scoped_ptr<U>&) const;
};

template <class T>
class scoped_array {
 public:
  explicit scoped_array(T* array) : array_(array) {}

  ~scoped_array() {
    enum { type_must_be_complete = sizeof(T) };
    delete[] array_;
  }

  T* get() const { return array_; }

  T& operator[](ptrdiff_t i) const {
    assert(i >= 0);
    assert(array_ != NULL);
    return array_[i];
  }

  bool operator==(T* ptr) const { return array_ == ptr; }
  bool operator!=(T* ptr) const { return array_ != ptr; }

 private:
  T* array_;

  // disable copy and assign
  scoped_array(const scoped_array&);
  scoped_array& operator=(const scoped_array&);

  // disable comparisons
  template <class U> bool operator==(scoped_array<U>&) const;
  template <class U> bool operator!=(scoped_array<U>&) const;
};

#endif    // _SCOPED_PRT_H_
