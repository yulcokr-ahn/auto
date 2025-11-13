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
