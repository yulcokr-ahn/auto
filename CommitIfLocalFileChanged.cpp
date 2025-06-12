#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

CImageList m_ImageList;

void SetupFileListWithIcons(CListCtrl& listCtrl, const std::vector<std::wstring>& filePaths)
{
    m_ImageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
    listCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

    for (size_t i = 0; i < filePaths.size(); ++i)
    {
        const std::wstring& filePath = filePaths[i];

        // 1. SHGetFileInfo로 확장자 기반 아이콘 가져오기
        SHFILEINFO sfi = { 0 };
        SHGetFileInfo(filePath.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
            SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

        int imageIndex = -1;
        if (sfi.hIcon)
        {
            imageIndex = m_ImageList.Add(sfi.hIcon);
            DestroyIcon(sfi.hIcon); // 리소스 누수 방지
        }

        // 2. CListCtrl에 항목 추가
        int idx = listCtrl.InsertItem((int)i, filePath.c_str(), imageIndex);
        listCtrl.SetItemText(idx, 1, L"is_new");
    }
}



#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <string>

using namespace web;
using namespace web::http;
using namespace web::http::client;

const utility::string_t GITLAB_HOST = U("https://gitlab.com");
const utility::string_t GITLAB_TOKEN = U("YOUR_GITLAB_ACCESS_TOKEN");
const utility::string_t PROJECT_ID = U("123456"); // 또는 group%2Fproject
const utility::string_t FROM_REF = U("main");
const utility::string_t TO_REF = U("feature-branch"); // 비교할 브랜치

pplx::task<void> get_gitlab_status_like_diff() {
    http_client client(GITLAB_HOST);

    // /projects/:id/repository/compare?from=main&to=feature-branch
    utility::string_t path = U("/api/v4/projects/") + PROJECT_ID +
                             U("/repository/compare?from=") + FROM_REF +
                             U("&to=") + TO_REF;

    http_request req(methods::GET);
    req.set_request_uri(path);
    req.headers().add(U("PRIVATE-TOKEN"), GITLAB_TOKEN);

    return client.request(req).then([](http_response response) {
        if (response.status_code() != status_codes::OK) {
            std::wcout << L"GitLab API 호출 실패: " << response.status_code() << std::endl;
            return utility::string_t();
        }
        return response.extract_string();
    }).then([](utility::string_t body) {
        json::value result = json::value::parse(body);

        if (!result.has_field(U("diffs"))) {
            std::wcout << L"diff 정보 없음\n";
            return;
        }

        auto diffs = result[U("diffs")].as_array();
        std::wcout << L"변경 파일 목록:\n";

        for (const auto& diff : diffs) {
            utility::string_t new_path = diff[U("new_path")].as_string();
            bool is_new = diff[U("new_file")].as_bool();
            bool is_renamed = diff[U("renamed_file")].as_bool();
            bool is_deleted = diff[U("deleted_file")].as_bool();

            // git status --porcelain 형식 흉내내기
            if (is_new) {
                std::wcout << L"?? " << new_path << std::endl;
            } else if (is_deleted) {
                std::wcout << L"D  " << new_path << std::endl;
            } else if (is_renamed) {
                std::wcout << L"R  " << new_path << std::endl;
            } else {
                std::wcout << L"M  " << new_path << std::endl;
            }
        }
    });
}

int main() {
    get_gitlab_status_like_diff().wait();
    return 0;
}

