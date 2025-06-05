//쳇_전체코드 압축으로 작성 해줘 이것도 빠져 있네요 
// Visual Studio 2010 MFC Dialog 프로젝트 전체 구조와 코드, 리소스 포함해서 압축된 형태로 드리는 게 필요하시군요.

//-------------- 1. 솔루션 및 프로젝트 구조 markdown 복사 편집
GitLabMFCClient/
  GitLabMFCClient.sln
  GitLabMFCClient/
    GitLabMFCClient.cpp
    GitLabMFCClient.h
    GitLabMFCClientDlg.cpp
    GitLabMFCClientDlg.h
    resource.h
    GitLabMFCClient.rc
    stdafx.cpp
    stdafx.h
    targetver.h
    GitLabMFCClientDlg.rc2

//--------------2. GitLabMFCClient.sln    ini 복사 편집
Microsoft Visual Studio Solution File, Format Version 11.00
# Visual Studio 2010
Project("{...GUID...}") = "GitLabMFCClient", "GitLabMFCClient\GitLabMFCClient.vcproj", "{...GUID...}"
EndProject
Global
    GlobalSection(SolutionConfigurationPlatforms) = preSolution
        Debug|Win32 = Debug|Win32
        Release|Win32 = Release|Win32
    EndGlobalSection
    GlobalSection(ProjectConfigurationPlatforms) = postSolution
        {...GUID...}.Debug|Win32.ActiveCfg = Debug|Win32
        {...GUID...}.Debug|Win32.Build.0 = Debug|Win32
        {...GUID...}.Release|Win32.ActiveCfg = Release|Win32
        {...GUID...}.Release|Win32.Build.0 = Release|Win32
    EndGlobalSection
EndGlobal
(위 솔루션 파일은 자동 생성되므로, 프로젝트를 생성 후 소스만 교체해도 됩니다)

//-------------- 3. GitLabMFCClientDlg.h   cpp 복사 편집
#pragma once
#include <afxwin.h>
#include <afxcmn.h>

#define GITLAB_API_URL L"https://gitlab.com/api/v4/"
#define PRIVATE_TOKEN L"YOUR_PRIVATE_TOKEN" // 본인 토큰
#define PROJECT_PATH L"your-namespace/your-project" // ex) root/htsmts

class CGitLabMFCClientDlg : public CDialogEx
{
public:
    CGitLabMFCClientDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GITLABMFCCLIENT_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnBnClickedBtnGetBranches();
    afx_msg void OnBnClickedBtnCreateBranch();
    afx_msg void OnBnClickedBtnCreateMR();
    afx_msg void OnBnClickedBtnResetCheckout();
    afx_msg void OnBnClickedBtnCommitFiles();

    DECLARE_MESSAGE_MAP()

private:
    void ShowMessage(LPCTSTR msg);

public:
    CListBox m_listOutput;
    CEdit m_editInput;
};

//-------------- 4. GitLabMFCClientDlg.cpp cpp 복사 편집
#include "stdafx.h"
#include "GitLabMFCClient.h"
#include "GitLabMFCClientDlg.h"
#include <afxinet.h>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// IDD_GITLABMFCCLIENT_DIALOG 정의 필요
#ifndef IDD_GITLABMFCCLIENT_DIALOG
#define IDD_GITLABMFCCLIENT_DIALOG 101
#endif

CGitLabMFCClientDlg::CGitLabMFCClientDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_GITLABMFCCLIENT_DIALOG, pParent)
{
}

void CGitLabMFCClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_OUTPUT, m_listOutput);
    DDX_Control(pDX, IDC_EDIT_INPUT, m_editInput);
}

BEGIN_MESSAGE_MAP(CGitLabMFCClientDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_GET_BRANCHES, &CGitLabMFCClientDlg::OnBnClickedBtnGetBranches)
    ON_BN_CLICKED(IDC_BTN_CREATE_BRANCH, &CGitLabMFCClientDlg::OnBnClickedBtnCreateBranch)
    ON_BN_CLICKED(IDC_BTN_CREATE_MR, &CGitLabMFCClientDlg::OnBnClickedBtnCreateMR)
    ON_BN_CLICKED(IDC_BTN_RESET_CHECKOUT, &CGitLabMFCClientDlg::OnBnClickedBtnResetCheckout)
    ON_BN_CLICKED(IDC_BTN_COMMIT_FILES, &CGitLabMFCClientDlg::OnBnClickedBtnCommitFiles)
END_MESSAGE_MAP()

BOOL CGitLabMFCClientDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    // 초기화 작업
    m_listOutput.ResetContent();
    return TRUE;
}

void CGitLabMFCClientDlg::ShowMessage(LPCTSTR msg)
{
    m_listOutput.AddString(msg);
}

// 간단한 HTTP GET 요청 함수
CString HttpGet(CString url)
{
    CInternetSession session;
    CString result;
    try
    {
        CHttpFile* pFile = (CHttpFile*)session.OpenURL(url);
        CString strLine;
        while (pFile->ReadString(strLine))
        {
            result += strLine;
        }
        pFile->Close();
        delete pFile;
    }
    catch (CInternetException* e)
    {
        TCHAR szErr[512];
        e->GetErrorMessage(szErr, 512);
        result.Format(L"Error: %s", szErr);
        e->Delete();
    }
    return result;
}

// TODO: POST, PUT 요청도 CInternetSession, CHttpConnection, CHttpFile 사용해 구현 필요

void CGitLabMFCClientDlg::OnBnClickedBtnGetBranches()
{
    m_listOutput.AddString(L"Get Branches...");

    CString encodedProjectPath;
    // URL 인코딩(간단하게 '/' -> %2F)
    encodedProjectPath = PROJECT_PATH;
    encodedProjectPath.Replace(L"/", L"%2F");

    CString url = GITLAB_API_URL + L"projects/" + encodedProjectPath + L"/repository/branches";

    CString response = HttpGet(url);

    m_listOutput.AddString(response);
}

void CGitLabMFCClientDlg::OnBnClickedBtnCreateBranch()
{
    m_listOutput.AddString(L"Create Branch 기능은 POST 요청 필요합니다.");
    // POST 요청 코드 구현 필요
}

void CGitLabMFCClientDlg::OnBnClickedBtnCreateMR()
{
    m_listOutput.AddString(L"Merge Request 생성 기능은 POST 요청 필요합니다.");
    // POST 요청 코드 구현 필요
}

void CGitLabMFCClientDlg::OnBnClickedBtnResetCheckout()
{
    m_listOutput.AddString(L"Reset/Checkout 기능은 POST 또는 PUT 요청 필요합니다.");
    // POST/PUT 요청 코드 구현 필요
}

void CGitLabMFCClientDlg::OnBnClickedBtnCommitFiles()
{
    m_listOutput.AddString(L"커밋 자동화 기능은 POST 요청 및 로컬 파일 변경 감지 필요합니다.");
    // POST 요청, 로컬 git 상태 확인 및 커밋 로직 구현 필요
}

//-------------- 5. resource.h cpp 복사 편집
#define IDD_GITLABMFCCLIENT_DIALOG        101
#define IDC_LIST_OUTPUT                  1001
#define IDC_EDIT_INPUT                   1002
#define IDC_BTN_GET_BRANCHES             1003
#define IDC_BTN_CREATE_BRANCH            1004
#define IDC_BTN_CREATE_MR                1005
#define IDC_BTN_RESET_CHECKOUT           1006
#define IDC_BTN_COMMIT_FILES             1007

//-------------- 6. GitLabMFCClient.rc (리소스 파일 일부) rc 복사 편집
#include "resource.h"

IDD_GITLABMFCCLIENT_DIALOG DIALOGEX 0, 0, 320, 240
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "GitLab MFC Client"
FONT 9, "MS Shell Dlg"
BEGIN
    LISTBOX         IDC_LIST_OUTPUT,7,7,306,120,LBS_STANDARD | WS_VSCROLL | WS_BORDER | WS_TABSTOP
    EDITTEXT        IDC_EDIT_INPUT,7,135,306,20,ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP
    PUSHBUTTON      "Get Branches",IDC_BTN_GET_BRANCHES,7,170,90,23
    PUSHBUTTON      "Create Branch",IDC_BTN_CREATE_BRANCH,107,170,90,23
    PUSHBUTTON      "Create MR",IDC_BTN_CREATE_MR,207,170,90,23
    PUSHBUTTON      "Reset/Checkout",IDC_BTN_RESET_CHECKOUT,7,200,90,23
    PUSHBUTTON      "Commit Files",IDC_BTN_COMMIT_FILES,107,200,90,23
END

//-------------- 7. stdafx.h cpp 복사 편집
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#include <windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxdisp.h>
#include <afxdtctl.h>
#include <afxcmn.h>
#include <afxcontrolbars.h>

//-------------- 8. stdafx.cpp cpp 복사 편집
#include "stdafx.h"

//-------------- 9. targetver.h cpp 복사 편집
#pragma once

#include <sdkddkver.h>
