#pragma once

#include <codecvt>
#include <locale>
#include <stdexcept>

#include "../7zip/CPP/Common/MyInitGuid.h"
#include "../7zip/CPP/Common/MyWindows.h"

#include "../7zip/CPP/Common/Defs.h"
#include "../7zip/CPP/Common/IntToString.h"
#include "../7zip/CPP/Common/StringConvert.h"

#include "../7zip/CPP/Windows/DLL.h"
#include "../7zip/CPP/Windows/FileDir.h"
#include "../7zip/CPP/Windows/FileFind.h"
#include "../7zip/CPP/Windows/FileName.h"
#include "../7zip/CPP/Windows/NtCheck.h"
#include "../7zip/CPP/Windows/PropVariant.h"
#include "../7zip/CPP/Windows/PropVariantConv.h"

#include "../7zip/CPP/7zip/Common/FileStreams.h"

#include "../7zip/CPP/7zip/Archive/IArchive.h"

#include "../7zip/CPP/7zip/IPassword.h"
#include "../7zip/CPP/Common/UTFConvert.h"

#include "CMultiVolumeInStream.hpp"

#ifdef _WIN32
extern HINSTANCE g_hInstance;
HINSTANCE g_hInstance = nullptr;
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
  UString ustr;
  if (!Convert_UTF8_Buf_To_Unicode(data.c_str(), data.size(), ustr)) {
    throw std::logic_error("Cannot convert the data code");
  }
  return {ustr.GetBuf(), ustr.Len()};
}

std::string wstringToString(const std::wstring &data) {
  CByteBuffer buffer;
  Convert_Unicode_To_UTF8_Buf({data.c_str()}, buffer);
  return {(const char *)buffer.ConstData(), buffer.Size()};
}

static void Convert_UString_to_AString(const UString &s, AString &temp)
{
  int codePage = CP_OEMCP;
  /*
  int g_CodePage = -1;
  int codePage = g_CodePage;
  if (codePage == -1)
    codePage = CP_OEMCP;
  if (codePage == CP_UTF8)
    ConvertUnicodeToUTF8(s, temp);
  else
  */
    UnicodeStringToMultiByte2(temp, s, (UINT)codePage);
}

static FString CmdStringToFString(const char *s)
{
  return us2fs(GetUnicodeString(s));
}

static void Print(const char *s)
{
  fputs(s, stdout);
}

static void Print(const AString &s)
{
  Print(s.Ptr());
}

static void Print(const UString &s)
{
  AString as;
  Convert_UString_to_AString(s, as);
  Print(as);
}

static void Print(const wchar_t *s)
{
  Print(UString(s));
}

static void PrintNewLine()
{
  Print("\n");
}

static void PrintStringLn(const char *s)
{
  Print(s);
  PrintNewLine();
}

static void PrintError(const char *message)
{
  Print("Error: ");
  PrintNewLine();
  Print(message);
  PrintNewLine();
}

static void PrintError(const char *message, const FString &name)
{
  PrintError(message);
  Print(name);
}


static HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
  NWindows::NCOM::CPropVariant prop;
  RINOK(archive->GetProperty(index, propID, &prop))
  if (prop.vt == VT_BOOL)
    result = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt == VT_EMPTY)
    result = false;
  else
    return E_FAIL;
  return S_OK;
}

static HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
  return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

//////////////////////////////////////////////////////////////
// Archive Extracting callback class

static const char * const kTestingString    =  "Testing     ";
static const char * const kExtractingString =  "Extracting  ";
static const char * const kSkippingString   =  "Skipping    ";
static const char * const kReadingString    =  "Reading     ";

static const char * const kUnsupportedMethod = "Unsupported Method";
static const char * const kCRCFailed = "CRC Failed";
static const char * const kDataError = "Data Error";
static const char * const kUnavailableData = "Unavailable data";
static const char * const kUnexpectedEnd = "Unexpected end of data";
static const char * const kDataAfterEnd = "There are some data after the end of the payload data";
static const char * const kIsNotArc = "Is not archive";
static const char * const kHeadersError = "Headers Error";

static const wchar_t * const kEmptyFileAlias = L"[Content]";

struct CArcTime
{
  FILETIME FT;
  UInt16 Prec;
  Byte Ns100;
  bool Def;

  CArcTime()
  {
    Clear();
  }

  void Clear()
  {
    FT.dwHighDateTime = FT.dwLowDateTime = 0;
    Prec = 0;
    Ns100 = 0;
    Def = false;
  }

  bool IsZero() const
  {
    return FT.dwLowDateTime == 0 && FT.dwHighDateTime == 0 && Ns100 == 0;
  }

  int GetNumDigits() const
  {
    if (Prec == k_PropVar_TimePrec_Unix ||
        Prec == k_PropVar_TimePrec_DOS)
      return 0;
    if (Prec == k_PropVar_TimePrec_HighPrec)
      return 9;
    if (Prec == k_PropVar_TimePrec_0)
      return 7;
    int digits = (int)Prec - (int)k_PropVar_TimePrec_Base;
    if (digits < 0)
      digits = 0;
    return digits;
  }

  void Write_To_FiTime(CFiTime &dest) const
  {
   #ifdef _WIN32
    dest = FT;
   #else
    if (FILETIME_To_timespec(FT, dest))
    if ((Prec == k_PropVar_TimePrec_Base + 8 ||
         Prec == k_PropVar_TimePrec_Base + 9)
        && Ns100 != 0)
    {
      dest.tv_nsec += Ns100;
    }
   #endif
  }

  void Set_From_Prop(const PROPVARIANT &prop)
  {
    FT = prop.filetime;
    unsigned prec = 0;
    unsigned ns100 = 0;
    const unsigned prec_Temp = prop.wReserved1;
    if (prec_Temp != 0
        && prec_Temp <= k_PropVar_TimePrec_1ns
        && prop.wReserved3 == 0)
    {
      const unsigned ns100_Temp = prop.wReserved2;
      if (ns100_Temp < 100)
      {
        ns100 = ns100_Temp;
        prec = prec_Temp;
      }
    }
    Prec = (UInt16)prec;
    Ns100 = (Byte)ns100;
    Def = true;
  }
};

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

class CArchiveExtractCallback Z7_final:
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  Z7_IFACES_IMP_UNK_2(IArchiveExtractCallback, ICryptoGetTextPassword)
  Z7_IFACE_COM7_IMP(IProgress)

  CMyComPtr<IInArchive> _archiveHandler;
  FString _directoryPath;  // Output directory
  UString _filePath;       // name inside arcvhive
  FString _diskFilePath;   // full path to file on disk
  bool _extractMode;
  struct CProcessedFileInfo
  {
    CArcTime MTime;
    UInt32 Attrib;
    bool isDir;
    bool Attrib_Defined;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;

public:
  void Init(IInArchive *archiveHandler, const FString &directoryPath);

  UInt64 NumErrors;
  bool PasswordIsDefined;
  UString Password;

  CArchiveExtractCallback() : PasswordIsDefined(false) {}
};

void CArchiveExtractCallback::Init(IInArchive *archiveHandler, const FString &directoryPath)
{
  NumErrors = 0;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NWindows::NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetTotal(UInt64 /* size */))
{
  return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */))
{
  return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::GetStream(UInt32 index,
    ISequentialOutStream **outStream, Int32 askExtractMode))
{
  *outStream = nullptr;
  _outFileStream.Release();

  {
    // Get Name
    NWindows::NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidPath, &prop))
    
    UString fullPath;
    if (prop.vt == VT_EMPTY)
      fullPath = kEmptyFileAlias;
    else
    {
      if (prop.vt != VT_BSTR)
        return E_FAIL;
      fullPath = prop.bstrVal;
    }
    _filePath = fullPath;
  }

  if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
    return S_OK;

  {
    // Get Attrib
    NWindows::NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop))
    if (prop.vt == VT_EMPTY)
    {
      _processedFileInfo.Attrib = 0;
      _processedFileInfo.Attrib_Defined = false;
    }
    else
    {
      if (prop.vt != VT_UI4)
        return E_FAIL;
      _processedFileInfo.Attrib = prop.ulVal;
      _processedFileInfo.Attrib_Defined = true;
    }
  }

  RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.isDir))

  {
    _processedFileInfo.MTime.Clear();
    // Get Modified Time
    NWindows::NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop))
    switch (prop.vt)
    {
      case VT_EMPTY:
        // _processedFileInfo.MTime = _utcMTimeDefault;
        break;
      case VT_FILETIME:
        _processedFileInfo.MTime.Set_From_Prop(prop);
        break;
      default:
        return E_FAIL;
    }

  }
  {
    // Get Size
    NWindows::NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop))
    UInt64 newFileSize;
    /* bool newFileSizeDefined = */ ConvertPropVariantToUInt64(prop, newFileSize);
  }

  
  {
    // Create folders for file
    int slashPos = _filePath.ReverseFind_PathSepar();
    if (slashPos >= 0)
      NWindows::NFile::NDir::CreateComplexDir(_directoryPath + us2fs(_filePath.Left(slashPos)));
  }

  FString fullProcessedPath = _directoryPath + us2fs(_filePath);
  _diskFilePath = fullProcessedPath;

  if (_processedFileInfo.isDir)
  {
    NWindows::NFile::NDir::CreateComplexDir(fullProcessedPath);
  }
  else
  {
    NWindows::NFile::NFind::CFileInfo fi;
    if (fi.Find(fullProcessedPath))
    {
      if (!NWindows::NFile::NDir::DeleteFileAlways(fullProcessedPath))
      {
        PrintError("Cannot delete output file", fullProcessedPath);
        return E_ABORT;
      }
    }
    
    _outFileStreamSpec = new COutFileStream;
    CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
    if (!_outFileStreamSpec->Create_ALWAYS(fullProcessedPath))
    {
      PrintError("Cannot open output file", fullProcessedPath);
      return E_ABORT;
    }
    _outFileStream = outStreamLoc;
    *outStream = outStreamLoc.Detach();
  }
  return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode))
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:  _extractMode = true; break;
  }
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:  Print(kExtractingString); break;
    case NArchive::NExtract::NAskMode::kTest:  Print(kTestingString); break;
    case NArchive::NExtract::NAskMode::kSkip:  Print(kSkippingString); break;
    case NArchive::NExtract::NAskMode::kReadExternal: Print(kReadingString); break;
    default:
      Print("??? "); break;
  }
  Print(_filePath);
  return S_OK;
}

Z7_COM7F_IMF(CArchiveExtractCallback::SetOperationResult(Int32 operationResult))
{
  switch (operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      NumErrors++;
      Print("  :  ");
      const char *s = nullptr;
      switch (operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
          s = kUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          s = kCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          s = kDataError;
          break;
        case NArchive::NExtract::NOperationResult::kUnavailable:
          s = kUnavailableData;
          break;
        case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
          s = kUnexpectedEnd;
          break;
        case NArchive::NExtract::NOperationResult::kDataAfterEnd:
          s = kDataAfterEnd;
          break;
        case NArchive::NExtract::NOperationResult::kIsNotArc:
          s = kIsNotArc;
          break;
        case NArchive::NExtract::NOperationResult::kHeadersError:
          s = kHeadersError;
          break;
      }
      if (s)
      {
        Print("Error : ");
        Print(s);
      }
      else
      {
        char temp[16];
        ConvertUInt32ToString((UInt32)operationResult, temp);
        Print("Error #");
        Print(temp);
      }
    }
  }

  if (_outFileStream)
  {
    if (_processedFileInfo.MTime.Def)
    {
      CFiTime ft;
      _processedFileInfo.MTime.Write_To_FiTime(ft);
      _outFileStreamSpec->SetMTime(&ft);
    }
    RINOK(_outFileStreamSpec->Close())
  }
  _outFileStream.Release();
  if (_extractMode && _processedFileInfo.Attrib_Defined)
    NWindows::NFile::NDir::SetFileAttrib_PosixHighDetect(_diskFilePath, _processedFileInfo.Attrib);
  PrintNewLine();
  return S_OK;
}


Z7_COM7F_IMF(CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password))
{
  if (!PasswordIsDefined)
  {
#if 0
    // You can ask real password here from user
    RINOK(GetPassword_HRESULT(&g_StdOut, Password))
    PasswordIsDefined = true;
#else
    PrintError("Password is not defined");
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
  Zip,
  BZip2,
  Sz,
  Xz,
  Tar,
  GZip
};

template <ArchiveType AT, CompressType ZT,
          class StreamType = CMultiVolumeInStream>
class SevenZipFile {
public:
  SevenZipFile(const std::vector<fs::path> &files,
               std::pmr::memory_resource *mr = std::pmr::get_default_resource())
      : stream_(files, mr), file_(&stream_) {}

  SevenZipFile(const SevenZipFile &) = delete;
  SevenZipFile(SevenZipFile &&szf) = delete;
  SevenZipFile &operator=(const SevenZipFile &) = delete;
  SevenZipFile &operator=(SevenZipFile &&) = delete;

  void reset() { file_->Seek(0, STREAM_SEEK_SET, nullptr); }

  CMyComPtr<IInStream> &getStream() { return file_; }

private:
  StreamType stream_;
  CMyComPtr<IInStream> file_;
};

template <ArchiveType AT, CompressType CT, class StreamType>
class SevenZipArchive {
public:
  SevenZipArchive(SevenZipLibrary &szl, SevenZipFile<AT, CT, StreamType> &file)
      : sevenZipLibrary_(szl), file_(file) {
    build();
  }

  SevenZipArchive(const SevenZipArchive &) = delete;
  SevenZipArchive(SevenZipArchive &&sza) noexcept = delete;
  SevenZipArchive &operator=(const SevenZipArchive &) = delete;
  SevenZipArchive &operator=(SevenZipArchive &&sza) noexcept = delete;

  ~SevenZipArchive() noexcept = default;

  CMyComPtr<IInArchive> &getArchive() { return archive_; }

  HRESULT open(const std::wstring &password) {
    file_.reset();
    CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
    openCallbackSpec->PasswordIsDefined = true;
    openCallbackSpec->Password = password.c_str();
    const UInt64 scanSize = 1 << 23;
    return archive_->Open(file_.getStream(), &scanSize, openCallback);
  }

  HRESULT open() {
    file_.reset();
    CArchiveOpenCallback *openCallbackSpec = new CArchiveOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback(openCallbackSpec);
    openCallbackSpec->PasswordIsDefined = false;
    const UInt64 scanSize = 1 << 23;
    return archive_->Open(file_.getStream(), &scanSize, openCallback);
  }

  HRESULT testExtract(const std::wstring &password)
  {
    CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
    extractCallbackSpec->Init(archive_, FString()); // second parameter is output folder path
    extractCallbackSpec->PasswordIsDefined = true;
      extractCallbackSpec->Password = password.c_str();
    return archive_->Extract(nullptr, (UInt32)(Int32)(-1), false, extractCallback);
  }

  HRESULT testExtract()
  {
    CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> extractCallback(extractCallbackSpec);
    extractCallbackSpec->Init(archive_, FString()); // second parameter is output folder path
    extractCallbackSpec->PasswordIsDefined = false;
    return archive_->Extract(nullptr, (UInt32)(Int32)(-1), false, extractCallback);
  }

private:
  void build() {
    auto &f_CreateObject = sevenZipLibrary_.getCreateObjectFunc();
    if constexpr (AT == ArchiveType::InType) {
      if constexpr (CT == CompressType::Zip) {
        if (f_CreateObject(&CLSID_Format_Zip, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::BZip2) {
        if (f_CreateObject(&CLSID_Format_BZip2, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Sz) {
        if (f_CreateObject(&CLSID_Format_7z, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Xz) {
        if (f_CreateObject(&CLSID_Format_Xz, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Tar) {
        if (f_CreateObject(&CLSID_Format_Tar, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::GZip) {
        if (f_CreateObject(&CLSID_Format_GZip, &IID_IInArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else {
        throw std::logic_error("Invalid compress type");
      }
    } else if constexpr (AT == ArchiveType::OutType) {
      if constexpr (CT == CompressType::Zip) {
        if (f_CreateObject(&CLSID_Format_Zip, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::BZip2) {
        if (f_CreateObject(&CLSID_Format_BZip2, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Sz) {
        if (f_CreateObject(&CLSID_Format_7z, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Xz) {
        if (f_CreateObject(&CLSID_Format_Xz, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::Tar) {
        if (f_CreateObject(&CLSID_Format_Tar, &IID_IOutArchive,
                           (void **)&archive_) != S_OK) {
          throw std::runtime_error("Cannot get class object");
        }
      } else if constexpr (CT == CompressType::GZip) {
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
  SevenZipFile<AT, CT, StreamType> &file_;
};
