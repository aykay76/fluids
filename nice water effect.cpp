// fluidsView.cpp : implementation of the CfluidsView class
//

#include "stdafx.h"
#include "fluids.h"

#include "fluidsDoc.h"
#include "fluidsView.h"

#include <gdiplus.h>
using namespace Gdiplus;

#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////////

typedef long freal;
#define FPP 9
#define X1_0 (1<<FPP)
#define IX(i,j) ((i)+(N+2)*(j))
#define I2X(x) ((freal)((x)<<FPP))
#define F2X(x) ((freal)((x)*X1_0))
#define X2I(x) ((int)((x)>>FPP))
#define X2F(x) ((float)(x)/X1_0)
#define XM(x,y) ((freal)(((x)*(y))>>FPP))
#define XD(x,y) ((freal)((((x))<<FPP)/(y)))
#define SWAP(x0,x) {float *tmp=x0;x0=x;x=tmp;}
#define size ((202)*(202))

static float u[size];
static float v[size];
static float dens[size];
static float u_prev[size];
static float v_prev[size];
static float dens_prev[size];
static int N = 200;

void add_source(float* x, float* s, float dt);
void diffuse(int b, float* x, float* x0, float diff, float dt);
void advect(int b, float* d, float* d0, float* u, float* v, float dt);
void dens_step(float* x, float* x0, float* u, float* v, float diff, float dt);
void vel_step(float* u, float* v, float* u0, float* v0, float visc, float dt);
void project(float* u, float* v, float* p, float* div);
void set_bnd(int b, float* x);

///////////////////////////////////////////////////////////////////////////////

// CfluidsView

IMPLEMENT_DYNCREATE(CfluidsView, CView)

BEGIN_MESSAGE_MAP(CfluidsView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

// CfluidsView construction/destruction

CfluidsView::CfluidsView()
	: m_leftButtonDown(false)
{
	// TODO: add construction code here

}

CfluidsView::~CfluidsView()
{
}

BOOL CfluidsView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CfluidsView drawing
Bitmap* bmp = NULL;

void CfluidsView::OnDraw(CDC* pDC)
{
	CfluidsDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	BitmapData bd;
	Rect rc(0, 0, N+2, N+2);
	Graphics graphics(pDC->GetSafeHdc());

	if (bmp)
	{
		bmp->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &bd);

		unsigned long* cp = (unsigned long *)bd.Scan0;
		for (int i = 0; i < size; i++)
		{
			unsigned char d = max(0, min(255, (unsigned char)(dens[i] * 1.0)));
			cp[i] = (d << 8) | (0xff << 24);
		}

		bmp->UnlockBits(&bd);

		graphics.DrawImage(bmp, 0, 0);
	}

	if (bmp)
	{
		bmp->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &bd);

		unsigned long* cp = (unsigned long *)bd.Scan0;
		for (int i = 0; i < size; i++)
		{
			unsigned char r = max(0, min(255, (unsigned char)(u[i] * 1.0)));
			unsigned char b = max(0, min(255, (unsigned char)(v[i] * 1.0)));
			cp[i] = (r << 16) | (0xff << 24) | (b << 0);
		}

		bmp->UnlockBits(&bd);

		graphics.DrawImage(bmp, N, 0);
	}

	CString str;
	str.Format(L"%.6f -> %d", dens[IX(3, 3)], (int)(min(255, dens[IX(3, 3)] * 1.0)));
	AfxGetMainWnd()->SetWindowText(str);
}


// CfluidsView printing

BOOL CfluidsView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CfluidsView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CfluidsView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CfluidsView diagnostics

#ifdef _DEBUG
void CfluidsView::AssertValid() const
{
	CView::AssertValid();
}

void CfluidsView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CfluidsDoc* CfluidsView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CfluidsDoc)));
	return (CfluidsDoc*)m_pDocument;
}
#endif //_DEBUG


// CfluidsView message handlers

void CfluidsView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	Graphics graphics(GetSafeHwnd());
	bmp = ::new Bitmap(N+2, N+2, &graphics);

	SetTimer(1, 40, NULL);
}

void add_source(float* x, float* s, float dt)
{
	int i;

	for (i = 0; i < size; i++ ) x[i] += dt * s[i];
}

void diffuse(int b, float * x, float * x0, float diff, float dt )
{
	int i, j;
	float a=dt*diff*N*N;
	for (int k = 0; k < 20; k++)
	{
		for (i = 1; i <= N; i++)
		{
			for (j = 1; j <= N; j++)
			{
				x[IX(i, j)] = (x0[IX(i, j)] + a * (x[IX(i - 1, j)] + x[IX(i + 1, j)] +
							    x[IX(i, j - 1)] + x[IX(i, j + 1)])) / (1 + 4* a);
			}
		}
	}
	set_bnd(b, x);
}

void advect(int b, float* d, float* d0, float* u, float* v, float dt)
{
	int i, j, i0, j0, i1, j1;
	float x, y, s0, t0, s1, t1, dt0;
	dt0 = dt*N;
	for ( i=1 ; i<=N ; i++ ) {
		for ( j=1 ; j<=N ; j++ ) {
			x = i-dt0*u[IX(i,j)]; y = j-dt0*v[IX(i,j)];
			if (x<0.5f) x=0.5f; if (x>N+0.5f) x=N+ 0.5f; i0=(int)x; i1=i0+1;
			if (y<0.5f) y=0.5f; if (y>N+0.5f) y=N+ 0.5f; j0=(int)y; j1=j0+1;
			s1 = x-i0; s0 = 1-s1; t1 = y-j0; t0 = 1-t1;
			d[IX(i,j)] = 
				s0 * (t0 * d0[IX(i0, j0)] +
				t1 * d0[IX(i0, j1)]) +
				s1 * (t0 * d0[IX(i1, j0)] + 
				t1 * d0[IX(i1, j1)]);
		}
	}
	set_bnd(b, d);
}

void dens_step(float* x, float* x0, float* u, float* v, float diff, float dt )
{
	add_source(x, x0, dt);
	SWAP(x0, x); diffuse(0, x, x0, diff, dt);
	SWAP(x0, x); advect(0, x, x0, u, v, dt);
}

void vel_step(float* u, float* v, float* u0, float* v0, float visc, float dt )
{
	add_source(u, u0, dt); add_source(v, v0, dt);
	SWAP(u0, u); diffuse(1, u, u0, visc, dt);
	SWAP(v0, v); diffuse(2, v, v0, visc, dt);
	project(u, v, u0, v0);
	SWAP(u0, u); SWAP (v0, v);
	advect(1, u, u0, u0, v0, dt); advect(2, v, v0, u0, v0, dt);
	project(u, v, u0, v0);
}

void project(float * u, float * v, float * p, float * div)
{
	int i, j;
	float h;
	h = 1.0f / N;
	for (i = 1; i <= N; i++)
	{
		for (j = 1; j <= N; j++)
		{
			div[IX(i,j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] +
									    v[IX(i, j + 1)] - v[IX(i, j - 1)]);
			p[IX(i,j)] = 0;
		}
	}

	// jacobi function
	set_bnd(0, div); set_bnd(0, p);
	for (int k = 0; k < 20; k++)
	{
		for (i = 1; i <= N ; i++)
		{
			for ( j=1 ; j<=N ; j++ )
			{
				p[IX(i,j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + 
							  p[IX(i + 1, j)] +	p[IX(i, j - 1)] + p[IX(i, j + 1)])
							  / 4;
			}
		}
		set_bnd(0, p);
	}

	// velocity gradient subtraction?
	for ( i=1 ; i<=N ; i++ )
	{
		for ( j=1 ; j<=N ; j++ )
		{
			u[IX(i,j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
			v[IX(i,j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
		}
	}
	set_bnd(1, u); set_bnd(2, v );
}

void set_bnd (int b, float* x)
{
	int i;
	for (i = 1; i <= N; i++)
	{
		x[IX(0, i)] = b==1?-x[IX(1, i)]:x[IX(1, i)];
		x[IX(N + 1, i)] = b==1 ?-x[IX(N,i)] : x[IX(N,i)];
		x[IX(i, 0)] = b==2 ?-x[IX(i,1)] : x[IX(i,1)];
		x[IX(i, N + 1)] = b==2 ?-x[IX(i,N)] : x[IX(i,N)];
	}
	x[IX(0 ,0 )] = 0.5f*(x[IX(1,0 )]+x[IX(0 ,1)]);
	x[IX(0 ,N+1)] = 0.5f*(x[IX(1,N+1)]+x[IX(0 ,N )]);
	x[IX(N+1,0 )] = 0.5f*(x[IX(N,0 )]+x[IX(N+1,1)]);
	x[IX(N+1,N+1)] = 0.5f*(x[IX(N,N+1)]+x[IX(N+1,N )]);
}

CPoint ptLast(0, 0);

void CfluidsView::OnTimer(UINT_PTR nIDEvent)
{
	float visc = 1.73e-5;
	float diff = 0.759f;
	float dt = 0.04f;

	dens_prev[IX(ptLast.x, ptLast.y)] = 0.0;
//	dens_prev[IX(10, 10)] = 100000.0;

	if (m_pt.x > 0 && m_pt.y > 0 && m_pt.x < N && m_pt.y < N)
	{
		if (m_leftButtonDown) dens_prev[IX(m_pt.x, m_pt.y)] = 100000.0;
		u_prev[IX(m_pt.x, m_pt.y)] = (m_pt.x - ptLast.x) * 50000.0;
		v_prev[IX(m_pt.x, m_pt.y)] = (m_pt.y - ptLast.y) * 50000.0;

		ptLast = m_pt;
	}

	vel_step(u, v, u_prev, v_prev, visc, dt);
	dens_step(dens, dens_prev, u, v, diff, dt);

	Invalidate();

	CView::OnTimer(nIDEvent);
}

void CfluidsView::OnMouseMove(UINT nFlags, CPoint point)
{
	m_pt = point;
	CView::OnMouseMove(nFlags, point);
}

BOOL CfluidsView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CfluidsView::OnDestroy()
{
	CView::OnDestroy();

	if (bmp) ::delete bmp;
}

void CfluidsView::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_leftButtonDown = true;

	CView::OnLButtonDown(nFlags, point);
}

void CfluidsView::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_leftButtonDown = false;

	CView::OnLButtonUp(nFlags, point);
}

void CfluidsView::OnRButtonDown(UINT nFlags, CPoint point)
{
	for (int i = 0; i < size; i++)
	{
		dens_prev[i] = 0.0f;
		dens[i] = 0.0f;
		u[i] = 0.0f;
		v[i] = 0.0f;
		u_prev[i] = 0.0f;
		v_prev[i] = 0.0f;
	}

	CView::OnRButtonDown(nFlags, point);
}
