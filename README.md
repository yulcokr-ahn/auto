#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

// 요일 문자열 테이블
const char* WEEKDAYS[] = { "일", "월", "화", "수", "목", "금", "토" };

// ISO8601 문자열 (예: "2025-08-31T13:45:20+09:00")
// → 로컬 시간 변환 후 포맷팅 (YYYY-MM-DD 오전/오후 hh:mm:ss 요일)
std::string parse_iso8601_to_local(const std::string& iso8601)
{
    // ---- 1. 기본 ISO8601 파싱 ----
    std::tm tm_utc = {};
    int tz_hour = 0, tz_min = 0;
    char tz_sign = '+';

    // GitLab committer_date 예: 2025-08-31T13:45:20+09:00
    sscanf_s(iso8601.c_str(), "%d-%d-%dT%d:%d:%d%c%d:%d",
        &tm_utc.tm_year, &tm_utc.tm_mon, &tm_utc.tm_mday,
        &tm_utc.tm_hour, &tm_utc.tm_min, &tm_utc.tm_sec,
        &tz_sign, &tz_hour, &tz_min);

    tm_utc.tm_year -= 1900; // struct tm 기준: 1900부터 시작
    tm_utc.tm_mon  -= 1;    // struct tm 기준: 0=1월
    tm_utc.tm_isdst = -1;   // DST 자동 계산

    // ---- 2. UTC 기준 time_t 변환 ----
    time_t utc_time = _mkgmtime(&tm_utc);

    // ---- 3. 타임존 보정 (GitLab 응답에 포함된 ±hh:mm 적용) ----
    int offset_sec = tz_hour * 3600 + tz_min * 60;
    if (tz_sign == '-') offset_sec = -offset_sec;
    utc_time -= offset_sec; // UTC로 환산

    // ---- 4. 로컬 시간 변환 ----
    std::tm local_tm;
    localtime_s(&local_tm, &utc_time);

    // ---- 5. 오전/오후 계산 ----
    int hour12 = local_tm.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    std::string ampm = (local_tm.tm_hour < 12) ? "오전" : "오후";

    // ---- 6. 출력 문자열 포맷 ----
    std::ostringstream oss;
    oss << std::setfill('0')
        << (local_tm.tm_year + 1900) << "-"
        << std::setw(2) << (local_tm.tm_mon + 1) << "-"
        << std::setw(2) << local_tm.tm_mday << " "
        << ampm << " "
        << std::setw(2) << hour12 << ":"
        << std::setw(2) << local_tm.tm_min << ":"
        << std::setw(2) << local_tm.tm_sec << " "
        << WEEKDAYS[local_tm.tm_wday]; // 요일

    return oss.str();
}
