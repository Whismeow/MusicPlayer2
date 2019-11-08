// LyricsWindow.cpp : 实现文件
//

#include "stdafx.h"
#include "LyricsWindow.h"
#include "Resource.h"
#include "Common.h"
#include "MusicPlayer2.h"
#include "GdiPlusTool.h"


// CLyricsWindow

const Gdiplus::REAL TRANSLATE_FONT_SIZE_FACTOR = 0.88f;		//歌词翻译文本大小占歌词文本大小的比例

IMPLEMENT_DYNAMIC(CLyricsWindow, CWnd)

CLyricsWindow::CLyricsWindow()
{
	HDC hDC=::GetDC(NULL);
	m_hCacheDC=::CreateCompatibleDC(hDC);
	::ReleaseDC(NULL,hDC);
	//---------------------------------
	m_nHighlight=NULL ; //高亮歌词的百分比 0--100
	m_TextGradientMode=LyricsGradientMode_Two ; //普通歌词渐变模式
	m_pTextPen=NULL ; //普通歌词边框画笔
	m_HighlightGradientMode=LyricsGradientMode_Two ; //高亮歌词渐变模式
	m_pHighlightPen=NULL ; //高亮歌词边框画笔
	m_pShadowBrush=NULL ; //阴影画刷,GDIPlus画刷 
	m_nShadowOffset=1 ; //阴影偏移
	m_pFont=NULL ; //GDIPlus字体
	m_FontStyle=NULL ; 
	m_FontSize=NULL ; 
	m_pTextFormat=NULL;
	//---------------------------------
	m_pFontFamily=new Gdiplus::FontFamily();
	m_pTextFormat=new Gdiplus::StringFormat();
	m_pTextFormat->SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);//不换行
	m_pTextFormat->SetAlignment(Gdiplus::StringAlignmentCenter); //置水平对齐方式
	m_pTextFormat->SetLineAlignment(Gdiplus::StringAlignmentNear); //置垂直对齐方式
	//---------------------------------
	//SetLyricsFont(L"微软雅黑", 40, Gdiplus::FontStyle::FontStyleRegular);
	//SetLyricsColor(Gdiplus::Color::Red,Gdiplus::Color(255,172,0),LyricsGradientMode_Three);
	//SetLyricsBorder(Gdiplus::Color::Black,1);
	SetLyricsShadow(Gdiplus::Color(150,0,0,0),1);
	//SetHighlightColor(Gdiplus::Color(255,100,26),Gdiplus::Color(255,255,0),LyricsGradientMode_Three);
	//SetHighlightBorder(Gdiplus::Color::Black,1);
	
}

CLyricsWindow::~CLyricsWindow()
{
	if(m_pTextPen){
		delete m_pTextPen;
		m_pTextPen=NULL;
	}
	if(m_pHighlightPen){
		delete m_pHighlightPen;
		m_pHighlightPen=NULL;
	}
	if(m_pShadowBrush){
		delete m_pShadowBrush;
		m_pShadowBrush=NULL;
	}
	if(m_pFontFamily){
		delete m_pFontFamily;
		m_pFontFamily=NULL;
	}	
	if(m_pTextFormat){
		delete m_pTextFormat;
		m_pTextFormat=NULL;
	}	
	if(m_pFont){
		delete m_pFont;
		m_pFont=NULL;
	}
}


BEGIN_MESSAGE_MAP(CLyricsWindow, CWnd)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSEHOVER()
    ON_WM_MOUSELEAVE()
    ON_WM_SIZING()
    ON_WM_RBUTTONUP()
    ON_WM_GETMINMAXINFO()
    ON_MESSAGE(WM_INITMENU, &CLyricsWindow::OnInitmenu)
END_MESSAGE_MAP()



BOOL CLyricsWindow::Create(int nHeight)
{
	return CLyricsWindow::Create(_T("CometLyricsWindow"), -1, nHeight);
}
BOOL CLyricsWindow::Create(LPCTSTR lpszClassName)
{
	return CLyricsWindow::Create(lpszClassName,-1,-1);
}
BOOL CLyricsWindow::Create(LPCTSTR lpszClassName,int nWidth,int nHeight)
{
	if(!RegisterWndClass(lpszClassName))
	{
		TRACE("Class　Registration　Failedn");
	}

    m_popupMenu.LoadMenu(IDR_DESKTOP_LYRIC_POPUP_MENU);

	//--------------------------------------------
	//取出桌面工作区域
	RECT rcWork;
	SystemParametersInfo (SPI_GETWORKAREA,NULL,&rcWork,NULL);
	int nWorkWidth=rcWork.right-rcWork.left;
	int nWorkHeight=rcWork.bottom-rcWork.top;
	//未传递宽度、高度参数时设置个默认值
	if(nWidth<0)nWidth=nWorkWidth*2/3;      //默认宽度为桌面宽度的2/3
	if(nHeight<0)nHeight=150;
	//设置左边、顶边位置,让窗口在屏幕下方
	int x=rcWork.left+( (nWorkWidth-nWidth)/2 );
	int y=rcWork.bottom-nHeight;
	//--------------------------------------------
	DWORD dwStyle=WS_POPUP|WS_VISIBLE|WS_THICKFRAME;
	DWORD dwExStyle=WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_LAYERED;
	return CWnd::CreateEx(dwExStyle, lpszClassName, NULL, dwStyle, x, y, nWidth, nHeight, NULL, NULL);
}
BOOL CLyricsWindow::RegisterWndClass(LPCTSTR lpszClassName)
{
	HINSTANCE hInstance=AfxGetInstanceHandle();
	WNDCLASSEX wndcls;
	memset(&wndcls,0,sizeof(WNDCLASSEX));
	wndcls.cbSize=sizeof(WNDCLASSEX);
	if(GetClassInfoEx(hInstance,lpszClassName,&wndcls))
	{
		return TRUE;
	}
	if(GetClassInfoEx(NULL,lpszClassName,&wndcls))
	{
		return TRUE;
	}

	wndcls.style=CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
	wndcls.lpfnWndProc=::DefWindowProc;
	wndcls.hInstance=hInstance;
	wndcls.hIcon=NULL;
	wndcls.hCursor=::LoadCursor(NULL,IDC_ARROW);
	wndcls.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);
	wndcls.lpszMenuName=NULL;
	wndcls.lpszClassName=lpszClassName;
	if(!RegisterClassEx(&wndcls))
	{
		return FALSE;
	}
	return TRUE;
}


//更新歌词(歌词文本,高亮进度百分比)
void CLyricsWindow::UpdateLyrics(LPCTSTR lpszLyrics,int nHighlight)
{
    m_lpszLyrics = lpszLyrics;
    UpdateLyrics(nHighlight);
}
//更新高亮进度(高亮进度百分比)
void CLyricsWindow::UpdateLyrics(int nHighlight)
{
	m_nHighlight=nHighlight;
	if(m_nHighlight<0)
		m_nHighlight=0;
	if(m_nHighlight>1000)
		m_nHighlight=1000;
	Draw();
}

void CLyricsWindow::UpdateLyricTranslate(LPCTSTR lpszLyricTranslate)
{
	m_strTranslate = lpszLyricTranslate;
}

//重画歌词窗口
void CLyricsWindow::Draw()
{
	CRect rcClient;
	GetWindowRect(rcClient);
	m_nWidth=rcClient.Width();
	m_nHeight=rcClient.Height();
	//----------------------------------
	BITMAPINFO bitmapinfo;
	bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapinfo.bmiHeader.biBitCount = 32;
	bitmapinfo.bmiHeader.biHeight = m_nHeight;
	bitmapinfo.bmiHeader.biWidth = m_nWidth;
	bitmapinfo.bmiHeader.biPlanes = 1;
	bitmapinfo.bmiHeader.biCompression=BI_RGB;
	bitmapinfo.bmiHeader.biXPelsPerMeter=0;
	bitmapinfo.bmiHeader.biYPelsPerMeter=0;
	bitmapinfo.bmiHeader.biClrUsed=0;
	bitmapinfo.bmiHeader.biClrImportant=0;
	bitmapinfo.bmiHeader.biSizeImage = bitmapinfo.bmiHeader.biWidth * bitmapinfo.bmiHeader.biHeight * bitmapinfo.bmiHeader.biBitCount / 8;
	HBITMAP hBitmap=CreateDIBSection (m_hCacheDC,&bitmapinfo, 0,NULL, 0, 0);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject (m_hCacheDC,hBitmap);
	//----------------------------------
	Gdiplus::Graphics* pGraphics=new Gdiplus::Graphics(m_hCacheDC);
	pGraphics->SetSmoothingMode (Gdiplus::SmoothingModeAntiAlias);
	pGraphics->SetTextRenderingHint (Gdiplus::TextRenderingHintAntiAlias);

	//绘制半透明背景
	if(m_bDrawBackground)
	{
        BYTE alpha = m_bHot ? 80 : 2;
		Gdiplus::Brush* pBrush = new Gdiplus::SolidBrush(Gdiplus::Color(alpha, 255, 255, 255));
		pGraphics->FillRectangle(pBrush, 0, 0, rcClient.Width(), rcClient.Height());
		delete pBrush;
	}
    if (m_bDoubleLine)
        DrawLyricsDoubleLine(pGraphics);
    else
        DrawLyrics(pGraphics);

    //绘制工具条
    if (m_bDrawBackground && m_bHot)
        DrawToolbar(pGraphics);

	delete pGraphics;
	//----------------------------------
	//设置透明窗口
	CPoint DestPt(0,0);
	CSize psize(m_nWidth,m_nHeight);
	BLENDFUNCTION blendFunc32bpp;
	blendFunc32bpp.AlphaFormat = AC_SRC_ALPHA;
	blendFunc32bpp.BlendFlags = 0;
	blendFunc32bpp.BlendOp = AC_SRC_OVER;
	blendFunc32bpp.SourceConstantAlpha = m_alpha;
	HDC hDC=::GetDC(m_hWnd);
	::UpdateLayeredWindow(m_hWnd,hDC,NULL,&psize,m_hCacheDC,&DestPt,0,&blendFunc32bpp,ULW_ALPHA);
	//----------------------------------
	//释放资源
	::SelectObject (m_hCacheDC,hOldBitmap);
	::DeleteObject(hBitmap);
	::ReleaseDC(m_hWnd,hDC);
}

void CLyricsWindow::DrawLyricText(Gdiplus::Graphics* pGraphics, LPCTSTR strText, Gdiplus::RectF rect, bool bDrawHighlight, bool bDrawTranslate)
{
	Gdiplus::REAL fontSize = bDrawTranslate ? m_FontSize * TRANSLATE_FONT_SIZE_FACTOR : m_FontSize;
	if (fontSize < 1)
		fontSize = m_FontSize;

    Gdiplus::REAL textWidth = rect.Width;
    Gdiplus::REAL highlighWidth = rect.Width * m_nHighlight / 1000;

    if (!bDrawHighlight && !bDrawTranslate)
    {
        if (rect.X < 0)
            rect.X = 0;
    }
    else
    {
        //如果文本宽度大于控件宽度，就要根据分割的位置滚动文本
        if (textWidth > m_nWidth)
        {
            //如果分割的位置（歌词进度）剩下的宽度已经小于控件宽度的一半，此时使文本右侧和控件右侧对齐
            if (textWidth - highlighWidth < m_nWidth / 2)
            {
                rect.X = m_nWidth - textWidth;
            }
            //分割位置剩下的宽度还没有到小于控件宽度的一半，但是分割位置的宽度已经大于控件宽度的一半时，需要移动文本使分割位置正好在控件的中间
            else if (highlighWidth > m_nWidth / 2)
            {
                rect.X = m_nWidth / 2 - highlighWidth;
            }
            //分割位置还不到控件宽度的一半时，使文本左侧和控件左侧对齐
            else
            {
                rect.X = 0;
            }
        }
    }

	//-----------------------------------------------------------
	//画出阴影
	if (m_pShadowBrush) {
		Gdiplus::RectF layoutRect(0, 0, 0, 0);
		layoutRect = rect;
		layoutRect.X = layoutRect.X + m_nShadowOffset;
		layoutRect.Y = layoutRect.Y + m_nShadowOffset;
		Gdiplus::GraphicsPath* pShadowPath = new Gdiplus::GraphicsPath(Gdiplus::FillModeAlternate);//创建路径
		pShadowPath->AddString(strText, -1, m_pFontFamily, m_FontStyle, fontSize, layoutRect, m_pTextFormat); //把文字加入路径
		pGraphics->FillPath(m_pShadowBrush, pShadowPath);//填充路径
		delete pShadowPath; //销毁路径
	}

	//-----------------------------------------------------------
	//画出歌词
	Gdiplus::GraphicsPath* pStringPath = new Gdiplus::GraphicsPath(Gdiplus::FillModeAlternate);//创建路径
	pStringPath->AddString(strText, -1, m_pFontFamily, m_FontStyle, fontSize, rect, m_pTextFormat); //把文字加入路径
	if (m_pTextPen) {
		pGraphics->DrawPath(m_pTextPen, pStringPath);//画路径,文字边框
	}
	Gdiplus::Brush* pBrush = CreateGradientBrush(m_TextGradientMode, m_TextColor1, m_TextColor2, rect);
	pGraphics->FillPath(pBrush, pStringPath);//填充路径
	delete pBrush;//销毁画刷
	if(bDrawHighlight)
		DrawHighlightLyrics(pGraphics, pStringPath, rect);
	delete pStringPath; //销毁路径
}

//绘制歌词
void CLyricsWindow::DrawLyrics(Gdiplus::Graphics* pGraphics)
{
	//先取出文字宽度和高度
	Gdiplus::RectF layoutRect(0,0,0,0);
	Gdiplus::RectF boundingBox;
	pGraphics->MeasureString (m_lpszLyrics, -1, m_pFont,layoutRect, m_pTextFormat,&boundingBox, 0, 0);
	//计算歌词画出的位置
	Gdiplus::RectF dstRect;		//文字的矩形
	Gdiplus::RectF transRect;	//翻译文本的矩形
    bool bDrawTranslate = m_bShowTranslate && !m_strTranslate.IsEmpty();
	if(!bDrawTranslate)
	{
		dstRect = Gdiplus::RectF((m_nWidth - boundingBox.Width) / 2, (m_nHeight - boundingBox.Height) / 2, boundingBox.Width, boundingBox.Height);
	}
	else
	{
		Gdiplus::RectF transBoundingBox;
		pGraphics->MeasureString(m_strTranslate, -1, m_pFont, layoutRect, m_pTextFormat, &transBoundingBox, 0, 0);
		Gdiplus::REAL translateHeight = boundingBox.Height * TRANSLATE_FONT_SIZE_FACTOR;
		Gdiplus::REAL maxWidth = max(boundingBox.Width, transBoundingBox.Width);
		Gdiplus::REAL gapHeight = boundingBox.Height * 0.2f;	//歌词和翻译之间的间隙
		Gdiplus::REAL height = boundingBox.Height + gapHeight + translateHeight;
		Gdiplus::RectF textRect((m_nWidth - maxWidth) / 2, (m_nHeight - height) / 2, maxWidth, height);
		dstRect = textRect;
		dstRect.Height = boundingBox.Height;
		transRect = textRect;
		transRect.Y += (boundingBox.Height + gapHeight);
		transRect.Height = translateHeight;
	}

	DrawLyricText(pGraphics, m_lpszLyrics, dstRect, true);
	if (bDrawTranslate)
		DrawLyricText(pGraphics, m_strTranslate, transRect, false, true);
}

void CLyricsWindow::DrawLyricsDoubleLine(Gdiplus::Graphics* pGraphics)
{
    static bool bSwap = false;
    if (m_lyricChangeFlag)      //如果歌词发生了改变，则交换当前歌词和下一句歌词的位置
        bSwap = !bSwap;
    //先取出文字宽度和高度
    Gdiplus::RectF layoutRect(0, 0, 0, 0);
    Gdiplus::RectF boundingBox;
    pGraphics->MeasureString(m_lpszLyrics, -1, m_pFont, layoutRect, m_pTextFormat, &boundingBox, 0, 0);
    Gdiplus::RectF nextBoundingBox;
    pGraphics->MeasureString(m_strNextLyric, -1, m_pFont, layoutRect, m_pTextFormat, &nextBoundingBox, 0, 0);
    //计算歌词画出的位置
    Gdiplus::RectF dstRect;		//文字的矩形
    Gdiplus::RectF nextRect;	//下一句文本的矩形

    dstRect = Gdiplus::RectF(0, (m_nHeight / 2 - boundingBox.Height) / 2, boundingBox.Width, boundingBox.Height);
    nextRect = Gdiplus::RectF(m_nWidth - nextBoundingBox.Width, dstRect.Y + m_nHeight / 2, nextBoundingBox.Width, nextBoundingBox.Height);

    if (bSwap)
    {
        std::swap(dstRect.Y, nextRect.Y);
        nextRect.X = 0;
        dstRect.X = m_nWidth - dstRect.Width;
    }

    DrawLyricText(pGraphics, m_lpszLyrics, dstRect, true);
    DrawLyricText(pGraphics, m_strNextLyric, nextRect, false);
}

//绘制高亮歌词
void CLyricsWindow::DrawHighlightLyrics(Gdiplus::Graphics* pGraphics,Gdiplus::GraphicsPath* pPath, Gdiplus::RectF& dstRect)
{
	if(m_nHighlight<=0)return;
	Gdiplus::Region* pRegion=NULL;
	if(m_nHighlight<1000){
		Gdiplus::RectF CliptRect(dstRect);
		CliptRect.Width=CliptRect.Width * m_nHighlight / 1000;
		pRegion=new Gdiplus::Region(CliptRect);
		pGraphics->SetClip(pRegion, Gdiplus::CombineModeReplace);
	}
	//--------------------------------------------
	if(m_pHighlightPen){
		pGraphics->DrawPath (m_pHighlightPen,pPath);//画路径,文字边框
	}
	Gdiplus::Brush* pBrush = CreateGradientBrush(m_HighlightGradientMode, m_HighlightColor1,m_HighlightColor2,dstRect);
	pGraphics->FillPath (pBrush,pPath);//填充路径
	delete pBrush;//销毁画刷
	//--------------------------------------------
	if(pRegion){
		pGraphics->ResetClip();
		delete pRegion;
	}
}

void CLyricsWindow::DrawToolbar(Gdiplus::Graphics* pGraphics)
{
    const int toolbar_num = 4;
    const int btn_size = theApp.DPI(24);
    int toolbar_width = toolbar_num * btn_size;
    Gdiplus::Rect toolbar_rect;
    toolbar_rect.Y = 0;
    toolbar_rect.X = (m_nWidth - toolbar_width) / 2;
    toolbar_rect.Width = toolbar_width;
    toolbar_rect.Height = btn_size;

    //绘制背景
    Gdiplus::Color back_color = CGdiPlusTool::COLORREFToGdiplusColor(theApp.m_app_setting_data.theme_color.light3, 180);
    Gdiplus::Brush* pBrush = new Gdiplus::SolidBrush(back_color);
    pGraphics->FillRectangle(pBrush, toolbar_rect);
    delete pBrush;

    HDC hDC = pGraphics->GetHDC();
    CDrawCommon drawer;
    drawer.Create(CDC::FromHandle(hDC));

    //
    CRect rcIcon = CRect(toolbar_rect.X, toolbar_rect.Y, toolbar_rect.GetRight(), toolbar_rect.GetBottom());
    rcIcon.right = rcIcon.left + btn_size;

    DrawToolIcon(pGraphics, drawer, theApp.m_icon_set.stop_l, rcIcon, BTN_STOP);
    rcIcon.MoveToX(rcIcon.right);
    DrawToolIcon(pGraphics, drawer, theApp.m_icon_set.previous, rcIcon, BTN_PREVIOUS);
    rcIcon.MoveToX(rcIcon.right);
    IconRes hPlayPauseIcon = (CPlayer::GetInstance().IsPlaying() ? theApp.m_icon_set.pause : theApp.m_icon_set.play);
    DrawToolIcon(pGraphics, drawer, hPlayPauseIcon, rcIcon, BTN_PLAY_PAUSE);
    rcIcon.MoveToX(rcIcon.right);
    DrawToolIcon(pGraphics, drawer, theApp.m_icon_set.next, rcIcon, BTN_NEXT);

    pGraphics->ReleaseHDC(hDC);
}

void CLyricsWindow::DrawToolIcon(Gdiplus::Graphics* pGraphics, CDrawCommon& drawer, IconRes icon, CRect rect, BtnKey btn_key, bool checked)
{
    rect.DeflateRect(theApp.DPI(2), theApp.DPI(2));
    auto& btn = m_buttons[btn_key];
    btn.rect = rect;

    if (btn.pressed)
        rect.MoveToXY(rect.left + theApp.DPI(1), rect.top + theApp.DPI(1));

    Gdiplus::Color back_color;
    if (btn.pressed && btn.hover)
        back_color = CGdiPlusTool::COLORREFToGdiplusColor(theApp.m_app_setting_data.theme_color.dark1, 160);
    else if (btn.hover)
        back_color = CGdiPlusTool::COLORREFToGdiplusColor(theApp.m_app_setting_data.theme_color.light1, 160);
    else if(checked)
        back_color = CGdiPlusTool::COLORREFToGdiplusColor(theApp.m_app_setting_data.theme_color.light2, 160);
    Gdiplus::Brush* pBrush = new Gdiplus::SolidBrush(back_color);
    pGraphics->FillRectangle(pBrush, rect.left, rect.top, rect.Width(), rect.Height());
    delete pBrush;

    //drawer.GetDC()->FillSolidRect(rect, theApp.m_app_setting_data.theme_color.light1);

    CRect rc_tmp = rect;
    //使图标在矩形中居中
    CSize icon_size = icon.GetSize(theApp.m_ui_data.full_screen);
    rc_tmp.left = rect.left + (rect.Width() - icon_size.cx) / 2;
    rc_tmp.top = rect.top + (rect.Height() - icon_size.cy) / 2;
    rc_tmp.right = rc_tmp.left + icon_size.cx;
    rc_tmp.bottom = rc_tmp.top + icon_size.cy;

    drawer.DrawIcon(icon.GetIcon(), rc_tmp.TopLeft(), rc_tmp.Size());
}

//创建渐变画刷
Gdiplus::Brush* CLyricsWindow::CreateGradientBrush(LyricsGradientMode TextGradientMode,Gdiplus::Color& Color1,Gdiplus::Color& Color2, Gdiplus::RectF& dstRect)
{
	Gdiplus::PointF pt1;
	Gdiplus::PointF pt2;
	Gdiplus::Brush* pBrush=NULL;
	switch (TextGradientMode)
	{
	case LyricsGradientMode_Two://两色渐变
		{
			Gdiplus::PointF point1(dstRect.X,dstRect.Y);
			Gdiplus::PointF point2(dstRect.X,dstRect.Y+dstRect.Height);
			pBrush=new Gdiplus::LinearGradientBrush(point1,point2,Color1,Color2);
			((Gdiplus::LinearGradientBrush*)pBrush)->SetWrapMode(Gdiplus::WrapModeTileFlipXY);
			break;
		}

	case LyricsGradientMode_Three://三色渐变
		{
			Gdiplus::PointF point1(dstRect.X,dstRect.Y);
			Gdiplus::PointF point2(dstRect.X,dstRect.Y+dstRect.Height/2);
			pBrush=new Gdiplus::LinearGradientBrush(point1,point2,Color1,Color2);
			((Gdiplus::LinearGradientBrush*)pBrush)->SetWrapMode(Gdiplus::WrapModeTileFlipXY);
			break;
		}

	default://无渐变
		{
			pBrush=new Gdiplus::SolidBrush(Color1);
			break;
		}
	}
	return pBrush;
}

//设置歌词颜色
void CLyricsWindow::SetLyricsColor(Gdiplus::Color TextColor1)
{
	CLyricsWindow::SetLyricsColor(TextColor1,Gdiplus::Color::Black,LyricsGradientMode_None);
}
void CLyricsWindow::SetLyricsColor(Gdiplus::Color TextColor1,Gdiplus::Color TextColor2,LyricsGradientMode TextGradientMode)
{
	m_TextColor1=TextColor1;
	m_TextColor2=TextColor2;
	m_TextGradientMode=TextGradientMode;

}
//设置歌词边框
void CLyricsWindow::SetLyricsBorder(Gdiplus::Color BorderColor, Gdiplus::REAL BorderWidth)
{
	if(m_pTextPen){
		delete m_pTextPen;
		m_pTextPen=NULL;
	}
	if(BorderColor.GetA()>0 && BorderWidth>0)
		m_pTextPen=new Gdiplus::Pen(BorderColor,BorderWidth);
}
//设置高亮歌词颜色
void CLyricsWindow::SetHighlightColor(Gdiplus::Color TextColor1)
{
	CLyricsWindow::SetHighlightColor(TextColor1,Gdiplus::Color::Black,LyricsGradientMode_None);
}
void CLyricsWindow::SetHighlightColor(Gdiplus::Color TextColor1,Gdiplus::Color TextColor2,LyricsGradientMode TextGradientMode)
{
	m_HighlightColor1=TextColor1;
	m_HighlightColor2=TextColor2;
	m_HighlightGradientMode=TextGradientMode;

}
//设置高亮歌词边框
void CLyricsWindow::SetHighlightBorder(Gdiplus::Color BorderColor, Gdiplus::REAL BorderWidth)
{
	if(m_pHighlightPen){
		delete m_pHighlightPen;
		m_pHighlightPen=NULL;
	}
	if(BorderColor.GetA()>0 && BorderWidth>0)
		m_pHighlightPen=new Gdiplus::Pen(BorderColor,BorderWidth);
}
//设置歌词阴影
void CLyricsWindow::SetLyricsShadow(Gdiplus::Color ShadowColor,int nShadowOffset)
{
	if(m_pShadowBrush){
		delete m_pShadowBrush;
		m_pShadowBrush=NULL;
	}
	if(ShadowColor.GetA()>0 && nShadowOffset>0){
		m_nShadowOffset=nShadowOffset;
		m_pShadowBrush=new Gdiplus::SolidBrush(ShadowColor);
	}else{
		m_nShadowOffset=0;
	}
}
//设置歌词字体
void CLyricsWindow::SetLyricsFont(const WCHAR * familyName, Gdiplus::REAL emSize,INT style, Gdiplus::Unit unit)
{
	if(m_pFont){
		delete m_pFont;
		m_pFont=NULL;
	}
	Gdiplus::FontFamily family(familyName,NULL);
	Gdiplus::Status lastResult = family.GetLastStatus();
	if (lastResult != Gdiplus::Ok)
	{
		HFONT hFont=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
		LOGFONTW lf;
		ZeroMemory(&lf,sizeof(LOGFONTW));
		GetObjectW(hFont,sizeof(LOGFONTW),&lf);
		Gdiplus::FontFamily family2(lf.lfFaceName,NULL);
		m_pFont=new Gdiplus::Font(&family2,emSize,style,unit);
	}else{
		m_pFont=new Gdiplus::Font(&family,emSize,style,unit);
	}
	 //----------------
	//保存一些字体属性,加入路径时要用到
	m_pFont->GetFamily (m_pFontFamily);
	m_FontSize=m_pFont->GetSize ();
	m_FontStyle=m_pFont->GetStyle ();

	
	
}

void CLyricsWindow::SetLyricDoubleLine(bool doubleLine)
{
	m_bDoubleLine = doubleLine;
}

void CLyricsWindow::SetNextLyric(LPCTSTR lpszNextLyric)
{
	m_strNextLyric = lpszNextLyric;
}

void CLyricsWindow::SetDrawBackground(bool drawBackground)
{
	m_bDrawBackground = drawBackground;
}

void CLyricsWindow::SetShowTranslate(bool showTranslate)
{
    m_bShowTranslate = showTranslate;
}

void CLyricsWindow::SetAlpha(int alpha)
{
    m_alpha = alpha;
}

const CString& CLyricsWindow::GetLyricStr() const
{
    return m_lpszLyrics;
}

void CLyricsWindow::SetLyricChangeFlag(bool bFlag)
{
    m_lyricChangeFlag = bFlag;
}

void CLyricsWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
    for (auto& btn : m_buttons)
    {
        if (btn.second.rect.PtInRect(point) != FALSE)
            btn.second.pressed = true;
    }

	CWnd::OnLButtonDown(nFlags, point);
	ReleaseCapture();
	SendMessage(WM_NCLBUTTONDOWN,HTCAPTION,NULL);
}

void CLyricsWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
    for (auto& btn : m_buttons)
    {
        btn.second.pressed = false;
    }
	CWnd::OnLButtonUp(nFlags, point);
}


void CLyricsWindow::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    for (auto& btn : m_buttons)
    {
        btn.second.hover = (btn.second.rect.PtInRect(point) != FALSE);
    }

    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.hwndTrack = m_hWnd;
    tme.dwFlags = TME_LEAVE | TME_HOVER;
    tme.dwHoverTime = 1;
    _TrackMouseEvent(&tme);

    CWnd::OnMouseMove(nFlags, point);
}


void CLyricsWindow::OnMouseHover(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (!m_bHot)
    {
        m_bHot = true;
        //Invalidate();
    }
    else
    {
        CWnd::OnMouseHover(nFlags, point);
    }
}


void CLyricsWindow::OnMouseLeave()
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    m_bHot = false;
    //Invalidate();

    CWnd::OnMouseLeave();
}


void CLyricsWindow::OnSizing(UINT fwSide, LPRECT pRect)
{
    CWnd::OnSizing(fwSide, pRect);

    // TODO: 在此处添加消息处理程序代码
    m_bHot = true;
}


void CLyricsWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    CPoint point1;		//定义一个用于确定光标位置的位置
    GetCursorPos(&point1);	//获取当前光标的位置，以便使得菜单可以跟随光标，该位置以屏幕左上角点为原点，point则以客户区左上角为原点
    CMenu* pMenu = m_popupMenu.GetSubMenu(0);
    if (pMenu != NULL)
        pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point1.x, point1.y, this);

    //CWnd::OnRButtonUp(nFlags, point);
}


BOOL CLyricsWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    // TODO: 在此添加专用代码和/或调用基类
    if (CCommon::IsMenuItemInMenu(m_popupMenu.GetSubMenu(0), wParam))
        AfxGetMainWnd()->SendMessage(WM_COMMAND, wParam, lParam);		//将菜单命令转发到主窗口

    return CWnd::OnCommand(wParam, lParam);
}


void CLyricsWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    lpMMI->ptMinTrackSize.x = theApp.DPI(400);
    lpMMI->ptMinTrackSize.y = theApp.DPI(100);

    CWnd::OnGetMinMaxInfo(lpMMI);
}


afx_msg LRESULT CLyricsWindow::OnInitmenu(WPARAM wParam, LPARAM lParam)
{
    AfxGetMainWnd()->SendMessage(WM_INITMENU, wParam, lParam);        //将WM_INITMENU消息转发到主窗口
    return 0;
}
