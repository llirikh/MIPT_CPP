#pragma once
#include <cstring>
#include <iostream>

class String {
public:
  String() : len_(0), cap_(0), string_(new char[cap_ + 1]) {
    string_[0] = '\0';
  }
  String(const char* first_symbol_ptr);
  String(int number, char symbol);
  String(const String& other_string);

  ~String();

  void swap(String& second_string);
  void reserve(size_t new_cap);

  String& operator=(const String& other_string);

  char& operator[](size_t index);
  const char& operator[](size_t index) const;

  String& operator+=(char symbol);
  String& operator+=(const char* right_string);
  String& operator+=(const String& right_string);

  size_t length() const;
  size_t size() const;
  size_t capacity() const;

  void push_back(char symbol);
  void pop_back();

  char& front();
  const char& front() const;
  char& back();
  const char& back() const;

  char* data();
  const char* data() const;

  String substr(size_t start, size_t count) const;
  size_t find(const String& substring) const;
  size_t rfind(const String& substring) const;

  bool empty() const;
  void clear();

  void shrink_to_fit();

private:
  size_t len_;
  size_t cap_;
  char* string_;
};

String::String(int number, char symbol)
    : len_(number), cap_(len_ + 1), string_(new char[cap_]) {
  std::fill(string_, string_ + len_, symbol);
  string_[len_] = '\0';
}

String::String(const char* first_symbol_ptr)
    : len_(std::strlen(first_symbol_ptr)),
      cap_(len_ + 1),
      string_(new char[cap_]) {
  std::copy(first_symbol_ptr, first_symbol_ptr + cap_, string_);
}

String::String(const String& other_string) : String(other_string.data()) {}

String::~String() { delete[] string_; }

void String::swap(String& second_string) {
  std::swap(len_, second_string.len_);
  std::swap(cap_, second_string.cap_);
  std::swap(string_, second_string.string_);
}

void String::reserve(size_t new_cap) {
  cap_ = new_cap;
  char* sub_string = new char[cap_];
  std::copy(string_, string_ + len_, sub_string);
  delete[] string_;
  string_ = sub_string;
}

String& String::operator=(const String& other_string) {
  String temp(other_string);
  swap(temp);
  return *this;
}

bool operator==(const String& string1, const String& string2) {
  size_t len1 = string1.length();
  size_t len2 = string2.length();
  if (len1 != len2) {
    return false;
  }
  const char* str1 = string1.data();
  const char* str2 = string2.data();
  for (size_t i = 0; i < len1; ++i) {
    if (str1[i] != str2[i]) {
      return false;
    }
  }
  return true;
}

bool operator!=(const String& string1, const String& string2) {
  return !(string1 == string2);
}

bool operator<(const String& string1, const String& string2) {
  size_t len1 = string1.length();
  size_t len2 = string2.length();
  const char* str1 = string1.data();
  const char* str2 = string2.data();
  for (size_t i = 0; i < std::min(len1, len2); ++i) {
    if (str1[i] != str2[i]) {
      return str1[i] < str2[i];
    }
  }
  if (len1 != len2) {
    return len1 < len2;
  }
  return true;
}

bool operator<=(const String& string1, const String& string2) {
  return !(string2 < string1);
}

bool operator>(const String& string1, const String& string2) {
  return string2 < string1;
}

bool operator>=(const String& string1, const String& string2) {
  return !(string1 < string2);
}

char& String::operator[](size_t index) { return string_[index]; }

const char& String::operator[](size_t index) const { return string_[index]; }

String& String::operator+=(char symbol) {
  push_back(symbol);
  return *this;
}

String& String::operator+=(const char* right_string) {
  size_t right_string_length = strlen(right_string);
  size_t union_length = len_ + right_string_length;
  if (cap_ > union_length) {
    std::copy(right_string, right_string + right_string_length, string_ + len_);
  } else {
    reserve(2 * union_length);
    std::copy(right_string, right_string + right_string_length, string_ + len_);
  }
  len_ = union_length;
  string_[len_] = '\0';
  return *this;
}

String& String::operator+=(const String& right_string) {
  *this += right_string.string_;
  return *this;
}

String operator+(const String& left_string, char symbol) {
  String union_string = left_string;
  union_string += symbol;
  return union_string;
}

String operator+(char symbol, const String& left_string) {
  String union_string(1, symbol);
  union_string += left_string;
  return union_string;
}
String operator+(const String& left_string, const String& right_string) {
  String union_string = left_string;
  union_string += right_string;
  return union_string;
}

std::ostream& operator<<(std::ostream& out, const String& string) {
  size_t len = string.length();
  const char* str = string.data();
  for (size_t i = 0; i < len; ++i) {
    out << str[i];
  }
  return out;
}

std::istream& operator>>(std::istream& input, String& string) {
  char cur_symbol;
  input.get(cur_symbol);
  while (!(input.eof()) && !static_cast<bool>(std::isspace(cur_symbol))) {
    string.push_back(cur_symbol);
    input.get(cur_symbol);
  }
  return input;
}

size_t String::length() const { return len_; }

size_t String::size() const { return len_; }

size_t String::capacity() const { return cap_ - 1; }

void String::push_back(char symbol) {
  if (len_ < cap_ - 1) {
    string_[len_] = symbol;
    string_[len_ + 1] = '\0';
    ++len_;
  } else {
    reserve(2 * cap_);
    string_[len_] = symbol;
    ++len_;
    string_[len_] = '\0';
  }
}

void String::pop_back() {
  --len_;
  string_[len_] = '\0';
}

char& String::front() { return string_[0]; }

const char& String::front() const { return string_[0]; }

char& String::back() {
  if (len_ == 0) {
    return string_[0];
  }
  return string_[len_ - 1];
}

const char& String::back() const {
  if (len_ == 0) {
    return string_[0];
  }
  return string_[len_ - 1];
}

String String::substr(size_t start, size_t count) const {
  String result_string(count, '\0');
  std::copy(string_ + start, string_ + start + count, result_string.string_);
  return result_string;
}

size_t String::find(const String& substring) const {
  char* result = strstr(string_, substring.string_);
  if (result == nullptr) {
    return len_;
  }
  return (result - string_);
}

size_t String::rfind(const String& substring) const {
  if (len_ < substring.len_) {
    return len_;
  }
  int ptr1 = len_ - 1;
  int ptr2 = substring.len_ - 1;
  while (ptr1 > 0 && ptr2 > 0) {
    if (string_[ptr1] == substring.string_[ptr2]) {
      --ptr2;
    } else {
      ptr2 = substring.len_ - 1;
    }
    --ptr1;
  }
  if (ptr1 < 0) {
    return len_;
  }
  return ptr1;
}

bool String::empty() const { return (len_ == 0); }

void String::clear() {
  string_[0] = '\0';
  len_ = 0;
}

void String::shrink_to_fit() {
  if (cap_ > len_ + 1) {
    cap_ = len_ + 1;
    char* sub_string = new char[cap_];
    std::copy(string_, string_ + len_, sub_string);
    delete[] string_;
    string_ = sub_string;
    string_[len_] = '\0';
  }
}

char* String::data() { return string_; }

const char* String::data() const { return string_; }