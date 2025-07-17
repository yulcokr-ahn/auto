
nclude <cpprest/http_client.h>
#include <cpprest/json.h>
#include <fstream>
#include <sstream>
#include <cpprest/filestream.h>
#include <cpprest/producerconsumerstream.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;

// 파일 내용을 Base64로 인코딩
std::string readBinaryFileBase64(const std::string& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    std::ostringstream oss;
    oss << in.rdbuf();
    std::string content = oss.str();
    // base64 인코딩 함수 필요 (사용자 정의 또는 외부 base64 라이브러리)
    return utility::conversions::to_base64(content); 
}

// GitLab에 커밋 생성
void createCommitToGitLab(
    const std::wstring& gitlabToken,
    const std::wstring& projectID,
    const std::wstring& branch,
    const std::wstring& filePath,
    const std::wstring& commitUserName,
    const std::wstring& commitUserEmail,
    const std::wstring& stockMessage // 예: "AAPL 230.15"
) {
    std::wstring fullMessage = L"~@작성자: " + commitUserName + L" <" + commitUserEmail + L">\n~@주식현재가: " + stockMessage;

    // 파일 base64 인코딩
    std::string encodedContent = readBinaryFileBase64(utility::conversions::to_utf8string(filePath));

    // JSON 구성
    json::value requestBody;
    requestBody[U("branch")] = json::value::string(branch);
    requestBody[U("commit_message")] = json::value::string(fullMessage);
    requestBody[U("author_name")] = json::value::string(commitUserName);
    requestBody[U("author_email")] = json::value::string(commitUserEmail);

    // 액션 설정 (create file)
    json::value action;
    action[U("action")] = json::value::string(U("create"));
    action[U("file_path")] = json::value::string(filePath);
    action[U("content")] = json::value::string(utility::conversions::to_string_t(encodedContent));

    json::value actionsArray = json::value::array();
    actionsArray[0] = action;
    requestBody[U("actions")] = actionsArray;

    // HTTP 클라이언트
    http_client client(U("https://gitlab.com/api/v4/projects/") + uri::encode_data_string(projectID) + U("/repository/commits"));

    // 요청 설정
    http_request req(methods::POST);
    req.headers().add(U("PRIVATE-TOKEN"), gitlabToken);
    req.headers().add(U("Content-Type"), U("application/json"));
    req.set_body(requestBody);

    // 요청 전송
    client.request(req).then([](http_response response) {
        if (response.status_code() == status_codes::Created) {
            ucout << U("✅ Commit created successfully!") << std::endl;
        } else {
            ucout << U("❌ Failed to create commit: ") << response.status_code() << std::endl;
            response.extract_string().then([](utility::string_t body) {
                ucout << body << std::endl;
            }).wait();
        }
    }).wait();
}
📌 사용 예:
cpp
복사
편집
createCommitToGitLab(
    L"glpat-XXXXXXXXXXXXXXXXXXXXXXXX", // 공용 토큰
    L"12345678",                      // GitLab Project ID
    L"main",                          // 브랜치명
    L"stocks/AAPL_2025_0714.txt",     // 파일 경로
    L"홍길동",                         // 사용자 이름
    L"hong@example.com",              // 사용자 이메일
    L"AAPL 230.15"                    // 주식 정보
);












void ch::OnDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;
	int nIndex = pNMItemActivate->iItem;
TRACE("CHistory::OnDblclkLoglist(0) nIndex=[%d] \r\n",nIndex );
	if (nIndex < 0) return;
TRACE("CHistory::OnDblclkLoglist(1) nIndex=[%d] \r\n",nIndex );

	CString short_id_new;
	CString short_id_old;
#if OLD_GRID_MODE
	int nPos = m_LogList.GetItemData(nIndex);
	if (m_mapMode == 1 && nPos >= 0 && m_fileLast.size() > 0)	{
		TR_LASTcommit *pL = &m_fileLast[nPos];
		if (pL)	{
			short_id_new = m_LogList.GetItemText(nIndex, 0);  // 최신 커밋
			short_id_old = (CString)pL->parent_ids.c_str();   // 직전 커밋m_fileHist
		}
	}
	else if (m_mapMode == 2 && nPos >= 0 && m_fileHist.size() > 0)	{
		TR_HISTORYcommit *pH = &m_fileHist[nPos];
		if (pH)	{
			short_id_new = m_LogList.GetItemText(nIndex, 0);    
			if(pH->parent_ids.size()==0)
			{
				short_id_old = short_id_new;
			}
			else
			{
				short_id_old = (CString)pH->parent_ids[0].c_str(); 
			}
		}
	}
#else
	int nPos = m_loglistGrid.GetItemData(nIndex , 0);
	if (m_mapMode == 1 && nPos >= 0 && m_fileLast.size() > 0)	{
		TR_LASTcommit *pL = &m_fileLast[nPos];
		if (pL)	{
//			short_id_new = m_loglistGrid.GetItemText(nIndex, 0);  // 최신 커밋
			short_id_old = (CString)pL->parent_ids.c_str();   // 직전 커밋m_fileHist
		}
	}
	else if (m_mapMode == 2 && nPos >= 0 && m_fileHist.size() > 0)	{
		TR_HISTORYcommit *pH = &m_fileHist[nPos];
		if (pH)	{
//			short_id_new = m_loglistGrid.GetItemText(nIndex, 0);    
			short_id_old = (CString)pH->parent_ids[0].c_str(); 
		}
	}
#endif


	if (short_id_new.IsEmpty() || short_id_old.IsEmpty())	{
		short_id_old = short_id_new; 
	}
	std::vector<json::value> diffOld, diffNew;
	theApp.m_RGA.GetDiffCompareGitLab(	utility::conversions::to_string_t((LPCTSTR)short_id_old),
										utility::conversions::to_string_t((LPCTSTR)short_id_new), diffOld, diffNew);

	if (diffNew.size()==0 && diffOld.size()==0) 
	{
		theApp.m_RGA.GetDiffFromGitLab( utility::conversions::to_string_t((LPCTSTR)short_id_new), diffNew);
		theApp.m_RGA.GetDiffFromGitLab( utility::conversions::to_string_t((LPCTSTR)short_id_old), diffOld);
	}
	else	
	{
		theApp.m_RGA.GetDiffCompareGitLab( utility::conversions::to_string_t((LPCTSTR) short_id_old ),utility::conversions::to_string_t((LPCTSTR) short_id_new) ,diffOld,  diffNew  );
	}
	CString newPath ;
	CString oldPath ;
	if(diffNew.size())	{
		const auto& newDiff = diffNew[0];	newPath = CString(newDiff[U("new_path")].as_string().c_str());
	}
	if(diffOld.size())	{
		const auto& oldDiff = diffOld[0];	oldPath = CString(oldDiff[U("old_path")].as_string().c_str());
	}
	CString oldFileContent, newFileContent;
	CString sbranch= "main";
	theApp.m_RGA.DownloadGitlabBuff( CString(g_tPROJECTPATH.c_str()) , oldPath ,"_old_", short_id_old, sbranch,CString(g_tPRIVATE_TOKEN.c_str())  ,oldFileContent );
	theApp.m_RGA.DownloadGitlabBuff( CString(g_tPROJECTPATH.c_str()) , newPath ,"_new_", short_id_new, sbranch,CString(g_tPRIVATE_TOKEN.c_str())  ,newFileContent );
	if (oldFileContent.Compare(newFileContent) !=0)
	{	
		TRACE(_T("파일 내용이 동일합니다. 변경된 내용이 없습니다."));  //base: → 기준 버전 (이전 파일)	//mine: → 현재 버전 (최신 파일)
		newFileContent+= _T(" \r\n");
	}
	CString safeNew = newPath; safeNew.Replace(_T("\\"), _T("_")); safeNew.Replace(_T("/"), _T("_"));
	CString safeOld = oldPath; safeOld.Replace(_T("\\"), _T("_")); safeOld.Replace(_T("/"), _T("_"));
	CString tempNewFile = WriteTXT(_T("d:\\_new_") + safeNew, newFileContent);
	CString tempOldFile = WriteTXT(_T("d:\\_old_") + safeOld, oldFileContent);
	CString exePath = GetRunTimePath() + _T("\\TortoiseGitMerge.exe");
	CString cmd;	
	cmd.Format(_T("\"%s\" /base:\"%s\" /mine:\"%s\""), exePath, tempOldFile, tempNewFile);
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else	{
		TRACE(L"CreateProcess failed: %s\n", cmd);
	}
}

CString ch::WriteTXT(CString FileName, CString dataTxt)
{   
	if ((*FileName=='\0')) return 0;	//	BITMAPFILEHEADER	hdr;

#if 0
	HANDLE	hFile;
	DWORD	nByteWrite;
	hFile=CreateFile(			// open if exist ini file
		FileName,				// pointer to name of the file 
		GENERIC_WRITE,			// access mode 
		0,						// share mode 
		NULL,					// pointer to security descriptor 
		TRUNCATE_EXISTING,		// how to create 
		FILE_ATTRIBUTE_NORMAL,	// file attributes 
		NULL				 	// handle to file with attributes to copy  
		);
	if (hFile==INVALID_HANDLE_VALUE) return 0;	//check if file exist
	WriteFile(			// write ini (sync mode <-> no overlapped)
		hFile,			// handle of file to write 
		(LPSTR) dataTxt.GetBuffer(),	// address of buffer that contains data  
		dataTxt.GetLength(),	// number of bytes to write 
		&nByteWrite,	// address of number of bytes written 
		NULL	 		// address of structure for data 
		);
	CloseHandle(hFile);		//free file handle
#endif


	CFile file;
	CFileException feError;
	if( file.Open( FileName, CFile::modeWrite | CFile::modeCreate, &feError ) )
	{
		int max = dataTxt.GetLength(); 
		file.Write(dataTxt.GetBuffer(),(unsigned int) max);	//file.WriteString(dataTxt.GetBuffer());
		file.Close();
	}
	else
	{
		TCHAR	errBuff[256];
		feError.GetErrorMessage( errBuff, 256 );
		TRACE(errBuff);
	}
	return FileName;
}











✅ 목표
GitLab REST API로 특정 커밋(e9ba3183...)의 변경 내용 조회

변경된 파일 리스트를 보여주고

각 파일의 diff(차이) 를 컬러로 표시 (+추가, -삭제)

변경된 파일 내용을 다운로드할 수 있도록 구현

전체 구현은 C++ (C++ REST SDK / Casablanca) 기반

🔧 필요한 GitLab API
작업	API Endpoint
커밋 정보 조회	GET /projects/:id/repository/commits/:sha
커밋 diff 조회	GET /projects/:id/repository/commits/:sha/diff
파일 내용 다운로드	GET /projects/:id/repository/files/:file_path/raw?ref=:sha

✅ 1. 커밋 Diff 조회 및 리스트 출력 (컬러 기반)
cpp
복사
편집
void getCommitDiffAndDisplay(
    const std::wstring& token,
    const std::wstring& projectId,
    const std::wstring& commitSha
) {
    // diff API
    http_client client(U("https://gitlab.com/api/v4/projects/") +
                       uri::encode_data_string(projectId) +
                       U("/repository/commits/") + commitSha + U("/diff"));

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), token);

    client.request(req).then([](http_response response) {
        if (response.status_code() == status_codes::OK) {
            response.extract_json().then([](json::value jsonResponse) {
                for (auto& diff : jsonResponse.as_array()) {
                    std::wstring file = diff[U("new_path")].as_string();
                    std::wstring diffText = diff[U("diff")].as_string();

                    std::wcout << L"📁 파일: " << file << std::endl;

                    std::wistringstream iss(diffText);
                    std::wstring line;
                    while (std::getline(iss, line)) {
                        if (line.empty()) continue;
                        if (line[0] == L'+') {
                            std::wcout << L"\033[32m" << line << L"\033[0m" << std::endl; // 초록색 추가
                        } else if (line[0] == L'-') {
                            std::wcout << L"\033[31m" << line << L"\033[0m" << std::endl; // 빨간색 삭제
                        } else {
                            std::wcout << line << std::endl;
                        }
                    }
                    std::wcout << L"──────────────────────" << std::endl;
                }
            }).wait();
        } else {
            std::wcout << L"❌ Diff 조회 실패: " << response.status_code() << std::endl;
        }
    }).wait();
}
※ 콘솔 컬러 출력은 Windows Terminal / Linux Bash에서만 작동
MFC GUI일 경우 SetTextColor() 등을 활용하여 CRichEditCtrl, CListCtrl에서 구현 필요

✅ 2. 변경된 파일 원본 내용 다운로드
cpp
복사
편집
void downloadFileAtCommit(
    const std::wstring& token,
    const std::wstring& projectId,
    const std::wstring& filePath,
    const std::wstring& commitSha
) {
    std::wstring encodedFile = uri::encode_data_string(filePath);
    std::wstring url = L"https://gitlab.com/api/v4/projects/" +
                       uri::encode_data_string(projectId) +
                       L"/repository/files/" + encodedFile +
                       L"/raw?ref=" + commitSha;

    http_client client(url);
    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), token);

    client.request(req).then([filePath](http_response response) {
        if (response.status_code() == status_codes::OK) {
            response.extract_string().then([filePath](utility::string_t fileContent) {
                std::wstring saveName = L"downloaded_" + filePath;
                std::ofstream out(utility::conversions::to_utf8string(saveName), std::ios::binary);
                out << utility::conversions::to_utf8string(fileContent);
                out.close();
                std::wcout << L"✅ 파일 다운로드 완료: " << saveName << std::endl;
            }).wait();
        } else {
            std::wcout << L"❌ 다운로드 실패: " << response.status_code() << std::endl;
        }
    }).wait();
}
✅ 3. 전체 실행 예시
cpp
복사
편집
std::wstring token = L"glpat-xxxxxxxxxxxxxxxxxxxx";
std::wstring projectId = L"12345678";
std::wstring commitSha = L"e9ba31837cd6eb4d799991fd19b10d3245f8d837";

// diff + 출력
getCommitDiffAndDisplay(token, projectId, commitSha);

// 예시: 특정 파일 다운로드
downloadFileAtCommit(token, projectId, L"src/main.cpp", commitSha);
🎨 MFC에서 컬러 차이 출력하려면?
CRichEditCtrl 또는 CListCtrl + OwnerDraw 로 행별 +, - 색 다르게 설정

OnPaint()에서 CDC::SetTextColor(RGB(255, 0, 0)) 등 사용

✅ 추가 기능 구현 가능
CListCtrl 에 변경된 파일 리스트 출력

파일 더블클릭 시 diff 비교 화면 팝업

GitLab의 MR(Merge Request)처럼 좌우 비교창 제작

커밋 간 변경 이력 트리 구조 출력

✅ 정리
기능	구현 완료
커밋 diff 조회	✅
변경 파일 리스트	✅
컬러로 변경 구분	✅ (콘솔/GUI 분기 필요)
원본 다운로드	✅



















✅ 1. 컬러 출력 구성 요소 선택
컨트롤	설명
CRichEditCtrl	텍스트 내 특정 단어 또는 라인에 컬러, 서식을 적용 가능
CListCtrl (OwnerDraw)	각 행(Row)마다 색상/폰트 커스터마이징 가능, 리스트 형식에 적합
CEdit + CCustomDraw	최소 기능 구현 가능, 하지만 확장성 낮음

추천:

단순 색상 비교 → CListCtrl + OwnerDraw

문장 단위 Diff 표현 → CRichEditCtrl

✅ 2. CListCtrl에서 행별 색상 지정 (예: +추가, -삭제)
🔧 준비 단계
LVS_OWNERDRAWFIXED 스타일 설정

OnDrawItem() 핸들러 구현

🧪 예제 코드
cpp
복사
편집
void CMyListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    int nItem = lpDrawItemStruct->itemID;

    CString strText = GetItemText(nItem, 0); // 0번째 컬럼 텍스트
    COLORREF textColor = RGB(0, 0, 0); // 기본 검정

    if (strText[0] == '+') textColor = RGB(0, 128, 0);   // 추가: 초록
    else if (strText[0] == '-') textColor = RGB(255, 0, 0); // 삭제: 빨강

    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(textColor);

    CRect rc = lpDrawItemStruct->rcItem;
    pDC->DrawText(strText, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
}
✅ 3. Diff 비교 구현 전략
🎯 사용 시나리오
리스트에서 파일 더블클릭 → diff 비교 팝업

🎨 방법
TortoiseGitMerge 실행 (앞서 구현한 방식)

또는 직접 만들기: 좌우 CRichEditCtrl 또는 CListCtrl 2개 배치

✅ 4. Git처럼 MR 스타일 좌우 비교창 만들기
구성	설명
좌측 컨트롤	old 파일 내용 (빨간색 강조)
우측 컨트롤	new 파일 내용 (초록색 강조)
라인 동기화 스크롤	OnVScroll()을 양쪽 동시에 적용
변경 라인 강조	Diff 알고리즘 적용 후 컬러 태그 삽입

추천 도구: CSplitterWnd 사용 → 좌우 창 분할

✅ 5. 커밋 이력 트리 구현 (Git 스타일)
방법	컨트롤
트리 + 리스트 조합	CTreeCtrl + CListCtrl 또는 CWnd 커스터마이즈
전체 트리뷰 직접 그리기	CScrollView 기반 커스터마이징, OnDraw()에서 노드/라인 직접 그림

커밋 간 선 연결 + 머지 브랜치 표현 필요 시 CScrollView 또는 GDI+로 직접 그림

🔧 보너스: CRichEditCtrl 로 Diff 색상 입히기
cpp
복사
편집
CHARFORMAT cfRed;
cfRed.cbSize = sizeof(CHARFORMAT);
cfRed.dwMask = CFM_COLOR;
cfRed.crTextColor = RGB(255, 0, 0);

m_richEditCtrl.SetSel(start, end);
m_richEditCtrl.SetSelectionCharFormat(cfRed);
