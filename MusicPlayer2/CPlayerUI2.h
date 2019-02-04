#pragma once
#include "DrawCommon.h"
#include "CPlayerUIBase.h"


class CPlayerUI2 : public CPlayerUIBase
{
public:
	CPlayerUI2(UIData& ui_data);
	~CPlayerUI2();

	virtual void DrawInfo(bool reset = false) override;		//绘制信息

	virtual void RButtonUp(CPoint point) override;
	virtual void MouseMove(CPoint point) override;
	virtual void LButtonUp(CPoint point) override;

	//virtual CRect GetThumbnailClipArea() override;

	virtual void AddMouseToolTip(BtnKey btn, LPCTSTR str) override;		//为一个按钮添加鼠标提示
	virtual void UpdateMouseToolTip(BtnKey btn, LPCTSTR str) override;
	virtual void UpdateToolTipPosition() override;

protected:
	//void DrawToolBar(bool draw_background, CRect rect);

protected:
	CFont m_title_font;
	CFont m_artist_font;
};

