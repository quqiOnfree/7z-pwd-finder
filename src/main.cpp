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
          std::size_t min_lenth, std::size_t max_lenth,
          std::uint8_t password_mode)
      : library_(library), pwd_gen_(min_lenth, max_lenth, password_mode),
        found_pwd_(false), files_(files) {}

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
    struct PasswordGeneratorDeleter {
      std::pmr::memory_resource *mr;
      void operator()(PasswordGenerator *pg) {
        std::pmr::polymorphic_allocator<PasswordGenerator>{mr}.delete_object(
            pg);
      }
    };

    Cracker &cracker;
    std::vector<fs::path> files;
    std::unique_ptr<PasswordGenerator, PasswordGeneratorDeleter> pwd_gen;
    std::pmr::memory_resource *memory_resource;

    Worker(Cracker &cr, const std::vector<fs::path> &fes,
           std::pmr::memory_resource *mr = std::pmr::get_default_resource())
        : cracker(cr), files(fes), memory_resource(mr), pwd_gen(nullptr, {mr}) {

    }

    void operator()() {
      SevenZipArchive<ArchiveType::InType, CT> archive(cracker.library_);
      SevenZipFile<ArchiveType::InType, CT> file(archive, files);
      while (!cracker.found_pwd_) {
        if (!pwd_gen || pwd_gen->isEnd()) {
          PasswordGenerator *ptr = reinterpret_cast<PasswordGenerator *>(
              memory_resource->allocate(sizeof(PasswordGenerator)));
          try {
            std::lock_guard lock(cracker.mutex_);
            new (ptr) PasswordGenerator(cracker.pwd_gen_.getCurrentLenth(),
                                        cracker.pwd_gen_.getPasswordMode(),
                                        cracker.pwd_gen_.getNextPassword(10000),
                                        10000);
            pwd_gen.reset(ptr);
          } catch (const std::exception &e) {
            memory_resource->deallocate(ptr, sizeof(PasswordGenerator));
            std::print("Error occurred: {}, thread id: {}\n", e.what(),
                       std::this_thread::get_id());
            return;
          }
        }
        while (!cracker.found_pwd_ && !pwd_gen->isEnd()) {
          std::string pwd = pwd_gen->getNextPassword();
          if (pwd == "1234567") {
            std::print("1234567, thread id: {}\n", std::this_thread::get_id());
          }
          file.reset();
          if (file.open(stringToWstring(pwd)) == S_OK) {
            cracker.found_pwd_ = true;
            {
              std::lock_guard lock(cracker.pwd_mutex_);
              cracker.pwd_ = stringToWstring(pwd);
            }
            std::print("return\n");
            return;
          }
        }
        pwd_gen.reset();
      }
    }
  };
  friend Worker;

  SevenZipLibrary &library_;
  std::vector<fs::path> files_;
  PasswordGenerator pwd_gen_;
  mutable std::mutex mutex_;
  std::atomic<bool> found_pwd_;
  mutable std::mutex pwd_mutex_;
  std::wstring pwd_;
  std::vector<std::thread> workers;
};

int main(int argc, char** argv) {
  if (argc <= 4) {
    std::print("Usage: exe \"[Number]+[Letter]+[SpecialCharacters]\" <min lenth> <max lenth> [file1, file2...]\n");
    return 0;
  }
  try {
    std::string mode = argv[1];
    std::uint8_t pwd_mode = 0;
    if (mode.contains("Number")) {
      pwd_mode |= PasswordGenerator::Number;
    }
    if (mode.contains("Letter")) {
      pwd_mode |= PasswordGenerator::Letter;
    }
    if (mode.contains("SpecialCharacters")) {
      pwd_mode |= PasswordGenerator::SpecialCharacters;
    }
    std::int64_t min_lenth{std::stoll(argv[2])}, max_lenth{std::stoll(argv[3])};
    std::vector<fs::path> files(argc - 4);
    std::vector<std::string> files2(argc - 4);
    for (std::size_t i = 4; i < argc; ++i) {
      files[i - 4] = argv[i];
      files2[i - 4] = argv[i];
    }
    std::print("mode: {}, min lenth: {}, max lenth: {}, files: {}\n", mode, min_lenth, max_lenth, files2);
    SevenZipLibrary lib;
    Cracker<CompressType::Sz> cracker(
        lib, files,
        min_lenth, max_lenth,
        pwd_mode);
    cracker.run();
  } catch (const std::exception &e) {
    std::print("error occurred: {}\n", e.what());
  }
  return 0;
}
