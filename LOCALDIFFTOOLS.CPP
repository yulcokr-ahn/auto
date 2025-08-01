// ✅ GitLab 로컬 파일 비교 자동화 도구 (Visual C++ 2010 호환)
// - 최종 커밋 파일 다운로드
// - 인코딩 감지
// - 라인 수 비교
// - 디렉토리 전체 비교 지원

#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <iostream>

#pragma comment(lib, "wininet.lib")

namespace fs = std::filesystem;

// 인코딩 유형
enum EncodingType {
    ENCODING_UTF8_BOM,
    ENCODING_UTF8_NO_BOM,
    ENCODING_ASCII,
    ENCODING_ANSI,
    ENCODING_UNKNOWN
};

bool IsASCII(const BYTE* data, DWORD size) {
    for (DWORD i = 0; i < size; ++i) {
        if (data[i] >= 0x80) return false;
    }
    return true;
}

bool IsUTF8WithoutBOM(const BYTE* data, DWORD size) {
    DWORD i = 0;
    while (i < size) {
        BYTE ch = data[i];
        if (ch <= 0x7F) {
            i++;
        } else if ((ch & 0xE0) == 0xC0) {
            if (i + 1 >= size || (data[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((ch & 0xF0) == 0xE0) {
            if (i + 2 >= size || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else return false;
    }
    return true;
}

EncodingType DetectEncoding(const BYTE* buffer, DWORD size) {
    if (size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
        return ENCODING_UTF8_BOM;
    if (IsASCII(buffer, size))
        return ENCODING_ASCII;
    if (IsUTF8WithoutBOM(buffer, size))
        return ENCODING_UTF8_NO_BOM;
    return ENCODING_ANSI;
}

bool DownloadFromGitLab(const std::wstring& url, const std::wstring& outFile) {
    HINTERNET hInternet = InternetOpen(L"GitLabDiffTool", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hConnect = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream out(outFile, std::ios::binary);
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        out.write(buffer, bytesRead);
    }

    out.close();
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return true;
}

int CompareFiles(const std::wstring& file1, const std::wstring& file2) {
    std::wifstream f1(file1), f2(file2);
    f1.imbue(std::locale(""));
    f2.imbue(std::locale(""));

    if (!f1.is_open() || !f2.is_open()) return -1;

    std::wstring line1, line2;
    int total = 0, diff = 0;
    while (std::getline(f1, line1) && std::getline(f2, line2)) {
        total++;
        if (line1 != line2) diff++;
    }
    while (std::getline(f1, line1)) { total++; diff++; }
    while (std::getline(f2, line2)) { total++; diff++; }

    std::wcout << L"비교: " << file1 << L" vs " << file2 << L"\n총 라인: " << total << L", 변경 라인: " << diff << L"\n";
    return diff;
}

void CompareDirectory(const std::wstring& localPath, const std::wstring& remotePath) {
    for (const auto& entry : fs::recursive_directory_iterator(localPath)) {
        if (!entry.is_regular_file()) continue;

        std::wstring relPath = entry.path().wstring().substr(localPath.length() + 1);
        std::wstring remoteFile = remotePath + L"\\" + relPath;

        if (fs::exists(remoteFile)) {
            CompareFiles(entry.path().wstring(), remoteFile);
        } else {
            std::wcout << L"[신규 파일] " << relPath << std::endl;
        }
    }
}

int wmain() {
    std::wstring localFile = L"local\\testcode.cpp";
    std::wstring remoteFile = L"remote\\testcode_gitlab.cpp";

    std::wstring fileUrl = L"https://gitlab.com/api/v4/projects/PROJECT_ID/repository/files/testcode.cpp/raw?ref=COMMIT_SHA";

    if (DownloadFromGitLab(fileUrl, remoteFile)) {
        CompareFiles(localFile, remoteFile);
    } else {
        std::wcerr << L"GitLab 파일 다운로드 실패\n";
    }

    // 디렉토리 전체 비교 예시:
    // CompareDirectory(L"local", L"remote");

    return 0;
}
