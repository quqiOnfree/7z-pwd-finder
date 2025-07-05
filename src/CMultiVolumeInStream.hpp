#pragma once

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../7zip/CPP/7zip/IStream.h"

namespace fs = std::filesystem;

class CMultiVolumeInStream : public IInStream {
public:
  CMultiVolumeInStream(
      const std::vector<fs::path> &files,
      std::pmr::memory_resource *mr = std::pmr::get_default_resource())
      : ref_count_(1), memory_resource_(mr), files_(files.size()),
        files_sizes_(files.size()), file_pos_(0), loc_pos_(0), glb_pos_(0),
        glb_size_(0) {
    if (files.empty()) {
      throw std::logic_error("Empty file path");
      return;
    }
    for (std::size_t iter = 0; iter < files.size(); ++iter) {
      if (!fs::exists(files.at(iter))) {
        throw std::logic_error("Invalid file path");
      }
      std::ifstream file(files.at(iter).generic_string(),
                         std::ios_base::binary);
      if (!file) {
        throw std::logic_error("Could not open the files");
      }
      file.seekg(0, std::ios_base::end);
      std::int64_t local_file_size = file.tellg();
      files_sizes_.at(iter) = local_file_size;
      glb_size_ += local_file_size;
      file.seekg(0, std::ios_base::beg);
      files_.at(iter) = std::move(file);
    }
  }

  CMultiVolumeInStream(const CMultiVolumeInStream &) = delete;
  CMultiVolumeInStream(CMultiVolumeInStream &&) noexcept = default;
  CMultiVolumeInStream &operator=(const CMultiVolumeInStream &) = delete;
  CMultiVolumeInStream &operator=(CMultiVolumeInStream &&) = default;

  virtual ~CMultiVolumeInStream() noexcept = default;

  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    std::int64_t loc_size = 0;
    switch (seekOrigin) {
    case STREAM_SEEK_SET:
      glb_pos_ = offset;
      for (std::size_t iter = 0; iter < files_sizes_.size(); ++iter) {
        std::int64_t this_file_size = files_sizes_.at(iter);
        if ((loc_size + this_file_size) > offset) {
          file_pos_ = iter;
          loc_pos_ = offset - loc_size;
          return S_OK;
        }
        loc_size += this_file_size;
      }
      return E_FAIL;
    case STREAM_SEEK_CUR:
      if (!offset) {
        *newPosition = glb_pos_;
        return S_OK;
      }
      if (offset > 0) {
        if (glb_pos_ + offset > glb_size_) {
          return E_FAIL;
        }
        glb_pos_ += offset;
        if (files_sizes_.at(file_pos_) <= loc_pos_ + offset) {
          loc_pos_ = loc_pos_ + offset;
          while (files_sizes_.at(file_pos_) <= loc_pos_) {
            loc_pos_ = loc_pos_ - files_sizes_.at(file_pos_++);
          }
        } else {
          loc_pos_ += offset;
        }
      } else {
        if (glb_pos_ < -offset) {
          return E_FAIL;
        }
        glb_pos_ += offset;
        if (loc_pos_ < -offset) {
          while (offset + files_sizes_.at(file_pos_ - 1) < 0) {
            offset += files_sizes_.at(--file_pos_);
          }
          loc_pos_ = files_sizes_.at(--file_pos_) + offset;
        } else {
          loc_pos_ += offset;
        }
      }
      return S_OK;
    case STREAM_SEEK_END:
      *newPosition = offset + glb_size_;
      return S_OK;

    default:
      return STG_E_INVALIDFUNCTION;
    }
    return S_OK;
  }

  STDMETHOD_(ULONG, AddRef)() { return ++ref_count_; }

  STDMETHOD_(ULONG, Release)() {
    if (--ref_count_ == 0) {
      delete this;
      return 0;
    }
    return ref_count_;
  }

  STDMETHOD(QueryInterface)(REFIID iid, void **outObject) {
    if (iid == IID_IInStream) {
      *outObject = static_cast<IInStream *>(this);
      AddRef();
      return S_OK;
    }
    *outObject = nullptr;
    return E_NOINTERFACE;
  }

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize) {
    if (files_.empty()) {
      *processedSize = 0;
      return S_OK;
    }
    std::size_t remaining = size;
    *processedSize = 0;
    auto &file = files_.at(file_pos_);
    if (file.eof()) {
      file.clear();
    }
    file.seekg(loc_pos_);
    std::size_t acturally_read =
        files_sizes_.at(file_pos_) - loc_pos_ < remaining
            ? files_sizes_.at(file_pos_) - loc_pos_
            : remaining;
    char *mem = (char *)memory_resource_->allocate(acturally_read + 1);
    std::memset(mem, 0, acturally_read + 1);
    file.read(mem, acturally_read);
    std::memcpy(data, mem, acturally_read);
    memory_resource_->deallocate(mem, acturally_read + 1);
    if (acturally_read < remaining) {
      remaining -= acturally_read;
      glb_pos_ += acturally_read;
      *processedSize += acturally_read;
      loc_pos_ = 0;
      ++file_pos_;
      return S_OK;
    } else {
      loc_pos_ += remaining;
      glb_pos_ += remaining;
      *processedSize += remaining;
      remaining = 0;
      return S_OK;
    }
    return S_OK;
  }

private:
  ULONG ref_count_;
  std::pmr::memory_resource *memory_resource_;

  std::vector<std::ifstream> files_;
  std::vector<std::int64_t> files_sizes_;
  std::size_t file_pos_;
  std::int64_t loc_pos_;
  std::int64_t glb_pos_;
  std::int64_t glb_size_;
};

class CMultiVolumeMemoryInStream : public IInStream {
public:
  CMultiVolumeMemoryInStream(
      const std::vector<fs::path> &files,
      std::pmr::memory_resource *mr = std::pmr::get_default_resource())
      : ref_count_(1), memory_resource_(mr), glb_pos_(0), glb_size_(0),
        buffer_(mr) {
    if (files.empty()) {
      throw std::logic_error("Empty file path");
      return;
    }
    std::vector<std::ifstream> files_(files.size());
    std::vector<std::int64_t> files_sizes_(files.size());
    for (std::size_t iter = 0; iter < files.size(); ++iter) {
      if (!fs::exists(files.at(iter))) {
        throw std::logic_error("Invalid file path");
      }
      std::ifstream file(files.at(iter).generic_string(),
                         std::ios_base::binary);
      if (!file) {
        throw std::logic_error("Could not open the files");
      }
      file.seekg(0, std::ios_base::end);
      std::int64_t local_file_size = file.tellg();
      files_sizes_.at(iter) = local_file_size;
      glb_size_ += local_file_size;
      file.seekg(0, std::ios_base::beg);
      files_.at(iter) = std::move(file);
    }
    buffer_.assign(glb_size_, 0);
    std::int64_t pos = 0;
    for (std::size_t iter = 0; iter < files_sizes_.size(); ++iter) {
      files_[iter].read(buffer_.data() + pos, files_sizes_[iter]);
      pos += files_sizes_[iter];
    }
  }

  CMultiVolumeMemoryInStream(const CMultiVolumeMemoryInStream &) = delete;
  CMultiVolumeMemoryInStream(CMultiVolumeMemoryInStream &&) noexcept = default;
  CMultiVolumeMemoryInStream &
  operator=(const CMultiVolumeMemoryInStream &) = delete;
  CMultiVolumeMemoryInStream &
  operator=(CMultiVolumeMemoryInStream &&) = default;

  ~CMultiVolumeMemoryInStream() noexcept = default;

  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
    switch (seekOrigin) {
    case STREAM_SEEK_SET:
      if (glb_size_ < offset) {
        return E_FAIL;
      }
      glb_pos_ = offset;
      return S_OK;
    case STREAM_SEEK_CUR:
      if (!offset) {
        *newPosition = glb_pos_;
        return S_OK;
      }
      if ((offset > 0 && glb_size_ < glb_pos_ + offset) ||
          (offset < 0 && glb_pos_ < -offset)) {
        return E_FAIL;
      }
      glb_pos_ += offset;
      return S_OK;
    case STREAM_SEEK_END:
      *newPosition = offset + glb_size_;
      return S_OK;

    default:
      return STG_E_INVALIDFUNCTION;
    }
    return S_OK;
  }

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize) {
    std::int64_t acturally_size =
        size > glb_size_ - glb_pos_ ? glb_size_ - glb_pos_ : size;
    std::memcpy(data, buffer_.data() + glb_pos_, acturally_size);
    if (processedSize != nullptr) {
      *processedSize = acturally_size;
    }
    glb_pos_ += acturally_size;
    return S_OK;
  }

  STDMETHOD(QueryInterface)(REFIID iid, void **outObject) {
    if (iid == IID_IInStream) {
      *outObject = static_cast<IInStream *>(this);
      AddRef();
      return S_OK;
    }
    *outObject = nullptr;
    return E_NOINTERFACE;
  }

  STDMETHOD_(ULONG, AddRef)() { return ++ref_count_; }

  STDMETHOD_(ULONG, Release)() {
    if (--ref_count_ == 0) {
      delete this;
      return 0;
    }
    return ref_count_;
  }

private:
  ULONG ref_count_;
  std::pmr::memory_resource *memory_resource_;

  std::pmr::string buffer_;
  std::int64_t glb_pos_;
  std::int64_t glb_size_;
};
