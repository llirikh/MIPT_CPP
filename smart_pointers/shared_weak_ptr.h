#include <memory>

struct BaseControlBlock {
  std::size_t shared_count = 0;
  std::size_t weak_count = 0;

  virtual ~BaseControlBlock() = default;

  virtual void* get_ptr() = 0;
  virtual void destroy() = 0;
  virtual void deallocate() = 0;
};

template <typename T, typename D, typename A>
struct ControlBlockRegular : public BaseControlBlock {
  T* ptr = std::nullptr_t();
  D deleter;
  A alloc;

  ControlBlockRegular(T* object, const D& deleter, const A& alloc)
      : ptr(object), deleter(deleter), alloc(alloc) {}

  void* get_ptr() override;
  void destroy() override;
  void deallocate() override;
};

template <typename T, typename A>
struct ControlBlockMakeShared : public BaseControlBlock {
  A alloc;
  T object;

  template <typename... Args>
  ControlBlockMakeShared(A alloc, Args&&... args)
      : alloc(alloc), object(std::forward<Args>(args)...) {}

  void* get_ptr() override;
  void destroy() override;
  void deallocate() override;
};

template <typename T>
class SharedPtr {
public:
  SharedPtr() {}
  SharedPtr(BaseControlBlock* control_block);
  SharedPtr(const SharedPtr& other) noexcept;
  SharedPtr(SharedPtr&& other) noexcept;
  template <typename D = std::default_delete<T>, typename A = std::allocator<T>>
  SharedPtr(T* ptr, const D& deleter = D(), const A& alloc = A());
  template <typename U>
  SharedPtr(const SharedPtr<U>& other) noexcept;
  template <typename U>
  SharedPtr(SharedPtr<U>&& other) noexcept;
  ~SharedPtr();

  const SharedPtr& operator=(const SharedPtr& other);
  const SharedPtr& operator=(SharedPtr&& other);
  template <typename U>
  const SharedPtr& operator=(const SharedPtr<U>& other);
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other);

  T& operator*() const;
  T* operator->() const;

  std::size_t use_count() const noexcept;
  void reset();
  T* get() const;
  template <typename U>
  void reset(U* ptr);
  template <typename U>
  void swap(SharedPtr<U>& other);

private:
  template <typename U, typename... Args>
  friend auto makeShared(Args&&... args);  // NOLINT

  template <typename U, typename... Args, typename A>
  friend auto allocateShared(Args&&... args, const A& alloc);  // NOLINT

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  BaseControlBlock* control_block_ = std::nullptr_t();
  T* ptr_ = std::nullptr_t();

  void decrement_count();
  void increment_count();
};

template <typename T>
class WeakPtr {
public:
  WeakPtr() {}
  WeakPtr(const WeakPtr<T>& other);
  WeakPtr(const SharedPtr<T>& other);
  template <typename U>
  WeakPtr(const WeakPtr<U>& other);
  template <typename U>
  WeakPtr(WeakPtr<U>&& other);
  template <typename U>
  WeakPtr(const SharedPtr<U>& other);
  ~WeakPtr();

  const WeakPtr& operator=(const SharedPtr<T>& other);
  template <typename U>
  const WeakPtr& operator=(const WeakPtr<U>& other);
  template <typename U>
  const WeakPtr& operator=(const SharedPtr<U>& other);

  bool expired() const;
  auto lock() const;
  std::size_t use_count() const;

private:
  template <typename U>
  friend class WeakPtr;

  BaseControlBlock* control_block_ = std::nullptr_t();

  void increment_count();
  void decrement_count();
};

// Control block regular methods
template <typename T, typename D, typename A>
void* ControlBlockRegular<T, D, A>::get_ptr() {
  return reinterpret_cast<void*>(ptr);
}

template <typename T, typename D, typename A>
void ControlBlockRegular<T, D, A>::destroy() {
  deleter(ptr);
  ptr = std::nullptr_t();
}

template <typename T, typename D, typename A>
void ControlBlockRegular<T, D, A>::deallocate() {
  using alloc_traits = typename std::allocator_traits<A>::template rebind_alloc<
      ControlBlockRegular<T, D, A>>;
  alloc_traits(alloc).deallocate(this, 1);
}

// Control block make shared methods
template <typename T, typename A>
void* ControlBlockMakeShared<T, A>::get_ptr() {
  return &object;
}

template <typename T, typename A>
void ControlBlockMakeShared<T, A>::destroy() {
  std::allocator_traits<A>::destroy(alloc, reinterpret_cast<T*>(get_ptr()));
}

template <typename T, typename A>
void ControlBlockMakeShared<T, A>::deallocate() {
  using alloc_traits = typename std::allocator_traits<A>::template rebind_alloc<
      ControlBlockMakeShared<T, A>>;
  alloc_traits(alloc).deallocate(this, 1);
}

// Shared pointer methods
template <typename T>
void SharedPtr<T>::increment_count() {
  this->control_block_->shared_count++;
}

template <typename T>
void SharedPtr<T>::decrement_count() {
  this->control_block_->shared_count--;
}

template <typename T>
SharedPtr<T>::SharedPtr(BaseControlBlock* control_block)
    : control_block_(control_block),
      ptr_(reinterpret_cast<T*>(control_block->get_ptr())) {
  increment_count();
}

template <typename T>
template <typename D, typename A>
SharedPtr<T>::SharedPtr(T* ptr, const D& deleter, const A& alloc)
    : ptr_(reinterpret_cast<T*>(ptr)) {
  using alloc_traits = std::allocator_traits<A>;
  using control_block_alloc = typename alloc_traits::template rebind_alloc<
      ControlBlockRegular<T, D, A>>;
  using control_block_alloc_traits =
      typename alloc_traits::template rebind_traits<
          ControlBlockRegular<T, D, A>>;

  control_block_alloc new_alloc = alloc;
  auto new_ptr = control_block_alloc_traits::allocate(new_alloc, 1);
  new (new_ptr) ControlBlockRegular<T, D, A>(ptr, deleter, new_alloc);
  control_block_ = new_ptr;
  control_block_->shared_count = 1;
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) noexcept
    : control_block_(other.control_block_), ptr_(other.ptr_) {
  if (control_block_ != std::nullptr_t()) {
    increment_count();
  }
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other) noexcept
    : control_block_(reinterpret_cast<BaseControlBlock*>(other.control_block_)),
      ptr_(other.ptr_) {
  increment_count();
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) noexcept
    : control_block_(other.control_block_), ptr_(other.ptr_) {
  other.ptr_ = std::nullptr_t();
  other.control_block_ = std::nullptr_t();
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other) noexcept
    : control_block_(other.control_block_), ptr_(other.ptr_) {
  other.ptr_ = std::nullptr_t();
  other.control_block_ = std::nullptr_t();
}

template <typename T>
template <typename U>
void SharedPtr<T>::swap(SharedPtr<U>& other) {
  std::swap(ptr_, other.ptr_);
  std::swap(control_block_, other.control_block_);
}

template <typename T>
const SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) {
  this->~SharedPtr();
  ptr_ = other.ptr_;
  control_block_ = reinterpret_cast<BaseControlBlock*>(other.control_block_);
  increment_count();
  return *this;
}

template <typename T>
template <typename U>
const SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& other) {
  this->~SharedPtr();
  ptr_ = other.ptr_;
  control_block_ = reinterpret_cast<BaseControlBlock*>(other.control_block_);
  increment_count();
  return *this;
}

template <typename T>
const SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) {
  this->~SharedPtr();
  ptr_ = std::move(other.ptr_);
  control_block_ =
      std::move(reinterpret_cast<BaseControlBlock*>(other.control_block_));
  other.ptr_ = std::nullptr_t();
  other.control_block_ = std::nullptr_t();
  return *this;
}

template <typename T>
template <typename U>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<U>&& other) {
  this->~SharedPtr();
  ptr_ = std::move(other.ptr_);
  control_block_ =
      std::move(reinterpret_cast<BaseControlBlock*>(other.control_block_));
  other.ptr_ = std::nullptr_t();
  other.control_block_ = std::nullptr_t();
  return *this;
}

template <typename T>
T& SharedPtr<T>::operator*() const {
  if (ptr_) {
    return *ptr_;
  }
  return *reinterpret_cast<T*>(control_block_->get_ptr());
}

template <typename T>
std::size_t SharedPtr<T>::use_count() const noexcept {
  return control_block_->shared_count;
}

template <typename T>
void SharedPtr<T>::reset() {
  *this = SharedPtr<T>();
}

template <typename T>
template <typename U>
void SharedPtr<T>::reset(U* ptr) {
  *this = ptr;
}

template <typename T>
T* SharedPtr<T>::get() const {
  if (ptr_) {
    return ptr_;
  }
  if (ptr_ == std::nullptr_t() && control_block_ == std::nullptr_t()) {
    return std::nullptr_t();
  }
  return reinterpret_cast<T*>(control_block_->get_ptr());
}

template <typename T>
T* SharedPtr<T>::operator->() const {
  return get();
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  if (control_block_ == std::nullptr_t() || control_block_->shared_count == 0) {
    return;
  }
  decrement_count();
  if (control_block_->shared_count == 0 && control_block_->weak_count == 0) {
    control_block_->destroy();
    control_block_->deallocate();
  } else if (control_block_->shared_count == 0) {
    control_block_->destroy();
  }
}

// Weak pointers methods
template <typename T>
void WeakPtr<T>::increment_count() {
  control_block_->weak_count++;
}

template <typename T>
void WeakPtr<T>::decrement_count() {
  control_block_->weak_count--;
}

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr<T>& other)
    : control_block_(
          reinterpret_cast<BaseControlBlock*>(other.control_block_)) {
  increment_count();
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& other)
    : control_block_(
          reinterpret_cast<BaseControlBlock*>(other.control_block_)) {
  increment_count();
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& other)
    : control_block_(
          reinterpret_cast<BaseControlBlock*>(other.control_block_)) {
  other.control_block_ = std::nullptr_t();
}

template <typename T>
WeakPtr<T>::WeakPtr(const SharedPtr<T>& other)
    : control_block_(other.control_block_) {
  increment_count();
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& other)
    : control_block_(
          reinterpret_cast<BaseControlBlock*>(other.control_block_)) {
  increment_count();
}

template <typename T>
template <typename U>
const WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<U>& other) {
  this->~WeakPtr();
  control_block_ = reinterpret_cast<BaseControlBlock*>(other.control_block_);
  control_block_->weak_count++;
  return *this;
}

template <typename T>
const WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<T>& other) {
  this->~WeakPtr();
  control_block_ = reinterpret_cast<BaseControlBlock*>(other.control_block_);
  increment_count();
  return *this;
}

template <typename T>
template <typename U>
const WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<U>& other) {
  this->~WeakPtr();
  control_block_ = reinterpret_cast<BaseControlBlock*>(other.control_block_);
  increment_count();
  return *this;
}

template <typename T>
bool WeakPtr<T>::expired() const {
  if (control_block_ == std::nullptr_t()) {
    return true;
  }
  return control_block_->shared_count == 0U;
}

template <typename T>
auto WeakPtr<T>::lock() const {
  if (expired()) {
    return SharedPtr<T>();
  }
  return SharedPtr<T>(control_block_);
}

template <typename T>
std::size_t WeakPtr<T>::use_count() const {
  if (expired()) {
    return 0;
  }
  return control_block_->shared_count;
}

template <typename T>
WeakPtr<T>::~WeakPtr() {
  if (control_block_ == std::nullptr_t()) {
    return;
  }
  if (control_block_->weak_count != 0U) {
    decrement_count();
  }
  if (control_block_->shared_count == 0) {
    if (control_block_->weak_count == 0) {
      control_block_->deallocate();
    }
  }
}

// User methods
template <typename V, typename A, typename... Args>
SharedPtr<V> allocateShared(A alloc, Args&&... args) {  // NOLINT
  using alloc_traits = typename std::template allocator_traits<A>;
  using shared_alloc = typename alloc_traits::template rebind_alloc<
      ControlBlockMakeShared<V, A>>;
  using shared_alloc_traits =
      typename std::template allocator_traits<shared_alloc>;

  shared_alloc new_alloc = alloc;
  auto* ptr = shared_alloc_traits::allocate(new_alloc, 1);

  shared_alloc_traits::construct(new_alloc, ptr, std::move(alloc),
                                 std::forward<Args>(args)...);
  return SharedPtr<V>(ptr);
}

template <typename V, typename... Args>
auto makeShared(Args&&... args) {  // NOLINT
  return allocateShared<V>(std::allocator<V>(), std::forward<Args>(args)...);
}
