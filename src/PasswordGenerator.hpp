#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class PasswordGenerator {
public:
  enum PasswordMode : std::uint8_t {
    Unknown = 0,
    Number = 0b1,
    Letter = 0b10,
    SpecialCharacters = 0b100
  };

  PasswordGenerator(std::size_t min_length, std::size_t max_lenth,
                    std::uint8_t password_mode)
      : min_length_(min_length), max_lenth_(max_lenth),
        password_mode_(password_mode), current_lenth_(min_length),
        is_first(false) {
    generatePasswordSet();
    generateDefaultPassword();
  }

  PasswordGenerator(std::size_t min_length, std::size_t max_lenth,
                    std::uint8_t password_mode,
                    const std::vector<std::size_t> &current_password,
                    const std::vector<std::size_t> &end_password)
      : min_length_(min_length), max_lenth_(max_lenth),
        password_mode_(password_mode), current_password_(current_password),
        end_password_(end_password), current_lenth_(min_length),
        is_first(false) {
    generatePasswordSet();
  }

  PasswordGenerator(std::size_t min_length, std::uint8_t password_mode,
                    const std::vector<std::size_t> &current_password,
                    std::size_t count)
      : min_length_(min_length), password_mode_(password_mode),
        current_password_(current_password), current_lenth_(min_length),
        is_first(false) {
    generatePasswordSet();
    auto local_password = current_password_;
    std::size_t bit = 0;
    std::size_t temp = count;
    bool need_to_carry = false;
    while (temp) {
      std::size_t need_to_be_added = temp % password_set_size_;
      if (need_to_carry) {
        ++need_to_be_added;
        need_to_carry = false;
      }
      if (local_password.size() <= bit) {
        while (bit - (local_password.size() - 1)) {
          local_password.push_back(0);
        }
      }
      if (local_password[bit] + need_to_be_added < password_set_size_) {
        local_password[bit] += need_to_be_added;
      } else {
        local_password[bit] =
            local_password[bit] + need_to_be_added - password_set_size_;
        need_to_carry = true;
      }
      temp /= password_set_size_;
      ++bit;
    }
    if (need_to_carry) {
      if (local_password.size() > bit) {
        ++local_password[bit];
      } else {
        local_password.push_back(1);
      }
    }
    max_lenth_ = local_password.size();
    end_password_ = std::move(local_password);
  }

  bool isEnd() const {
    if (current_lenth_ == max_lenth_) {
      for (std::size_t iter = 0; iter < max_lenth_; ++iter) {
        if (current_password_[iter] != end_password_[iter]) {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  std::string getNextPassword() {
    if (!is_first) {
      is_first = true;
      std::string str(current_lenth_, 0);
      for (std::size_t iter = current_lenth_; iter > 0; --iter) {
        str[current_lenth_ - iter] = password_set_[current_password_[iter - 1]];
      }
      return str;
    }
    if (isEnd()) {
      throw std::logic_error("Current password is the maximum");
    }
    if (current_password_[0] + 1 < password_set_size_) {
      ++current_password_[0];
      std::string str(current_lenth_, 0);
      for (std::size_t iter = current_lenth_; iter > 0; --iter) {
        str[current_lenth_ - iter] = password_set_[current_password_[iter - 1]];
      }
      return str;
    }
    auto current_password = current_password_;
    current_password[0] = 0;
    bool need_to_carry = true;
    std::size_t current_lenth = current_lenth_;
    for (std::size_t iter = 1; iter < current_lenth; ++iter) {
      if (need_to_carry) {
        if (current_password[iter] + 1 >= password_set_size_) {
          current_password[iter] = 0;
        } else {
          ++current_password[iter];
          need_to_carry = false;
          break;
        }
      }
    }
    if (need_to_carry) {
      current_password.push_back(0);
      ++current_lenth;
    }
    current_password_ = std::move(current_password);
    current_lenth_ = current_lenth;
    std::string str(current_lenth_, 0);
    for (std::size_t iter = current_lenth_; iter > 0; --iter) {
      str[current_lenth_ - iter] = password_set_[current_password_[iter - 1]];
    }
    return str;
  }

  std::vector<std::size_t> getNextPassword(std::size_t num) {
    if (!is_first) {
      is_first = true;
      return current_password_;
    }
    if (isEnd()) {
      throw std::logic_error("Current password is the maximum");
    }
    auto local_password = current_password_;
    std::size_t bit = 0;
    std::size_t temp = num;
    bool need_to_carry = false;
    std::size_t current_lenth = current_lenth_;
    while (temp) {
      std::size_t need_to_be_added = temp % password_set_size_;
      if (need_to_carry) {
        ++need_to_be_added;
        need_to_carry = false;
      }
      if (local_password.size() <= bit) {
        while (bit - (local_password.size() - 1)) {
          local_password.push_back(0);
          ++current_lenth;
        }
      }
      if (local_password[bit] + need_to_be_added < password_set_size_) {
        local_password[bit] += need_to_be_added;
      } else {
        local_password[bit] =
            local_password[bit] + need_to_be_added - password_set_size_;
        need_to_carry = true;
      }
      temp /= password_set_size_;
      ++bit;
    }
    for (std::size_t iter = bit; iter < current_lenth; ++iter) {
      if (need_to_carry) {
        if (local_password[iter] + 1 >= password_set_size_) {
          local_password[iter] = 0;
        } else {
          ++local_password[iter];
          need_to_carry = false;
          break;
        }
      }
    }
    if (need_to_carry) {
      local_password.push_back(0);
      ++current_lenth;
    }
    if (current_lenth_ != current_lenth) {
      std::print("{}\n", current_lenth);
    }
    current_lenth_ = current_lenth;
    current_password_ = std::move(local_password);
    if (current_lenth_ > max_lenth_) {
      current_lenth_ = max_lenth_;
      current_password_ = end_password_;
    }
    return current_password_;
  }

  std::size_t getCurrentLenth() const { return current_lenth_; }

  std::uint8_t getPasswordMode() const { return password_mode_; }

private:
  void generatePasswordSet() {
    if (password_mode_ == Unknown) {
      throw std::logic_error("Unknown mode");
    }
    if (password_mode_ & Number) {
      password_set_.append("0123456789");
    }
    if (password_mode_ & Letter) {
      password_set_.append(
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    }
    if (password_mode_ & SpecialCharacters) {
      password_set_.resize(password_set_.size() + 32);
      auto iter = password_set_.end() - 32;
      for (char i = 33; i <= 47; ++i) {
        *(iter++) = i;
      }
      for (char i = 58; i <= 64; ++i) {
        *(iter++) = i;
      }
      for (char i = 91; i <= 96; ++i) {
        *(iter++) = i;
      }
      for (char i = 123; i <= 126; ++i) {
        *(iter++) = i;
      }
    }
    password_set_size_ = password_set_.size();
  }

  void generateDefaultPassword() {
    if (password_mode_ == Unknown) {
      throw std::logic_error("Unknown mode");
    }
    current_password_.resize(min_length_);
    end_password_.resize(max_lenth_);
    for (std::size_t iter = 0; iter < min_length_; ++iter) {
      current_password_[iter] = 0;
    }
    for (std::size_t iter = 0; iter < max_lenth_; ++iter) {
      end_password_[iter] = password_set_size_ - 1;
    }
  }

  std::size_t min_length_;
  std::size_t max_lenth_;
  std::uint8_t password_mode_;
  std::string password_set_;
  std::size_t password_set_size_;

  bool is_first;
  std::size_t current_lenth_;
  std::vector<std::size_t> current_password_;
  std::vector<std::size_t> end_password_;
};
