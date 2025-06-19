// 구조: [파일내용 길이 10자리 ASCII + "\r\n"][파일내용][FileHeader = CRC32 + 파일명길이 + 파일명]

#include "stdafx.h"
#include <afx.h>
#include <afxcoll.h>
#include <iostream>
#include <zlib.h>

bool WriteLengthAsAscii(CFile& file, DWORD length)
{
    char lenStr[13] = {};
    sprintf_s(lenStr, "%010u\r\n", length); // 길이를 10자리 ASCII + \r\n
    return file.Write(lenStr, 12) == 12;
}

bool ReadLengthFromAscii(CFile& file, DWORD& length)
{
    char lenStr[13] = {};
    if (file.Read(lenStr, 12) != 12) return false;
    lenStr[10] = 0; // \r\n 제외
    length = (DWORD)atoi(lenStr);
    return true;
}

bool MergeFiles(const CStringArray& files, const CString& outPath)
{
    CFile outFile;
    if (!outFile.Open(outPath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) return false;

    for (int i = 0; i < files.GetSize(); ++i) {
        const CString& path = files[i];
        CFile inFile;
        if (!inFile.Open(path, CFile::modeRead | CFile::typeBinary)) continue;

        DWORD fileLength = (DWORD)inFile.GetLength();
        BYTE* buffer = new BYTE[fileLength];
        inFile.Read(buffer, fileLength);

        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, buffer, fileLength);

        DWORD nameLength = path.GetLength() * sizeof(TCHAR);

        // 파일내용 길이 (10자리 + \r\n)
        WriteLengthAsAscii(outFile, fileLength);
        // 파일내용
        outFile.Write(buffer, fileLength);

        // FileHeader 작성: CRC32, 파일명 길이, 파일명
        outFile.Write(&crc, sizeof(DWORD));
        outFile.Write(&nameLength, sizeof(DWORD));
        outFile.Write(path, nameLength);

        delete[] buffer;
        inFile.Close();
    }

    outFile.Close();
    return true;
}

bool SplitFiles(const CString& inPath)
{
    CFile inFile;
    if (!inFile.Open(inPath, CFile::modeRead | CFile::typeBinary)) return false;

    while (inFile.GetPosition() < inFile.GetLength()) {
        DWORD fileLength = 0;
        if (!ReadLengthFromAscii(inFile, fileLength)) break;

        BYTE* buffer = new BYTE[fileLength];
        if (inFile.Read(buffer, fileLength) != fileLength) { delete[] buffer; break; }

        DWORD crcStored = 0;
        if (inFile.Read(&crcStored, sizeof(DWORD)) != sizeof(DWORD)) { delete[] buffer; break; }

        DWORD nameLength = 0;
        if (inFile.Read(&nameLength, sizeof(DWORD)) != sizeof(DWORD)) { delete[] buffer; break; }

        CString path;
        if (inFile.Read(path.GetBuffer(nameLength / sizeof(TCHAR)), nameLength) != nameLength) {
            delete[] buffer;
            break;
        }
        path.ReleaseBuffer(nameLength / sizeof(TCHAR));

        uLong crcCalc = crc32(0L, Z_NULL, 0);
        crcCalc = crc32(crcCalc, buffer, fileLength);

        if (crcStored != crcCalc) {
            AfxMessageBox(_T("CRC 오류: ") + path);
            delete[] buffer;
            continue;
        }

        CFile outFile;
        if (outFile.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) {
            outFile.Write(buffer, fileLength);
            outFile.Close();
        }

        delete[] buffer;
    }

    inFile.Close();
    return true;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    CStringArray files;
    files.Add(_T("A.cpp"));
    files.Add(_T("B.cpp"));
    files.Add(_T("C.cpp"));
    files.Add(_T("D.cpp"));
    files.Add(_T("E.cpp"));
    files.Add(_T("F.cpp"));

    if (!MergeFiles(files, _T("CombinedData.bin"))) {
        std::wcout << L"병합 실패\n";
    } else {
        std::wcout << L"병합 성공\n";
    }

    if (!SplitFiles(_T("CombinedData.bin"))) {
        std::wcout << L"분리 실패\n";
    } else {
        std::wcout << L"분리 성공\n";
    }
    return 0;
}
