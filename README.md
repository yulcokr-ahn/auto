// --------------------------- 구조체 ---------------------------
struct COMMIT_info12
{
	CString FileName;
	CString Title;
	CString Author;
	CString Date;
	CString msg;
	CString shortid;
};

typedef std::map<CString, COMMIT_info12> CommitMap;

// 파일 인덱스 정보
struct FileIndex
{
	__int64 offset;   // .dat 파일에서 시작 위치
	DWORD length;     // 블록 길이
};

// --------------------------- 전역 ---------------------------
std::map<CString, FileIndex> g_FileIndexMap;  // .idx 인덱스
CString g_datFilePath;                         // 데이터 파일 경로
CString g_idxFilePath;                         // 인덱스 파일 경로

// --------------------------- 저장 ---------------------------
BOOL CRestGitApi::saveMemdb_commitTreeToIdxDat(LPCTSTR lpszIdxPath, LPCTSTR lpszDatPath)
{
	if (!lpszIdxPath || !lpszDatPath) return FALSE;

	// 1️⃣ 데이터 파일 열기
	HANDLE hDat = CreateFile(lpszDatPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDat == INVALID_HANDLE_VALUE) return FALSE;

	// 2️⃣ 인덱스 파일 열기
	HANDLE hIdx = CreateFile(lpszIdxPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIdx == INVALID_HANDLE_VALUE) { CloseHandle(hDat); return FALSE; }

	// 3️⃣ 초기화
	__int64 curOffset = 0;
	g_FileIndexMap.clear();

	for (auto itFile = g_FileComMap12.begin(); itFile != g_FileComMap12.end(); ++itFile)
	{
		const CString& fileKey = itFile->first;
		CommitMap& commitMap = itFile->second;

		// 메모리 블록 직렬화 (임시 버퍼)
		std::vector<BYTE> buffer;
		auto appendString = [&](const CString& s) {
			DWORD len = (DWORD)s.GetLength();
			buffer.insert(buffer.end(), (BYTE*)&len, (BYTE*)&len + sizeof(len));
			if (len > 0) buffer.insert(buffer.end(), (BYTE*)(LPCTSTR)s, (BYTE*)(LPCTSTR)s + len * sizeof(wchar_t));
		};

		DWORD commitCount = (DWORD)commitMap.size();
		buffer.insert(buffer.end(), (BYTE*)&commitCount, (BYTE*)&commitCount + sizeof(commitCount));

		for (auto itCommit = commitMap.begin(); itCommit != commitMap.end(); ++itCommit)
		{
			appendString(itCommit->first);          // commitHash
			const COMMIT_info12& info = itCommit->second;
			appendString(info.Date);
			appendString(info.FileName);
			appendString(info.Title);
			appendString(info.Author);
			appendString(info.msg);
			appendString(info.shortid);
		}

		// 4️⃣ 데이터 파일에 쓰기
		DWORD written = 0;
		WriteFile(hDat, buffer.data(), (DWORD)buffer.size(), &written, NULL);

		// 5️⃣ 인덱스에 기록
		FileIndex fidx;
		fidx.offset = curOffset;
		fidx.length = (DWORD)buffer.size();
		g_FileIndexMap[fileKey] = fidx;

		// 6️⃣ 인덱스용 현재 offset 증가
		curOffset += buffer.size();
	}

	// 7️⃣ 인덱스 파일 저장
	DWORD fileCount = (DWORD)g_FileIndexMap.size();
	WriteFile(hIdx, &fileCount, sizeof(fileCount), &fileCount, NULL);
	for (auto it = g_FileIndexMap.begin(); it != g_FileIndexMap.end(); ++it)
	{
		DWORD keyLen = (DWORD)it->first.GetLength();
		WriteFile(hIdx, &keyLen, sizeof(keyLen), &keyLen, NULL);
		if (keyLen > 0) WriteFile(hIdx, (LPCTSTR)it->first, keyLen * sizeof(wchar_t), &keyLen, NULL);

		WriteFile(hIdx, &it->second.offset, sizeof(it->second.offset), &keyLen, NULL);
		WriteFile(hIdx, &it->second.length, sizeof(it->second.length), &keyLen, NULL);
	}

	CloseHandle(hDat);
	CloseHandle(hIdx);

	g_datFilePath = lpszDatPath;
	g_idxFilePath = lpszIdxPath;

	TRACE(_T("saveMemdb_commitTreeToIdxDat: 저장 완료\n"));
	return TRUE;
}

// --------------------------- 인덱스 로드 ---------------------------
BOOL CRestGitApi::loadMemdb_commitTreeIdx(LPCTSTR lpszIdxPath, LPCTSTR lpszDatPath)
{
	if (!lpszIdxPath || !lpszDatPath) return FALSE;

	HANDLE hIdx = CreateFile(lpszIdxPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIdx == INVALID_HANDLE_VALUE) return FALSE;

	DWORD read = 0;
	DWORD fileCount = 0;
	ReadFile(hIdx, &fileCount, sizeof(fileCount), &read, NULL);

	g_FileIndexMap.clear();
	for (DWORD i = 0; i < fileCount; ++i)
	{
		DWORD keyLen = 0;
		ReadFile(hIdx, &keyLen, sizeof(keyLen), &read, NULL);
		CString fileKey;
		if (keyLen > 0)
		{
			std::vector<wchar_t> buf(keyLen);
			ReadFile(hIdx, buf.data(), keyLen * sizeof(wchar_t), &read, NULL);
			fileKey = CString(buf.data(), keyLen);
		}

		FileIndex fidx;
		ReadFile(hIdx, &fidx.offset, sizeof(fidx.offset), &read, NULL);
		ReadFile(hIdx, &fidx.length, sizeof(fidx.length), &read, NULL);

		g_FileIndexMap[fileKey] = fidx;
	}

	CloseHandle(hIdx);
	g_datFilePath = lpszDatPath;
	g_idxFilePath = lpszIdxPath;

	TRACE(_T("loadMemdb_commitTreeIdx: 인덱스 적재 완료\n"));
	return TRUE;
}

// --------------------------- 블록 단위 읽기 (Lazy Loading) ---------------------------
BOOL CRestGitApi::loadCommitMapBlock(const CString& fileKey, CommitMap& outCommitMap)
{
	if (g_FileIndexMap.find(fileKey) == g_FileIndexMap.end()) return FALSE;

	FileIndex fidx = g_FileIndexMap[fileKey];

	HANDLE hDat = CreateFile(g_datFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDat == INVALID_HANDLE_VALUE) return FALSE;

	SetFilePointer(hDat, (LONG)fidx.offset, NULL, FILE_BEGIN);

	std::vector<BYTE> buffer(fidx.length);
	DWORD read = 0;
	ReadFile(hDat, buffer.data(), fidx.length, &read, NULL);
	CloseHandle(hDat);

	// 파싱
	LPBYTE p = buffer.data();
	DWORD commitCount = *(DWORD*)p; p += sizeof(DWORD);

	auto readString = [&](CString& s) {
		DWORD len = *(DWORD*)p; p += sizeof(DWORD);
		if (len > 0) { s = CString((LPCTSTR)p, len); p += len * sizeof(wchar_t); }
		else s.Empty();
	};

	for (DWORD i = 0; i < commitCount; ++i)
	{
		CString commitHash, date, fileName, title, author, msg, shortid;
		readString(commitHash);
		readString(date);
		readString(fileName);
		readString(title);
		readString(author);
		readString(msg);
		readString(shortid);

		COMMIT_info12 info;
		info.Date = date;
		info.FileName = fileName;
		info.Title = title;
		info.Author = author;
		info.msg = msg;
		info.shortid = shortid;

		outCommitMap[commitHash] = info;
	}

	return TRUE;
}











































extern	http_client* g_httpClient; //2025.11.14
::~(void) 
{ 
	if(g_httpClient)//2025.11.14
	{
		delete g_httpClient;
		g_httpClient = NULL;
	}
}

CString CgitAPI32App::getRunPath() // 실행 디렉터리구하기 2025.11.14
{
	char lpszModulePath[MAX_PATH];	CString strAppPath(_T(""));
	GetModuleFileName(NULL,lpszModulePath,MAX_PATH);	*strrchr(lpszModulePath,'\\') = '\0';	strAppPath = lpszModulePath;		
	return strAppPath;
}

void ::makeDirectoryAll(CString strFilePath) //xx
{
	if(strFilePath.Find('.') == -1)	{					
		if(!strFilePath.Right(1).Compare(_T("/"))) 			
			strFilePath = strFilePath.Left(strFilePath.GetLength() - 1);	
	}
	int dindex = strFilePath.ReverseFind('/');			
	if(dindex == -1) {									
		if(!::PathIsDirectory ( strFilePath )) 
			::CreateDirectory(strFilePath, NULL);		
		return;
	}
	strFilePath = strFilePath.Left(dindex);				
	if(!::PathIsDirectory ( strFilePath )) {
		makeDirectoryAll(strFilePath);						
		::CreateDirectory(strFilePath, NULL);		
	}
}
CString ::LoadComTreeCase(const CString& filenamedb)
{
//	char buffer[512]; DWORD length = GetCurrentDirectory(512, buffer);	if (length > 0 && length <= 512) 	{	TRACE("현재 폴더: %s\n", buffer);	} 	CString tag_path =CString(buffer)+"\\dmacashedb";  //makeDirectoryAll(runPath);


	CString runPath =theApp.getRunPath()+ _T("\\screen");	
	CString tag_path = runPath+ _T("\\dmacashedb");
	CFileFind find;
	if(!find.FindFile(runPath)) {
		find.Close(); 
		BOOL bCreate = CreateDirectory(runPath, NULL); 		
	}
	if(!find.FindFile(tag_path))	{
		find.Close(); 
		BOOL bCreate = CreateDirectory(tag_path, NULL); 		
	}

	BOOL bDownModezip = FALSE;
	CString fileNamezip = _T("dmacashedbin.zip");
	CFileFind finder;
	if (!finder.FindFile(tag_path+"\\"+fileNamezip))	{
		bDownModezip = TRUE; //download 필요
	}
	else	{
		bDownModezip = FALSE;
	}
	if(bDownModezip)	{
		json::value Outjson;
		CString sbranch    = _T("master");
		CString commitHash = _T("7fdca04ee23185c3cd55ae4d3209e3deff1dbe32");
		string outfileContent;
		DownloadGit_stringBuff(	CString(g_tPROJECTPATH.c_str()) , fileNamezip, tag_path+"\\",commitHash, sbranch, CString(g_tPRIVATE_TOKEN.c_str()) , outfileContent );
		CString savePath = tag_path+ "\\"+fileNamezip;
		CString extractFolder = tag_path;
		if (finder.FindFile(savePath))	{
			if (!UnzipFile(savePath, extractFolder))	{
				TRACE(_T("dmacashedbin.zip 압축 해제 실패\n"));
				return tag_path+ "\\"+filenamedb;
			}
		}
	}
	if (finder.FindFile(tag_path+ "\\"+filenamedb))	
	{
		loadMemdb_commitTreeBinfile(tag_path+ "\\"+filenamedb);	//		LoadComTreeFromFileTag(tag_path+ "\\"+filenamedb);	LoadComTreeFromBinaryFile(tag_path+ "\\"+filenamedb);
		TRACE(_T("LoadComTreeFromFileTag: 적재 완료 (%s)\n"), filenamedb);
	}
	return tag_path+ "\\"+filenamedb;
}
