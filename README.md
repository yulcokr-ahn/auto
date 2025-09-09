<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <title>JSON → Excel Export</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/xlsx/0.18.5/xlsx.full.min.js"></script>
</head>
<body>
  <button id="exportBtn">엑셀로 저장</button>

  <script>
    // JSON 데이터 예시
    const data = {
      "info": { "code": "DH", "target": "S01" },
      "record": [
        {
          "name": "Real",
          "io": 1,
          "array": 0,
          "arrayInfo": "",
          "items": [
            { "len": 10, "type": 8, "item": "RLKEY", "key": 1 },
            { "len": 5,  "type": 4, "item": "VAL01", "key": 2 },
            { "len": 7,  "type": 2, "item": "VAL02", "key": 3 },
            { "len": 12, "type": 6, "item": "VAL03", "key": 4 },
            { "len": 8,  "type": 3, "item": "VAL04", "key": 5 },
            { "len": 11, "type": 1, "item": "VAL05", "key": 6 }
          ]
        }
      ]
    };

    document.getElementById("exportBtn").addEventListener("click", () => {
      // 워크시트 데이터 배열 생성
      let ws_data = [];
      ws_data.push(["열 키", "키명", "길이", "타입"]);

      for (let i = 0; i < data.record.length; i++) {
        let rec = data.record[i];
        for (let j = 0; j < rec.items.length; j++) {
          let it = rec.items[j];
          if (typeof it.item === "string" && it.item.startsWith("_")) {
            continue; // "_" 로 시작하면 skip
          }
          ws_data.push([it.key, it.item, it.len, it.type]);
        }
      }

      // 워크시트 생성
      let ws = XLSX.utils.aoa_to_sheet(ws_data);

      // 전체 셀 스타일 적용
      const range = XLSX.utils.decode_range(ws['!ref']);
      for (let R = range.s.r; R <= range.e.r; R++) {
        for (let C = range.s.c; C <= range.e.c; C++) {
          const cell_address = XLSX.utils.encode_cell({ r: R, c: C });
          if (!ws[cell_address]) continue;

          // 기본 스타일 (얇은 테두리 + 흰색)
          let style = {
            fill: { patternType: "solid", fgColor: { rgb: "FFFFFF" } },
            border: {
              top:    { style: "thin", color: { rgb: "000000" } },
              bottom: { style: "thin", color: { rgb: "000000" } },
              left:   { style: "thin", color: { rgb: "000000" } },
              right:  { style: "thin", color: { rgb: "000000" } }
            }
          };

          // 헤더 색상
          if (R === 0) {
            style.fill.fgColor.rgb = "CCCCCC"; // 진한 회색
          } else {
            // 짝수/홀수 행 교차 색상
            if ((R % 2) === 0) {
              style.fill.fgColor.rgb = "EEEEEE"; // 짝수 행 연한 회색
            } else {
              style.fill.fgColor.rgb = "FFFFFF"; // 홀수 행 흰색
            }
          }

          // 5열마다 굵은 세로선
          if ((C + 1) % 5 === 0) {
            style.border.right = { style: "medium", color: { rgb: "000000" } };
          }

          // 5행마다 굵은 가로선
          if ((R + 1) % 5 === 0) {
            style.border.bottom = { style: "medium", color: { rgb: "000000" } };
          }

          ws[cell_address].s = style;
        }
      }

      // 워크북 생성
      let wb = XLSX.utils.book_new();
      XLSX.utils.book_append_sheet(wb, ws, "Result");

      // 엑셀 파일 저장
      XLSX.writeFile(wb, "result.xlsx");
    });
  </script>







  
</body>
</html>
