// fluidsView.h : interface of the CfluidsView class
//


#pragma once


class CfluidsView : public CView
{
protected: // create from serialization only
	CfluidsView();
	DECLARE_DYNCREATE(CfluidsView)

// Attributes
public:
	CfluidsDoc* GetDocument() const;
	bool m_leftButtonDown;
	CPoint m_pt;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view

// Implementation
public:
	virtual ~CfluidsView();

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void OnInitialUpdate();
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
public:
	afx_msg void OnDestroy();
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

#ifndef _DEBUG  // debug version in fluidsView.cpp
inline CfluidsDoc* CfluidsView::GetDocument() const
   { return reinterpret_cast<CfluidsDoc*>(m_pDocument); }
#endif

