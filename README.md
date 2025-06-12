clone --recursive https://gitlab.com/tortoisegit/tortoisegit.git

--recursive: .gitmodules 파일에 정의된 모든 서브모듈을 자동으로 함께 다운로드합니다.

이미 clone 했는데 누락된 경우
git submodule update --init --recursive
