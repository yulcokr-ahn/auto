//로컬 파일 읽기  GitLab 원격 파일 읽기 (REST API)
//두 내용을 비교 변경 시 자동 커밋 (REST API)
// 실행 결과
// 로컬 파일과 GitLab 원격 파일이 같음	"변경 없음 - 커밋 생략"
// 다름	GitLab에 자동 커밋 요청, 성공 시 메시지 표시

// ----------main
void CGitApi32Dlg::OnBnClickedCommit()
{
    CString localPath = _T("D:\\project\\htsmts\\test.txt");
    utility::string_t gitlabProject = U("root/htsmts");
    utility::string_t filePath = U("test.txt");
    utility::string_t message = U("자동 커밋: 파일 내용 변경됨");

    CommitIfLocalFileChanged(localPath, gitlabProject, filePath, message);
}



//------------------- GitLabHelper.h
#pragma once
#include <cpprest/http_client.h>
#include <cpprest/json.h>

void CommitIfLocalFileChanged(
    const CString& localFilePath,
    const utility::string_t& gitlabProjectPath,
    const utility::string_t& remoteFilePath,
    const utility::string_t& commitMessage
);

//------------------- GitLabHelper.cpp
#include "GitLabHelper.h"
#include <fstream>
#include <atlstr.h>

using namespace web;
using namespace web::http;
using namespace web::http::client;

#define GITLAB_API_URL U("http://localhost/api/v4/")
#define PRIVATE_TOKEN U("YOUR_ACCESS_TOKEN_HERE") // <-- GitLab Personal Access Token

CString ReadLocalFile(const CString& path)
{
    CStdioFile file;
    CString result, line;
    if (file.Open(path, CFile::modeRead | CFile::typeText))
    {
        while (file.ReadString(line))
            result += line + _T("\n");
        file.Close();
    }
    return result;
}

pplx::task<utility::string_t> GetGitLabRemoteFileContent(
    const utility::string_t& projectPath,
    const utility::string_t& filePath
)
{
    http_client client(GITLAB_API_URL);
    uri_builder builder(U("projects/"));
    builder.append(uri::encode_data_string(projectPath));
    builder.append(U("repository/files/"));
    builder.append(uri::encode_data_string(filePath));
    builder.append(U("/raw"));
    builder.append_query(U("ref"), U("main"));

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    req.set_request_uri(builder.to_uri());

    return client.request(req).then([](http_response resp) -> pplx::task<utility::string_t>
    {
        if (resp.status_code() == status_codes::OK)
            return resp.extract_string();
        return pplx::task_from_result(utility::string_t(U("")));
    });
}

void CommitIfLocalFileChanged(
    const CString& localFilePath,
    const utility::string_t& gitlabProjectPath,
    const utility::string_t& remoteFilePath,
    const utility::string_t& commitMessage
)
{
    CString localContent = ReadLocalFile(localFilePath);
    utility::string_t localUtf8 = utility::conversions::to_utf8string(localContent);

    GetGitLabRemoteFileContent(gitlabProjectPath, remoteFilePath).then([=](utility::string_t remoteContent)
    {
        if (remoteContent == localUtf8)
        {
            AfxMessageBox(_T("변경 없음 - 커밋 생략"));
            return;
        }

        // 커밋 수행
        http_client client(GITLAB_API_URL);
        uri_builder builder(U("projects/"));
        builder.append(uri::encode_data_string(gitlabProjectPath));
        builder.append(U("repository/commits"));

        json::value commitBody;
        commitBody[U("branch")] = json::value::string(U("main"));
        commitBody[U("commit_message")] = json::value::string(commitMessage);

        json::value action;
        action[U("action")] = json::value::string(U("update"));
        action[U("file_path")] = json::value::string(remoteFilePath);
        action[U("content")] = json::value::string(localUtf8);

        json::value actions = json::value::array();
        actions[0] = action;
        commitBody[U("actions")] = actions;

        http_request req(methods::POST);
        req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
        req.headers().add(U("Content-Type"), U("application/json"));
        req.set_request_uri(builder.to_uri());
        req.set_body(commitBody);

        client.request(req).then([](http_response resp)
        {
            if (resp.status_code() == status_codes::Created)
                AfxMessageBox(_T("✅ 커밋 성공"));
            else
            {
                CString msg;
                msg.Format(_T("❌ 커밋 실패 - HTTP %d"), resp.status_code());
                AfxMessageBox(msg);
            }
        }).wait();

    }).wait();
}
