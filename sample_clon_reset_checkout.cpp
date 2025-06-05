//항목	구현 방식	비고
//커밋 리스트 보기	REST API	GET /commits
//diff 보기	REST API	GET /commits/:sha/diff
//파일 수정 + 커밋 생성	REST API	POST /repository/commits
//reset, checkout, clone	직접 지원 ❌	로컬 Git 실행 또는 libgit2 필요
///////////// [ full code]
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <atlstr.h>
#include <process.h> // _wsystem
using namespace web;
using namespace web::http;
using namespace web::http::client;

#define GITLAB_API_URL   U("http://localhost/api/v4/")
#define PRIVATE_TOKEN    U("your_private_token")
#define PROJECT_ID       U("root%2Fyour_project")

// Git 명령 실행 함수
void RunGitCommand(LPCTSTR command)
{
    CString fullCmd;
    fullCmd.Format(_T("cmd.exe /C git %s"), command);
    _wsystem(fullCmd);
}

// git clone
void GitClone(LPCTSTR repoURL, LPCTSTR folder)
{
    CString cmd;
    cmd.Format(_T("clone %s %s"), repoURL, folder);
    RunGitCommand(cmd);
}

// git reset --hard <commit>
void GitResetHard(LPCTSTR commitID)
{
    CString cmd;
    cmd.Format(_T("reset --hard %s"), commitID);
    RunGitCommand(cmd);
}

// git checkout <branch>
void GitCheckout(LPCTSTR branch)
{
    CString cmd;
    cmd.Format(_T("checkout %s"), branch);
    RunGitCommand(cmd);
}

// 커밋 리스트 가져오기
void GetCommitList()
{
    http_client client(GITLAB_API_URL);
    uri_builder builder(U("projects/"));
    builder.append(PROJECT_ID).append(U("/repository/commits")).append_query(U("per_page"), U("10"));

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    req.set_request_uri(builder.to_uri());

    client.request(req).then([](http_response response)
    {
        if (response.status_code() == status_codes::OK)
        {
            json::value jsonVal = response.extract_json().get();
            for (size_t i = 0; i < jsonVal.size(); ++i)
            {
                const json::value& item = jsonVal[i];
                CString id(item[U("id")].as_string().c_str());
                CString title(item[U("title")].as_string().c_str());
                TRACE("Commit ID: %s, Title: %s\n", id, title);
            }
        }
        else
        {
            std::wcout << L"[Error] GetCommitList() failed: " << response.status_code() << std::endl;
        }
    }).wait();
}

// 특정 커밋의 diff 보기
void GetDiff(const utility::string_t& commit_id)
{
    utility::string_t url = GITLAB_API_URL + U("projects/") + PROJECT_ID + U("/repository/commits/") + commit_id + U("/diff");
    http_client client(url);

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);

    client.request(req).then([commit_id](http_response response)
    {
        if (response.status_code() == status_codes::OK)
        {
            json::value jsonVal = response.extract_json().get();
            for (size_t i = 0; i < jsonVal.size(); ++i)
            {
                const json::value& file = jsonVal[i];
                if (file.has_field(U("new_path")))
                {
                    CString path(file[U("new_path")].as_string().c_str());
                    TRACE("Changed File: %s\n", path);
                }
            }
        }
        else
        {
            std::wcout << L"[Error] GetDiff() failed: " << response.status_code() << std::endl;
        }
    }).wait();
}

// 파일 수정 후 커밋 생성
void CreateCommitViaFileChange()
{
    http_client client(GITLAB_API_URL);
    uri_builder builder(U("projects/"));
    builder.append(PROJECT_ID).append(U("/repository/commits"));

    http_request req(methods::POST);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    req.headers().add(U("Content-Type"), U("application/json"));

    json::value postData;
    postData[U("branch")] = json::value::string(U("main"));
    postData[U("commit_message")] = json::value::string(U("Update file via API"));

    json::value action;
    action[U("action")] = json::value::string(U("update"));
    action[U("file_path")] = json::value::string(U("README.md"));
    action[U("content")] = json::value::string(U("Hello from C++ API commit"));

    postData[U("actions")] = json::value::array({ action });

    req.set_body(postData);

    client.request(req).then([](http_response response)
    {
        if (response.status_code() == status_codes::CREATED)
        {
            std::wcout << L"[OK] Commit created via API\n";
        }
        else
        {
            std::wcout << L"[Error] CreateCommitViaFileChange failed: " << response.status_code() << std::endl;
        }
    }).wait();
}

/////////////////////[chat ]//////////////////////////////////////////////////////////////////////////////////////////////////////
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <atlstr.h>  // CString 사용
using namespace web;
using namespace web::http;
using namespace web::http::client;

#define GITLAB_API_URL   U("http://localhost/api/v4/")
#define PRIVATE_TOKEN    U("your_private_token")    // ✅ 실제 토큰
#define PROJECT_ID       U("root%2Fyour_project")    // ✅ URI 인코딩된 프로젝트 경로

// 커밋 리스트 가져오기
void GetCommitList()
{
    http_client client(GITLAB_API_URL);
    uri_builder builder(U("projects/"));
    builder.append(PROJECT_ID).append(U("/repository/commits")).append_query(U("per_page"), U("10"));

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    req.set_request_uri(builder.to_uri());

    client.request(req).then([](http_response response)
    {
        if (response.status_code() == status_codes::OK)
        {
            json::value jsonVal = response.extract_json().get();
            for (size_t i = 0; i < jsonVal.size(); ++i)
            {
                const json::value& item = jsonVal[i];
                CString id(item[U("id")].as_string().c_str());
                CString title(item[U("title")].as_string().c_str());
                TRACE("Commit ID: %s, Title: %s\n", id, title);
            }
        }
        else
        {
            std::wcout << L"[Error] GetCommitList() failed: " << response.status_code() << std::endl;
        }
    }).wait();
}

// 특정 커밋의 diff 보기
void GetDiff(const utility::string_t& commit_id)
{
    utility::string_t url = GITLAB_API_URL + U("projects/") + PROJECT_ID + U("/repository/commits/") + commit_id + U("/diff");
    http_client client(url);

    http_request req(methods::GET);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);

    client.request(req).then([commit_id](http_response response)
    {
        if (response.status_code() == status_codes::OK)
        {
            json::value jsonVal = response.extract_json().get();
            for (size_t i = 0; i < jsonVal.size(); ++i)
            {
                const json::value& file = jsonVal[i];
                if (file.has_field(U("new_path")))
                {
                    CString path(file[U("new_path")].as_string().c_str());
                    TRACE("Changed File: %s\n", path);
                }
            }
        }
        else
        {
            std::wcout << L"[Error] GetDiff() failed: " << response.status_code() << std::endl;
        }
    }).wait();
}

// 커밋 생성: GitLab API는 직접 커밋을 만들 수 없음 → 파일 수정 API로 가능
void CreateCommitViaFileChange()
{
    http_client client(GITLAB_API_URL);
    uri_builder builder(U("projects/"));
    builder.append(PROJECT_ID).append(U("/repository/commits"));

    http_request req(methods::POST);
    req.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    req.headers().add(U("Content-Type"), U("application/json"));

    json::value postData;
    postData[U("branch")] = json::value::string(U("main"));
    postData[U("commit_message")] = json::value::string(U("Update file via API"));

    json::value action;
    action[U("action")] = json::value::string(U("update"));
    action[U("file_path")] = json::value::string(U("README.md"));
    action[U("content")] = json::value::string(U("Hello from C++ API commit"));

    postData[U("actions")] = json::value::array({ action });

    req.set_body(postData);

    client.request(req).then([](http_response response)
    {
        if (response.status_code() == status_codes::CREATED)
        {
            std::wcout << L"[OK] Commit created via API\n";
        }
        else
        {
            std::wcout << L"[Error] CreateCommitViaFileChange failed: " << response.status_code() << std::endl;
        }
    }).wait();
}
