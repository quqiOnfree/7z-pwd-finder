#pragma once

#include <cstdint>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// class PasswordGenerator {
// public:
//   enum PasswordMode : std::uint8_t {
//     Unknown = 0,
//     Number = 0b1,
//     Letter = 0b10,
//     SpecialCharacters = 0b100
//   };

//   PasswordGenerator(std::size_t min_length, std::size_t max_lenth,
//                     std::uint8_t password_mode)
//       : min_length_(min_length), max_length_(max_lenth),
//         password_mode_(password_mode), current_lenth_(min_length),
//         is_first(false) {
//     generatePasswordSet();
//     generateDefaultPassword();
//   }

//   PasswordGenerator(std::size_t min_length, std::size_t max_lenth,
//                     std::uint8_t password_mode,
//                     const std::vector<std::size_t> &current_password,
//                     const std::vector<std::size_t> &end_password)
//       : min_length_(min_length), max_length_(max_lenth),
//         password_mode_(password_mode), current_password_(current_password),
//         end_password_(end_password), current_lenth_(min_length),
//         is_first(false) {
//     generatePasswordSet();
//   }

//   PasswordGenerator(std::size_t min_length, std::uint8_t password_mode,
//                     const std::vector<std::size_t> &current_password,
//                     std::size_t count)
//       : min_length_(min_length), password_mode_(password_mode),
//         current_password_(current_password), current_lenth_(min_length),
//         is_first(false) {
//     generatePasswordSet();
//     auto local_password = current_password_;
//     std::size_t bit = 0;
//     std::size_t temp = count;
//     bool need_to_carry = false;
//     while (temp) {
//       std::size_t need_to_be_added = temp % password_set_size_;
//       if (need_to_carry) {
//         ++need_to_be_added;
//         need_to_carry = false;
//       }
//       if (local_password.size() <= bit) {
//         while (bit - (local_password.size() - 1)) {
//           local_password.push_back(0);
//         }
//       }
//       if (local_password[bit] + need_to_be_added < password_set_size_) {
//         local_password[bit] += need_to_be_added;
//       } else {
//         local_password[bit] =
//             local_password[bit] + need_to_be_added - password_set_size_;
//         need_to_carry = true;
//       }
//       temp /= password_set_size_;
//       ++bit;
//     }
//     if (need_to_carry) {
//       if (local_password.size() > bit) {
//         ++local_password[bit];
//       } else {
//         local_password.push_back(1);
//       }
//     }
//     max_length_ = local_password.size();
//     end_password_ = std::move(local_password);
//     if (max_length_ > min_length_) {
//       for (std::size_t iter = 0; iter < max_length_; ++iter) {
//         end_password_[iter] = password_set_size_ - 1;
//       }
//     }
//   }

//   bool isEnd() const {
//     if (current_lenth_ == max_length_) {
//       for (std::size_t iter = 0; iter < max_length_; ++iter) {
//         if (current_password_[iter] != end_password_[iter]) {
//           return false;
//         }
//       }
//       return true;
//     }
//     return false;
//   }

//   std::string getNextPassword() {
//     if (!is_first) {
//       is_first = true;
//       std::string str(current_lenth_, 0);
//       for (std::size_t iter = current_lenth_; iter > 0; --iter) {
//         str[current_lenth_ - iter] = password_set_[current_password_[iter -
//         1]];
//       }
//       return str;
//     }
//     if (isEnd()) {
//       throw std::logic_error("Current password is the maximum");
//     }
//     if (current_password_[0] + 1 < password_set_size_) {
//       ++current_password_[0];
//       std::string str(current_lenth_, 0);
//       for (std::size_t iter = current_lenth_; iter > 0; --iter) {
//         str[current_lenth_ - iter] = password_set_[current_password_[iter -
//         1]];
//       }
//       return str;
//     }
//     auto current_password = current_password_;
//     current_password[0] = 0;
//     bool need_to_carry = true;
//     std::size_t current_lenth = current_lenth_;
//     for (std::size_t iter = 1; iter < current_lenth; ++iter) {
//       if (need_to_carry) {
//         if (current_password[iter] + 1 >= password_set_size_) {
//           current_password[iter] = 0;
//         } else {
//           ++current_password[iter];
//           need_to_carry = false;
//           break;
//         }
//       }
//     }
//     if (need_to_carry) {
//       current_password.push_back(0);
//       ++current_lenth;
//     }
//     current_password_ = std::move(current_password);
//     current_lenth_ = current_lenth;
//     std::string str(current_lenth_, 0);
//     for (std::size_t iter = current_lenth_; iter > 0; --iter) {
//       str[current_lenth_ - iter] = password_set_[current_password_[iter -
//       1]];
//     }
//     return str;
//   }

//   std::vector<std::size_t> getNextPassword(std::size_t num) {
//     if (!is_first) {
//       is_first = true;
//       return current_password_;
//     }
//     if (isEnd()) {
//       throw std::logic_error("Current password is the maximum");
//     }
//     auto local_password = current_password_;
//     std::size_t bit = 0;
//     std::size_t temp = num;
//     bool need_to_carry = false;
//     std::size_t current_lenth = current_lenth_;
//     while (temp) {
//       std::size_t need_to_be_added = temp % password_set_size_;
//       if (need_to_carry) {
//         ++need_to_be_added;
//         need_to_carry = false;
//       }
//       if (local_password.size() <= bit) {
//         std::size_t size = local_password.size();
//         local_password.resize(size + 1);
//         for (auto &i : local_password) {
//           i = 0;
//         }
//         if (current_lenth_ != size + 1) {
//           std::print("{}\n", size + 1);
//         }
//         current_lenth_ = size + 1;
//         current_password_ = std::move(local_password);
//         return current_password_;
//       }
//       if (local_password[bit] + need_to_be_added < password_set_size_) {
//         local_password[bit] += need_to_be_added;
//       } else {
//         local_password[bit] =
//             local_password[bit] + need_to_be_added - password_set_size_;
//         need_to_carry = true;
//       }
//       temp /= password_set_size_;
//       ++bit;
//     }
//     for (std::size_t iter = bit; iter < current_lenth; ++iter) {
//       if (need_to_carry) {
//         if (local_password[iter] + 1 >= password_set_size_) {
//           local_password[iter] = 0;
//         } else {
//           ++local_password[iter];
//           need_to_carry = false;
//           break;
//         }
//       }
//     }
//     if (need_to_carry) {
//       local_password.push_back(0);
//       ++current_lenth;
//     }
//     if (current_lenth_ != current_lenth) {
//       std::print("{}\n", current_lenth);
//     }
//     current_lenth_ = current_lenth;
//     current_password_ = std::move(local_password);
//     if (current_lenth_ > max_length_) {
//       current_lenth_ = max_length_;
//       current_password_ = end_password_;
//     }
//     return current_password_;
//   }

//   std::size_t getCurrentLenth() const { return current_lenth_; }

//   std::uint8_t getPasswordMode() const { return password_mode_; }

// private:
//   void generatePasswordSet() {
//     if (password_mode_ == Unknown) {
//       throw std::logic_error("Unknown mode");
//     }
//     if (password_mode_ & Number) {
//       password_set_.append("0123456789");
//     }
//     if (password_mode_ & Letter) {
//       password_set_.append(
//           "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
//     }
//     if (password_mode_ & SpecialCharacters) {
//       password_set_.resize(password_set_.size() + 32);
//       auto iter = password_set_.end() - 32;
//       for (char i = 33; i <= 47; ++i) {
//         *(iter++) = i;
//       }
//       for (char i = 58; i <= 64; ++i) {
//         *(iter++) = i;
//       }
//       for (char i = 91; i <= 96; ++i) {
//         *(iter++) = i;
//       }
//       for (char i = 123; i <= 126; ++i) {
//         *(iter++) = i;
//       }
//     }
//     password_set_size_ = password_set_.size();
//   }

//   void generateDefaultPassword() {
//     if (password_mode_ == Unknown) {
//       throw std::logic_error("Unknown mode");
//     }
//     current_password_.resize(min_length_);
//     end_password_.resize(max_length_);
//     for (std::size_t iter = 0; iter < min_length_; ++iter) {
//       current_password_[iter] = 0;
//     }
//     for (std::size_t iter = 0; iter < max_length_; ++iter) {
//       end_password_[iter] = password_set_size_ - 1;
//     }
//   }

//   std::size_t min_length_;
//   std::size_t max_length_;
//   std::uint8_t password_mode_;
//   std::string password_set_;
//   std::size_t password_set_size_;

//   bool is_first;
//   std::size_t current_lenth_;
//   std::vector<std::size_t> current_password_;
//   std::vector<std::size_t> end_password_;
// };

enum PasswordMode : std::uint8_t {
  UnknownMode = 0,
  NumberMode = 0b1,
  LetterMode = 0b10,
  SpecialCharactersMode = 0b100
};

class PasswordSet {
public:
  PasswordSet(std::uint8_t password_mode) : password_mode_(password_mode) {
    generatePasswordSet();
  }

  const std::string &getPasswordSet() const { return password_set_; }

private:
  void generatePasswordSet() {
    if (password_mode_ == PasswordMode::UnknownMode) {
      throw std::logic_error("Unknown mode");
    }
    if (password_mode_ & PasswordMode::NumberMode) {
      password_set_.append("0123456789");
    }
    if (password_mode_ & PasswordMode::LetterMode) {
      password_set_.append(
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    }
    if (password_mode_ & PasswordMode::SpecialCharactersMode) {
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

  std::uint8_t password_mode_;
  std::string password_set_;
  std::size_t password_set_size_;
};

class PasswordGeneratorList {
public:
  PasswordGeneratorList(std::size_t min_length, std::size_t max_lenth,
                        const PasswordSet &set)
      : pwd_set_(set), min_length_(min_length), max_length_(max_lenth),
        current_lenth_(min_length), is_end_(false) {
    password_set_.resize(current_lenth_);
    for (auto &i : password_set_) {
      i = 0;
    }
  }

  bool isEnd() const {
    std::unique_lock lock(mutex_);
    return is_end_;
  }

  std::vector<std::uint8_t> getPasswordList() {
    std::unique_lock lock(mutex_);
    const std::size_t pwd_size = pwd_set_.getPasswordSet().size();
    std::size_t temp = 10000;
    std::size_t bit = 0;
    bool need_to_carry = false;
    while (temp) {
      std::size_t need_to_add = temp % pwd_size;
      if (need_to_carry) {
        ++need_to_add;
        need_to_carry = false;
      }
      if (bit >= current_lenth_) {
        if (++current_lenth_ > max_length_) {
          is_end_ = true;
        }
        if (password_set_.size() != current_lenth_) {
          password_set_.resize(current_lenth_);
          for (auto &i : password_set_) {
            i = 0;
          }
        }
        return password_set_;
      }
      if (password_set_[bit] + need_to_add >= pwd_size) {
        password_set_[bit] += need_to_add - pwd_size;
        need_to_carry = true;
      } else {
        password_set_[bit] += need_to_add;
      }
      temp /= pwd_size;
      ++bit;
    }
    while (need_to_carry) {
      if (bit >= password_set_.size()) {
        if (++current_lenth_ > max_length_) {
          is_end_ = true;
        }
        if (password_set_.size() != current_lenth_) {
          password_set_.resize(current_lenth_);
          for (auto &i : password_set_) {
            i = 0;
          }
        }
        return password_set_;
      }
      if (password_set_[bit] + 1 >= pwd_size) {
        password_set_[bit] = 0;
        need_to_carry = true;
      } else {
        password_set_[bit] += 1;
        need_to_carry = false;
      }
      ++bit;
    }
    return password_set_;
  }

  const PasswordSet &getPasswordSet() const { return pwd_set_; }

  std::size_t getMinLenth() const { return min_length_; }

  std::size_t getMaxLenth() const { return max_length_; }

  ~PasswordGeneratorList() noexcept = default;

  PasswordGeneratorList(const PasswordGeneratorList &) = delete;
  PasswordGeneratorList(PasswordGeneratorList &&) = delete;
  PasswordGeneratorList &operator=(const PasswordGeneratorList &) = delete;
  PasswordGeneratorList &operator=(PasswordGeneratorList &&) = delete;

private:
  const PasswordSet &pwd_set_;
  mutable std::mutex mutex_;
  std::size_t min_length_;
  std::size_t max_length_;
  std::size_t current_lenth_;
  std::vector<std::uint8_t> password_set_;
  bool is_end_;
};

class LocalPasswordGenerator {
public:
  LocalPasswordGenerator(PasswordGeneratorList &list)
      : pwd_set_(list.getPasswordSet()), list_(list),
        min_length_(list.getMinLenth()), max_length_(list.getMaxLenth()),
        is_end_(false), count_(0) {}

  bool isEnd() const { return is_end_; }

  std::string getNextPassword() {
    if (count_ > 10000) {
      is_end_ = true;
      throw std::logic_error("End");
    }
    if (password_set_.empty() && !list_.isEnd()) {
      auto list = list_.getPasswordList();
      count_ = 0;
      if (list.size() > max_length_) {
        is_end_ = true;
        throw std::logic_error("End");
      }
      password_set_ = std::move(list);
      std::string str(password_set_.size(), 0);
      for (std::size_t iter = password_set_.size(); iter > 0; --iter) {
        str[password_set_.size() - iter] =
            pwd_set_.getPasswordSet()[password_set_[iter - 1]];
      }
      // std::print("{}\n", str);
      return str;
    }
    const std::size_t pwd_size_ = pwd_set_.getPasswordSet().size();
    ++count_;
    if (password_set_[0] + 1 < pwd_size_) {
      ++password_set_[0];
      std::string str(password_set_.size(), 0);
      for (std::size_t iter = password_set_.size(); iter > 0; --iter) {
        str[password_set_.size() - iter] =
            pwd_set_.getPasswordSet()[password_set_[iter - 1]];
      }
      return str;
    }
    password_set_[0] = 0;
    std::size_t bit = 1;
    while (true) {
      if (password_set_.size() <= bit) {
        is_end_ = true;
        throw std::logic_error("End");
      }
      if (password_set_[bit] + 1 >= pwd_size_) {
        password_set_[bit] = 0;
      } else {
        password_set_[bit] += 1;
        std::string str(password_set_.size(), 0);
        for (std::size_t iter = password_set_.size(); iter > 0; --iter) {
          str[password_set_.size() - iter] =
              pwd_set_.getPasswordSet()[password_set_[iter - 1]];
        }
        return str;
      }
      ++bit;
    }
  }

private:
  const PasswordSet &pwd_set_;
  PasswordGeneratorList &list_;
  std::size_t min_length_;
  std::size_t max_length_;
  std::vector<std::uint8_t> password_set_;
  std::size_t count_;
  bool is_end_;
};
