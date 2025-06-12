1. clone --recursive  //gitlab.com/_tortoisegit/_tortoisegit.git
  --recursive: .gitmodules 파일에 정의된 모든 서브모듈을 자동으로 함께 다운로드합니다.

2. 이미 clone 했는데 누락된 경우
git submodule update --init --recursive

3.
ext/libgit2	Git 백엔드 라이브러리
ext/Lexilla	코드 하이라이팅 및 렉서 모듈
ext/crashrpt	크래시 리포트 도구
ext/libsasl	인증용 SASL 라이브러리
ext/OpenSSL	HTTPS용 보안 라이브러리

4.
├── src\                     ← 메인 소스 코드
├── ext\
│   ├── libgit2\
│   ├── Lexilla\
│   ├── crashrpt\
│   └── ...
├── _TortoiseGit.sln         ← Visual Studio 솔루션 파일
└── .gitmodules             ← 서브모듈 정보
