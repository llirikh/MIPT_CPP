#pragma once
#include <cstdio>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

template <typename T>
class Deque {
public:
  Deque() = default;
  Deque(const Deque<T>& deque);
  Deque(size_t n);
  Deque(size_t n, const T& val);
  ~Deque();

  Deque& operator=(const Deque& deque);

  T& operator[](size_t idx);
  const T& operator[](size_t idx) const;
  T& at(size_t idx);
  const T& at(size_t idx) const;

  void push_back(const T& val);
  void push_front(const T& val);
  void pop_back();
  void pop_front();

  size_t size() const;

  template <bool IsConst>
  class deque_iterator;
  using iterator = deque_iterator<false>;
  using const_iterator = deque_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin();
  iterator end();

  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const;
  const_iterator end() const { return cend(); }
  const_iterator cend() const;

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const noexcept { return crbegin(); }
  const_reverse_iterator crbegin() const noexcept { return {cend()}; }
  const_reverse_iterator rend() const noexcept { return crend(); }
  const_reverse_iterator crend() const noexcept { return {cbegin()}; }

  void erase(iterator iter);
  void insert(iterator iter, const T& val);

private:
  static const size_t kBuffSz = 8;
  std::vector<T*> map_;
  std::pair<size_t, size_t> begin_{0, 0};
  std::pair<size_t, size_t> end_{0, 0};

  void resize(size_t size);
  void swap(Deque& deque);
  void plus(std::pair<size_t, size_t>& pair);
  void minus(std::pair<size_t, size_t>& pair);
};

template <typename T>
Deque<T>::Deque(size_t n, const T& val) {
  resize(4 * (n + kBuffSz) / kBuffSz);
  try {
    for (size_t i = 0; i < n; ++i) {
      new (map_[end_.first] + end_.second) T(val);
      plus(end_);
    }
  } catch (...) {
    for (size_t j = 0; j < size(); ++j) {
      operator[](j).~T();
    }
    for (T* item : map_) {
      delete[] reinterpret_cast<char*>(item);
    }
    this->end_ = this->begin_;
    throw;
  }
}

template <typename T>
Deque<T>::Deque(size_t n) try : Deque(n, T()) {
} catch (...) {
  throw;
}

template <typename T>
Deque<T>::Deque(const Deque<T>& deque)
    : map_(deque.map_.size()), begin_(deque.begin_), end_(deque.end_) {
  for (size_t i = 0; i < map_.size(); ++i) {
    map_[i] = reinterpret_cast<T*>(new char[kBuffSz * sizeof(T)]);
  }
  for (size_t i = 0; i < deque.size(); ++i) {
    new (&operator[](i)) T(deque[i]);
  }
}

template <typename T>
Deque<T>::~Deque() {
  for (size_t i = 0; i < size(); ++i) {
    operator[](i).~T();
  }
  for (T* item : map_) {
    delete[] reinterpret_cast<char*>(item);
  }
}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque& deque) {
  Deque tmp(deque);
  swap(tmp);
  return *this;
}

template <typename T>
T& Deque<T>::operator[](size_t idx) {
  return map_[begin_.first + (begin_.second + idx) / kBuffSz]
             [(begin_.second + idx) % kBuffSz];
}

template <typename T>
const T& Deque<T>::operator[](size_t idx) const {
  return map_[begin_.first + (begin_.second + idx) / kBuffSz]
             [(begin_.second + idx) % kBuffSz];
}

template <typename T>
T& Deque<T>::at(size_t idx) {
  if (idx >= size()) {
    throw std::out_of_range("criminal scum");
  }
  return operator[](idx);
}

template <typename T>
const T& Deque<T>::at(size_t idx) const {
  if (idx >= size()) {
    throw std::out_of_range("criminal scum");
  }
  return operator[](idx);
}

template <typename T>
void Deque<T>::push_back(const T& val) {
  if ((end_.first == map_.size() - 1 && end_.second == kBuffSz - 1) ||
      size() == 0) {
    try {
      resize(2 * (map_.size() + 1));
    } catch (...) {
      throw;
    }
  }
  new (map_[end_.first] + end_.second) T(val);
  plus(end_);
}

template <typename T>
void Deque<T>::push_front(const T& val) {
  if ((begin_.first == 0 && begin_.second == 0) || size() == 0) {
    try {
      resize(2 * (map_.size() + 1));
    } catch (...) {
      throw;
    }
  }
  minus(begin_);
  new (map_[begin_.first] + begin_.second) T(val);
}

template <typename T>
void Deque<T>::pop_back() {
  operator[](size() - 1).~T();
  minus(end_);
}

template <typename T>
void Deque<T>::pop_front() {
  operator[](0).~T();
  plus(begin_);
}

template <typename T>
size_t Deque<T>::size() const {
  return (kBuffSz - begin_.second) + (end_.second) +
         kBuffSz * (end_.first - begin_.first - 1);
}

template <typename T>
typename Deque<T>::iterator Deque<T>::begin() {
  if (map_.empty()) {
    return {nullptr, nullptr, nullptr, nullptr};
  }
  return {*(map_.data() + begin_.first) + begin_.second,
          *(map_.data() + begin_.first),
          *(map_.data() + begin_.first) + kBuffSz, map_.data() + begin_.first};
}

template <typename T>
typename Deque<T>::iterator Deque<T>::end() {
  if (map_.empty()) {
    return {nullptr, nullptr, nullptr, nullptr};
  }
  return {*(map_.data() + end_.first) + end_.second,
          *(map_.data() + end_.first), *(map_.data() + end_.first) + kBuffSz,
          map_.data() + end_.first};
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
  if (map_.empty()) {
    return {nullptr, nullptr, nullptr, nullptr};
  }
  return {*(map_.data() + begin_.first) + begin_.second,
          *(map_.data() + begin_.first),
          *(map_.data() + begin_.first) + kBuffSz, map_.data() + begin_.first};
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
  if (map_.empty()) {
    return {nullptr, nullptr, nullptr, nullptr};
  }
  return {*(map_.data() + end_.first) + end_.second,
          *(map_.data() + end_.first), *(map_.data() + end_.first) + kBuffSz,
          map_.data() + end_.first};
}

template <typename T>
void Deque<T>::erase(iterator iter) {
  for (iterator i = iter; i < end() - 1; ++i) {
    *(i) = *(i + 1);
  }
  pop_back();
}

template <typename T>
void Deque<T>::insert(iterator iter, const T& val) {
  T tmp(val);
  for (iterator i = iter; i < end(); ++i) {
    std::swap(*(i), tmp);
  }
  push_back(tmp);
}

template <typename T>
void Deque<T>::resize(size_t size) {
  if (size <= map_.size()) {
    return;
  }
  std::vector<T*> new_map(size);
  size_t shift = (size - map_.size()) / 2;
  size_t i_mem = 0;
  try {
    for (; i_mem < size; ++i_mem) {
      if (i_mem - shift < map_.size()) {
        new_map[i_mem] = map_[i_mem - shift];
      } else {
        new_map[i_mem] = reinterpret_cast<T*>(new char[kBuffSz * sizeof(T)]);
      }
    }
  } catch (...) {
    for (size_t j = 0; j < i_mem; ++j) {
      if (j - shift > map_.size()) {
        delete[] reinterpret_cast<char*>(new_map[i_mem]);
      }
    }
    throw;
  }
  begin_.first += shift;
  end_.first += shift;
  std::swap(map_, new_map);
}

template <typename T>
void Deque<T>::swap(Deque<T>& deque) {
  std::swap(map_, deque.map_);
  std::swap(begin_, deque.begin_);
  std::swap(end_, deque.end_);
}

template <typename T>
void Deque<T>::plus(std::pair<size_t, size_t>& pair) {
  if (pair.second == kBuffSz - 1) {
    pair.second = 0;
    ++pair.first;
  } else {
    ++pair.second;
  }
}

template <typename T>
void Deque<T>::minus(std::pair<size_t, size_t>& pair) {
  if (pair.second == 0) {
    pair.second = kBuffSz - 1;
    --pair.first;
  } else {
    --pair.second;
  }
}

template <typename T>
template <bool IsConst>
class Deque<T>::deque_iterator {
public:
  using value_type = std::conditional_t<IsConst, const T, T>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::random_access_iterator_tag;
  using double_pointer = std::conditional_t<IsConst, pointer const*, pointer*>;

  deque_iterator(pointer cur, pointer first, pointer last, double_pointer node)
      : cur_(cur), first_(first), last_(last), node_(node) {}
  operator const_iterator() const { return const_iterator(*this); }

  reference operator*() { return *cur_; }
  pointer operator->() { return cur_; }

  deque_iterator& operator+=(difference_type n);
  deque_iterator& operator-=(difference_type n);
  deque_iterator& operator++();
  deque_iterator& operator--();
  deque_iterator operator++(int);
  deque_iterator operator--(int);
  deque_iterator operator+(difference_type n) const;
  deque_iterator operator-(difference_type n) const;
  difference_type operator-(const deque_iterator& iter) const;

  std::strong_ordering operator<=>(const deque_iterator& iter) const;
  bool operator==(const deque_iterator& iter) const {
    return std::is_eq(*this <=> iter);
  }

private:
  pointer cur_;
  pointer first_;
  pointer last_;
  double_pointer node_;

  void set_node(double_pointer new_node) {
    node_ = new_node;
    first_ = *new_node;
    last_ = first_ + kBuffSz;
  }
};

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>&
Deque<T>::deque_iterator<IsConst>::operator+=(difference_type n) {
  if (cur_ == nullptr && first_ == nullptr && last_ == nullptr) {
    return *this;
  }
  std::ptrdiff_t shift = n + (cur_ - first_);
  if (shift >= 0 && shift < std::ptrdiff_t(kBuffSz)) {
    cur_ += n;
    return *this;
  }
  std::ptrdiff_t node_shift = shift / std::ptrdiff_t(kBuffSz);
  if (shift <= 0) {
    node_shift = -((-shift - 1) / std::ptrdiff_t(kBuffSz)) - 1;
  }
  set_node(node_ + node_shift);
  cur_ = first_ + (shift - node_shift * kBuffSz);
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>&
Deque<T>::deque_iterator<IsConst>::operator-=(difference_type n) {
  return *this += -n;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>&
Deque<T>::deque_iterator<IsConst>::operator++() {
  *this += 1;
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>&
Deque<T>::deque_iterator<IsConst>::operator--() {
  *this -= 1;
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>
Deque<T>::deque_iterator<IsConst>::operator++(int) {
  deque_iterator tmp(*this);
  *this += 1;
  return tmp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>
Deque<T>::deque_iterator<IsConst>::operator--(int) {
  deque_iterator tmp(*this);
  *this -= 1;
  return tmp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>
Deque<T>::deque_iterator<IsConst>::operator+(difference_type n) const {
  deque_iterator tmp(*this);
  tmp += n;
  return tmp;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>
Deque<T>::deque_iterator<IsConst>::operator-(difference_type n) const {
  return *this + (-n);
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template deque_iterator<IsConst>::difference_type
Deque<T>::deque_iterator<IsConst>::operator-(const deque_iterator& iter) const {
  return (node_ - iter.node_) * kBuffSz + (cur_ - first_) -
         (iter.cur_ - iter.first_);
}

template <typename T>
template <bool IsConst>
std::strong_ordering Deque<T>::deque_iterator<IsConst>::operator<=>(
    const deque_iterator& iter) const {
  if (node_ < iter.node_) {
    return std::strong_ordering::less;
  }
  if (node_ > iter.node_) {
    return std::strong_ordering::greater;
  }
  if (cur_ < iter.cur_) {
    return std::strong_ordering::less;
  }
  if (cur_ > iter.cur_) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}