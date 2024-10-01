#include <iostream>
#include <string>

template <typename T, typename Allocator = std::allocator<T>>
class Deque {
 private:
  T** arr_;
  size_t size_ = 0;
  size_t ptr_cnt_ = 0;
  const size_t kBucketSize = 32;
  size_t head_bucket_ = 0;
  size_t head_cell_ = 0;
  size_t end_bucket_ = 0;
  size_t end_cell_ = -1;

  using bucket_alloc =
      typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;
  using alloc_traits = std::allocator_traits<Allocator>;
  using bucket_alloc_traits = std::allocator_traits<bucket_alloc>;

  Allocator alloc_;
  bucket_alloc buck_alloc_;

  template <typename Iterator>
  void bucket_filler_with_iterator(size_t& cnt, Iterator iter);

  void bucket_filler(size_t& cnt, const T& value);
  void bucket_filler(size_t& cnt);
  void bucket_allocator(size_t count);
  void bucket_deallocator(size_t until, size_t head, size_t end);

  void my_swap(Deque& other);

  template <typename... Args>
  void memory_helper(Args&&... args, bool is_end);

  struct IsCopyAssigned {
    bool value = false;

    IsCopyAssigned(bool value) : value(value) {}
  };

 public:
  Deque() : arr_(bucket_alloc_traits::allocate(buck_alloc_, 1)), ptr_cnt_(1) {
    arr_[0] = alloc_traits::allocate(alloc_, kBucketSize);
  }

  Deque(const Allocator& alloc)
      : arr_(bucket_alloc_traits::allocate(buck_alloc_, 1)),
        ptr_cnt_(1),
        alloc_(alloc),
        buck_alloc_(alloc) {
    arr_[0] = alloc_traits::allocate(alloc_, kBucketSize);
  }

  Deque(const Deque& other, const IsCopyAssigned& flag)
      : arr_(bucket_alloc_traits::allocate(buck_alloc_, other.ptr_cnt_)),
        size_(other.size_),
        ptr_cnt_(other.ptr_cnt_),
        head_bucket_(other.head_bucket_),
        head_cell_(other.head_cell_),
        end_bucket_(other.end_bucket_),
        end_cell_(other.end_cell_) {
    // Code of copy constructor can be useful for purposes of
    // copy assignment operator, therefore flag is used.
    if (flag.value &&
        alloc_traits::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
      if (bucket_alloc_traits::propagate_on_container_copy_assignment::value) {
        buck_alloc_ = other.buck_alloc_;
      }
    } else {
      alloc_ =
          alloc_traits::select_on_container_copy_construction(other.alloc_);
      buck_alloc_ = bucket_alloc_traits::select_on_container_copy_construction(
          other.buck_alloc_);
    }
    size_t bucket_cord = 0;
    size_t cell_cord = 0;
    size_t cnt = 0;
    try {
      for (size_t i = 0; i < ptr_cnt_; ++i) {
        arr_[i] = alloc_traits::allocate(alloc_, kBucketSize);
      }
      const_iterator iter = other.begin();
      bucket_filler_with_iterator(cnt, iter);
    } catch (...) {
      bucket_deallocator(cnt, 0, ptr_cnt_ - 1);
      size_ = 0;
      ptr_cnt_ = 0;
      head_bucket_ = 0;
      head_cell_ = 0;
      end_bucket_ = 0;
      end_cell_ = -1;
      alloc_ = Allocator();
      throw;
    }
  }

  Deque(const Deque& other) : Deque(other, IsCopyAssigned(false)) {}

  Deque(size_t count, const Allocator& alloc = Allocator())
      : size_(count), alloc_(alloc), buck_alloc_(alloc) {
    size_t cnt = 0;
    try {
      bucket_allocator(count);
      bucket_filler(cnt);
    } catch (...) {
      bucket_deallocator(cnt, 0, ptr_cnt_ - 1);
      size_ = 0;
      alloc_ = Allocator();
      throw;
    }
    end_bucket_ = (size_ - 1) / kBucketSize;
    end_cell_ = (size_ - 1) % kBucketSize;
  }

  Deque(int count, const T& value, const Allocator& alloc = Allocator())
      : size_(count), alloc_(alloc), buck_alloc_(alloc) {
    size_t cnt = 0;
    try {
      bucket_allocator(count);
      bucket_filler(cnt, value);
    } catch (...) {
      bucket_deallocator(cnt, 0, ptr_cnt_ - 1);
      size_ = 0;
      alloc_ = Allocator();
      throw;
    }
    end_bucket_ = (size_ - 1) / kBucketSize;
    end_cell_ = (size_ - 1) % kBucketSize;
  }

  Deque(Deque&& other)
      : arr_(other.arr_),
        ptr_cnt_(other.ptr_cnt_),
        size_(other.size_),
        head_bucket_(other.head_bucket_),
        head_cell_(other.head_cell_),
        end_bucket_(other.end_bucket_),
        end_cell_(other.end_cell_),
        alloc_(std::move(other.alloc_)),
        buck_alloc_(std::move(other.buck_alloc_)) {
    other.buck_alloc_ = bucket_alloc();
    other.alloc_ = Allocator();
    other.arr_ = bucket_alloc_traits::allocate(other.buck_alloc_, 1);
    other.ptr_cnt_ = 1;
    other.arr_[0] = alloc_traits::allocate(other.alloc_, kBucketSize);
    other.size_ = 0;
    other.head_bucket_ = 0;
    other.head_cell_ = 0;
    other.end_bucket_ = 0;
    other.end_cell_ = -1;
  }

  Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : size_(init.size()), alloc_(alloc), buck_alloc_(alloc) {
    size_t cnt = 0;
    try {
      const T* iter = init.begin();
      bucket_allocator(size_);
      bucket_filler_with_iterator(cnt, iter);
    } catch (...) {
      bucket_deallocator(cnt, 0, ptr_cnt_ - 1);
      size_ = 0;
      alloc_ = Allocator();
      throw;
    }
    end_bucket_ = (size_ - 1) / kBucketSize;
    end_cell_ = (size_ - 1) % kBucketSize;
  }

  Deque& operator=(const Deque& other) {
    Deque copy(other, IsCopyAssigned(true));
    my_swap(copy);
    return *this;
  }

  Deque& operator=(Deque&& other) {
    Deque copy = std::move(other);
    my_swap(copy);
    return *this;
  }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  T& operator[](size_t index) {
    return arr_[head_bucket_ + (head_cell_ + index) / kBucketSize]
               [(head_cell_ + index) % kBucketSize];
  }

  const T& operator[](size_t index) const {
    return arr_[head_bucket_ + (head_cell_ + index) / kBucketSize]
               [(head_cell_ + index) % kBucketSize];
  }

  T& at(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("out of range!!!");
    }
    return arr_[head_bucket_ + (head_cell_ + index) / kBucketSize]
               [(head_cell_ + index) % kBucketSize];
  }

  const T& at(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("out of range!!!");
    }
    return arr_[head_bucket_ + (head_cell_ + index) / kBucketSize]
               [(head_cell_ + index) % kBucketSize];
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    if ((end_bucket_ == ptr_cnt_ - 1) && (end_cell_ == kBucketSize - 1)) {
      memory_helper<Args...>(std::forward<Args>(args)..., true);
    } else if (end_cell_ == kBucketSize - 1) {
      try {
        alloc_traits::construct(alloc_, arr_[end_bucket_ + 1],
                                std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      ++end_bucket_;
      end_cell_ = 0;
      ++size_;
    } else {
      try {
        alloc_traits::construct(alloc_, arr_[end_bucket_] + end_cell_ + 1,
                                std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      ++end_cell_;
      ++size_;
    }
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    if ((head_bucket_ == 0) && (head_cell_ == 0)) {
      memory_helper<Args...>(std::forward<Args>(args)..., false);
    } else if (head_cell_ == 0) {
      try {
        alloc_traits::construct(alloc_,
                                arr_[head_bucket_ - 1] + kBucketSize - 1,
                                std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      --head_bucket_;
      head_cell_ = kBucketSize - 1;
      ++size_;
    } else {
      try {
        alloc_traits::construct(alloc_, arr_[head_bucket_] + head_cell_ - 1,
                                std::forward<Args>(args)...);
      } catch (...) {
        throw;
      }
      --head_cell_;
      ++size_;
    }
  }

  void push_back(const T& value) { emplace_back(value); }

  void push_back(T&& value) { emplace_back(std::move(value)); }

  void push_front(const T& value) { emplace_front(value); }

  void push_front(T&& value) { emplace_front(std::move(value)); }

  void pop_back() {
    T* ptr = arr_[end_bucket_] + end_cell_;
    alloc_traits::destroy(alloc_, ptr);
    end_cell_ = (kBucketSize + (--end_cell_) % kBucketSize) % kBucketSize;
    end_bucket_ -= (end_cell_ + 1) / kBucketSize;
    --size_;
  }

  void pop_front() {
    T* ptr = arr_[head_bucket_] + head_cell_;
    alloc_traits::destroy(alloc_, ptr);
    head_bucket_ = (++head_cell_) / kBucketSize;
    head_cell_ %= kBucketSize;
    --size_;
  }

  Allocator get_allocator() const { return alloc_; }

  template <bool IsConst>
  struct PreIterator {
   private:
    T** bucket_ptr_;
    int bucket_;
    int cell_;
    const size_t kBucketSize = 32;

    friend struct Deque<T, Allocator>::PreIterator<!IsConst>;

   public:
    using difference_type = int;
    using value_type = T;
    using reference = std::conditional_t<IsConst, const T&, T&>;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using iterator_category = std::random_access_iterator_tag;

    operator PreIterator<true>() const {
      return PreIterator<true>(bucket_ptr_, bucket_, cell_);
    }

    PreIterator(T** ptr, size_t bucket, size_t cell)
        : bucket_ptr_(ptr), bucket_(bucket), cell_(cell) {}

    PreIterator(const PreIterator& other)
        : bucket_ptr_(other.bucket_ptr_),
          bucket_(other.bucket_),
          cell_(other.cell_) {}

    PreIterator& operator=(const PreIterator& other) {
      bucket_ptr_ = other.bucket_ptr_;
      bucket_ = other.bucket_;
      cell_ = other.cell_;
      return *this;
    }

    reference operator*() const { return bucket_ptr_[bucket_][cell_]; }

    pointer operator->() const { return &(bucket_ptr_[bucket_][cell_]); }

    template <bool V>
    bool operator<(const PreIterator<V>& other) const {
      if (bucket_ > other.bucket_) {
        return false;
      }
      if (bucket_ == other.bucket_) {
        return cell_ < other.cell_;
      }
      return true;
    }

    template <bool V>
    bool operator>(const PreIterator<V>& other) const {
      return (other < *this);
    }

    template <bool V>
    bool operator==(const PreIterator<V>& other) const {
      return !(*this < other || other < *this);
    }

    template <bool V>
    bool operator<=(const PreIterator<V>& other) const {
      return (*this < other || *this == other);
    }

    template <bool V>
    bool operator>=(const PreIterator<V>& other) const {
      return (*this > other || *this == other);
    }

    template <bool V>
    bool operator!=(const PreIterator<V>& other) const {
      return !(*this == other);
    }

    PreIterator& operator+=(int num) {
      if (num < 0) {
        return *this -= (-num);
      }
      bucket_ += (cell_ + num) / kBucketSize;
      cell_ = (cell_ + num) % kBucketSize;
      return *this;
    }

    PreIterator& operator-=(int num) {
      if (num < 0) {
        return *this += (-num);
      }
      cell_ = (kBucketSize + (cell_ - num) % kBucketSize) % kBucketSize;
      bucket_ -= (cell_ + num) / kBucketSize;
      return *this;
    }

    PreIterator& operator++() {
      *this += 1;
      return *this;
    }

    PreIterator& operator--() {
      *this -= 1;
      return *this;
    }

    PreIterator operator++(int) {
      PreIterator copy(*this);
      ++*this;
      return copy;
    }

    PreIterator operator--(int) {
      PreIterator copy(*this);
      --*this;
      return copy;
    }

    PreIterator operator-(int num) const {
      PreIterator copy(*this);
      copy -= num;
      return copy;
    }

    PreIterator operator+(int num) const {
      PreIterator copy(*this);
      copy += num;
      return copy;
    }

    template <bool V>
    int operator-(const PreIterator<V>& other) const {
      if (bucket_ == other.bucket_) {
        return cell_ - other.cell_;
      }
      int diff = kBucketSize * (bucket_ - other.bucket_ - 1) +
                 (kBucketSize - other.cell_ + cell_);
      if (*this > other) {
        return diff;
      }
      return -diff;
    }
  };

  using iterator = PreIterator<false>;
  using const_iterator = PreIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(arr_, head_bucket_, head_cell_); }

  iterator end() {
    return iterator(arr_, (end_bucket_ + (end_cell_ + 1) / kBucketSize),
                    (end_cell_ + 1) % kBucketSize);
  }

  const_iterator begin() const {
    return const_iterator(arr_, head_bucket_, head_cell_);
  }

  const_iterator end() const {
    return const_iterator(arr_, (end_bucket_ + (end_cell_ + 1) / kBucketSize),
                          (end_cell_ + 1) % kBucketSize);
  }

  const_iterator cbegin() const {
    return const_iterator(arr_, head_bucket_, head_cell_);
  }

  const_iterator cend() const {
    return const_iterator(arr_, (end_bucket_ + (end_cell_ + 1) / kBucketSize),
                          (end_cell_ + 1) % kBucketSize);
  }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator crbegin() { return const_reverse_iterator(cend()); }

  const_reverse_iterator crend() { return const_reverse_iterator(cbegin()); }

  template <typename... Args>
  void emplace(const_iterator iter, Args&&... args);

  void insert(const_iterator iter, const T& value) { emplace(iter, value); }

  void erase(const_iterator iter);

  ~Deque() { bucket_deallocator(size_, 0, ptr_cnt_ - 1); }
};

template <typename T, typename Allocator>
template <typename Iterator>
void Deque<T, Allocator>::bucket_filler_with_iterator(size_t& cnt,
                                                      Iterator iter) {
  for (; cnt < size_; ++cnt, ++iter) {
    T* ptr = arr_[head_bucket_ + ((head_cell_ + cnt) / kBucketSize)] +
             ((head_cell_ + cnt) % kBucketSize);
    alloc_traits::construct(alloc_, ptr, *iter);
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::bucket_filler(size_t& cnt, const T& value) {
  for (; cnt < size_; ++cnt) {
    T* ptr = arr_[head_bucket_ + ((head_cell_ + cnt) / kBucketSize)] +
             ((head_cell_ + cnt) % kBucketSize);
    alloc_traits::construct(alloc_, ptr, value);
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::bucket_filler(size_t& cnt) {
  for (; cnt < size_; ++cnt) {
    T* ptr = arr_[head_bucket_ + ((head_cell_ + cnt) / kBucketSize)] +
             ((head_cell_ + cnt) % kBucketSize);
    alloc_traits::construct(alloc_, ptr);
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::bucket_allocator(size_t count) {
  ptr_cnt_ = (count + kBucketSize - 1) / kBucketSize;
  arr_ = bucket_alloc_traits::allocate(buck_alloc_, ptr_cnt_);
  for (size_t i = 0; i < ptr_cnt_; ++i) {
    arr_[i] = alloc_traits::allocate(alloc_, kBucketSize);
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::bucket_deallocator(size_t until, size_t head,
                                             size_t end) {
  for (size_t i = 0; i < until; ++i) {
    T* ptr = arr_[head_bucket_ + ((head_cell_ + i) / kBucketSize)] +
             ((head_cell_ + i) % kBucketSize);
    alloc_traits::destroy(alloc_, ptr);
  }
  if (ptr_cnt_ != 0) {
    for (size_t i = head; i <= end; ++i) {
      alloc_traits::deallocate(alloc_, arr_[i], kBucketSize);
    }
  }
  bucket_alloc_traits::deallocate(buck_alloc_, arr_, ptr_cnt_);
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::my_swap(Deque& other) {
  std::swap(arr_, other.arr_);
  std::swap(ptr_cnt_, other.ptr_cnt_);
  std::swap(size_, other.size_);
  std::swap(head_bucket_, other.head_bucket_);
  std::swap(head_cell_, other.head_cell_);
  std::swap(end_bucket_, other.end_bucket_);
  std::swap(end_cell_, other.end_cell_);
  std::swap(alloc_, other.alloc_);
  std::swap(buck_alloc_, other.buck_alloc_);
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::memory_helper(Args&&... args, bool is_end) {
  size_t shift = 1 + ((2 * ptr_cnt_ - (end_bucket_ - head_bucket_ + 1)) / 2);
  T** tmp_arr = bucket_alloc_traits::allocate(buck_alloc_, 2 * ptr_cnt_ + 1);
  for (size_t i = 0; i < end_bucket_ - head_bucket_ + 1; ++i) {
    tmp_arr[i + shift] = &*arr_[i + head_bucket_];
  }
  size_t old_end1 = end_bucket_;
  size_t old_head1 = head_bucket_;
  end_bucket_ = (end_bucket_ - head_bucket_) + shift;
  head_bucket_ = shift;
  for (size_t i = 0; i < head_bucket_; ++i) {
    tmp_arr[i] = alloc_traits::allocate(alloc_, kBucketSize);
  }
  for (size_t i = end_bucket_ + 1; i < 2 * ptr_cnt_ + 1; ++i) {
    tmp_arr[i] = alloc_traits::allocate(alloc_, kBucketSize);
  }

  // Choosing place to add, depending on 'is_end' bool argument.
  size_t bucket_cord = is_end ? end_bucket_ + 1 : head_bucket_ - 1;
  size_t cell_cord = is_end ? 0 : kBucketSize - 1;
  T* where_to_construct = tmp_arr[bucket_cord] + cell_cord;

  try {
    alloc_traits::construct(alloc_, where_to_construct,
                            std::forward<Args>(args)...);
  } catch (...) {
    for (size_t i = 0; i < head_bucket_; ++i) {
      alloc_traits::deallocate(alloc_, tmp_arr[i], kBucketSize);
    }
    for (size_t i = end_bucket_ + 1; i < 2 * ptr_cnt_ + 1; ++i) {
      alloc_traits::deallocate(alloc_, tmp_arr[i], kBucketSize);
    }
    bucket_alloc_traits::deallocate(buck_alloc_, tmp_arr, 2 * ptr_cnt_ + 1);
    head_bucket_ = old_head1;
    end_bucket_ = old_end1;
    throw;
  }
  for (size_t i = 0; i < old_head1; ++i) {
    alloc_traits::deallocate(alloc_, arr_[i], kBucketSize);
  }
  for (size_t i = old_end1 + 1; i < ptr_cnt_; ++i) {
    alloc_traits::deallocate(alloc_, arr_[i], kBucketSize);
  }
  bucket_alloc_traits::deallocate(buck_alloc_, arr_, ptr_cnt_);
  is_end ? (++end_bucket_, end_cell_ = 0)
         : (--head_bucket_, head_cell_ = kBucketSize - 1);
  ++(ptr_cnt_ *= 2);
  ++size_;
  arr_ = tmp_arr;
}

template <typename T, typename Allocator>
template <typename... Args>
void Deque<T, Allocator>::emplace(const_iterator iter, Args&&... args) {
  if (iter == end()) {
    emplace_back(std::forward<Args>(args)...);
  } else if (iter == begin()) {
    emplace_front(std::forward<Args>(args)...);
  } else {
    // Working with indexes, because of iterators invalidation.
    // Getting index of element where iterator points to.
    size_t index = iter - begin();
    size_t diff = end() - iter;
    emplace_back(std::forward<Args>(args)...);
    size_t current = size_ - 1;
    size_t previous;
    size_t cnt = 0;
    try {
      for (; cnt < diff; ++cnt) {
        previous = current - 1;
        (*this)[current] = std::move_if_noexcept((*this)[previous]);
        --current;
      }
      alloc_traits::construct(alloc_, &(*this)[index],
                              std::forward<Args>(args)...);
    } catch (...) {
      for (size_t j = 0; j < cnt; ++j) {
        previous = current;
        ++current;
        (*this)[previous] = std::move_if_noexcept((*this)[current]);
      }
      --size_;
      end_cell_ = (kBucketSize + ((--end_cell_) % kBucketSize)) % kBucketSize;
      end_bucket_ -= (end_cell_ + 1) / kBucketSize;
      throw;
    }
  }
}

template <typename T, typename Allocator>
void Deque<T, Allocator>::erase(const_iterator iter) {
  T safe = std::move_if_noexcept(*iter);

  size_t index = iter - begin();
  size_t diff = end() - iter;
  --diff;

  size_t current = index;
  size_t next;

  size_t cnt = 0;
  try {
    for (; cnt < diff; ++cnt) {
      next = current + 1;
      (*this)[current] = std::move_if_noexcept((*this)[next]);
      ++current;
    }
  } catch (...) {
    throw;
  }
  end_cell_ = (kBucketSize + ((--end_cell_) % kBucketSize)) % kBucketSize;
  end_bucket_ -= (end_cell_ + 1) / kBucketSize;
  --size_;
}
