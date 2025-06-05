//브랜치 목록 가져오기 새 브랜치 생성 Merge Request 자동 생성
//여러 파일 자동 커밋 (간접적인) reset / checkout 처리

//-----------GitLab_API_Client.cpp
#include <afx.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace web;
using namespace web::http;
using namespace web::http::client;

#define GITLAB_URL L"http://localhost/api/v4"
#define PRIVATE_TOKEN L"YOUR_PRIVATE_TOKEN"
#define PROJECT_PATH L"root%2Fyourproject"

void list_branches()
{
    http_client client(GITLAB_URL);
    uri_builder builder(U("/projects/"));
    builder.append(PROJECT_PATH).append(U("/repository/branches"));

    http_request request(methods::GET);
    request.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    request.set_request_uri(builder.to_uri());

    client.request(request).then([](http_response response) {
        if (response.status_code() == status_codes::OK)
        {
            auto branches = response.extract_json().get();
            for (size_t i = 0; i < branches.size(); ++i)
            {
                std::wcout << L"Branch: " << branches[i][U("name")].as_string() << std::endl;
            }
        }
    }).wait();
}

void create_branch(const utility::string_t& new_branch, const utility::string_t& ref_branch)
{
    http_client client(GITLAB_URL);
    uri_builder builder(U("/projects/"));
    builder.append(PROJECT_PATH).append(U("/repository/branches"));

    uri_builder body;
    body.append_query(U("branch"), new_branch);
    body.append_query(U("ref"), ref_branch);

    http_request request(methods::POST);
    request.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    request.set_request_uri(builder.to_uri() + L"?" + body.query());

    client.request(request).then([](http_response response) {
        std::wcout << L"Create branch status: " << response.status_code() << std::endl;
    }).wait();
}

void create_merge_request(const utility::string_t& source, const utility::string_t& target)
{
    http_client client(GITLAB_URL);
    uri_builder builder(U("/projects/"));
    builder.append(PROJECT_PATH).append(U("/merge_requests"));

    json::value payload;
    payload[U("source_branch")] = json::value::string(source);
    payload[U("target_branch")] = json::value::string(target);
    payload[U("title")] = json::value::string(L"Auto Merge: " + source + L" -> " + target);

    http_request request(methods::POST);
    request.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    request.headers().add(U("Content-Type"), U("application/json"));
    request.set_request_uri(builder.to_uri());
    request.set_body(payload);

    client.request(request).then([](http_response response) {
        std::wcout << L"Merge Request status: " << response.status_code() << std::endl;
    }).wait();
}

void commit_files(const std::vector<std::wstring>& file_paths, const std::wstring& branch, const std::wstring& message)
{
    http_client client(GITLAB_URL);
    uri_builder builder(U("/projects/"));
    builder.append(PROJECT_PATH).append(U("/repository/commits"));

    json::value payload;
    payload[U("branch")] = json::value::string(branch);
    payload[U("commit_message")] = json::value::string(message);

    json::value actions = json::value::array();

    for (size_t i = 0; i < file_paths.size(); ++i)
    {
        std::wifstream wif(file_paths[i].c_str());
        std::wstringstream wss;
        wss << wif.rdbuf();
        wif.close();

        json::value action;
        action[U("action")] = json::value::string(U("update"));
        action[U("file_path")] = json::value::string(file_paths[i]);
        action[U("content")] = json::value::string(wss.str());
        actions[i] = action;
    }

    payload[U("actions")] = actions;

    http_request request(methods::POST);
    request.headers().add(U("PRIVATE-TOKEN"), PRIVATE_TOKEN);
    request.headers().add(U("Content-Type"), U("application/json"));
    request.set_request_uri(builder.to_uri());
    request.set_body(payload);

    client.request(request).then([](http_response response) {
        std::wcout << L"Commit status: " << response.status_code() << std::endl;
    }).wait();
}

int _tmain(int argc, _TCHAR* argv[])
{
    list_branches();

    create_branch(L"feature/new-ui", L"main");

    std::vector<std::wstring> files;
    files.push_back(L"README.md");
    files.push_back(L"src/test.cpp");

    commit_files(files, L"feature/new-ui", L"Updated readme and test");

    create_merge_request(L"feature/new-ui", L"main");

    return 0;
}
