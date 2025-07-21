
#include "stdafx.h"
#include "AutoLockOn.h"
#include "DemoDlg.h"
#include "ColorShow.h"
#include "FOHyperLink.h"

#include <dwmapi.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "DemoDlg.h"
#define IDB_MY_ICONS    101
#define IDB_BACKGROUND  102  
#define IDC_TREE1       103
#define IDC_CHECK_TREE	 0x3E8
#define WM_CHANGECHECK	 WM_USER + 2000

DECLARE_MESSAGE(UWM_POINT_MAIN)
DECLARE_MESSAGE(UWM_PICKER_MAIN)

#include <UIAutomation.h>
#pragma comment(lib, "UIAutomationCore.lib") //#import "UIAutomationCore.dll" no_namespace
#include <GdiPlusGpStubs.h>


std::vector<INFO_treeItem> g_vChkButCor; 
extern ULONG_PTR gdiplusToken;
CBezierOverlayWnd m_MultOverlay;
CBezierOverlayWnd m_SingleLine;				// 실시간 락온(시각화)
IUIAutomation* pAutomation = nullptr;		// 2. UIAutomation 객체 생성




#define TREE_TEXT_COLOR		RGB(0,0,0)
#define TREE_NORAM_BACK		RGB(255,255,255)
#define TREE_NORAM_LINE		RGB(255,255,255)
#define TREE_HILITE_BACK	RGB(240,240,240)
#define TREE_HILITE_LINE	RGB(189,189,189)

BEGIN_MESSAGE_MAP(CBezierOverlayWnd, CWnd) 
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CBezierOverlayWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
//	cs.style |= WS_OVERLAPPEDWINDOW;
	return CWnd::PreCreateWindow(cs);
}
BOOL CBezierOverlayWnd::PreTranslateMessage(MSG* pMsg) // 2025.04.21
{
	if (m_ToolTip.GetSafeHwnd())
		m_ToolTip.RelayEvent(pMsg);
	return CWnd::PreTranslateMessage(pMsg);
}
bool CBezierOverlayWnd::IsPointInBezierCurve(CPoint point)
{
	return (point.x > 100 && point.x < 300 && point.y > 50 && point.y < 150);
}
void CBezierOverlayWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 마우스 클릭 시 곡선 영역 선택 여부 확인
	if (IsPointInBezierCurve(point)) {
		m_bSelected = !m_bSelected;  // 선택 반전
		Invalidate();  // 화면 갱신
	}
}

CBezierOverlayWnd::CBezierOverlayWnd(){
}
BOOL CBezierOverlayWnd::Create(CWnd* pParent)
{
	CString className = AfxRegisterWndClass(0);
	BOOL result = CreateEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,className,_T(""),
	WS_POPUP,0, 0,	GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),pParent->GetSafeHwnd(), nullptr);
	if (!result) {	return FALSE;	}
	SetLayeredWindowAttributes(RGB(255, 255, 255), 0, LWA_COLORKEY);

	ShowWindow(SW_SHOW);
	m_ToolTip.Create(this, TTS_ALWAYSTIP);
	m_ToolTip.Activate(TRUE);
	CRect tipRect(300, 300, 400, 320);
	m_ToolTip.AddTool(this, _T("바탕화면 툴팁입니다!"), &tipRect, 1);
	return TRUE;
}

void CBezierOverlayWnd::AddLink(const CPoint& from, const CPoint& to)	
{
	m_Links.push_back(std::make_pair(from, to)); // ( [first] from= treeItem 영역    ,  [second] to= 바탕아이콘영역 )
	Invalidate();
}
void CBezierOverlayWnd::ClearLinks()									
{
	m_Links.clear();
	Invalidate();
}
struct BezierPos
{
	POINT ptS;
	POINT pt1;
	POINT pt2;
	POINT ptE;
};

// 98% 끝점 화살표 그리기
void DrawArrow(Graphics& g, const BezierPos& bz,SolidBrush &brush  , float t = 0.7f, float arrowSize = 20.0f, float angleOffset = 0.6f)
{
	PointF P0((REAL)bz.ptS.x, (REAL)bz.ptS.y);
	PointF P1((REAL)bz.pt1.x, (REAL)bz.pt1.y);
	PointF P2((REAL)bz.pt2.x, (REAL)bz.pt2.y);
	PointF P3((REAL)bz.ptE.x, (REAL)bz.ptE.y);
	float u = 1 - t;
	PointF B(
		u * u * u * P0.X + 3 * u * u * t * P1.X + 3 * u * t * t * P2.X + t * t * t * P3.X,
		u * u * u * P0.Y + 3 * u * u * t * P1.Y + 3 * u * t * t * P2.Y + t * t * t * P3.Y
		);
	PointF dB(
		3 * u * u * (P1.X - P0.X) + 6 * u * t * (P2.X - P1.X) + 3 * t * t * (P3.X - P2.X),
		3 * u * u * (P1.Y - P0.Y) + 6 * u * t * (P2.Y - P1.Y) + 3 * t * t * (P3.Y - P2.Y)
		);
	float angle = atan2f(dB.Y, dB.X);
	PointF pt1(	B.X - arrowSize * cosf(angle - angleOffset),	B.Y - arrowSize * sinf(angle - angleOffset)		);
	PointF pt2(	B.X - arrowSize * cosf(angle + angleOffset),	B.Y - arrowSize * sinf(angle + angleOffset)		);

	PointF arrowHead[3] = { B, pt1, pt2 };
	g.FillPolygon(&brush, arrowHead, 3);
	Pen border(Color(255, 0, 0, 0), 1.0f);
	g.DrawPolygon(&border, arrowHead, 3);
}
//Bezier 곡선
BezierPos getCalcBz(const CRect& sR, const CRect& eR) //s=Start , e=End
{
	POINT sPt[4] = {
		{ (sR.left + sR.right) / 2 , sR.top                  },   // top
		{ (sR.left + sR.right) / 2 , sR.bottom               },   // bottom
		{  sR.left                 , (sR.top + sR.bottom) / 2 },  // left
		{  sR.right                , (sR.top + sR.bottom) / 2 }   // right
	};
	POINT ePt[4] = {
		{ (eR.left + eR.right) / 2 , eR.top                   },
		{ (eR.left + eR.right) / 2 , eR.bottom                },
		{ eR.left                  , (eR.top + eR.bottom) / 2 },
		{ eR.right                 , (eR.top + eR.bottom) / 2 }
	};
	double minDistSq = DBL_MAX;									// minDistSq= 1.79 
	POINT ptS={}, ptE={};										// 가장 가까운 edge 중심점 조합 찾기
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			double dx = (double)(sPt[i].x - ePt[j].x);
			double dy = (double)(sPt[i].y - ePt[j].y);
			double distSq = dx * dx + dy * dy;
			if (distSq < minDistSq)
			{
				minDistSq = distSq;
				ptS = sPt[i];
				ptE = ePt[j];
			}
		}
	}
	// Bezier 제어점 계산
	POINT ctrl1 = ptS;
	POINT ctrl2 = ptE;
	// 수평 또는 수직 차이에 따라 제어점 배치
	if (abs(ptS.x - ptE.x) < abs(ptS.y - ptE.y)) // 가로 방향 중심
	{
		int midX = (ptS.x + ptE.x) / 2;
		ctrl1.x  = midX;
		ctrl2.x  = midX;
	}
	else // 세로 방향 중심
	{
		int midY = (ptS.y + ptE.y) / 2;
		ctrl1.y  = midY;
		ctrl2.y  = midY;
	}
	BezierPos rs;
	rs.ptS  = ptS;
	rs.pt1 = ctrl1;
	rs.pt2 = ctrl2;
	rs.ptE  = ptE;
	return rs;
}
//다음은 Bezier 곡선에서 70% 지점의 좌표를 계산하는 코드입니다.
Point GetBezierPointAtT(Point P0, Point P1, Point P2, Point P3, float t)
{
	float u = 1.0f - t; 	// Bezier 곡선에서 t에 해당하는 지점 계산
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;
	float x = uuu * P0.X + 3 * uu * t * P1.X + 3 * u * tt * P2.X + ttt * P3.X; // 각 점의 x, y 좌표 계산
	float y = uuu * P0.Y + 3 * uu * t * P1.Y + 3 * u * tt * P2.Y + ttt * P3.Y;
	return Point((int)x, (int)y);
}
// 툴팁 곡선 마이스 포인트 
std::vector<CPoint> SampleBezierPoints(const BezierPos& bz, int segments = 30)
{
	std::vector<CPoint> pts;
	for (int i = 0; i <= segments; ++i)
	{
		float t = (float)i / segments;
		float u = 1.0f - t;
		float x = u * u * u * bz.ptS.x + 3 * u * u * t * bz.pt1.x + 3 * u * t * t * bz.pt2.x + t * t * t * bz.ptE.x;
		float y = u * u * u * bz.ptS.y + 3 * u * u * t * bz.pt1.y + 3 * u * t * t * bz.pt2.y + t * t * t * bz.ptE.y;
		CPoint pt((int)x, (int)y);
		pts.push_back(pt);
	}
	return pts;
}
// 툴팁 마우스 좌표 곡선에 있는지
bool IsMouseNearCurve(const CPoint& mousePt, const std::vector<CPoint>& curvePts, int threshold = 5)
{
	for (size_t i = 0; i < curvePts.size() - 1; ++i)
	{
		POINT p1 = curvePts[i];
		POINT p2 = curvePts[i + 1];		
		double dx = p2.x - p1.x;// 점 ~ 선분 거리 계산
		double dy = p2.y - p1.y;
		double lengthSq = dx * dx + dy * dy;
		if (lengthSq == 0) continue;
		double t = ((mousePt.x - p1.x) * dx + (mousePt.y - p1.y) * dy) / lengthSq;
		t = max(0.0, min(1.0, t));
		double px = p1.x + t * dx;
		double py = p1.y + t * dy;
		double distSq = (mousePt.x - px) * (mousePt.x - px) + (mousePt.y - py) * (mousePt.y - py);
		if (distSq < threshold * threshold)
			return true;
	}
	return false;
}
//Box radius 값만큼 모서리를 둥글게 처리하는 GraphicsPath 생성 함수
void DrawRoundBox(Graphics& g, Pen &pp, const CRect& rect, int radius, const Color& baseColor, const Color& borderColor, float borderWidth = 1.0f, BYTE opacity = 255)
{
	if (radius < 0) radius = 0;
	int diameter = radius * 2;
	// 경로 생성
	GraphicsPath path;
	if (radius == 0) {
		path.AddRectangle(Rect(rect.left, rect.top, rect.Width(), rect.Height()));
	} else {
		Rect arcRect(rect.left, rect.top, diameter, diameter); 	path.AddArc(arcRect, 180, 90);	// 왼쪽 위
		arcRect.X = rect.right  - diameter;                path.AddArc(arcRect, 270, 90);	// 오른쪽 위
		arcRect.Y = rect.bottom - diameter;                path.AddArc(arcRect,   0, 90);	// 오른쪽 아래
		arcRect.X = rect.left;                             path.AddArc(arcRect,  90, 90);	// 왼쪽 아래
		path.CloseFigure();
	}
	// 🎨 입체 효과용 그라디언트 색상 (위쪽 밝고 아래 어둡게)
	Color topColor(opacity, baseColor.GetR(), baseColor.GetG(), baseColor.GetB());
	Color bottomColor(opacity, baseColor.GetR() * 0.7, baseColor.GetG() * 0.7, baseColor.GetB() * 0.7); // 어둡게

	// 브러시: 위에서 아래로 입체 그라디언트
	// LinearGradientBrush brush(	Point(rect.left, rect.top),	Point(rect.left, rect.bottom),	topColor,	bottomColor	);
	Gdiplus::SolidBrush brush(baseColor); // 검정색
	// 테두리
	//Pen pen(borderColor, borderWidth);
	//pen.SetDashStyle(DashStyleDash); // 필요 시만 점선
	// 그리기
	g.FillPath(&brush, &path);
	g.DrawPath(&pp , &path);
}

void CBezierOverlayWnd::OnPaint()
{
	CPaintDC dc(this);
	CDC memDC;
	CBitmap bmp;
	CRect rect;
	GetClientRect(&rect);
	memDC.CreateCompatibleDC(&dc);
	bmp.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&bmp);
	memDC.FillSolidRect(&rect, RGB(255, 255, 255));
	memDC.SetBkMode(TRANSPARENT);
	memDC.SelectStockObject(NULL_PEN);
	CRect    rcS;
	CRect    rcE;
	COLORREF clr;
	CString  txt;
	float fPenW = 2.0f; // 멀티 기본 편굵기
	m_SINGLE_LINE_MODE = m_Links.size()==1 ? TRUE: FALSE;
	Graphics g(memDC);
	// '굴림체'로 폰트 생성
	Gdiplus::FontFamily fontFamily(L"굴림체");  // 한글 폰트 '굴림체'
	Gdiplus::Font font(&fontFamily, 15.5f, FontStyleBold, UnitPixel);// 굵은 글꼴 Gdiplus::Font font(&fontFamily, 14, FontStyleRegular, UnitPixel);
	Gdiplus::SolidBrush txtbrush(Color(255, 200, 200, 200)); // 검정색

	int radius = 10; 
	for (int i = 0; i < m_Links.size(); ++i)
	{
		if(m_SINGLE_LINE_MODE)
		{	
			fPenW = 5.0f;			// 싱글 편굵기
			clr   = RGB(0,255,0);   // 싱글 칼라
		}
		else
		{
			if (g_vChkButCor.size() > i ) //2025.04.21
			{
				if(g_vChkButCor[i].isChecked)
					clr = g_vChkButCor[i].color;
				else
					clr= RGB(220,220,220);
			}
		}
		if (m_vEBox.size() > i )
		{
			rcS = m_vSBox[i];	// 시작 박스 영역
			rcE = m_vEBox[i];	// 끝 (바탕 아이콘)
		}
		if (m_bSelected) {
			clr= RGB(255,255,255);
		}
		//----------------------------------------------------------------------------
		int cR = GetRValue(clr);
		int cG = GetGValue(clr);
		int cB = GetBValue(clr);
		Color  fillColor(255, 255, 255, 255); ; // A=26 → 90% 투명; // ((255 * (1.0f-0.98f)), 230, 255, 230);   // 연보라색 A=51은 80% 투명, 20% 불투명 90% 투명은 GDI+에서 알파 값(A)을 255 × 0.1 = 약 25.5 → 26 으로 설정하면 됩니다.
		Color borderColor(255, cR, cG, cB);  // 짙은 파랑
		Gdiplus::SolidBrush brush(Color(255,  cR, cG, cB)); // 검정색

		BezierPos bz = getCalcBz(rcS, rcE); //Bezier
		float ic = 1.0f;
		int nPkt=0;
		if(m_SINGLE_LINE_MODE==FALSE)	{
			if (g_vChkButCor.size() > i )	
			{
				if(g_vChkButCor[i].nPacketCnt % 2)
				{
					ic = 0.5f;
				}
				nPkt =g_vChkButCor[i].nPacketCnt;
			}
		}

		Pen pn(Color(255, cR* ic, cG * ic, cB * ic), fPenW);	

		g.DrawRectangle(&pn, rcS.left, rcS.top,	rcS.Width(), rcS.Height() ); // ▶ 사각형 그리기 g.DrawRectangle(&pn, rcE.left, rcE.top,	rcE.Width(), rcE.Height() );

		DrawRoundBox(g,pn, rcE, radius, fillColor, borderColor, 2.0f);

		g.DrawBezier(&pn,
			Point(bz.ptS.x , bz.ptS.y),
			Point(bz.pt1.x , bz.pt1.y),
			Point(bz.pt2.x , bz.pt2.y),
			Point(bz.ptE.x , bz.ptE.y) );

		Point pCtrl(bz.pt2.x, bz.pt2.y); // 벡터 방향으로 화살표 위치 약간 이동 (선과 겹침 방지)
		Point pEnd( bz.ptE.x, bz.ptE.y);
		float vx = pEnd.X - pCtrl.X;
		float vy = pEnd.Y - pCtrl.Y;
		float len = sqrtf(vx * vx + vy * vy);
		if (len > 0.01f) {
			vx /= len;
			vy /= len;
			pEnd.X -= vx * 6.0f; // 화살표 약간 앞 당김
			pEnd.Y -= vy * 6.0f;
		}
		DrawArrow(g, bz, brush , 0.98f, 12.0f, 0.6f);

		Point pointAt70 = GetBezierPointAtT(Point(bz.ptS.x, bz.ptS.y), // 80% 지점에 해당하는 위치에 곡선 번호를 텍스트로 표시합니다.
			Point(bz.pt1.x, bz.pt1.y), 
			Point(bz.pt2.x, bz.pt2.y), 
			Point(bz.ptE.x, bz.ptE.y), 0.8f); //80% 지점
		const CRect& box = m_vEBox[i];
		CStringW text;
		text.Format(L"%d (%4d X %4d) %5d", static_cast<int>(i + 1)  , box.Width(), box.Height()  , nPkt); // 곡선 번호 텍스트 그리기
		g.DrawString(text, -1, &font,   PointF(pointAt70.X + 5, pointAt70.Y + 5) , &brush); //txtbrush);
	}

	dc.BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(pOldBmp);
}

void CBezierOverlayWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	for (int i = 0; i < m_Links.size(); ++i)
	{
		BezierPos bz = getCalcBz(m_vSBox[i], m_vEBox[i]);
		auto bezierPoints = SampleBezierPoints(bz, 30);
		if (IsMouseNearCurve(point, bezierPoints, 6))
		{
			if (m_ToolTip.GetSafeHwnd())
			{
				CString msg;
				msg.Format(_T("Bezier 연결선 %d"), i);
				m_ToolTip.UpdateTipText(msg, this, i + 1000); // ID는 충돌 피해서 다르게
		
				MSG tooltipMsg;
				tooltipMsg.hwnd = m_hWnd;
				tooltipMsg.message = WM_MOUSEMOVE;
				tooltipMsg.wParam = 0;
				tooltipMsg.lParam = MAKELPARAM(point.x, point.y);
				tooltipMsg.time = ::GetTickCount();
				tooltipMsg.pt = point;

				m_ToolTip.RelayEvent(&tooltipMsg);
			}
			break;
		}
	}

	CWnd::OnMouseMove(nFlags, point);
}
void CBezierOverlayWnd::UpdateToolTips()
{
	if (m_ToolTip.GetSafeHwnd() == NULL) return;
	m_ToolTip.DelTool(this); // 기존 툴 제거
	for (int i = 0; i < m_vEBox.size(); ++i)
	{
		CRect rc = m_vEBox[i];
		CString strTip;
		strTip.Format(_T("끝점 박스 %d"), i); // ← C++11 없이 안전하게 처리
		m_ToolTip.AddTool(this, strTip, &rc, i + 1);
	}
}

BEGIN_MESSAGE_MAP(CDemoDlg, CDialog)
ON_WM_TIMER()
ON_WM_DESTROY()
ON_NOTIFY(NM_CLICK, IDC_CHECK_TREE, OnClick)
ON_WM_LBUTTONDOWN()
ON_REGISTERED_MESSAGE(UWM_PICKER_MAIN, OnPicker_MAIN)
ON_REGISTERED_MESSAGE(UWM_POINT_MAIN, OnPoint_MAIN)
ON_WM_PAINT()
ON_WM_MOUSEMOVE()
ON_WM_NCLBUTTONDOWN()
ON_WM_NCLBUTTONUP()
ON_WM_NCMOUSEMOVE()
ON_WM_WINDOWPOSCHANGED()
ON_NOTIFY(NM_CUSTOMDRAW, IDC_CHECK_TREE, OnCustomDraw)
ON_BN_CLICKED(IDC_RADIO2_MAIN, &CDemoDlg::OnBnClickedRadio2Main)
ON_BN_CLICKED(IDC_RADIO1_MAIN, &CDemoDlg::OnBnClickedRadio1Main)
ON_BN_CLICKED(IDC_RADIO3_MAIN, &CDemoDlg::OnBnClickedRadio3Main)
ON_BN_CLICKED(IDC_RADIO3_MAIN2, &CDemoDlg::OnBnClickedRadio3Main2)
ON_BN_CLICKED(IDC_BUTTON_SAVE_MAP, &CDemoDlg::OnBnClickedButtonSaveMap)
END_MESSAGE_MAP()

void CDemoDlg::DrawMultLine_BezierLinks(HWND hWCur)  
{
	if (!m_MultOverlay.GetSafeHwnd()) return;

	// 바탕화면 모드일 경우 아이콘 정보 가져오기
	if (m_Mode_isDeskTopIcon)
		PrintDesktopIconsUIAutomation(); // -B
	else
		InitTreeWithWindows(m_Tree, hWCur); // -A

	// EBox 변화 감지에 따른 최적화
	int sumMult = 0;
	for (int i = 0; i < m_vEBox.size(); ++i) {
		sumMult += (m_vEBox[i].left + m_vEBox[i].top + m_vEBox[i].left + m_vEBox[i].top) ;	
	}	

	if(m_IconNameItem.size()!=m_Tree.GetCount())  //핸들 삭제시 갯수가 다르다
		m_Tree.DeleteAllItems();
	
	if(m_Tree.GetCount() > 0)
	{
		if (m_sumMult == sumMult && !m_reChkDraw_SingleLogic)
		{
			
			int vCnt  = g_vChkButCor.size();
			int rdID = rand() % vCnt;
			if (vCnt > rdID )
			{
				g_vChkButCor[rdID].nPacketCnt++;
				m_MultOverlay.Invalidate();
			}


			return;
		}
		m_sumMult = sumMult;
	}
	m_sumMult = sumMult;
	m_reChkDraw_MultLogic = TRUE;
	m_reChkDraw_SingleLogic = FALSE;

	TRACE(_T("DrawMultLine_BezierLinks() m_reChkDraw_MultLogic: %4d m_sumMult :%8d \r\n"), m_reChkDraw_MultLogic, m_sumMult);

	// 트리에 아이템이 없는 경우, 아이콘 이름 추가
	if (m_Tree.GetCount() <= 0)
	{
		for (int i = 0; i < m_vEBox.size(); ++i)
		{
			HTREEITEM hItem = m_Tree.InsertItem(m_IconNameItem[i], i, i);
			m_Tree.SetCheck(hItem, TRUE);
			m_Tree.SetItemData(hItem, (DWORD)i);
		}
	}

	if (!m_Mode_isDeskTopIcon)
		m_vSBox.clear(); // 시작 위치 박스 초기화
	m_MultOverlay.ClearLinks();
	// Tree 윈도우 좌표 획득
	CRect treeWndRect;
	m_Tree.GetWindowRect(&treeWndRect);
	const int xFix = treeWndRect.left + 20;


	// 트리 루트 아이템부터 순차 처리
	HTREEITEM hItem = m_Tree.GetRootItem();
	int i = 0;
	while (hItem && i < m_IconPositions.size())
	{
		m_Tree.SetCheck(hItem, TRUE);		
		CRect itemRect;
		if (m_Tree.GetItemRect(hItem, &itemRect, TRUE)) // Tree item의 위치 계산
		{
			CPoint treePt = itemRect.CenterPoint();
			m_Tree.ClientToScreen(&treePt);
			treePt.x = xFix;
			treePt.y -= itemRect.Height() / 2;			
			m_MultOverlay.AddLink(treePt, m_IconPositions[i]); // Bezier 링크 추가			
			m_vSBox.emplace_back(CRect(treePt.x, treePt.y, treePt.x + 80 + itemRect.Width(), treePt.y + itemRect.Height())) ; // 시작 박스 위치 저장
		}
		hItem = m_Tree.GetNextSiblingItem(hItem);
		++i;
	}

	m_MultOverlay.m_vSBox = m_vSBox;
	m_MultOverlay.m_vEBox = m_vEBox;
	m_MultOverlay.Invalidate();
	TRACE("DrawMultLine_BezierLinks() m_MultOverlay.Invalidate() m_vEBox:  %4d \r\n", m_vEBox.size() );
}

void CDemoDlg::OnBnClickedRadio1Main()
{
	SetCheck_tree(TRUE);
}
void CDemoDlg::OnBnClickedRadio2Main()
{
	SetCheck_tree(FALSE);
}
void CDemoDlg::OnBnClickedRadio3Main()
{
	HandleMainRadioClick(FALSE);
}
void CDemoDlg::OnBnClickedRadio3Main2()
{
	HandleMainRadioClick(TRUE);
}

void CDemoDlg::SetCheck_tree(bool bChk)
{	// 공통 초기화
	HTREEITEM hItem = m_Tree.GetRootItem();
	int i = 0;
	while (hItem && i < m_IconPositions.size())
	{
		m_Tree.SetCheck(hItem, bChk);		
		if (g_vChkButCor.size() > i ) {
			g_vChkButCor[i].isChecked = bChk;
			g_vChkButCor[i].sName = m_IconNameItem[i];
			m_Tree.SetItemData( hItem, (DWORD)i); 
		}
		hItem = m_Tree.GetNextSiblingItem(hItem);
		++i;
	}
	m_MultOverlay.Invalidate();
}
void CDemoDlg::HandleMainRadioClick(bool bIsDesktopIcon)
{
	// 공통 초기화
	m_SingleLine.ClearLinks();					
	m_SingleLine.m_vSBox.clear();
	m_SingleLine.m_vEBox.clear();
	m_SingleLine.Invalidate();

	m_IconPositions.clear();
	m_vSBox.clear();
	m_vEBox.clear();
	m_IconNameItem.clear();
	m_Tree.DeleteAllItems();
	m_MultOverlay.ClearLinks();
	m_MultOverlay.m_vSBox = m_vSBox;
	m_MultOverlay.m_vEBox = m_vEBox;
	m_MultOverlay.Invalidate();

	m_hTargetWnd = NULL;
	m_Mode_isDeskTopIcon = bIsDesktopIcon;
	// 라디오 버튼 상태 갱신
	CButton* pBtnTree = (CButton*)GetDlgItem(IDC_RADIO3_MAIN);
	CButton* pBtnList = (CButton*)GetDlgItem(IDC_RADIO3_MAIN2);
	pBtnTree->SetCheck(!bIsDesktopIcon);
	pBtnList->SetCheck(bIsDesktopIcon);
	// 분기 처리
	if (bIsDesktopIcon)
	{
		PrintDesktopIconsUIAutomation();           // 바탕화면 아이콘 탐색
		DrawMultLine_BezierLinks(this->GetSafeHwnd());
	}
	else
	{
		InitTreeWithWindows(m_Tree, m_hTargetWnd);  // 윈도우 자식 핸들 트리 구성
	}
}

void CDemoDlg::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	DWORD dw = GetMessagePos();                   
	CPoint p(GET_X_LPARAM(dw), GET_Y_LPARAM(dw)); 
	m_Tree.ScreenToClient(&p);                   
	UINT htFlags = 0;
	HTREEITEM hItem = m_Tree.HitTest(p, &htFlags);  
	if (hItem != NULL && htFlags==TVHT_ONITEMSTATEICON) {  
		BOOL bChecked = m_Tree.GetCheck(hItem);
		bChecked = bChecked==0 ? 1 : 0;
		long nIndex = m_Tree.GetItemData( hItem);
		CString strTxt = m_Tree.GetItemText( hItem);
		if (nIndex == -1) return;	

		g_vChkButCor[nIndex].isChecked = bChecked;
		m_MultOverlay.Invalidate(); 
		TRACE("CDemoDlg::OnClick() strTxt [%s] bChecked[%d] nIndex[%d] \r\n", strTxt , bChecked , nIndex);
	}
	*pResult = 0;
}

void CDemoDlg::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMTVCUSTOMDRAW* pTVCD = reinterpret_cast<NMTVCUSTOMDRAW*>(pNMHDR);
	*pResult = CDRF_DODEFAULT;
	if (pTVCD->nmcd.dwDrawStage == CDDS_PREPAINT)	{  
		*pResult = CDRF_NOTIFYITEMDRAW;				
	}
	else if (pTVCD->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)  {
		*pResult = CDRF_NOTIFYPOSTPAINT;				
	}
	else if (pTVCD->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT)  
	{
		HTREEITEM hItem = (HTREEITEM)pTVCD->nmcd.dwItemSpec;  
		CString strItem = m_Tree.GetItemText(hItem);		//strItem.Replace("&", "&&");
		CRect rc; 
		m_Tree.GetItemRect(hItem, &rc, TRUE);
		UINT uState = m_Tree.GetItemState(hItem, TVIS_SELECTED);
		CDC *pDC = CDC ::FromHandle(pTVCD->nmcd.hdc);		
		CFont m_Font;
		int nFontSize = 9;
		LONG lfHeight = -MulDiv(nFontSize, GetDeviceCaps(pDC->GetSafeHdc(), LOGPIXELSY), 72);	
		m_Font.CreateFont(lfHeight, 0, 0, 0, FW_BOLD     , FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "굴림체"); //		m_Font.CreateFont(lfHeight, 0, 0, 0, FW_NORMAL   , FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, 	CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "굴림체");
		CFont *pNormalFont = pDC->SelectObject(&m_Font);
		int nPrevBkMode = pDC->SetBkMode(TRANSPARENT);
		CRect rcText	= rc;		
		rcText.top		+= 2;
		if(uState & TVIS_SELECTED)
		{
			CPen pen;
			CBrush brush;
			pen.CreatePen(PS_SOLID, 1, TREE_HILITE_LINE);
			brush.CreateSolidBrush(TREE_HILITE_BACK);
			CPen *pPenOld = pDC->SelectObject(&pen);
			CBrush *pBrushOld = pDC->SelectObject(&brush);
			pDC->Rectangle(&rc);
			pDC->SelectObject(pPenOld);
			pDC->SelectObject(pBrushOld);
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(TREE_TEXT_COLOR);
			pDC->DrawText(strItem, &rcText, DT_SINGLELINE | DT_VCENTER | DT_SINGLELINE);
		}
		else
		{  
			CPen pen;
			CBrush brush;
			pen.CreatePen(PS_SOLID, 1, TREE_NORAM_LINE);
			brush.CreateSolidBrush(TREE_NORAM_BACK);
			CPen *pPenOld = pDC->SelectObject(&pen);
			CBrush *pBrushOld = pDC->SelectObject(&brush);
			pDC->Rectangle(&rc);
			pDC->SelectObject(pPenOld);
			pDC->SelectObject(pBrushOld);
			pDC->SetBkMode(TRANSPARENT);
			pDC->SetTextColor(TREE_TEXT_COLOR);
			pDC->DrawText(strItem, &rcText, DT_SINGLELINE | DT_VCENTER | DT_SINGLELINE);
		}
		pDC->SetBkMode(nPrevBkMode);
		pDC->SelectObject(pNormalFont);
		m_Font.DeleteObject();
		*pResult = CDRF_SKIPDEFAULT;	 
	} 
	else 
	{		
		*pResult = CDRF_SKIPDEFAULT;	 
	}
}
void CDemoDlg::OnWindowPosChanged(WINDOWPOS FAR* lpwndpos) {
	m_reChkDraw_SingleLogic= TRUE; //2025.04.21
CDialog::OnWindowPosChanged(lpwndpos);
}
void CDemoDlg::OnNcMouseMove(UINT nHitTest, CPoint point)  {
}
void CDemoDlg::OnNcLButtonDown(UINT nHitTest, CPoint point) {
CDialog::OnNcLButtonDown(nHitTest, point);
}
void CDemoDlg::OnNcLButtonUp(UINT nHitTest, CPoint point) {

CDialog::OnNcLButtonUp(nHitTest, point);
}
LRESULT CDemoDlg::OnNcHitTest(CPoint point)  {
TRACE("CDemoDlg::OnNcHitTest() x[%d] y[%d] \r\n", point.x, point.y);
return CDialog::OnNcHitTest(point);
}                                   
void CDemoDlg::OnMouseMove(UINT nFlags, CPoint point) {

}

void CDemoDlg::DrawSingleLine_Node(HWND hWnd1, HWND hWnd2)
{
	if (!hWnd1 || !hWnd2) return;
	CRect rc1, rc2;
	::GetWindowRect(hWnd1, &rc1); 
	::GetWindowRect(hWnd2, &rc2);
	m_SingleLine.ClearLinks();					
	m_SingleLine.AddLink(CPoint(rc1.left, rc1.top), CPoint(rc2.left, rc2.top)); // ( [first] from= treeItem 영역    ,  [second] to= 바탕아이콘영역 ) EndBox

	std::vector<CRect> vSBox;	vSBox.push_back(rc1);
	std::vector<CRect> vEBox;	vEBox.push_back(rc2);

	m_SingleLine.m_vSBox = vSBox;
	m_SingleLine.m_vEBox = vEBox;
	m_SingleLine.Invalidate();

	int sumSingle = rc1.left + rc1.top + rc2.left + rc2.top;	
	if(m_sumSingle!=sumSingle)
	m_sumSingle =sumSingle ;
}


// -----------A 원도우 자식 핸들 탐색
void CDemoDlg::InitTreeWithWindows(CTreeCtrl& tree, HWND parent) {
	m_IconPositions.clear();
	m_vSBox.clear();
	m_vEBox.clear();
	m_IconNameItem.clear();
	HTREEITEM hRoot= NULL; 
	FindChildWindows(tree, hRoot, parent);
}

void CDemoDlg::FindChildWindows(CTreeCtrl& tree, HTREEITEM hParent, HWND hWnd) {
	HWND child = ::GetWindow(hWnd, GW_CHILD);				// 첫 번째 자식 윈도우 가져오기
	CRect rc2;
	CString nodeText;
	TCHAR className[256] = {0};
	TCHAR windowText[256] = {0};
	while (child) {	
		::GetClassName(child, className, sizeof(className) / sizeof(TCHAR));// 윈도우 클래스명 및 타이틀 가져오기
		::GetWindowText(child, windowText, sizeof(windowText) / sizeof(TCHAR));
		//--Windows-RECT----------------------------
		::GetWindowRect(child, &rc2);
		m_vEBox.push_back(rc2);
		nodeText.Format(_T("HWND: %p | Class: %-16s | Title: %-16s  [%4d,%4d X %4d,%d]"), child, className, windowText , rc2.left, rc2.top , rc2.right, rc2.bottom);		
		m_IconNameItem.push_back(nodeText);
		m_IconPositions.push_back(CPoint(rc2.left, rc2.top)); // x,y
		FindChildWindows(tree, hParent, child);					// 자식 윈도우가 있으면 재귀 탐색
		child = ::GetWindow(child, GW_HWNDNEXT);				// 다음 형제 윈도우로 이동
	}
}
																// B 윈도우는 바탕화면 자식 아이콘 탐색
HWND GetDesktopListView()
{
	HWND hShellViewWin = nullptr;
	HWND hWorkerW = nullptr;
	HWND hProgman = ::FindWindow("Progman", nullptr);			// 바탕화면 셸 뷰 핸들 얻기
	if (hProgman)	{		
		HWND hDefView = ::FindWindowEx(hProgman, nullptr,"SHELLDLL_DefView", nullptr); // "SHELLDLL_DefView" 윈도우는 바탕화면 아이콘의 부모 윈도우
		if (!hDefView)	{			
			hWorkerW = ::FindWindowEx(nullptr, nullptr, "WorkerW", nullptr); // Windows 8+의 경우 WorkerW 윈도우 내부에 존재할 수 있음
			while (hWorkerW && !hDefView)	{
				hDefView = ::FindWindowEx(hWorkerW, nullptr,"SHELLDLL_DefView", nullptr);
				hWorkerW = ::FindWindowEx(nullptr, hWorkerW,"WorkerW", nullptr);
			}
		}
		if (hDefView)		
			hShellViewWin = ::FindWindowEx(hDefView, nullptr, "SysListView32", nullptr); // "SysListView32" 컨트롤이 실제 아이콘 리스트
	}
	return hShellViewWin;
}
void CDemoDlg::PrintDesktopIconsUIAutomation()					// 아이콘 이름과 위치를 출력하는 UIAutomation 방식
{
	m_IconPositions.clear();
	m_vSBox.clear();
	m_vEBox.clear();
	m_IconNameItem.clear();
	HWND hListView = GetDesktopListView(); 						// 3. 바탕화면 리스트뷰 핸들 가져오기
	if (!hListView)	{	pAutomation->Release(); 	return;	}
	IUIAutomationElement* pRoot = nullptr; 						// 4. 루트 엘리먼트 얻기 (오류 발생 위치)
	HRESULT hr = pAutomation->ElementFromHandle(hListView, &pRoot);
	if (FAILED(hr) || !pRoot)	{	pAutomation->Release();	return;	}	
	IUIAutomationCondition* pCondition = nullptr;				// 5. 모든 자식 요소 가져오기
	pAutomation->CreateTrueCondition(&pCondition);
	IUIAutomationElementArray* pElements = nullptr;
	hr = pRoot->FindAll(TreeScope_Children, pCondition, &pElements);
	int index =0;
	int length = 0;
	if (SUCCEEDED(hr) && pElements)	{
		pElements->get_Length(&length);
		for (int i = 0; i < length; ++i)	{
			IUIAutomationElement* pItem = nullptr;
			if (SUCCEEDED(pElements->GetElement(i, &pItem)) && pItem)	{
				BSTR bstrName;
				if (SUCCEEDED(pItem->get_CurrentName(&bstrName)))	{
					CRect rc2;
					CString line;
					if (SUCCEEDED(pItem->get_CurrentBoundingRectangle(&rc2)))	{	
						CString strName(bstrName);				// BSTR → CString
						line.Format("%2d. 이름: %-100s / 위치: (%4d, %4d) 사각영역: (%4d, %4d) - (%4d, %4d), 크기: %3d x %3d \r\n", i + 1,strName , rc2.left, rc2.top, rc2.left, rc2.top, rc2.right, rc2.bottom, rc2.Width(), rc2.Height()  );
						m_IconNameItem.push_back(strName);
						
						m_vEBox.push_back(rc2);
						m_IconPositions.push_back(CPoint(rc2.left, rc2.top+40)); // x,y
						++index;					
					}
					SysFreeString(bstrName);
				}
				pItem->Release();
			}
		}
		pElements->Release();
	}
	if (pCondition) pCondition->Release();						// 6. 리소스 해제
	if (pRoot) pRoot->Release();
}


BOOL CDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();	
	GdiplusStartupInput gdiplusStartupInput;	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	m_CurrentProcessId = GetCurrentProcessId();
	InitializeTaskbarHandles();
	m_MultOverlay.Create(this);									//이 위치에 꼭 pAutomation NULL 발생한다
	m_SingleLine.Create(this);
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); // 1. UIAutomation 객체 생성
	if(SUCCEEDED(hr)) {  CoUninitialize();		}
	hr = CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&pAutomation);
	if (FAILED(hr) || !pAutomation)	{	}
	InitTreeCtrl();
	c_Picker_MAIN.nModeType  = MODE_MAIN;
	c_Image_MAIN.nModeType   = MODE_MAIN;
	SetIcon(m_hIcon, TRUE);										// Set big icon
	SetIcon(m_hIcon, FALSE);		
	picker = (HCURSOR)::LoadImage(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDC_PICKER_DEMO),IMAGE_CURSOR,0, 0,LR_SHARED | LR_LOADTRANSPARENT);
	c_Picker_MAIN.SetIcon(picker);

	m_Mode_isDeskTopIcon = TRUE;
	CButton	*pBtnTree, *pBtnList;
	pBtnTree = (CButton*)GetDlgItem(IDC_RADIO3_MAIN);
	pBtnTree->SetCheck(FALSE);
	pBtnList = (CButton*)GetDlgItem(IDC_RADIO3_MAIN2);
	pBtnList->SetCheck(TRUE);

	return TRUE;
}
void CDemoDlg::InitTreeCtrl()
{
	CRect rect(10, 40, 1200, 600); 
	DWORD style = WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;
	m_Tree.Create(style, rect, this, IDC_CHECK_TREE);																// 1. 이미지 리스트 생성 무작위 색상 아이콘 20개 생성
	m_ImageList.Create(25, 16, ILC_COLOR32 | ILC_MASK, 21, 1);
	for (int i = 0; i < 21; ++i)	
	{
		COLORREF color = RGB(rand() % 256, rand() % 256, rand() % 256);
		CBitmap bmp;
		CClientDC dc(this);										// GDI+ 대신 GDI로 비트맵 생성
		CDC memDC;
		memDC.CreateCompatibleDC(&dc);
		bmp.CreateCompatibleBitmap(&dc, 25, 16);
		CBitmap* pOldBmp = memDC.SelectObject(&bmp);
		CBrush brush(color);
		CBrush* pOldBrush = memDC.SelectObject(&brush);
		memDC.Rectangle(0, 0, 25, 16);
		memDC.SelectObject(pOldBrush);
		memDC.SelectObject(pOldBmp);
		INFO_treeItem ar;
		ar.color     = color;
		ar.isChecked = true;
		ar.sName     = _T("");
		ar.nPacketCnt= 0;
		g_vChkButCor.push_back(ar);
		m_ImageList.Add(&bmp, RGB(0, 0, 0));
	}	
	m_Tree.SetImageList(&m_ImageList, TVSIL_NORMAL);				// 2. 트리 컨트롤에 이미지 리스트 연결
	DWORD dwStyle = GetWindowLong(m_Tree.GetSafeHwnd(), GWL_STYLE);	// 3. 체크박스 스타일 추가
	SetWindowLong(m_Tree.GetSafeHwnd(), GWL_STYLE, dwStyle | TVS_CHECKBOXES);
	SetTimer(1,200, NULL); 
}
void CDemoDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		CPoint pt;
		GetCursorPos(&pt);											//	int  chkSumDraw = pt.x + pt.y;
		HWND hWCur = ::WindowFromPoint(pt);
		if (hWCur)		
		{															//A. 현재 핸들이 싱글 라인 다른경우  , 락온핸들 널
			if( hWCur && m_On_Picke  && (m_hTargetWnd == NULL))
			{
				if (isCheckHandles(hWCur) ==FALSE) return; // 핸들 조건 있다 return   
				CString sMsg;
				sMsg.Format("Cur Time: 0x%x08",hWCur );
				m_edtHex_MAIN.SetWindowText(sMsg);    				//A1 싱글 락온 그린다 
				DrawSingleLine_Node(c_Picker_MAIN.GetSafeHwnd(), hWCur);
				CRect windowRect;  ::GetWindowRect(m_SingleLine.GetSafeHwnd(), windowRect);
				::SetWindowPos(m_SingleLine.GetSafeHwnd(), HWND_TOPMOST,	windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE);
				m_hwndSingle = hWCur;								//A2 싱글 핸들에 현재값을 치환
			}
			else
			{																	// A3. 자신 목록 핸들과 지금 핸들 비교 싱글만 그린다
				if(m_Mode_isDeskTopIcon || m_On_Picke ==FALSE && m_hTargetWnd )	{	
					DrawSingleLine_Node(c_Picker_MAIN.GetSafeHwnd() , m_hTargetWnd);		

					TRACE("CDemoDlg::OnTimer() m_reChkDraw_SingleLogic:  %4d \r\n",m_reChkDraw_SingleLogic );
					if(m_Mode_isDeskTopIcon)
						DrawMultLine_BezierLinks(hWCur);
					else
						DrawMultLine_BezierLinks(m_hTargetWnd);				
				}
			}
		}
	}
	CDialog::OnTimer(nIDEvent);
}

void CDemoDlg::OnPaint()
{
	if (IsIconic())	{
		CPaintDC dc(this); 
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

LRESULT CDemoDlg::OnPoint_MAIN(WPARAM wParam, LPARAM lParam)
{
	CPoint pt;
	pt.x = (int)(short)LOWORD(wParam);
	pt.y = (int)(short)HIWORD(wParam);
	COLORREF color = (COLORREF)lParam;
	CString s;
	s.Format(_T("RGB (%3d,%3d,%3d)"), GetRValue(color), GetGValue(color), GetBValue(color));	
	GetCursorPos(&pt);
	HWND hWCur = ::WindowFromPoint(pt);
	if (hWCur && (hWCur != c_Picker_MAIN.GetSafeHwnd()))		
	{
		CString sMsg;
		if(m_On_Picke)
			sMsg.Format("OnPoint_MAIN(POINT) m_On_Picke on: 0x%x08",m_hTargetWnd );
		else
		{
			sMsg.Format("OnPoint_MAIN(POINT) m_On_Picke off: 0x%x08",m_hTargetWnd );
		}

		m_edtHex_MAIN.SetWindowText(sMsg);
		TRACE(s+"\r\n");
	}
	return 0;
} 
LRESULT CDemoDlg::OnPicker_MAIN(WPARAM wParam, LPARAM lParam)
{
	CPoint pt;
	GetCursorPos(&pt);
	HWND hWCur = ::WindowFromPoint(pt);
	CString sMsg;
	if(wParam)
	{ /* pickup ON */
		c_Picker_MAIN.SetIcon(NULL);
		c_Image_MAIN.StartPick(CPoint((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)));		
		if (hWCur != c_Picker_MAIN.GetSafeHwnd())	{
			m_On_Picke = TRUE;
			sMsg.Format("OnPicker_MAIN() ON : 0x%x08",hWCur );
			GetDlgItem(IDC_RGB_MAIN)->SetWindowText(sMsg);
			DrawSingleLine_Node(c_Picker_MAIN.GetSafeHwnd(), hWCur); //2025.04.25
		}	
	} 
	else
	{ /* drop END */
		c_Picker_MAIN.SetIcon(picker);
		if (hWCur != m_hTargetWnd && (hWCur != c_Picker_MAIN.GetSafeHwnd()) )	
		{
			m_On_Picke = FALSE;
			if (isCheckHandles(hWCur) ==FALSE) return 0; // 핸들 조건 있다 return   
			m_hTargetWnd = hWCur;
			m_Tree.DeleteAllItems(); // tree삭제 
			sMsg.Format("OnPicker_MAIN() END: 0x%x08",hWCur );
			GetDlgItem(IDC_RGB_MAIN)->SetWindowText(sMsg);
			DrawMultLine_BezierLinks(hWCur);			// 상단 위에	고정		CRect windowRect;  ::GetWindowRect(m_MultOverlay.GetSafeHwnd(), windowRect);	::SetWindowPos(m_MultOverlay.GetSafeHwnd(), HWND_TOPMOST,	windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE);
		}
	} 
	return 0;
} 


CDemoDlg::CDemoDlg(CWnd* pParent)
	: CDialog(IDD_AutoLockOn_DIALOG, pParent)
	, m_RadioID(0)
	, m_dError(0)
	, m_hOverW(NULL)
	, m_hTargetWnd(NULL)
	, m_fAlpha(0.5f)
	, m_hwndSingle(NULL)
	, m_dwOriginalStyle(0)
{
	m_pParentWnd =NULL;
	m_pChildWnd= NULL;
	m_Mode_isDeskTopIcon= TRUE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
		m_On_Picke = FALSE;
}
void CDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_PICKER_MAIN, c_Picker_MAIN);
	DDX_Control(pDX, IDC_EDITHEX_MAIN, m_edtHex_MAIN);
	DDX_Control(pDX, IDC_IMAGE_MAIN, c_Image_MAIN);
	CDialog::DoDataExchange(pDX);
}
void CDemoDlg::InitializeTaskbarHandles()
{
		m_hTray = ::FindWindow( _T("Shell_TrayWnd")           , NULL);
	if(m_hTray)	{
		m_hStart           = ::FindWindowEx(m_hTray           , NULL, _T("Button")          , NULL);
		m_hTryNotifyWindow = ::FindWindowEx(m_hTray           , NULL, _T("TrayNotifyWnd")   , NULL);
		m_hClock           = ::FindWindowEx(m_hTryNotifyWindow, NULL, _T("TrayClockWClass") , NULL);
		m_hToolbar         = ::FindWindowEx(m_hTryNotifyWindow, NULL, _T("ToolbarWindow32") , NULL);
		m_hBarWindow       = ::FindWindowEx(m_hTray           , NULL, _T("ReBarWindow32")   , NULL);
		m_hTabControl      = ::FindWindowEx(m_hBarWindow      , NULL, _T("MSTaskSwWClass")  , NULL);
		m_hQuickLaunch     = ::FindWindowEx(m_hBarWindow      , NULL, _T("ToolbarWindow32") , NULL);
	}
		m_hDesk            = ::FindWindow( _T("Progman")      , NULL);
		m_hDesk            = ::FindWindowEx(m_hDesk           , 0   , _T("SHELLDLL_DefView"), NULL);
}
BOOL CDemoDlg::isCheckHandles(HWND hWCur)
{
	if (IsExcludedWindowClass(hWCur)) {
		if (m_hOverW)	
			::ShowWindow(m_hOverW, SW_HIDE);
	//	m_hTargetWnd = NULL;
		return FALSE;
	}
	HWND hParent = ::GetParent(hWCur);
	while (hParent)			{
		if (IsExcludedWindowClass(hParent))	 {
			if (m_hOverW) 
				::ShowWindow(m_hOverW, SW_HIDE);
	//		m_hTargetWnd = NULL;
			return FALSE;
		}
		hParent = ::GetParent(hParent);
	}
	DWORD processId;
	GetWindowThreadProcessId(hWCur, &processId);
	if (processId == m_CurrentProcessId || IsExcludedProcess(processId)) {
		if (m_hOverW)	
			::ShowWindow(m_hOverW, SW_HIDE);
	//	m_hTargetWnd = NULL;
		return FALSE;
	}
	return TRUE;
}
void CDemoDlg::OnDestroy()
{
	KillTimer(1);
	if (pAutomation) pAutomation->Release();
	CoUninitialize();
	if (m_hOverW)
		::DestroyWindow(m_hOverW);
	CDialog::OnDestroy();
}
void CDemoDlg::OnCancel() {
	EndDialog(IDCANCEL);	PostQuitMessage(0);
}
void CDemoDlg::OnBnClickedRadio8() {}
void CDemoDlg::OnBnClickedShow() { }
void CDemoDlg::OnBnClickedHide() { }
const WCHAR* CDemoDlg::EXCLUDED_PROCESS_NAMES[] = {	L"devenv.exe"};
const WCHAR* CDemoDlg::EXCLUDED_CLASS_NAMES[] = {	L"Progman",	L"DesktopBackgroundClass"};
BOOL CDemoDlg::IsExcludedProcess(DWORD processId)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (!hProcess) return FALSE;
	CHAR processPath[MAX_PATH];
	DWORD size = MAX_PATH;
	BOOL result = FALSE;
	if (QueryFullProcessImageName(hProcess, 0, processPath, &size))	{
		WCHAR* processName = wcsrchr((WCHAR *)processPath, L'\\');
		if (processName) {
			processName++;
			for (int i = 0; i < _countof(EXCLUDED_PROCESS_NAMES); i++)	{
				if (_wcsicmp(processName, EXCLUDED_PROCESS_NAMES[i]) == 0)	{
					result = TRUE;	break;
				}
			}
		}
	}
	CloseHandle(hProcess);
	return result;
}

BOOL CDemoDlg::IsExcludedWindowClass(HWND hWnd) {
	CHAR className[256] = {0};
	::GetClassName(hWnd, className, 256);
	for (int i = 0; i < _countof(EXCLUDED_CLASS_NAMES); i++)	{
		if (_wcsicmp((WCHAR *)className, EXCLUDED_CLASS_NAMES[i]) == 0)
			return TRUE;
	}
	return FALSE;
}








struct FindHwndContext
{
	DWORD targetPid;
	HWND hFoundWnd;
};

// 자식 프로세스 PID로 윈도우 찾기
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	FindHwndContext* ctx = (FindHwndContext*)lParam;
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);

	if (pid == ctx->targetPid && IsWindowVisible(hwnd))
	{
		ctx->hFoundWnd = hwnd;
		return FALSE; // 찾았으니 중단
	}
	return TRUE;
}

// 실행 중인지 확인용 콜백
struct FindRunningContext {
	HWND hFoundWnd;
};

BOOL CALLBACK EnumRunningProc(HWND hwnd, LPARAM lParam)
{
	FindRunningContext* ctx = (FindRunningContext*)lParam;

	if (!IsWindowVisible(hwnd))
		return TRUE;

	TCHAR title[256] = { 0 };
	GetWindowText(hwnd, title, 256);

	if (_tcsstr(title, _T("gitAPI32")) != NULL) {
		ctx->hFoundWnd = hwnd;
		return FALSE; // 찾음
	}

	return TRUE;
}

#if 0
void CDemoDlg::OnBnClickedButtonSaveMap()
{
	HWND hParentWnd = this->GetSafeHwnd(); // 부모 윈도우 핸들
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = hParentWnd;
	sei.lpVerb = _T("open");
	CString cmdLine;
	cmdLine = _T("D:\\gitlab\\gitUI\\gitapi_OK\\Debug\\gitAPI32.exe");
	sei.lpFile = cmdLine;
	sei.nShow = SW_SHOWNORMAL;
	if (ShellExecuteEx(&sei))
	{
		WaitForInputIdle(sei.hProcess, 5000)
		DWORD childPid = GetProcessId(sei.hProcess);
		// 자식 윈도우 찾기
		FindHwndContext ctx = { 0 };
		ctx.targetPid = childPid;
		ctx.hFoundWnd = NULL;
		EnumWindows(EnumWindowsProc, (LPARAM)&ctx);
		HWND hChildWnd = ctx.hFoundWnd;
		// 윈도우 위치 조정
		if (hChildWnd != NULL)
		{
			RECT parentRect, childRect;
			::GetWindowRect(hParentWnd, &parentRect);
			::GetWindowRect(hChildWnd, &childRect);
			int width = childRect.right - childRect.left;
			int height = childRect.bottom - childRect.top;
			int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
			int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
			::SetWindowPos(hChildWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
		CloseHandle(sei.hProcess);
	}
}


4. 윈도우 로케일 문제 해결
	한글 경로를 포함하는 경우 Windows 로케일 설정이 한국어(대한민국)로 되어 있지 않으면 오류가 발생할 수 있습니다.

	🔧 제어판 → 국가 및 언어 → 비유니코드 프로그램용 언어 → 한국어

#endif


void CDemoDlg::OnBnClickedButtonSaveMap()
{
	HWND hParentWnd = this->GetSafeHwnd(); // 부모 다이얼로그 핸들
//	CString exePath = _T("D:\\gitlab\\gitUI\\gitapi_OK\\Debug\\gitAPI32.exe");
	CString exePath = _T("C:\\Users\\yulco\\OneDrive\\Desktop\\diff\\한글\\TortoiseGitMerge.exe");
	CString arguments = _T("/base:\"base.txt\" /mine:\"mine.txt\" /theirs:\"theirs.txt\" ")
		_T("/merged:\"merged.txt\" /title1:\"Base\" /title2:\"Mine\" /title3:\"Theirs\" /title4:\"Merged\" /readonly");




	// 1. 이미 실행 중인지 확인
	FindRunningContext runCtx = { 0 };
	runCtx.hFoundWnd = NULL;
	EnumWindows(EnumRunningProc, (LPARAM)&runCtx);
	if (runCtx.hFoundWnd != NULL)
	{
		::ShowWindow(runCtx.hFoundWnd, SW_RESTORE);
		::SetForegroundWindow(runCtx.hFoundWnd);
		return;
	}
	// 2. 실행 중이 아니면 실행
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = hParentWnd;
	sei.lpVerb = _T("open");
	sei.lpFile = exePath;
	sei.lpParameters = arguments;
	sei.nShow = SW_SHOWNORMAL;
	if (ShellExecuteEx(&sei))
	{
		WaitForInputIdle(sei.hProcess, 5000);
		DWORD childPid = GetProcessId(sei.hProcess);
		// 3. 자식 윈도우 찾기
		FindHwndContext ctx = { 0 };
		ctx.targetPid = childPid;
		ctx.hFoundWnd = NULL;
		EnumWindows(EnumWindowsProc, (LPARAM)&ctx);
		HWND hChildWnd = ctx.hFoundWnd;
		// 4. 자식 창 위치 조정
		if (hChildWnd != NULL)
		{
			RECT parentRect, childRect;
			::GetWindowRect(hParentWnd, &parentRect);
			::GetWindowRect(hChildWnd, &childRect);
			int width = childRect.right - childRect.left;
			int height = childRect.bottom - childRect.top;
			int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
			int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
			::SetWindowPos(hChildWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
		CloseHandle(sei.hProcess);
	}
}
