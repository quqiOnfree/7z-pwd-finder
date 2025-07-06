#include <print>

#include "7zipLib.hpp"
#include "CMultiVolumeInStream.hpp"
#include "PasswordGenerator.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

template <CompressType CT> class Cracker {
public:
  Cracker(SevenZipLibrary &library, const std::vector<fs::path> &files,
          std::size_t min_length, std::size_t max_length,
          std::uint8_t password_mode)
      : library_(library), pwd_set_(password_mode),
        pwd_gen_(min_length, max_length, pwd_set_), found_pwd_(false),
        files_(files) {}

  void run() {
    found_pwd_ = false;
    pwd_.clear();
    workers.clear();
    workers.resize(std::thread::hardware_concurrency());
    std::size_t vec_size = workers.size();
    for (auto iter = 0; iter < vec_size; ++iter) {
      workers[iter] = std::thread(Worker{*this, files_});
    }
    for (auto iter = 0; iter < vec_size; ++iter) {
      if (workers[iter].joinable()) {
        workers[iter].join();
      }
    }
    std::lock_guard lock(pwd_mutex_);
    if (found_pwd_) {
      std::print("Found password: {}\n", wstringToString(pwd_));
    } else {
      std::print("Not found password\n");
    }
  }

private:
  struct Worker {
    Cracker &cracker;
    std::vector<fs::path> files;
    std::pmr::memory_resource *memory_resource;

    Worker(Cracker &cr, const std::vector<fs::path> &fes,
           std::pmr::memory_resource *mr = std::pmr::get_default_resource())
        : cracker(cr), files(fes), memory_resource(mr) {}

    void operator()() {
      SevenZipFile<ArchiveType::InType, CT, CMultiVolumeMemoryInStream> file(
          files, memory_resource);
      SevenZipArchive archive(cracker.library_, file);
      while (!cracker.found_pwd_ && !cracker.pwd_gen_.isEnd()) {
        LocalPasswordGenerator lp(cracker.pwd_gen_);
        try {
          while (!cracker.found_pwd_ && !lp.isEnd()) {
            std::string pwd = lp.getNextPassword();
            if (archive.open(stringToWstring(pwd)) == S_OK && archive.testExtract(stringToWstring(pwd)) == S_OK) {
              cracker.found_pwd_ = true;
              {
                std::lock_guard lock(cracker.pwd_mutex_);
                cracker.pwd_ = stringToWstring(pwd);
              }
              std::print("return\n");
              return;
            }
          }
        } catch (...) {
        }
      }
    }
  };
  friend Worker;

  SevenZipLibrary &library_;
  std::vector<fs::path> files_;
  PasswordGeneratorList pwd_gen_;
  PasswordSet pwd_set_;
  std::atomic<bool> found_pwd_;
  mutable std::mutex pwd_mutex_;
  std::wstring pwd_;
  std::vector<std::thread> workers;
};

SevenZipLibrary lib;

template <CompressType CT>
inline int testPassword(std::uint8_t pwd_mode, std::size_t min_length,
                        std::size_t max_length,
                        const std::vector<fs::path> &files) {
  SevenZipFile<ArchiveType::InType, CT, CMultiVolumeMemoryInStream> file(files);
  SevenZipArchive archive(lib, file);
  PasswordSet set(pwd_mode);
  PasswordGeneratorList list(min_length, max_length, set);
  while (!list.isEnd()) {
    try {
      LocalPasswordGenerator gen(list);
      while (!gen.isEnd()) {
        std::string pwd = gen.getNextPassword();
        if (archive.open(stringToWstring(pwd)) == S_OK) {
          std::print("Password is: {}\n", pwd);
          return 0;
        }
      }
    } catch (...) {
    }
  }
  std::print("Not found password\n");
  return 0;
}

int main(int argc, char **argv) {
  if (argc <= 4) {
    std::print("Usage: exe <Compress type>(Zip, BZip2, 7z, Xz, Tar, GZip) "
               "<[Number]+[Letter]+[SpecialCharacters]> <min "
               "lenth> <max lenth> [file1, file2...]\n");
    return 0;
  }
  try {
    std::string mode = argv[2];
    std::uint8_t pwd_mode = 0;
    if (mode.contains("Number")) {
      pwd_mode |= PasswordMode::NumberMode;
    }
    if (mode.contains("Letter")) {
      pwd_mode |= PasswordMode::LetterMode;
    }
    if (mode.contains("SpecialCharacters")) {
      pwd_mode |= PasswordMode::SpecialCharactersMode;
    }
    if (!pwd_mode) {
      std::print("Error password mode\n");
      return 1;
    }
    std::size_t min_length{std::stoull(argv[3])},
        max_length{std::stoull(argv[4])};
    std::vector<fs::path> files(argc - 5);
    std::vector<std::string> files2(argc - 5);
    for (std::size_t i = 5; i < argc; ++i) {
      files[i - 5] = argv[i];
      files2[i - 5] = argv[i];
    }
    std::print("type: {}, mode: {}, min lenth: {}, max lenth: {}, files: {}\n",
               argv[1], mode, min_length, max_length, files2);
    if (!std::strcmp(argv[1], "Zip")) {
      // Cracker<CompressType::Zip> cracker(lib, files, min_length, max_length,
      // pwd_mode); cracker.run();
      return testPassword<CompressType::Zip>(pwd_mode, min_length, max_length,
                                             files);
    } else if (!std::strcmp(argv[1], "BZip2")) {
      // Cracker<CompressType::BZip2> cracker(lib, files, min_length,
      // max_length, pwd_mode); cracker.run();
      return testPassword<CompressType::BZip2>(pwd_mode, min_length, max_length,
                                               files);
    } else if (!std::strcmp(argv[1], "7z")) {
      // Cracker<CompressType::Sz> cracker(lib, files, min_length, max_length,
      // pwd_mode); cracker.run();
      return testPassword<CompressType::Sz>(pwd_mode, min_length, max_length,
                                            files);
    } else if (!std::strcmp(argv[1], "Xz")) {
      // Cracker<CompressType::Xz> cracker(lib, files, min_length, max_length,
      // pwd_mode); cracker.run();
      return testPassword<CompressType::Xz>(pwd_mode, min_length, max_length,
                                            files);
    } else if (!std::strcmp(argv[1], "Tar")) {
      // Cracker<CompressType::Tar> cracker(lib, files, min_length, max_length,
      // pwd_mode); cracker.run();
      return testPassword<CompressType::Tar>(pwd_mode, min_length, max_length,
                                             files);
    } else if (!std::strcmp(argv[1], "GZip")) {
      // Cracker<CompressType::GZip> cracker(lib, files, min_length, max_length,
      // pwd_mode); cracker.run();
      return testPassword<CompressType::GZip>(pwd_mode, min_length, max_length,
                                              files);
    } else {
      std::print("Error compress type\n");
    }
  } catch (const std::exception &e) {
    std::print("Error occurred: {}\n", e.what());
  }
  return 0;
}
