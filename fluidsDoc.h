// fluidsDoc.h : interface of the CfluidsDoc class
//


#pragma once


class CfluidsDoc : public CDocument
{
protected: // create from serialization only
	CfluidsDoc();
	DECLARE_DYNCREATE(CfluidsDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CfluidsDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


