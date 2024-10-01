#pragma once
#include <memory>
#include <type_traits>

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr;

class BaseSharedPtr {
 private:
  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> AllocateShared(const Allocator& alloc, Args&&... args);

  template <typename U, typename... Args>
  friend SharedPtr<U> MakeShared(Args&&... args);

  template <typename T>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

  struct CommonBlock {
    size_t shared_cnt;
    size_t weak_cnt;

    CommonBlock(size_t sh_cnt, size_t w_cnt)
        : shared_cnt(sh_cnt), weak_cnt(w_cnt) {}

    // virtual T* get() = 0; // could solve that with void*
    virtual void destroy_block(CommonBlock* cblock) const = 0;
    virtual void destroy_object() = 0;
    virtual ~CommonBlock() = default;
  };

  template <typename U, typename Allocator, typename Deleter>
  struct RegularBlock : CommonBlock {
    U* ptr;
    Allocator alloc;  // Works with internals
    Deleter deleter;  // Instructs what to do with ptr

    RegularBlock(size_t sh_cnt, size_t w_cnt, U* ptr, Allocator alloc,
                 Deleter deleter)
        : CommonBlock(sh_cnt, w_cnt),
          ptr(ptr),
          alloc(alloc),
          deleter(deleter) {}

    // virtual T* get() { return ptr; }

    virtual void destroy_block(CommonBlock* cblock) const {
      using block_allocator =
          typename std::allocator_traits<Allocator>::template rebind_alloc<
              RegularBlock<U, Allocator, Deleter>>;
      using block_traits = std::allocator_traits<block_allocator>;
      block_allocator block_alloc(alloc);

      block_traits::destroy(block_alloc,
                            reinterpret_cast<RegularBlock*>(cblock));
      block_traits::deallocate(block_alloc,
                               reinterpret_cast<RegularBlock*>(cblock), 1);
    }

    virtual void destroy_object() { deleter(ptr); }

    ~RegularBlock() {}
  };

  template <typename U, typename Allocator>
  struct MakeSharedBlock : CommonBlock {
    // have to use array of chars, because after last shared pointer
    // to the object is destroyed, object's destructor is called
    // but when MakeSharedBlock's destructor called, destructors
    // of members including 'obj' are called again.
    alignas(U) char obj[sizeof(U)];
    Allocator alloc;

    template <typename... Args>
    MakeSharedBlock(size_t sh_cnt, size_t w_cnt, Allocator alloc,
                    Args&&... args)
        : CommonBlock(sh_cnt, w_cnt), alloc(alloc) {
      ::new (obj) U(std::forward<Args>(args)...);
    }

    // virtual U* get() { return &obj; }

    virtual void destroy_block(CommonBlock* cblock) const {
      using block_allocator = typename std::allocator_traits<
          Allocator>::template rebind_alloc<MakeSharedBlock<U, Allocator>>;
      using block_traits = std::allocator_traits<block_allocator>;
      block_allocator block_alloc(alloc);

      block_traits::destroy(block_alloc,
                            reinterpret_cast<MakeSharedBlock*>(cblock));
      block_traits::deallocate(block_alloc,
                               reinterpret_cast<MakeSharedBlock*>(cblock), 1);
    }

    virtual void destroy_object() { (reinterpret_cast<U*>(obj))->~U(); }

    ~MakeSharedBlock() {}
  };
};

template <typename T>
class SharedPtr : BaseSharedPtr {
 private:
  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> AllocateShared(const Allocator& alloc, Args&&... args);

  template <typename U, typename... Args>
  friend SharedPtr<U> MakeShared(Args&&... args);

  template <typename U>
  friend class WeakPtr;

  // This is used to see members of derived classes.
  template <typename U>
  friend class SharedPtr;

  CommonBlock* cblock_ = nullptr;
  T* ptr_ = nullptr;

  template <typename BlockType>
  SharedPtr(BlockType* cblock, T* ptr) : cblock_(cblock), ptr_(ptr) {}

 public:
  SharedPtr() = default;

  SharedPtr(std::nullptr_t) {}

  template <typename U>
  SharedPtr(U* ptr)
      : SharedPtr(ptr, std::default_delete<U>(), std::allocator<U>()) {}

  template <typename U, typename Deleter>
  SharedPtr(U* ptr, Deleter deleter)
      : SharedPtr(ptr, deleter, std::allocator<U>()) {}

  template <typename U, typename Deleter, typename Allocator>
  SharedPtr(U* ptr, Deleter deleter, Allocator alloc) : ptr_(ptr) {
    using block_allocator = typename std::allocator_traits<
        Allocator>::template rebind_alloc<RegularBlock<U, Allocator, Deleter>>;
    using traits = std::allocator_traits<block_allocator>;
    block_allocator block_alloc(alloc);

    cblock_ = traits::allocate(block_alloc, 1);
    traits::construct(
        block_alloc,
        reinterpret_cast<RegularBlock<U, Allocator, Deleter>*>(cblock_), 1, 0,
        ptr, alloc, deleter);
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  SharedPtr(const WeakPtr<U>& other) : cblock_(other.cblock_) {
    if (cblock_ != nullptr) {
      ++cblock_->shared_cnt;
    }
  }

  SharedPtr(const SharedPtr& other) : cblock_(other.cblock_), ptr_(other.ptr_) {
    if (cblock_ != nullptr) {
      ++cblock_->shared_cnt;
    }
  }

  SharedPtr(SharedPtr&& other) : cblock_(other.cblock_), ptr_(other.ptr_) {
    other.cblock_ = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  SharedPtr(const SharedPtr<U>& other)
      : cblock_(other.cblock_), ptr_(other.ptr_) {
    if (cblock_ != nullptr) {
      ++cblock_->shared_cnt;
    }
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    SharedPtr copy = other;
    copy.swap(*this);
    return *this;
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  SharedPtr(SharedPtr<U>&& other) : cblock_(other.cblock_), ptr_(other.ptr_) {
    other->reset();
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    SharedPtr copy = std::move(other);
    copy.swap(*this);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    SharedPtr copy(other);
    copy.swap(*this);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) {
    SharedPtr copy = std::move(other);
    copy.swap(*this);
    return *this;
  }

  size_t use_count() const {
    if (cblock_ == nullptr) {
      return 0;
    }
    return cblock_->shared_cnt;
  }

  T* get() const {
    if (cblock_ == nullptr) {
      return std::nullptr_t();
    }
    return ptr_;
  }

  T& operator*() const { return *get(); }

  T* operator->() const { return get(); }

  void reset() { SharedPtr().swap(*this); }

  void swap(SharedPtr& other) {
    std::swap(cblock_, other.cblock_);
    std::swap(ptr_, other.ptr_);
  }

  ~SharedPtr() {
    if (cblock_ == nullptr) {
      return;
    }
    --cblock_->shared_cnt;
    if (cblock_->shared_cnt == 0) {
      cblock_->destroy_object();
      if (cblock_->weak_cnt == 0) {
        cblock_->destroy_block(cblock_);
      }
    }
  }
};

template <typename U, typename Allocator, typename... Args>
SharedPtr<U> AllocateShared(const Allocator& alloc, Args&&... args) {
  using block_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<
          typename BaseSharedPtr::template MakeSharedBlock<U, Allocator>>;
  using block_traits = std::allocator_traits<block_allocator>;
  block_allocator block_alloc(alloc);

  auto cblock = block_traits::allocate(block_alloc, 1);
  block_traits::construct(
      block_alloc,
      reinterpret_cast<
          typename SharedPtr<U>::template MakeSharedBlock<U, Allocator>*>(
          cblock),
      1, 0, alloc, std::forward<Args>(args)...);
  return SharedPtr<U>(cblock, reinterpret_cast<U*>(cblock->obj));
}

template <typename U, typename... Args>
SharedPtr<U> MakeShared(Args&&... args) {
  return AllocateShared<U>(std::allocator<U>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 private:
  typename SharedPtr<T>::CommonBlock* cblock_ = nullptr;

 public:
  WeakPtr() = default;

  WeakPtr(const WeakPtr& other) : cblock_(other.cblock_) {
    if (cblock_) {
      ++cblock_->weak_cnt;
    }
  }

  template <typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
  WeakPtr(const SharedPtr<U>& other) : cblock_(other.cblock_) {
    if (cblock_) {
      ++cblock_->weak_cnt;
    }
  }

  WeakPtr(WeakPtr&& other) : cblock_(other.cblock_) { other.cblock_ = nullptr; }

  WeakPtr& operator=(const WeakPtr& other) {
    WeakPtr copy(other);
    std::swap(copy.cblock_, cblock_);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    WeakPtr copy = std::move(other);
    std::swap(copy.cblock_, cblock_);
    return *this;
  }

  bool expired() const { return cblock_->shared_cnt == 0; }

  SharedPtr<T> lock() const { return SharedPtr<T>(*this); }

  ~WeakPtr() {
    if (cblock_ == nullptr) {
      return;
    }
    --cblock_->weak_cnt;
    if (cblock_->shared_cnt == 0 && cblock_->weak_cnt == 0) {
      cblock_->destroy_block(cblock_);
    }
  }
};

