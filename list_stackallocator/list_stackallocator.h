#pragma once
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>

template <auto N>
struct StackStorage {
  StackStorage() = default;
  StackStorage(const StackStorage<N>& other) = delete;
  StackStorage(const StackStorage<N>&& other) = delete;
  StackStorage operator=(const StackStorage<N>& other) = delete;
  StackStorage operator=(const StackStorage<N>&& other) = delete;
  ~StackStorage() = default;
  char storage[N];
  std::ptrdiff_t top{0};
};

template <typename T, auto N>
struct StackAllocator {
  using value_type = T;
  using pointer = value_type*;

  StackAllocator() = default;
  StackAllocator(StackStorage<N>& storage) : storage(&storage) {}
  StackAllocator(const StackAllocator& other)
      : storage(other.storage), space_left(other.space_left) {}
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other)
      : storage(other.storage), space_left(other.space_left) {}
  StackAllocator& operator=(const StackAllocator& other) {
    auto copy(other);
    std::swap(storage, copy.storage);
    std::swap(space_left, copy.space_left);
    return *this;
  }

  auto allocate(std::size_t size) -> pointer;
  void deallocate(T* /*unused*/, std::size_t /*unused*/) {}

  bool operator==(const StackAllocator& other) const {
    return storage == other.storage;
  }

  template <class U>
  struct rebind {  // NOLINT
    using other = StackAllocator<U, N>;
  };

  StackStorage<N>* storage{nullptr};
  std::size_t space_left{static_cast<size_t>(N) - storage->top};
};

template <typename T, auto N>
auto StackAllocator<T, N>::allocate(std::size_t size) -> pointer {
  void* ptr = storage->storage + storage->top;
  if (std::align(alignof(value_type), sizeof(value_type) * size, ptr,
                 space_left) != nullptr) {
    auto result = reinterpret_cast<pointer>(ptr);
    ptr = reinterpret_cast<char*>(ptr) + sizeof(value_type) * size;
    space_left -= sizeof(value_type) * size;
    storage->top = reinterpret_cast<char*>(ptr) - storage->storage;
    return result;
  }
  throw std::bad_alloc();
}

template <typename T, typename Allocator = std::allocator<T>>
class List {
  struct BaseNode;
  struct Node;
  using allocator_type =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using allocator_traits = std::allocator_traits<allocator_type>;

public:
  List() = default;
  List(const Allocator& alloc) : alloc_(alloc){};
  List(size_t n, const Allocator& alloc = Allocator());
  List(size_t n, const T& val, const Allocator& alloc = Allocator());
  List(const List& list);
  ~List();

  List& operator=(const List& list);

  template <bool IsConst>
  class list_iterator;
  using iterator = list_iterator<false>;
  using const_iterator = list_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  void push_back(const T& val);
  void push_front(const T& val);
  void pop_back();
  void pop_front();

  void insert(const_iterator iter, const T& val);
  void erase(const_iterator iter);

  size_t size() const { return sz_; }
  allocator_type get_allocator() const { return alloc_; }

  iterator begin();
  iterator end();

  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const { return const_iterator(begin_, 0); }
  const_iterator end() const { return cend(); }
  const_iterator cend() const {
    return const_iterator(end_, static_cast<std::ptrdiff_t>(sz_));
  }

  reverse_iterator rbegin() noexcept {
    return std::make_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept {
    return std::make_reverse_iterator(begin());
  }

  const_reverse_iterator rbegin() const noexcept { return crbegin(); }
  const_reverse_iterator crbegin() const noexcept {
    return std::make_reverse_iterator(cend());
  }
  const_reverse_iterator rend() const noexcept { return crend(); }
  const_reverse_iterator crend() const noexcept {
    return std::make_reverse_iterator(cbegin());
  }

private:
  allocator_type alloc_;

  BaseNode sub_node_;
  Node* begin_{static_cast<Node*>(&sub_node_)};
  Node* end_{static_cast<Node*>(&sub_node_)};

  size_t sz_{0};

  void create_node(Node* right);
  void create_node(Node* right, const T& val);
  void delete_node(Node* node);

  void clear();

  void swap(List& list);
};

template <typename T, typename Allocator>
struct List<T, Allocator>::Node : BaseNode {
  T val;

  Node() = default;
  Node(T& val) : val(val){};
};

template <typename T, typename Allocator>
struct List<T, Allocator>::BaseNode {
  Node* prev{nullptr};
  Node* next{nullptr};

  BaseNode() = default;
};

template <typename T, typename Allocator>
void List<T, Allocator>::create_node(List::Node* right) {
  Node* new_node = nullptr;
  try {
    new_node = allocator_traits::allocate(alloc_, 1);
    allocator_traits::construct(alloc_, &new_node->val);
  } catch (...) {
    allocator_traits::deallocate(alloc_, new_node, 1);
    throw;
  }

  if (right == begin_) {
    begin_ = new_node;
  }
  new_node->next = right;
  new_node->prev = right->prev;
  if (right->prev != nullptr) {
    right->prev->next = new_node;
  }
  right->prev = new_node;

  ++sz_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::create_node(List::Node* right, const T& val) {
  Node* new_node = nullptr;
  try {
    new_node = allocator_traits::allocate(alloc_, 1);
    allocator_traits::construct(alloc_, &new_node->val, val);
  } catch (...) {
    allocator_traits::deallocate(alloc_, new_node, 1);
    throw;
  }

  if (right == begin_) {
    begin_ = new_node;
  }
  new_node->next = right;
  new_node->prev = right->prev;
  if (right->prev != nullptr) {
    right->prev->next = new_node;
  }
  right->prev = new_node;

  ++sz_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::delete_node(List::Node* node) {
  if (node == begin_) {
    begin_ = node->next;
  }

  if (node->prev != nullptr) {
    node->prev->next = node->next;
  }
  if (node->next != nullptr) {
    node->next->prev = node->prev;
  }

  allocator_traits::destroy(alloc_, &node->val);
  allocator_traits::deallocate(alloc_, node, 1);

  --sz_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::clear() {
  while (sz_ > 0) {
    delete_node(begin_);
  }
}

template <typename T, typename Allocator>
void List<T, Allocator>::swap(List<T, Allocator>& list) {
  std::swap(alloc_, list.alloc_);
  std::swap(sub_node_, list.sub_node_);
  std::swap(begin_, list.begin_);
  std::swap(end_, list.end_);
  std::swap(sz_, list.sz_);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n, const Allocator& alloc) : alloc_(alloc) {
  while (sz_ != n) {
    try {
      create_node(end_);
    } catch (...) {
      clear();
      throw;
    }
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t n, const T& val, const Allocator& alloc)
    : alloc_(alloc) {
  while (sz_ != n) {
    try {
      create_node(end_, val);
    } catch (...) {
      clear();
      throw;
    }
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& list)
    : List(allocator_traits::select_on_container_copy_construction(
          list.get_allocator())) {
  for (auto& item : list) {
    try {
      push_back(item);
    } catch (...) {
      clear();
      throw;
    }
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::~List() {
  clear();
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& list) {
  auto tmp(list);
  if (allocator_traits::propagate_on_container_copy_assignment::value) {
    tmp.alloc_ = list.alloc_;
  }
  swap(tmp);
  return *this;
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& val) try {
  create_node(end_, val);
} catch (...) {
  throw;
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& val) try {
  create_node(begin_, val);
} catch (...) {
  throw;
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  delete_node(end_->prev);
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  delete_node(begin_);
}

template <typename T, typename Allocator>
void List<T, Allocator>::erase(List::const_iterator iter) {
  delete_node(iter.operator->());
}

template <typename T, typename Allocator>
void List<T, Allocator>::insert(List::const_iterator iter, const T& val) {
  create_node(iter.operator->(), val);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
  return {begin_, 0};
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() {
  return {end_, static_cast<std::ptrdiff_t>(sz_)};
}

template <typename T, typename Allocator>
template <bool IsConst>
class List<T, Allocator>::list_iterator {
public:
  using value_type = std::conditional_t<IsConst, const T, T>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::bidirectional_iterator_tag;

  list_iterator(Node* node, difference_type shift)
      : node_(node), shift_(shift) {}

  operator const_iterator() const { return const_iterator(node_, shift_); }
  reference operator*() { return node_->val; }
  Node* operator->() { return node_; }

  auto& operator+=(difference_type n);
  auto& operator-=(difference_type n);
  auto& operator++();
  auto& operator--();
  auto operator++(int);
  auto operator--(int);
  auto operator+(difference_type n) const;
  auto operator-(difference_type n) const;
  auto operator-(const list_iterator& iter) const;

  std::strong_ordering operator<=>(const list_iterator& iter) const;
  bool operator==(const list_iterator& iter) const {
    return std::is_eq(*this <=> iter);
  }

private:
  Node* node_;
  difference_type shift_;
};

template <typename T, typename Allocator>
template <bool IsConst>
auto& List<T, Allocator>::list_iterator<IsConst>::operator+=(
    difference_type n) {
  for (size_t i = 0; i < static_cast<size_t>(std::abs(n)); ++i) {
    if (n > 0) {
      node_ = node_->next;
      ++shift_;
    } else {
      node_ = node_->prev;
      --shift_;
    }
  }
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto& List<T, Allocator>::list_iterator<IsConst>::operator-=(
    difference_type n) {
  return *this += -n;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto& List<T, Allocator>::list_iterator<IsConst>::operator++() {
  return *this += 1;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto& List<T, Allocator>::list_iterator<IsConst>::operator--() {
  return *this -= 1;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto List<T, Allocator>::list_iterator<IsConst>::operator++(int) {
  list_iterator tmp(*this);
  *this += 1;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto List<T, Allocator>::list_iterator<IsConst>::operator--(int) {
  list_iterator tmp(*this);
  *this -= 1;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto List<T, Allocator>::list_iterator<IsConst>::operator+(
    difference_type n) const {
  list_iterator tmp(*this);
  tmp += n;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto List<T, Allocator>::list_iterator<IsConst>::operator-(
    difference_type n) const {
  list_iterator tmp(*this);
  tmp -= n;
  return tmp;
}

template <typename T, typename Allocator>
template <bool IsConst>
auto List<T, Allocator>::list_iterator<IsConst>::operator-(
    const list_iterator& iter) const {
  return shift_ - iter.shift_;
}

template <typename T, typename Allocator>
template <bool IsConst>
std::strong_ordering List<T, Allocator>::list_iterator<IsConst>::operator<=>(
    const list_iterator& iter) const {
  if (shift_ < iter.shift_) {
    return std::strong_ordering::less;
  }
  if (shift_ > iter.shift_) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}