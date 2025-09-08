<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <title>JSON → CSV 다운로드</title>
</head>
<body>
<script>
async function downloadJsonMetaAsCsv(url, csvFilename) {
  try {
    // 1. JSON fetch
    const response = await fetch(url);
    const data = await response.json();

    if (!Array.isArray(data) || data.length === 0) {
      console.warn("JSON이 배열이 아니거나 비어 있습니다.");
      return;
    }

    // 2. CSV 헤더
    let csvContent = "열 키,키명,길이,타입\n";

    const keys = Object.keys(data[0]);
    keys.forEach((key, index) => {
      const length = data.filter(item => key in item).length;
      const type = typeof data[0][key];
      csvContent += `${index},${key},${length},${type}\n`;
    });

    // 3. Blob 생성
    const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });

    // 4. anchor 생성 + 다운로드 트리거
    const anchor = document.createElement("a");
    anchor.href = URL.createObjectURL(blob);
    anchor.setAttribute("download", csvFilename);
    anchor.style.display = "none";
    document.body.appendChild(anchor);
    anchor.click();
    document.body.removeChild(anchor);

  } catch (err) {
    console.error("에러 발생:", err);
  }
}

// ✅ 사용 예시
downloadJsonMetaAsCsv("https://example.com/data.json", "json_meta.csv");
</script>
</body>
</html>
