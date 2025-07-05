#pragma once

#include <codecvt>
#include <locale>
#include <stdexcept>

#include "../7zip/CPP/7zip/Archive/7z/7zHandler.h"
#include "../7zip/CPP/7zip/Archive/IArchive.h"
#include "../7zip/CPP/7zip/Common/FileStreams.h"
#include "../7zip/CPP/7zip/IPassword.h"
#include "../7zip/CPP/7zip/UI/Common/ArchiveExtractCallback.h"
#include "../7zip/CPP/Common/MyInitGuid.h"
#include "../7zip/CPP/Common/MyWindows.h"
#include "../7zip/CPP/Windows/DLL.h"

#include "CMultiVolumeInStream.hpp"

#ifdef _WIN32
extern HINSTANCE g_hInstance;
HINSTANCE g_hInstance = NULL;
#endif

Z7_DIAGNOSTIC_IGNORE_CAST_FUNCTION

#ifdef _WIN32
#define kDllName "7z.dll"
#else
#define kDllName "7z.so"
#endif

#define DEFINE_GUID_ARC(name, id)                                              \
  Z7_DEFINE_GUID(name, 0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01,     \
                 0x10, id, 0x00, 0x00);
enum {
  kId_Zip = 1,
  kId_BZip2 = 2,
  kId_7z = 7,
  kId_Xz = 0xC,
  kId_Tar = 0xEE,
  kId_GZip = 0xEF
};

DEFINE_GUID_ARC(CLSID_Format_Zip, kId_Zip)
DEFINE_GUID_ARC(CLSID_Format_BZip2, kId_BZip2)
DEFINE_GUID_ARC(CLSID_Format_Xz, kId_Xz)
DEFINE_GUID_ARC(CLSID_Format_Tar, kId_Tar)
DEFINE_GUID_ARC(CLSID_Format_GZip, kId_GZip)
DEFINE_GUID_ARC(CLSID_Format_7z, kId_7z)

std::wstring stringToWstring(const std::string &data) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.from_bytes(data);
}

std::string wstringToString(const std::wstring &data) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.to_bytes(data);
}

//////////////////////////////////////////////////////////////
// Archive Open callback class

class CArchiveOpenCallback Z7_final : public IArchiveOpenCallback,
                                      public ICryptoGetTextPassword,
                                      public CMyUnknownImp {
  Z7_IFACES_IMP_UNK_2(IArchiveOpenCallback, ICryptoGetTextPassword)
public:
  bool PasswordIsDefined;
  UString Password;

  CArchiveOpenCallback() : PasswordIsDefined(false) {}
};

Z7_COM7F_IMF(CArchiveOpenCallback::SetTotal(const UInt64 * /* files */,
                                            const UInt64 * /* bytes */)) {
  return S_OK;
}

Z7_COM7F_IMF(CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */,
                                                const UInt64 * /* bytes */)) {
  return S_OK;
}

Z7_COM7F_IMF(CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)) {
  if (!PasswordIsDefined) {
    // You can ask real password here from user
#if 0
    RINOK(GetPassword_HRESULT(&g_StdOut, Password))
    PasswordIsDefined = true;
#else
    return E_ABORT;
#endif
  }
  return StringToBstr(Password, password);
}

class SevenZipLibrary {
public:
  SevenZipLibrary() {
    FString dllPrefix;
#ifdef _WIN32
    dllPrefix = NWindows::NDLL::GetModuleDirPrefix();
#else
    {
      AString s(args[0]);
      int sep = s.ReverseFind_PathSepar();
      s.DeleteFrom(sep + 1);
      dllPrefix = s;
    }
#endif

    if (!library_.Load(dllPrefix + FTEXT(kDllName))) {
      throw std::runtime_error("Cannot load 7-zip library");
    }

    f_CreateObject_ = Z7_GET_PROC_ADDRESS(
        Func_CreateObject, library_.Get_HMODULE(), "CreateObject");
  }
  ~SevenZipLibrary() = default;

  NWindows::NDLL::CLibrary &getLibrary() { return library_; }

  Func_CreateObject &getCreateObjectFunc() { return f_CreateObject_; }

private:
  NWindows::NDLL::CLibrary library_;
  Func_CreateObject f_CreateObject_;
};

enum class ArchiveType : std::uint8_t { UnknownType = 0, InType, OutType };

enum class CompressType : std::uint8_t {
  UnknownType = 0,
  ZipType,
  BZip2,
  Sz,
  Xz,
  Tar,
  GZip
};

template <ArchiveType AType, CompressType ZType> class SevenZipArchive {
public:
  SevenZipArchive(SevenZipLibrary &szl) : sevenZipLibrary_(szl) { build(); }

  ~SevenZipArchive() noexcept = default;

  CMyComPtr<IInArchive> &getArchive() { return archive_; }

private:
  void build() {
    auto &f_CreateObject = sevenZipLibrary_.getCreateObjectFunc();
    if constexpr (AType == ArchiveType::InType) {
      if constexpr (ZType == CompressType::ZipType) {
        if (f_CreateObject(&CLSID_Format_Zip, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::BZip2) {
        if (f_CreateObject(&CLSID_Format_BZip2, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Sz) {
        if (f_CreateObject(&CLSID_Format_7z, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Xz) {
        if (f_CreateObject(&CLSID_Format_Xz, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Tar) {
        if (f_CreateObject(&CLSID_Format_Tar, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::GZip) {
        if (f_CreateObject(&CLSID_Format_GZip, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else {
        throw std::logic_error("Invalid compress type");
      }
    } else if constexpr (AType == ArchiveType::OutType) {
      if constexpr (ZType == CompressType::ZipType) {
        if (f_CreateObject(&CLSID_Format_Zip, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::BZip2) {
        if (f_CreateObject(&CLSID_Format_BZip2, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Sz) {
        if (f_CreateObject(&CLSID_Format_7z, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Xz) {
        if (f_CreateObject(&CLSID_Format_Xz, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::Tar) {
        if (f_CreateObject(&CLSID_Format_Tar, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (ZType == CompressType::GZip) {
        if (f_CreateObject(&CLSID_Format_GZip, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else {
        throw std::logic_error("Invalid compress type");
      }
    } else {
      throw std::logic_error("Invalid archive type");
    }
  }

  CMyComPtr<IInArchive> archive_;
  SevenZipLibrary &sevenZipLibrary_;
};

template <ArchiveType AT, CompressType ZT,
          class StringType = CMultiVolumeMemoryInStream>
class SevenZipFile {
public:
  SevenZipFile(SevenZipArchive<AT, ZT> &archive,
               const std::vector<fs::path> &files,
               std::pmr::memory_resource *mr = std::pmr::get_default_resource())
      : archive_(archive), stream_(files, mr) {
    file_ = &stream_;
  }

  HRESULT open(const std::wstring &password) {
    CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
    openCallbackSpec->PasswordIsDefined = true;
    openCallbackSpec->Password = password.c_str();
    const UInt64 scanSize = 1 << 23;
    return archive_.getArchive()->Open(file_, &scanSize, openCallback);
  }

  HRESULT open() {
    CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
    openCallbackSpec->PasswordIsDefined = false;
    const UInt64 scanSize = 1 << 23;
    return archive_.getArchive()->Open(file_, &scanSize, openCallback);
  }

  void reset() { stream_.Seek(0, STREAM_SEEK_SET, nullptr); }

  StringType &get_stream() { return stream_; }

private:
  SevenZipArchive<AT, ZT> &archive_;
  StringType stream_;
  CMyComPtr<IInStream> file_;
};
