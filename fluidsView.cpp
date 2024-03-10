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

#define IX(x,y) ((x)+(N+2)*(y))
//#define RED(r) (r << 16)
//#define GREEN(g) (g << 8)
//#define BLUE(b) (b << 0)
static const int size = (402 * 402);
static int N = 400;
static float dt = 0.2f;

__declspec(align(16)) static float u[size];
__declspec(align(16)) static float v[size];
__declspec(align(16)) static float dens[size];
__declspec(align(16)) static float u_prev[size];
__declspec(align(16)) static float v_prev[size];
__declspec(align(16)) static float dens_prev[size];
static int jacobi = 10;

void add_source(float* x, float* s, float dt);
void diffuse(int b, float* x, float* x0);
void advect(int b, float* d, float* d0, float* u, float* v, float dt);
void vel_step(float* u, float* v, float* u0, float* v0, float dt);
void project(float* u, float* v, float* p, float* div);
void set_bnd(int b, float* x);
void buoyancy(float* f);

LARGE_INTEGER freq, start, finish;

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

// CfluidsView drawing
Bitmap* bmp = NULL;

void CfluidsView::OnDraw(CDC* pDC)
{
	CfluidsDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc) return;

	bool showVelocity = true;
	BitmapData bd;
	Rect rc(0, 0, N+2, N+2);
	Graphics graphics(pDC->GetSafeHdc());

	if (bmp)
	{
		bmp->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &bd);

		unsigned long* cp = (unsigned long *)bd.Scan0;
		for (int i = 0; i < size; i++)
		{
			unsigned char d = (unsigned char)min(255, dens[i] * 255.0);
			cp[i] = (d << 8) | (0xff << 24);
		}

		bmp->UnlockBits(&bd);

		graphics.DrawImage(bmp, 0, 0);
	}

	if (bmp && showVelocity)
	{
		bmp->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &bd);

		unsigned long* cp = (unsigned long *)bd.Scan0;
		for (int i = 0; i < size; i++)
		{
			unsigned char r = max(0, min(255, (unsigned char)(u[i] * 255.0)));
			unsigned char b = max(0, min(255, (unsigned char)(v[i] * 255.0)));
			cp[i] = (r << 16) | (0xff << 24) | (b << 0);
		}

		bmp->UnlockBits(&bd);

		graphics.DrawImage(bmp, N, 0);
	}

	dt = (float)((double)(finish.QuadPart - start.QuadPart) / (double)freq.QuadPart);

	CString str;
	str.Format(L"%.6f", dt);
	AfxGetMainWnd()->SetWindowText(str);

	PostMessage(WM_TIMER, 1, 0);
}

void CfluidsView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	Graphics graphics(GetSafeHwnd());
	bmp = ::new Bitmap(N+2, N+2, &graphics);

	QueryPerformanceFrequency(&freq);

	PostMessage(WM_TIMER, 1, 0);
}

void add_source(float* x, float* s, float dt)
{
	int i;

	for (i = 0; i < size; i++) 
	{
		*x += dt * *s;
		x++;s++;
	}
}

void diffuse(int b, float* x, float* x0)
{
	for (int i = 0; i < size; i++)
	{
		x[i] = x0[i];
	}

	set_bnd(b, x);
}

void advect(int b, float* d, float* d0, float* u, float* v, float dt)
{
	register int i, j, i0, j0, i1, j1;
	register float x, y, s0, t0, s1, t1, dt0;
	
	dt0 = dt * N;

	for (i = 1; i <= N; i++)
	{
		for (j = 1; j <= N; j++)
		{
			x = i - dt0 * u[IX(i, j)];
			y = j - dt0 * v[IX(i, j)];

			// clamp x,y to >= 0.5 <= N + 0.5
			if (x < 0.5f) x = 0.5f; if (x > N + 0.5f) x = N + 0.5f; 
			if (y < 0.5f) y = 0.5f; if (y > N + 0.5f) y = N + 0.5f;
			i0 = (int)x; i1 = i0 + 1;
			j0 = (int)y; j1 = j0 + 1;

			s1 = x - i0; s0 = 1 - s1; t1 = y - j0; t0 = 1 - t1;
			d[IX(i,j)] = 
				s0 * (t0 * d0[IX(i0, j0)] +
				t1 * d0[IX(i0, j1)]) +
				s1 * (t0 * d0[IX(i1, j0)] + 
				t1 * d0[IX(i1, j1)]);
		}
	}

	set_bnd(b, d);
}

void vel_step(float* u, float* v, float* u0, float* v0, float dt)
{
	// acceleration, takes a proportion of velocity relative to time and adds it to the main velocity array
	add_source(u, u0, dt); add_source(v, v0, dt);

	// add buoyancy - this will affect vertical velocity which then gets looped in again
	//buoyancy(v0);
	//add_source(v, v0, dt);

	// diffuse the velocity
	diffuse(1, u0, u); diffuse(2, v0, v);
	project(u0, v0, u, v);
	advect(1, u, u0, u0, v0, dt); advect(2, v, v0, u0, v0, dt);
	project(u, v, u0, v0);
}

void project(float* u, float* v, float* p, float* div)
{
	register int x, y;
	register float h = 1.0f / N;
	register float hr = 1.0f / h;

	for (x = 1; x <= N; x++)
	{
		for (y = 1; y <= N; y++)
		{
			div[IX(x,y)] = -0.5f * h * (u[IX(x + 1, y)] - u[IX(x - 1, y)] +
									    v[IX(x, y + 1)] - v[IX(x, y - 1)]);
			p[IX(x,y)] = 0;
		}
	}

	// jacobi function
	set_bnd(0, div); set_bnd(0, p);
	for (int k = 0; k < jacobi; k++)
	{
		for (x = 1; x <= N ; x++)
		{
			for (y = 1; y <= N; y++)
			{
				p[IX(x,y)] = (div[IX(x, y)] + p[IX(x - 1, y)] + 
							  p[IX(x + 1, y)] +	p[IX(x, y - 1)] + p[IX(x, y + 1)])
							  * 0.25f;
			}
		}
		set_bnd(0, p);
	}

	// velocity gradient subtraction?
	for (x = 1; x <= N; x++)
	{
		for (y = 1; y <= N; y++)
		{
			u[IX(x, y)] -= 0.5f * (p[IX(x + 1, y)] - p[IX(x - 1, y)]) * hr;
			v[IX(x, y)] -= 0.5f * (p[IX(x, y + 1)] - p[IX(x, y - 1)]) * hr;
		}
	}

	set_bnd(1, u); set_bnd(2, v);
}

// buoyancy is calculated based on the ambient temperature and the relative viscosity of each "fluid"
//void buoyancy(float* f)
//{
//	float Tamb = 0;
//	//    float a = 0.000625f;
//	float a = 0.0000625f;
//	//    float b = 0.025f; 
//	float b = 0.005f;
//
//	// sum all temperatures - temperature = density??
//	for (int x = 1; x <= N; x++)
//	{
//		for (int y = 1; y <= N; y++)
//		{
//			Tamb += dens[IX(x, y)];
//		}
//	}
//
//	// get average temperature
//	Tamb /= (N * N);
//
//	// for each cell compute buoyancy force
//	for (int x = 1; x <= N; x++)
//	{
//		for (int y = 1; y <= N; y++)
//		{
//			f[IX(x, y)] = a * dens[IX(x, y)] - b * (dens[IX(x, y)] - Tamb);
//		}
//	}
//}

void buoyancy(float* f)
{
    float Tamb = 0;
//    float a = 0.000625f;
	float a = 0.0000625f;
//    float b = 0.025f; 
	float b = 0.005f;

    // sum all temperatures - temperature = density??
    for (int x = 0; x < size; x++)
    {
        Tamb += dens[x];
    }

    // get average temperature
    Tamb /= size;

    // for each cell compute buoyancy force
    for (int x = 1; x <= N; x++)
    {
        for (int y = 1; y <= N; y++)
        {
            f[IX(x, y)] = a * dens[IX(x, y)] - b * (dens[IX(x, y)] - Tamb);
        }
    }
}

// set the boundary values - depending on b this will either absorb values in the boundary or reflect them
// away.. 0 = density, 1 = velocity(x), 2 = velocity(y)
void set_bnd(int b, float* x)
{
	int i;
	if (b == 0)
	{
		for (i = 1; i <= N; i++)
		{
			x[IX(0, i)] = x[IX(1, i)];
			x[IX(N + 1, i)] = x[IX(N, i)];
			x[IX(i, 0)] = x[IX(i, 1)];
			x[IX(i, N + 1)] = x[IX(i, N)];
		}
	}
	else if (b == 1)
	{
		for (i = 1; i <= N; i++)
		{
			x[IX(0, i)] = -x[IX(1, i)];
			x[IX(N + 1, i)] = -x[IX(N, i)];
			x[IX(i, 0)] = x[IX(i, 1)];
			x[IX(i, N + 1)] = x[IX(i, N)];
		}
	}
	else if (b == 2)
	{
		for (i = 1; i <= N; i++)
		{
			x[IX(0, i)] = x[IX(1, i)];
			x[IX(N + 1, i)] = x[IX(N, i)];
			x[IX(i, 0)] = -x[IX(i, 1)];
			x[IX(i, N + 1)] = -x[IX(i, N)];
		}
	}

	// the corner values are an average of their adjacent neighbours
	x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
	x[IX(0,N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
	x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
	x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

CPoint ptLast(0, 0);

void CfluidsView::OnTimer(UINT_PTR nIDEvent)
{
	QueryPerformanceCounter(&start);

	// reset previous ready for what's to come
	for (int i = 0; i < size; i++) dens_prev[i] = 0.0f;

	if (m_pt.x > 0 && m_pt.y > 0 && m_pt.x < N && m_pt.y < N)
	{
		if (m_leftButtonDown) 
		{
			dens_prev[IX(m_pt.x, m_pt.y)] = 1000.0f;
		}
		u_prev[IX(m_pt.x, m_pt.y)] = (m_pt.x - ptLast.x) * 100.0f;
		v_prev[IX(m_pt.x, m_pt.y)] = (m_pt.y - ptLast.y) * 100.0f;

		ptLast = m_pt;
	}

	vel_step(u, v, u_prev, v_prev, dt);

	// add a proportion of the new density according to time, this will get diffused so the
	// additional density rolls off with time
	add_source(dens, dens_prev, dt);
	diffuse(0, dens_prev, dens);
	advect(0, dens, dens_prev, u, v, dt);

	QueryPerformanceCounter(&finish);

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

	if (bmp)
		::delete bmp;
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
