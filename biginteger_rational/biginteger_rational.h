#pragma once
#include <iomanip>
#include <iostream>
#include <vector>

enum CompareType { LOWER, EQUAL, GREATER };

class BigInteger {
public:
  BigInteger() = default;
  BigInteger(const std::string& str);
  BigInteger(int num);

  explicit operator bool() const {
    return !(digits_.empty() || (digits_.size() == 1 && digits_[0] == 0));
  }

  BigInteger operator-() const;

  BigInteger& operator+=(const BigInteger& num);
  BigInteger& operator-=(const BigInteger& num);
  BigInteger& operator*=(const BigInteger& num);
  BigInteger& operator/=(const BigInteger& num);
  BigInteger& operator%=(const BigInteger& num);

  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);

  bool isNegative() const { return is_negative_; }
  const std::vector<int>& data() const { return digits_; }

  std::string toString() const;

private:
  static const int kBase = 1e9;
  bool is_negative_{false};
  std::vector<int> digits_;

  void absPlus(const BigInteger& num);
  void absMinusFromGreater(const BigInteger& greater_num,
                           const BigInteger& lower_num);

  int findQuotient(const BigInteger& divider) const;

  void deleteLeadingZero();
};

BigInteger::BigInteger(const std::string& str) {
  static const int kMaxDigitsNum = 9;
  static const int kBaseFactor = 10;
  static const char kZeroSymbol = '0';

  if (str.length() == 0) {
    return;
  }
  int first_digit_idx = 0;
  if (str[0] == '-') {
    is_negative_ = true;
    ++first_digit_idx;
  }
  int cur_digit = 0;
  int cur_factor = 1;
  int count_digits = -1;
  for (int i = static_cast<int>(str.size()) - 1; i >= first_digit_idx; --i) {
    ++count_digits;
    if (count_digits >= kMaxDigitsNum) {
      digits_.push_back(cur_digit);
      cur_digit = 0;
      count_digits = 0;
      cur_factor = 1;
    }
    cur_digit += static_cast<int>(str[i] - kZeroSymbol) * cur_factor;
    cur_factor *= kBaseFactor;
  }
  digits_.push_back(cur_digit);
}

BigInteger::BigInteger(int num) {
  if (num < 0) {
    is_negative_ = true;
    num *= -1;
  }
  while (num != 0) {
    digits_.push_back(num % kBase);
    num /= kBase;
  }
}

BigInteger operator""_bi(unsigned long long num) { return BigInteger(num); }

CompareType AbsCompare(const BigInteger& num_left, const BigInteger& num_right);

bool operator==(const BigInteger& lhs, const BigInteger& rhs) {
  return (lhs.isNegative() == rhs.isNegative() &&
          AbsCompare(lhs, rhs) == EQUAL);
}

bool operator!=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(lhs == rhs);
}

bool operator<(const BigInteger& lhs, const BigInteger& rhs) {
  bool is_negative1 = lhs.isNegative();
  bool is_negative2 = rhs.isNegative();

  if (is_negative1 != is_negative2) {
    return is_negative1;
  }
  if (is_negative1 && is_negative2) {
    return AbsCompare(lhs, rhs) == GREATER;
  }
  return AbsCompare(lhs, rhs) == LOWER;
}

bool operator>(const BigInteger& lhs, const BigInteger& rhs) {
  return rhs < lhs;
}

bool operator<=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const BigInteger& lhs, const BigInteger& rhs) {
  return !(lhs < rhs);
}

BigInteger BigInteger::operator-() const {
  BigInteger sub_num(*this);
  sub_num.is_negative_ = !sub_num.is_negative_;
  return sub_num;
}

BigInteger operator+(BigInteger lhs, const BigInteger& rhs) {
  lhs += rhs;
  return lhs;
}

BigInteger operator-(BigInteger lhs, const BigInteger& rhs) {
  lhs -= rhs;
  return lhs;
}

BigInteger operator*(BigInteger lhs, const BigInteger& rhs) {
  lhs *= rhs;
  return lhs;
}

BigInteger operator/(BigInteger lhs, const BigInteger& rhs) {
  lhs /= rhs;
  return lhs;
}

BigInteger operator%(BigInteger lhs, const BigInteger& rhs) {
  lhs %= rhs;
  return lhs;
}

BigInteger& BigInteger::operator+=(const BigInteger& num) {
  if (is_negative_ == num.is_negative_) {
    absPlus(num);
    return *this;
  }

  if (AbsCompare(*this, num) == GREATER) {
    absMinusFromGreater(*this, num);
  } else {
    absMinusFromGreater(num, *this);
  }

  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& num) {
  is_negative_ = !(is_negative_);
  *this += num;
  is_negative_ = !(is_negative_);
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& num) {
  if (num == -1) {
    is_negative_ = !is_negative_;
    return *this;
  }
  if (num == 1) {
    return *this;
  }
  BigInteger result_num(0);
  result_num.digits_.resize(digits_.size() + num.digits_.size());
  for (size_t i = 0; i < digits_.size(); ++i) {
    long long carry = 0;
    for (size_t j = 0; j < num.digits_.size() || carry != 0; ++j) {
      long long cur_digit = 0;
      if (j < num.digits_.size()) {
        cur_digit += result_num.digits_[i + j] +
                     static_cast<long long>(digits_[i]) * num.digits_[j] +
                     carry;
      } else {
        cur_digit += result_num.digits_[i + j] + carry;
      }
      result_num.digits_[i + j] = static_cast<int>(cur_digit % kBase);
      carry = static_cast<int>(cur_digit / kBase);
    }
  }
  result_num.is_negative_ = (is_negative_ != num.is_negative_);
  result_num.deleteLeadingZero();
  *this = result_num;
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& num) {
  if (num == -1) {
    is_negative_ = !(is_negative_);
    return *this;
  }
  if (num == 1) {
    return *this;
  }
  BigInteger result_num;
  BigInteger sub_num;
  BigInteger abs_num;
  abs_num += num;
  abs_num.is_negative_ = false;
  for (size_t i = digits_.size() - 1; i != size_t(-1); --i) {
    sub_num *= kBase;
    sub_num += digits_[i];
    if (sub_num >= abs_num || sub_num == 0) {
      int factor = sub_num.findQuotient(abs_num);
      sub_num -= factor * abs_num;
      result_num *= kBase;
      result_num += factor;
    }
  }
  result_num.is_negative_ = (is_negative_ != num.is_negative_);
  *this = result_num;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& num) {
  *this -= (*this / num) * num;
  return *this;
}

BigInteger& BigInteger::operator++() { return *this += 1; }

BigInteger BigInteger::operator++(int) {
  *this += 1;
  return *this - 1;
}

BigInteger& BigInteger::operator--() { return *this -= 1; }

BigInteger BigInteger::operator--(int) {
  *this -= 1;
  return *this + 1;
}

std::string BigInteger::toString() const {
  static const int kMaxDigitsNum = 9;
  if (digits_.empty()) {
    return "0";
  }
  std::string result_str;
  if (is_negative_) {
    result_str += '-';
  }
  result_str += std::to_string(digits_.back());
  for (size_t i = digits_.size() - 2; i != size_t(-1); --i) {
    std::string add_str = std::to_string(digits_[i]);
    result_str += std::string((kMaxDigitsNum - add_str.length()), '0');
    result_str += add_str;
  }
  return result_str;
}

CompareType AbsCompare(const BigInteger& num_left,
                       const BigInteger& num_right) {
  const std::vector<int>& digits_left = num_left.data();
  const std::vector<int>& digits_right = num_right.data();

  if (digits_left.empty() &&
      (digits_right.size() == 1 && digits_right[0] == 0)) {
    return EQUAL;
  }
  if (digits_right.empty() &&
      (digits_left.size() == 1 && digits_left[0] == 0)) {
    return EQUAL;
  }
  if (digits_left.size() > digits_right.size()) {
    return GREATER;
  }
  if (digits_left.size() < digits_right.size()) {
    return LOWER;
  }
  for (size_t i = digits_left.size() - 1; i != size_t(-1); --i) {
    if (digits_left[i] > digits_right[i]) {
      return GREATER;
    }
    if (digits_left[i] < digits_right[i]) {
      return LOWER;
    }
  }
  return EQUAL;
}

void BigInteger::absPlus(const BigInteger& num) {
  int carry = 0;
  bool is_empty_digit = false;
  size_t max_size = std::max(digits_.size(), num.digits_.size());
  for (size_t i = 0; i < max_size; ++i) {
    if (i >= num.digits_.size()) {
      is_empty_digit = true;
      int old_carry = carry;
      carry = (digits_[i] + carry) / kBase;
      digits_[i] = (digits_[i] + old_carry) % kBase;
    }
    if (i >= digits_.size()) {
      is_empty_digit = true;
      digits_.push_back((num.digits_[i] + carry) % kBase);
      carry = (num.digits_[i] + carry) / kBase;
    }
    if (!is_empty_digit) {
      int old_carry = carry;
      carry = (digits_[i] + num.digits_[i] + carry) / kBase;
      digits_[i] = (digits_[i] + num.digits_[i] + old_carry) % kBase;
    }
  }
  if (carry != 0) {
    digits_.push_back(carry);
  }
}

void BigInteger::absMinusFromGreater(const BigInteger& greater_num,
                                     const BigInteger& lower_num) {
  digits_.resize(greater_num.digits_.size());

  is_negative_ = greater_num.is_negative_;

  int carry = 0;
  for (size_t i = 0; i < greater_num.digits_.size(); ++i) {
    int temp = carry;
    if (i >= lower_num.digits_.size()) {
      carry = static_cast<int>(greater_num.digits_[i] == 0 && carry == 1);
      digits_[i] = (kBase + greater_num.digits_[i] - temp) % kBase;
    } else {
      carry = static_cast<int>(greater_num.digits_[i] <
                               (lower_num.digits_[i] + carry));
      digits_[i] =
          (kBase + greater_num.digits_[i] - lower_num.digits_[i] - temp) %
          kBase;
    }
  }

  deleteLeadingZero();
}

int BigInteger::findQuotient(const BigInteger& divider) const {
  int right_bound = kBase;
  int left_bound = 0;
  while (right_bound - 1 != left_bound) {
    int middle = left_bound + (right_bound - left_bound) / 2;
    if (divider * middle > *this) {
      right_bound = middle;
    } else {
      left_bound = middle;
    }
  }
  return left_bound;
}

void BigInteger::deleteLeadingZero() {
  size_t idx = digits_.size() - 1;
  while ((!digits_.empty()) && digits_[idx] == 0) {
    digits_.pop_back();
    --idx;
  }
}

std::ostream& operator<<(std::ostream& output, const BigInteger& num) {
  bool is_negative = num.isNegative();
  std::vector<int> const& digits = num.data();

  if (digits.empty()) {
    output << 0;
  } else {
    if (is_negative) {
      output << '-';
    }
    output << digits.back();
    static const int kMaxDigitsNum = 9;
    for (size_t i = digits.size() - 2; i != size_t(-1); --i) {
      output << std::setw(kMaxDigitsNum) << std::setfill('0') << digits[i];
    }
  }

  return output;
}

std::istream& operator>>(std::istream& input, BigInteger& num) {
  std::string input_str;
  input >> input_str;
  num = BigInteger(input_str);
  return input;
}

BigInteger Gcd(BigInteger num1, BigInteger num2) {
  while (num2) {
    num1 %= num2;
    std::swap(num1, num2);
  }
  return num1;
}

class Rational {
public:
  Rational() = default;
  Rational(int num);
  Rational(const BigInteger& num);

  explicit operator double() const { return std::stod(asDecimal(kPrecision)); }

  Rational operator-() const;
  Rational& operator+=(const Rational& frac);
  Rational& operator-=(const Rational& frac);
  Rational& operator*=(const Rational& frac);
  Rational& operator/=(const Rational& frac);

  bool isNegative() const { return is_negative_; }
  const BigInteger& numeratorData() const { return numerator_; }
  const BigInteger& denominatorData() const { return denominator_; }

  std::string toString() const;
  std::string asDecimal(size_t precision = kPrecision) const;

private:
  bool is_negative_{false};
  BigInteger numerator_{0};
  BigInteger denominator_{1};

  static const int kPrecision = 30;

  void simplify();
};

void Rational::simplify() {
  BigInteger common_divider = Gcd(numerator_, denominator_);

  // if fraction is unsimplified (common_divider = 1), all operations will take
  // O(1) in common
  numerator_ /= common_divider;
  denominator_ /= common_divider;

  if (numerator_ == 0) {
    is_negative_ = false;
  }
}

std::string Rational::toString() const {
  std::string str;

  if (is_negative_) {
    str += '-';
  }
  str += numerator_.toString();
  if (denominator_ == 1) {
    return str;
  }
  str += '/';
  str += denominator_.toString();

  return str;
}

std::string Rational::asDecimal(size_t precision) const {
  std::string result_str;
  if (is_negative_) {
    result_str += '-';
  }

  BigInteger sub_num = numerator_ / denominator_;
  result_str += sub_num.toString();
  if (precision == 0) {
    return result_str;
  }
  result_str += '.';

  sub_num = numerator_ % denominator_;
  std::string factor_str = "1";
  factor_str += std::string(precision, '0');
  sub_num *= BigInteger(factor_str);
  sub_num /= denominator_;

  std::string sub_num_str = sub_num.toString();
  for (size_t i = 0; i < precision - sub_num_str.length(); ++i) {
    result_str += '0';
  }
  result_str += sub_num_str;

  return result_str;
}

Rational::Rational(int num) {
  if (num < 0) {
    is_negative_ = true;
  }
  numerator_ = std::abs(num);
  denominator_ = 1;
}

Rational::Rational(const BigInteger& num) {
  is_negative_ = num.isNegative();
  if (num < 0) {
    numerator_ = -num;
  } else {
    numerator_ = num;
  }
  denominator_ = 1;
}

Rational Rational::operator-() const {
  Rational result_frac(*this);
  result_frac.is_negative_ = !(result_frac.is_negative_);
  return result_frac;
}

Rational& Rational::operator+=(const Rational& frac) {
  if (is_negative_ == frac.is_negative_) {
    numerator_ =
        numerator_ * frac.denominator_ + frac.numerator_ * denominator_;
  } else {
    if (is_negative_) {
      numerator_ =
          frac.numerator_ * denominator_ - numerator_ * frac.denominator_;
    } else {
      numerator_ =
          numerator_ * frac.denominator_ - frac.numerator_ * denominator_;
    }
  }

  if (numerator_.isNegative()) {
    is_negative_ = !is_negative_;
    numerator_ *= -1;
  }

  denominator_ *= frac.denominator_;

  simplify();
  return *this;
}

Rational& Rational::operator-=(const Rational& frac) {
  *this += -frac;

  simplify();
  return *this;
}

Rational& Rational::operator*=(const Rational& frac) {
  is_negative_ = is_negative_ != frac.is_negative_;

  numerator_ *= frac.numerator_;
  denominator_ *= frac.denominator_;

  simplify();

  return *this;
}

Rational& Rational::operator/=(const Rational& frac) {
  is_negative_ = is_negative_ != frac.is_negative_;

  numerator_ *= frac.denominator_;
  denominator_ *= frac.numerator_;

  simplify();

  return *this;
}

Rational operator+(Rational lhs, const Rational& rhs) {
  lhs += rhs;
  return lhs;
}

Rational operator-(Rational lhs, const Rational& rhs) {
  lhs -= rhs;
  return lhs;
}

Rational operator*(Rational lhs, const Rational& rhs) {
  lhs *= rhs;
  return lhs;
}

Rational operator/(Rational lhs, const Rational& rhs) {
  lhs /= rhs;
  return lhs;
}

bool operator==(const Rational& lhs, const Rational& rhs) {
  return !(lhs.isNegative() != rhs.isNegative() ||
           lhs.numeratorData() != rhs.numeratorData() ||
           lhs.denominatorData() != rhs.denominatorData());
}

bool operator!=(const Rational& lhs, const Rational& rhs) {
  return !(lhs == rhs);
}

bool operator<(const Rational& lhs, const Rational& rhs) {
  if (lhs.isNegative() != rhs.isNegative()) {
    return lhs.isNegative();
  }

  return !(lhs.isNegative()) && (lhs.numeratorData() * rhs.denominatorData() <
                                 rhs.numeratorData() * lhs.denominatorData());
}

bool operator>(const Rational& lhs, const Rational& rhs) { return rhs < lhs; }

bool operator<=(const Rational& lhs, const Rational& rhs) {
  return !(rhs < lhs);
}

bool operator>=(const Rational& lhs, const Rational& rhs) {
  return !(lhs < rhs);
}