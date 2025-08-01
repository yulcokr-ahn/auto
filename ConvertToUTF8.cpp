✅ 포함 기능 요약
기능	지원 여부
UTF-8 with BOM 감지	✅
UTF-8 without BOM 감지	✅
ASCII 감지	✅
ANSI (CP949) 감지 및 변환	✅
UTF-16LE/UTF-16BE 감지 및 변환	✅
EUC-KR 감지 (기본적으로 CP949로 처리)	✅
UTF-8 BOM으로 저장	✅

🧩 전체 코드: ConvertToUTF8.cpp
cpp
복사
편집
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>

enum EncodingType {
    ENCODING_UTF8_BOM,
    ENCODING_UTF8_NO_BOM,
    ENCODING_ASCII,
    ENCODING_ANSI,
    ENCODING_UTF16_LE,
    ENCODING_UTF16_BE,
    ENCODING_UNKNOWN
};

// ASCII 검사
bool IsASCII(const BYTE* buffer, DWORD size) {
    for (DWORD i = 0; i < size; ++i) {
        if (buffer[i] >= 0x80) return false;
    }
    return true;
}

// UTF-8 without BOM 검사
bool IsUTF8WithoutBOM(const BYTE* data, DWORD size) {
    DWORD i = 0;
    while (i < size) {
        BYTE ch = data[i];
        if (ch <= 0x7F) {
            i++;
        } else if ((ch & 0xE0) == 0xC0) {
            if (i + 1 >= size || (data[i + 1] & 0xC0) != 0x80)
                return false;
            i += 2;
        } else if ((ch & 0xF0) == 0xE0) {
            if (i + 2 >= size ||
                (data[i + 1] & 0xC0) != 0x80 ||
                (data[i + 2] & 0xC0) != 0x80)
                return false;
            i += 3;
        } else if ((ch & 0xF8) == 0xF0) {
            if (i + 3 >= size ||
                (data[i + 1] & 0xC0) != 0x80 ||
                (data[i + 2] & 0xC0) != 0x80 ||
                (data[i + 3] & 0xC0) != 0x80)
                return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

// UTF-16 감지
EncodingType DetectUTF16(const BYTE* data, DWORD size) {
    if (size >= 2) {
        if (data[0] == 0xFF && data[1] == 0xFE)
            return ENCODING_UTF16_LE;
        if (data[0] == 0xFE && data[1] == 0xFF)
            return ENCODING_UTF16_BE;
    }
    return ENCODING_UNKNOWN;
}

// 인코딩 감지
EncodingType DetectEncoding(const BYTE* buffer, DWORD size) {
    if (size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
        return ENCODING_UTF8_BOM;

    EncodingType utf16 = DetectUTF16(buffer, size);
    if (utf16 != ENCODING_UNKNOWN)
        return utf16;

    if (IsASCII(buffer, size))
        return ENCODING_ASCII;
    if (IsUTF8WithoutBOM(buffer, size))
        return ENCODING_UTF8_NO_BOM;

    return ENCODING_ANSI;
}

// 파일을 UTF-8로 저장 (BOM 포함)
void SaveAsUTF8(const std::wstring& filename, const std::wstring& wideText) {
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8Str(utf8Len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideText.c_str(), -1, &utf8Str[0], utf8Len, NULL, NULL);

    std::ofstream file(filename.c_str(), std::ios::binary);
    if (file.is_open()) {
        unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        file.write(reinterpret_cast<char*>(bom), 3);
        file.write(utf8Str.c_str(), utf8Str.size() - 1); // remove null
        file.close();
    }
}

// 인코딩 감지 및 변환 저장
bool ConvertFileToUTF8(const std::wstring& filename) {
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return false;
    }

    std::vector<BYTE> buffer(fileSize);
    DWORD bytesRead;
    ReadFile(hFile, &buffer[0], fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    EncodingType encoding = DetectEncoding(&buffer[0], fileSize);
    std::wstring wideText;

    switch (encoding) {
        case ENCODING_UTF8_BOM:
            // already UTF-8, skip
            return true;

        case ENCODING_UTF8_NO_BOM: {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&buffer[0], fileSize, NULL, 0);
            if (wlen <= 0) return false;
            wideText.resize(wlen);
            MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&buffer[0], fileSize, &wideText[0], wlen);
            break;
        }

        case ENCODING_UTF16_LE: {
            wideText.assign((wchar_t*)&buffer[2], (fileSize - 2) / 2); // skip BOM
            break;
        }

        case ENCODING_UTF16_BE: {
            // swap bytes
            for (DWORD i = 2; i + 1 < fileSize; i += 2) {
                wchar_t ch = (buffer[i] << 8) | buffer[i + 1];
                wideText.push_back(ch);
            }
            break;
        }

        case ENCODING_ASCII:
        case ENCODING_ANSI: {
            int wlen = MultiByteToWideChar(949, 0, (LPCSTR)&buffer[0], fileSize, NULL, 0);
            if (wlen <= 0) return false;
            wideText.resize(wlen);
            MultiByteToWideChar(949, 0, (LPCSTR)&buffer[0], fileSize, &wideText[0], wlen);
            break;
        }

        default:
            return false;
    }

    SaveAsUTF8(filename, wideText);
    return true;
}
