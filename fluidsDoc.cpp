// fluidsDoc.cpp : implementation of the CfluidsDoc class
//

#include "stdafx.h"
#include "fluids.h"

#include "fluidsDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CfluidsDoc

IMPLEMENT_DYNCREATE(CfluidsDoc, CDocument)

BEGIN_MESSAGE_MAP(CfluidsDoc, CDocument)
END_MESSAGE_MAP()


// CfluidsDoc construction/destruction

CfluidsDoc::CfluidsDoc()
{
	// TODO: add one-time construction code here

}

CfluidsDoc::~CfluidsDoc()
{
}

BOOL CfluidsDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CfluidsDoc serialization

void CfluidsDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CfluidsDoc diagnostics

#ifdef _DEBUG
void CfluidsDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CfluidsDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CfluidsDoc commands
