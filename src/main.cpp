#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <filesystem>
#include <stdexcept>

#include "7zFile.h"
#include "7z.h"
#include "7zAlloc.h"
#include "7zBuf.h"
#include "7zCrc.h"
#include "7zVersion.h"

#include "CpuArch.h"

#define kInputBufSize ((size_t)1 << 18)

static const ISzAlloc g_Alloc = { SzAlloc, SzFree };

namespace fs = std::filesystem;

class VolumeReader
{
public:
    VolumeReader(const std::vector<fs::path>& files):
        files_(files.size()),
        files_sizes_(files.size()),
        file_pos_(0),
        loc_pos_(0),
        glb_pos_(0),
        glb_size_(0)
    {
        if (files.empty()) {
            return;
        }
        for (std::size_t iter = 0; iter < files.size(); ++iter) {
            if (!fs::exists(files.at(iter))) {
                throw std::logic_error("Invalid file path");
            }
            std::ifstream file(files.at(iter).generic_string(), std::ios_base::binary);
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

    ~VolumeReader() noexcept = default;

    SRes read(void *buf, std::size_t *size)
    {
        if (files_.empty()) {
            *size = 0;
            return SZ_OK;
        }
        if ((uintptr_t)buf == 0xcccccccccccccccc || 
            (uintptr_t)buf == 0xcdcdcdcdcdcdcdcd ||
            (uintptr_t)buf == 0xdeadbeef) {
            // 记录错误信息
            std::cerr << "Invalid buffer pointer\n";
            std::cout << "remaining: " << *size << ", file_pos: " << file_pos_ << ", loc_pos: " << loc_pos_ << ", glb_pos: " << glb_pos_ << '\n';
            return SZ_ERROR_DATA;
        }
        std::size_t remaining = *size;
        *size = 0;
        while (remaining) {
            if (file_pos_ >= files_.size() - 1 && files_sizes_.at(files_sizes_.size() - 1) - 1 <= loc_pos_) {
                return SZ_OK;
            }
            std::cout << "remaining: " << remaining << ", file_pos: " << file_pos_ << ", loc_pos: " << loc_pos_ << ", glb_pos: " << glb_pos_ << '\n';
            auto& file = files_.at(file_pos_);
            if (file.eof()) {
                file.clear();
            }
            file.seekg(loc_pos_);
            char *mem = (char*)g_Alloc.Alloc(&g_Alloc, remaining + 1);
            std::memset(mem, 0, remaining + 1);
            file.read(mem, remaining);
            std::memcpy(buf, mem, remaining);
            g_Alloc.Free(&g_Alloc, mem);
            if (files_sizes_.at(file_pos_) < remaining) {
                remaining -= files_sizes_.at(file_pos_);
                glb_pos_ += files_sizes_.at(file_pos_);
                *size += files_sizes_.at(file_pos_);
                loc_pos_ = 0;
                ++file_pos_;
            }
            else {
                loc_pos_ += remaining;
                glb_pos_ += remaining;
                *size += remaining;
                remaining = 0;
                return SZ_OK;
            }
        }

        return SZ_OK;
    }

    SRes seek(std::int64_t *pos, ESzSeek origin)
    {
        std::int64_t loc_size = 0;
        switch (origin)
        {
        case SZ_SEEK_SET:
            glb_pos_ = *pos;
            for (std::size_t iter = 0; iter < files_sizes_.size(); ++iter) {
                std::int64_t this_file_size = files_sizes_.at(iter);
                if ((loc_size + this_file_size) > *pos) {
                    file_pos_ = iter;
                    loc_pos_ = *pos - loc_size;
                    return SZ_OK;
                }
                loc_size += this_file_size;
            }
            return SZ_ERROR_FAIL;
        case SZ_SEEK_CUR:
            if (!*pos) {
                return SZ_OK;
            }
            if (*pos > 0) {
                if (glb_pos_ + *pos > glb_size_) {
                    return SZ_ERROR_FAIL;
                }
                glb_pos_ += *pos;
                if (files_sizes_.at(file_pos_) <= loc_pos_ + *pos) {
                    loc_pos_ = loc_pos_ + *pos - files_sizes_.at(file_pos_++);
                }
                else {
                    loc_pos_ += *pos;
                }
            }
            else {
                if (glb_pos_ < -*pos) {
                    return SZ_ERROR_FAIL;
                }
                glb_pos_ += *pos;
                if (loc_pos_ < -*pos) {
                    loc_pos_ = loc_pos_ + *pos + files_sizes_.at(--file_pos_);
                }
                else {
                    loc_pos_ += *pos;
                }
            }
            return SZ_OK;
        case SZ_SEEK_END:
            *pos += glb_size_;
            return SZ_OK;
        
        default:
            return SZ_ERROR_PARAM;
        }
        return SZ_OK;
    }
private:
    std::vector<std::ifstream>  files_;
    std::vector<std::int64_t>   files_sizes_;
    std::size_t                 file_pos_;
    std::int64_t                loc_pos_;
    std::int64_t                glb_pos_;
    std::int64_t                glb_size_;
};

struct LookStreamAdapter {
    ISeekInStream stream;
    VolumeReader reader;
};

SRes Read(ISeekInStreamPtr s, void *buf, std::size_t *size) {
    LookStreamAdapter *adapter = (LookStreamAdapter*)s;
    return adapter->reader.read(buf, size);
}

SRes Seek(ISeekInStreamPtr s, std::int64_t *pos, ESzSeek origin) {
    LookStreamAdapter *adapter = (LookStreamAdapter*)s;
    return adapter->reader.seek(pos, origin);
}

int main() {
    ISzAlloc allocImp = g_Alloc;
    ISzAlloc allocTempImp = g_Alloc;
    LookStreamAdapter adapter{{}, {{"hello.7z"}}};
    adapter.stream.Read = Read;
    adapter.stream.Seek = Seek;
    CLookToRead2 lookStream;
    LookToRead2_CreateVTable(&lookStream, false);
    lookStream.realStream = &adapter.stream;

    CrcGenerateTable();
    CSzArEx db;
    SzArEx_Init(&db);
    
    SRes res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
    if (res != SZ_OK) return 1;
    return 0;
}
