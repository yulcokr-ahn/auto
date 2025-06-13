#include "stdafx.h"
#include "CommitDlg.h"
#include "afxdialogex.h"




#include <cpprest/http_client.h>
#include <cpprest/json.h>
using namespace std;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace utility;     // string_t 정의
using namespace web::json;   // json::value
using namespace utility;                    // Common utilities like string conversions
using namespace concurrency::streams;       // Asynchronous streams
using namespace pplx;



#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <afx.h>      // CString
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

struct GitDiffLine
{
	CString line;
	bool isAddition;
	bool isDeletion;
};

struct GitDiffLine2
{
	enum LineType { FileHeader, Index, OldFile, NewFile, HunkHeader, Added, Removed, Context };
	LineType type;
	CString content;
};

std::vector<GitDiffLine2> ParseGitDiffOutput(const CString& output)
{
	std::vector<GitDiffLine2> lines;
	std::istringstream input((LPCSTR)output);
	std::string lineStr;

	while (std::getline(input, lineStr))
	{
		GitDiffLine2 line;

		if (lineStr.find("diff --git") == 0)
			line.type = GitDiffLine2::FileHeader;
		else if (lineStr.find("index ") == 0)
			line.type = GitDiffLine2::Index;
		else if (lineStr.find("---") == 0)
			line.type = GitDiffLine2::OldFile;
		else if (lineStr.find("+++") == 0)
			line.type = GitDiffLine2::NewFile;
		else if (lineStr.find("@@") == 0)
			line.type = GitDiffLine2::HunkHeader;
		else if (lineStr.find("+") == 0)
			line.type = GitDiffLine2::Added;
		else if (lineStr.find("-") == 0)
			line.type = GitDiffLine2::Removed;
		else
			line.type = GitDiffLine2::Context;

		line.content = lineStr.c_str();  // std::wstring → CString
		lines.push_back(line);
	}

	return lines;
}

CString DetermineGitAction(TCHAR statusCode)
{
	switch (statusCode)
	{
	case 'A':
		return _T("create");
	case 'M':
	case 'R': // renamed
	case 'C': // copied
		return _T("update");
	case 'D':
		return _T("delete");
	default:
		return _T(""); // 알 수 없음
	}
}
/*
 git status --porcelain
A  new_file.txt    → create
M  modified.txt    → update
D  deleted.txt     → delete


// git status --porcelain 으로 한 줄 읽을 때
CString line = _T("A  new_file.txt");
TCHAR status = line[0]; // 'A'
CString path = line.Mid(3); // "new_file.txt"
CString action = DetermineGitAction(status);



*/







bool starts_with(const std::string& str, const std::string& prefix) {
	return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

void parse_file_paths(const std::string& line) {
	TRACE(_T("[파일 변경 시작]: %hs\n"), line.c_str());
}

void parse_index_info(const std::string& line) {
	TRACE(_T("[인덱스 정보]: %hs\n"), line.c_str());
}

std::string parse_file(const std::string& line) {
	std::string filepath = line.substr(line.find_last_of(' ') + 1);
	TRACE(_T("[파일]: %hs\n"), filepath.c_str());
	return filepath;
}

void parse_hunk_header(const std::string& line) {
	TRACE(_T("[청크 헤더]: %hs\n"), line.c_str());
}

void parse_git_diff_output(FILE* fp) {
	char buffer[1024];
	std::string line;
	std::string old_file, new_file;

	while (fgets(buffer, sizeof(buffer), fp)) {
		line = buffer;

		// 개행 제거
		if (!line.empty() && line[line.length() - 1] == '\n')
			line.erase(line.length() - 1);

		if (starts_with(line, "diff --git")) {
			parse_file_paths(line);
		} else if (starts_with(line, "index")) {
			parse_index_info(line);
		} else if (starts_with(line, "---")) {
			old_file = parse_file(line);
		} else if (starts_with(line, "+++")) {
			new_file = parse_file(line);
		} else if (starts_with(line, "@@")) {
			parse_hunk_header(line);
		} else if (starts_with(line, "-")) {
			TRACE(_T("삭제된 라인: %hs\n"), line.c_str());
		} else if (starts_with(line, "+")) {
			TRACE(_T("추가된 라인: %hs\n"), line.c_str());
		} else {
			TRACE(_T("  변경 없음: %hs\n"), line.c_str());
		}
	}
}

/*
git.exe status --porcelain
git.exe diff

:\gittest\htsmts>git.exe status --porcelain
M Window1.xaml.cs
M new_file.txt

D:\gittest\htsmts>git.exe diff
diff --git a/Window1.xaml.cs b/Window1.xaml.cs
index 8740259..82c9cba 100644
--- a/Window1.xaml.cs
+++ b/Window1.xaml.cs
@@ -5,7 +5,7 @@ using MSAvalon.Windows.Documents;
using MSAvalon.Windows.Navigation;
using MSAvalon.Windows.Shapes;
using MSAvalon.Windows.Media;
-
+hhhhhhhhhhhhhhhhhhhhhhhhhhhh^M
namespace XAMLTreemaps
{
/// <summary>
diff --git a/new_file.txt b/new_file.txt
index 9a7fdbf..77a9a49 100644
--- a/new_file.txt
+++ b/new_file.txt
@@ -1 +1,2 @@
-{"error":"404 Not Found"}
\ No newline at end of file
+{"error":"404 Not Found"}
+eeeeeeeeeeeeeeeeeeeeeeeeeeeee
\ No newline at end of file

D:\gittest\htsmts>


t 명령어의 출력 결과를 파싱하는 방법과 그 의미를 설명해 달라는 말씀이시죠?
특히, git status --porcelain 와 git diff 출력 결과를 어떻게 해석하고 파싱할지 알려드릴게요.

1. git status --porcelain 파싱
출력 예시
nginx
복사
편집
M Window1.xaml.cs
M new_file.txt
의미
출력 형식은 2문자 상태 코드 + 공백 + 파일 경로 형태입니다.

두 문자 각각 의미가 다릅니다.

문자 위치	의미	예시
1번째 문자	Index 상태 (스테이징 영역)	M = Modified (수정됨), A = Added, D = Deleted 등
2번째 문자	작업 디렉터리 상태	M = Modified, ? = untracked, = unchanged

예를 들어
" M Window1.xaml.cs" : 1번째 문자가 공백이고 2번째 문자가 M → 작업 디렉터리에서 수정됨, 아직 스테이징 영역에는 없음.

"M file.txt" : 인덱스에서 수정됨, 작업 디렉터리는 변경 없음.

파싱 방법 (예: C++/C# pseudocode)
cpp
복사
편집
string line = " M Window1.xaml.cs";
char indexStatus = line[0];     // 첫 문자
char workTreeStatus = line[1];  // 두번째 문자
string filePath = line.substr(3); // 3번째 문자부터 파일 경로 시작

// 예시 출력
printf("Index: %c, WorkTree: %c, File: %s\n", indexStatus, workTreeStatus, filePath.c_str());
2. git diff 출력 파싱
git diff 출력은 매우 상세한 패치 정보입니다. 주요 구성은 다음과 같습니다.

구조
diff
복사
편집
diff --git a/Window1.xaml.cs b/Window1.xaml.cs
index 8740259..82c9cba 100644
--- a/Window1.xaml.cs
+++ b/Window1.xaml.cs
@@ -5,7 +5,7 @@ using MSAvalon.Windows.Documents;
using MSAvalon.Windows.Navigation;
using MSAvalon.Windows.Shapes;
using MSAvalon.Windows.Media;
-
+hhhhhhhhhhhhhhhhhhhhhhhhhhhh^M
diff --git a/파일 b/파일 : 변경된 파일 이름 (a/ 이전 버전, b/ 새 버전)

index <hash1>..<hash2> <mode> : Git 내부 인덱스 체크섬과 파일 모드

--- a/파일 : 이전 파일

+++ b/파일 : 변경된 파일

@@ -startLineOld,countOld +startLineNew,countNew @@ : 변경 범위 (구간)

변경 내용은 - 로 시작하면 삭제된 라인, + 로 시작하면 추가된 라인, 공백으로 시작하면 변경 없는 라인

예시
diff
복사
편집
@@ -5,7 +5,7 @@ using MSAvalon.Windows.Documents;
using MSAvalon.Windows.Navigation;
using MSAvalon.Windows.Shapes;
using MSAvalon.Windows.Media;
-
+hhhhhhhhhhhhhhhhhhhhhhhhhhhh^M
-5,7 은 원본 파일 5번째 줄부터 7줄이 변경 범위

+5,7 은 새 파일 5번째 줄부터 7줄이 변경 범위

-가 붙은 줄은 삭제, +가 붙은 줄은 추가

3. 파싱 팁 및 방법
git status --porcelain 는 줄 단위로 읽어 첫 2문자 + 파일경로를 분리

git diff 는 다음과 같은 패턴으로 파싱

diff --git 줄을 만나면 새 파일 패치 시작

index 줄에서 체크섬 정보 읽기

---, +++ 줄에서 이전/새 파일명 읽기

@@ ... @@ 줄에서 변경 범위 읽기

그 이후 줄에서 -, +, 로 시작하는 라인들을 변경 내용으로 저장

4. 파싱 예시 (의사코드)
cpp
복사
편집
while (getline(input, line)) {
if (line.starts_with("diff --git")) {
// 새 파일 변경 시작
parse_file_paths(line);
} else if (line.starts_with("index")) {
parse_index_info(line);
} else if (line.starts_with("---")) {
old_file = parse_file(line);
} else if (line.starts_with("+++")) {
new_file = parse_file(line);
} else if (line.starts_with("@@")) {
parse_hunk_header(line);
} else if (line.starts_with("-")) {
// 삭제 라인
} else if (line.starts_with("+")) {
// 추가 라인
} else {
// 변경 없음 라인
}
}
5. 요약
명령어	주요 내용	파싱 포인트
git status --porcelain	변경된 파일 상태를 간단히 표시	첫 2문자 상태 코드 + 파일 경로
git diff	파일별 변경 내용 및 라인 단위 변경 내역	diff --git, index, ---, +++, @@, +, -,









const int nListItems = m_ListCtrl.GetItemCount();
for (int i = 0; i < nListItems && !m_bCommitMessageOnly; ++i)
{
	auto entry = m_ListCtrl.GetListEntry(i);
	if (!entry->m_Checked || !entry->IsDirectory())
		continue;

	bool dirty = false;
	if (entry->m_Action & CTGitPath::LOGACTIONS_UNVER)
	{
		CGit subgit;
		subgit.m_IsUseGitDLL = false;
		subgit.m_CurrentDir = g_Git.CombinePath(entry);
		CString subcmdout;
		subgit.Run(L"git.exe status --porcelain", &subcmdout, CP_UTF8);
		dirty = !subcmdout.IsEmpty();
	}
	else
	{
		CString cmd, cmdout;
		cmd.Format(L"git.exe diff -- \"%s\"", entry->GetWinPath());
		g_Git.Run(cmd, &cmdout, CP_UTF8);
		dirty = CStringUtils::EndsWith(cmdout, L"-dirty\n");
	}

	if (dirty)
	{
		CString message;
		message.Format(IDS_COMMITDLG_SUBMODULEDIRTY, static_cast<LPCWSTR>(entry->GetGitPathString()));
		const auto result = CMessageBox::Show(m_hWnd, message, IDS_APPNAME, 1, IDI_QUESTION, IDS_PROGRS_CMD_COMMIT, IDS_MSGBOX_IGNORE, IDS_MSGBOX_CANCEL);
		if (result == 1)
		{
			CString cmdCommit;
			cmdCommit.Format(L"/command:commit /path:\"%s\\%s\"", static_cast<LPCWSTR>(g_Git.m_CurrentDir), entry->GetWinPath());
			CAppUtils::RunTortoiseGitProc(cmdCommit);
			return;
		}
		else if (result == 2)
			continue;
		else
			return;
	}
}

if (!m_bCommitMessageOnly)
	m_ListCtrl.WriteCheckedNamesToPathList(m_selectedPathList);
*/

IMPLEMENT_DYNAMIC(CCommitDlg, CDialog)

	CCommitDlg::CCommitDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommitDlg::IDD, pParent), m_bNewBranch(FALSE)
{
}

CCommitDlg::~CCommitDlg() { }

void CCommitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGMSG, m_logMsgCtrl); //DDX_Text(pDX, IDC_LOGMESSAGE, m_strCommitMsg);
	DDX_Text(pDX, IDC_BUGID, m_strBugID);
	DDX_Text(pDX, IDC_COMMIT_AUTHORDATA, m_strAuthor);
	DDX_Check(pDX, IDC_CHECK_NEWBRANCH, m_bNewBranch);
	DDX_Text(pDX, IDC_NEWBRANCH, m_strNewBranchName);
	DDX_Control(pDX, IDC_FILELIST, m_fileList);

	if (pDX->m_bSaveAndValidate) 
	{
		m_logMsgCtrl.GetWindowText(m_strCommitMsg);
	}
	else 
	{
		m_logMsgCtrl.SetWindowText(m_strCommitMsg);
	}

}

BEGIN_MESSAGE_MAP(CCommitDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CCommitDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_REFRESH, &CCommitDlg::OnBnClickedRefresh)
END_MESSAGE_MAP()

BOOL CCommitDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	AfxInitRichEdit2();
	m_logMsgCtrl.SetFont(&CFont()); 
	m_logMsgCtrl.SendMessage(EM_EXLIMITTEXT, 0, (LPARAM)4000); 
	SetWindowText(_T("Commit!!!!!!!!"));
	CString privateToken = _T("glpat-qYj17RdNrDxAWKqvt16o"); 
	utility::string_t projectID = U("root");
	utility::string_t projectPath = U("htsmts");
	CString projectID_Path  = _T("root%2Fhtsmts");
	CString Content_Type =_T("application/json");
	m_projectID = projectID_Path;
	m_accessToken = privateToken;
	m_fileList.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	m_fileList.InsertColumn(0, _T("파일 경로"), LVCFMT_LEFT, 300);

	RefreshFileList();



	// git diff 명령 실행
	CString gitDiffOutput;
	{
		CString cmd;
		CString gitDir = "D:\\gittest\\htsmts";
		cmd.Format("cd /d \"%s\" && git.exe diff", CStringA(gitDir));

		FILE* pipe = _popen(cmd, "r");
		if (pipe)
		{
			char buffer[128];
			std::string result;
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
			{
				result += buffer;
			}
			_pclose(pipe);

			// 멀티바이트 -> CString 변환 (유니코드 빌드 기준)
			gitDiffOutput = CString(CA2W(result.c_str()));
		}
	}

	// 파싱
	std::vector<GitDiffLine2> parsedLines = ParseGitDiffOutput(gitDiffOutput);

	// 출력
	for (size_t i = 0; i < parsedLines.size(); ++i)
	{
		const GitDiffLine2 & line = parsedLines[i];
		TRACE(_T("Line type: %d, Content: %s\n"), line.type, line.content);
	}


	return TRUE;
}

void CCommitDlg::OnBnClickedRefresh()
{
	RefreshFileList();








}

void CCommitDlg::RefreshFileList()
{
	m_fileList.DeleteAllItems();
	CStringArray arr;
	arr.Add(_T("CommitDlg.cpp"));	
	for (int i = 0; i < arr.GetSize(); i++)
		m_fileList.InsertItem(i, arr[i]);

	bool m_bCommitMessageOnly = TRUE;

	const int nListItems = m_fileList.GetItemCount();
	for (int i = 0; i < nListItems && m_bCommitMessageOnly; ++i)
	{
		CString filePath = m_fileList.GetItemText(i, 0);
		bool dirty = false;
		CString cmd, output;

		char buffer[1024] = {0};
		CString gitDir = "D:\\gittest\\htsmts";
		cmd.Format("cd /d \"%s\" && git.exe diff", CStringA(gitDir));
		FILE* fp = _popen(cmd, "r");
		if (fp) {
			parse_git_diff_output(fp);
			_pclose(fp);
		} else {
			MessageBox("git 명령 실행 실패", "오류", MB_ICONERROR);
		}










		dirty = CString(buffer);
		if (dirty)
		{
			CString message;
			message.Format("파일 %s 에 변경 사항이 있습니다. 커밋하시겠습니까?", filePath);
			const int result = 1;
//			const int result = CMessageBox::Show(m_hWnd, message, IDS_APPNAME, 1, IDI_QUESTION, IDS_PROGRS_CMD_COMMIT, IDS_MSGBOX_IGNORE, IDS_MSGBOX_CANCEL);
			if (result == 1)
			{
				CString cmdCommit;
				cmdCommit.Format("/command:commit /path:\"%s\"", filePath);
//				CAppUtils::RunTortoiseGitProc(cmdCommit);
				return;
			}
			else if (result == 2)
				continue;
			else
				return;
		}
	}

}

/*
D:\gittest\htsmts>git.exe diff new_file.txt
diff --git a/new_file.txt b/new_file.txt
index 9a7fdbf..77a9a49 100644
--- a/new_file.txt
+++ b/new_file.txt
@@ -1 +1,2 @@
-{"error":"404 Not Found"}
\ No newline at end of file
+{"error":"404 Not Found"}
+eeeeeeeeeeeeeeeeeeeeeeeeeeeee
\ No newline at end of file

D:\gittest\htsmts>
*/

void CCommitDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	if (m_strCommitMsg.IsEmpty()) {
		AfxMessageBox(_T("커밋 메시지를 입력하세요."));
		return;
	}

	if (!CommitToGitLab()) {
		AfxMessageBox(_T("커밋 실패"));
		return;
	}

	AfxMessageBox(_T("커밋 성공"));
	OnOK();
}


BOOL CCommitDlg::CommitToGitLab()
{
	try {
		http_client client(U("http://127.0.0.1/api/v4"));

		json::value root;
		CString branch = m_bNewBranch ? m_strNewBranchName : _T("main");
		CString commitMsg = m_strCommitMsg;
		if (!m_strBugID.IsEmpty()) {
			commitMsg += _T(" (") + m_strBugID + _T(")");
		}

		wchar_t* buf;
		std::wstring utf8branch; int len = MultiByteToWideChar(CP_ACP, 0, branch.GetString(), -1, NULL, 0);
		if (len > 0) 
		{	
			buf = new wchar_t[len];	MultiByteToWideChar(CP_ACP, 0, branch.GetString(), -1, buf, len);	utf8branch = buf;	delete[] buf;
		}

		std::wstring utf8commitMsg;		len = MultiByteToWideChar(CP_ACP, 0, commitMsg.GetString(), -1, NULL, 0);
		if (len > 0) 
		{	
			buf = new wchar_t[len];		MultiByteToWideChar(CP_ACP, 0, commitMsg.GetString(), -1, buf, len);	utf8commitMsg = buf;	delete[] buf;
		}

		std::wstring utf8strAuthor;		len = MultiByteToWideChar(CP_ACP, 0, m_strAuthor.GetString(), -1, NULL, 0);
		if (len > 0) 
		{	
			buf = new wchar_t[len];	MultiByteToWideChar(CP_ACP, 0, m_strAuthor.GetString(), -1, buf, len);	utf8strAuthor = buf;	delete[] buf;
		}
		root[U("branch")] = json::value::string(utility::conversions::to_string_t(utf8branch));	
		root[U("commit_message")] = json::value::string(utility::conversions::to_string_t(utf8commitMsg));
		if (!m_strAuthor.IsEmpty())
			root[U("author_name")] = json::value::string(utility::conversions::to_string_t(utf8strAuthor));	

		json::value actions = json::value::array();
		int count = m_fileList.GetItemCount();
		int idx = 0;
		for (int i = 0; i < count; ++i) {
			if (m_fileList.GetCheck(i)) 
			{
				CString filePath = m_fileList.GetItemText(i, 0);
				std::wstring utf8filePath;	len = MultiByteToWideChar(CP_ACP, 0, filePath.GetString(), -1, NULL, 0);
				if (len > 0) 
				{	
					buf = new wchar_t[len];	MultiByteToWideChar(CP_ACP, 0, filePath.GetString(), -1, buf, len);	utf8filePath = buf;	delete[] buf;
				}
				json::value act;
				act[U("action")]    = json::value::string(utility::conversions::to_string_t(U("update")));	
		//		act[U("action")]    = json::value::string(utility::conversions::to_string_t(U("create")));	

				utility::string_t tfilePath = utf8filePath;
				act[U("file_path")] = json::value::string(utility::conversions::to_string_t(tfilePath));	
				act[U("content")]   = json::value::string(utility::conversions::to_string_t(U("자동 커밋된 내용입니다."))); 

				actions[idx++] = act;
			}
		}

		root[U("actions")] = actions;

		// 요청 URL
		CString url;
		url.Format(_T("/projects/%s/repository/commits"), m_projectID);
		CStringA apiUrlA = url; // ANSI 문자열
		std::wstring apiUrlW;	len = MultiByteToWideChar(CP_ACP, 0, apiUrlA.GetString(), -1, NULL, 0);
		if (len > 0) 
		{	
			wchar_t* buf1 = new wchar_t[len];	MultiByteToWideChar(CP_ACP, 0, apiUrlA.GetString(), -1, buf1, len);		apiUrlW = buf1;	delete[] buf1;
		}

		http_request request(methods::POST);	
		request.set_request_uri(utility::conversions::to_string_t(apiUrlW));
		request.headers().add(U("PRIVATE-TOKEN"), utility::conversions::to_string_t((LPCTSTR)m_accessToken));
		request.headers().add(U("Content-Type"), U("application/json"));
		request.set_body(root);	//request.set_body(root.serialize());

		http_response response = client.request(request).get();	// 동기 방식 호출 (2010에서는 async task 안됨)
		if (response.status_code() == status_codes::Created) {
			return TRUE;
		}
		else {
			std::wcout << L"에러: " << response.status_code() << std::endl;
			std::wcout << L"❌ 커밋 실패. 상태 코드: " << response.status_code() << std::endl;
		}
	}
	catch (const std::exception& ex) {
		AfxMessageBox(CString("예외 발생: ") + CString(ex.what()));
	}
	return FALSE;
}
