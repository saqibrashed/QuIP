#include "quip_config.h"

/* plot subroutines using pixrect library */
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "quip_prot.h"
#include "viewer.h"
#include "xsupp.h"
#include "data_obj.h"


#define X0	0
#define Y0	0

// BUG static globals are not thread-safe!

static Viewer *plot_vp=NO_VIEWER;
static int _currx=0, _curry=0;		/* current posn, in screen (pixel) units */
/* static float _screenx, _screeny;
static float _x0,_y0,delx,dely; */

static void remember_space(Viewer *vp,float _x0,float _y0,float delx,float dely)
{
	if( vp == NO_VIEWER ){
		return;
	}

	SET_VW_XMIN(vp, _x0);
	SET_VW_YMIN(vp, _y0);
	SET_VW_XDEL(vp, delx);
	SET_VW_YDEL(vp, dely);
}

void xp_space(int x1,int y1,int x2,int y2)
{
	float _x0,_y0,delx,dely;

	_x0=x1;
	_y0=y1;
	delx=x2-x1;
	dely=y2-y1;
	remember_space(plot_vp,_x0,_y0,delx,dely);
}

/** like libplot space, but takes float args */
void xp_fspace(float x1,float y1,float x2,float y2)
{
	float _x0,_y0,delx,dely;

	_x0=x1;
	_y0=y1;
	delx=x2-x1;
	dely=y2-y1;
	remember_space(plot_vp,_x0,_y0,delx,dely);
}

/* This routine takes a coordinate pair from screen coords to plotting units */

static void unscalefxy(Viewer *vp,float *px,float *py)
{
	float x,y;

	x = (*px) - X0;
	// This flips the y coord!?  good for X11, but???
#ifdef BUILD_FOR_MACOS
	y = (*py) - Y0;
#else // ! BUILD_FOR_MACOS
	y = Y0 + VW_HEIGHT(vp) - ((*py)+1) ;
#endif // ! BUILD_FOR_MACOS

	x /= (VW_WIDTH(vp)-1);	y /= (VW_HEIGHT(vp)-1);
	x *= VW_XDEL(vp);	y *= VW_YDEL(vp);
	*px = x + VW_XMIN(vp);	*py = y + VW_YMIN(vp);
}

void tell_plot_space(SINGLE_QSP_ARG_DECL)
{
	sprintf(msg_str,"Viewer %s:",VW_NAME(plot_vp));
	prt_msg(msg_str);
	sprintf(msg_str,"\twidth = %d\theight = %d",VW_WIDTH(plot_vp),VW_HEIGHT(plot_vp));
	prt_msg(msg_str);
	sprintf(msg_str,"\txmin = %g\txdel = %g",VW_XMIN(plot_vp),VW_XDEL(plot_vp));
	prt_msg(msg_str);
	sprintf(msg_str,"\tymin = %g\tydel = %g",VW_YMIN(plot_vp),VW_YDEL(plot_vp));
	prt_msg(msg_str);
}

void xp_circle(float radius)
{
	float cx,cy;

	if( plot_vp == NO_VIEWER ) return;

	/* current posn is remembered in screen units */
	cx=_currx; cy=_curry;

	/* convert from screen coords to plotting units */
	unscalefxy(plot_vp,&cx,&cy);

	xp_farc(cx,cy,cx+radius,cy,cx+radius,cy);
}

void xp_fill_polygon(int num_points,
			float* x_vals, float* y_vals)
{
	int *xp,*yp;
	int i;
	float fx,fy;

	if( plot_vp == NO_VIEWER ) return;

	/* BUG should scale the points */
	xp = (int *) getbuf( num_points * sizeof(int) );
	yp = (int *) getbuf( num_points * sizeof(int) );

	for(i=0;i<num_points;i++){
		fx=x_vals[i]; fy=y_vals[i];
		scale_fxy(plot_vp,&fx,&fy);
		xp[i] = (int) nearbyintf(fx);
		yp[i] = (int) nearbyintf(fy);
	}

	_xp_fill_polygon(plot_vp, num_points, xp, yp);

	givbuf(xp);
	givbuf(yp);
}

void xp_ffill_arc(float cx,float cy,float x1,float y1,float x2,float y2)
{
	float _cx,_cy,_x1,_y1,_x2,_y2;
	int xl,yu,w,h,a1,a2;
	int start,delta;

	if( plot_vp == NO_VIEWER ) return;

	_cx=cx; _cy=cy; scale_fxy(plot_vp,&_cx,&_cy);
	_x1=x1; _y1=y1; scale_fxy(plot_vp,&_x1,&_y1);
	_x2=x2; _y2=y2; scale_fxy(plot_vp,&_x2,&_y2);


	/* compute the radius */

	w = (int) sqrt( (_cx-_x1)*(_cx-_x1) + (_cy-_y1)*(_cy-_y1) );
	xl= (int)(_cx - w);
	yu= (int)(_cy - w);
	w *= 2;
	h=w;

/*
printf("xp_farc:  ctr %f %f   p1 %f %f   p2 %f %f\n",
_cx,_cy,_x1,_y1,_x2,_y2);
*/

	a1=(int)(64.0 * 45.0 * atan2(_cy-_y1,_x1-_cx) / atan(1.0));
	a2=(int)(64.0 * 45.0 * atan2(_cy-_y2,_x2-_cx) / atan(1.0));

	while( a1 < 0 ) a1 += 64*360;
	while( a2 < 0 ) a2 += 64*360;

	/* we want to draw CCW from first pt... */
	delta = a2 - a1;
	start = a1;
	while( delta <= 0 ) delta += 64*360;

	_xp_fill_arc(plot_vp,xl,yu,w,h,start,delta);  
}

void xp_arc(int cx,int cy,int x1,int y1,int x2,int y2)
{
	xp_farc((float)cx,(float)cy,
		(float)x1,(float)y1,
		(float)x2,(float)y2 );
}

void xp_farc(float cx,float cy,float x1,float y1,float x2,float y2)
{
	float _cx,_cy,_x1,_y1,_x2,_y2;
	int xl,yu,w,h,a1,a2;
	int start,delta;

	if( plot_vp == NO_VIEWER ) return;

	_cx=cx; _cy=cy; scale_fxy(plot_vp,&_cx,&_cy);
	_x1=x1; _y1=y1; scale_fxy(plot_vp,&_x1,&_y1);
	_x2=x2; _y2=y2; scale_fxy(plot_vp,&_x2,&_y2);


	/* compute the radius */

	w = (int) sqrt( (_cx-_x1)*(_cx-_x1) + (_cy-_y1)*(_cy-_y1) );
	xl= (int)(_cx - w);
	yu= (int)(_cy - w);
	w *= 2;
	h=w;

/*
printf("xp_farc:  ctr %f %f   p1 %f %f   p2 %f %f\n",
_cx,_cy,_x1,_y1,_x2,_y2);
*/

	a1=(int)(64.0 * 45.0 * atan2(_cy-_y1,_x1-_cx) / atan(1.0));
	a2=(int)(64.0 * 45.0 * atan2(_cy-_y2,_x2-_cx) / atan(1.0));

	while( a1 < 0 ) a1 += 64*360;
	while( a2 < 0 ) a2 += 64*360;

	/* we want to draw CCW from first pt... */
	delta = a2 - a1;
	start = a1;
	while( delta <= 0 ) delta += 64*360;

	_xp_arc(plot_vp,xl,yu,w,h,start,delta);
}

void scale_fxy(Viewer *vp,float *px,float *py)
{
	float x,y;

	x = (*px);		y = (*py);
	x -= VW_XMIN(vp);	y -= VW_YMIN(vp);
	x /= VW_XDEL(vp);	y /= VW_YDEL(vp);
	x *= (VW_WIDTH(vp)-1);	y *= (VW_HEIGHT(vp)-1);
	*px = X0 + x;
#ifdef BUILD_FOR_MACOS
	*py = Y0 + y;
#else // ! BUILD_FOR_MACOS
	// This flips the y coord - good for X11, but???
	*py = Y0 + VW_HEIGHT(vp) - (y+1);
#endif // ! BUILD_FOR_MACOS
}

void scalexy(Viewer *vp,int *px,int *py)
{
	float x,y;

	x = (*px);		y = (*py);
	x -= VW_XMIN(vp);	y -= VW_YMIN(vp);
	x /= VW_XDEL(vp);	y /= VW_YDEL(vp);
	x *= (VW_WIDTH(vp)-1);	y *= (VW_HEIGHT(vp)-1);
	*px = (int)( X0+ x);		*py = (int)( Y0 + VW_HEIGHT(vp) - (y+1));
}

void xp_fmove(float x,float y)
{
	float fx,fy;

	if( plot_vp == NO_VIEWER ) return;

	fx=x; fy=y;
	scale_fxy(plot_vp,&fx,&fy);

	_currx=(int) floor(fx+0.5);
	_curry=(int) floor(fy+0.5);
	_xp_move(plot_vp,_currx,_curry);
}

void xp_move(int x,int y)
{
	if( plot_vp == NO_VIEWER ) return;

	scalexy(plot_vp,&x,&y);
	_currx=x;
	_curry=y;
	_xp_move(plot_vp,_currx,_curry);
}

void xp_fcont(float x,float y)
{
	int ix,iy;
	float fx,fy;

	if( plot_vp == NO_VIEWER ){
		return;
	}

	fx=x; fy=y;
	scale_fxy(plot_vp,&fx,&fy);
	ix=(int)floor(fx+0.5);
	iy=(int)floor(fy+0.5);

	_xp_line(plot_vp,_currx,_curry,ix,iy);

	_currx=ix;
	_curry=iy;
}

void xp_point(int x,int y)
{
	xp_move(x,y);
	xp_cont(x,y);
}

void xp_fpoint(float x,float y)
{
	xp_fmove(x,y);
	xp_fcont(x,y);
}

void xp_cont(int x,int y)
{
	if( plot_vp == NO_VIEWER ) return;

	scalexy(plot_vp,&x,&y);
	_xp_line(plot_vp,_currx,_curry,x,y);
	/* why don't we use cont here??? something to do with scaling??? */
	/* _xp_cont(plot_vp,x,y); */

	_currx=x;
	_curry=y;
}

void xp_line(int x1,int y1,int x2,int y2)
{
	xp_move(x1,y1);
	xp_cont(x2,y2);
}

void xp_fline(float x1,float y1,float x2,float y2)
{
	xp_fmove(x1,y1);
	xp_fcont(x2,y2);
}

#define DEFAULT_FONT_WIDTH	10

/* a hack so that labels produced by graph(1) don't overlap the frame */

/* BUG this should be determined by the font... */
#define PLOT_TEXT_OFFSET	0
#define PIXELS_PER_CHAR		8

void xp_text(const char *s)
{
	float delta;

	if( plot_vp != NO_VIEWER ){
		_xp_text(plot_vp,_currx,_curry+PLOT_TEXT_OFFSET,s);
	}

	/* Probably graph(1) expects that this will move the pointer... */
	/* this is a hack... */
	/* BUG need to get the font size (string length) */
	delta = strlen(s)*PIXELS_PER_CHAR;	/* screen units */

	_currx += delta;
}

void xp_setup(QSP_ARG_DECL  Viewer *vp)
{
	if( ! IS_PLOTTER(vp) ){
		sprintf(ERROR_STRING,"xp_setup:  viewer %s is not a plotter!?",
			VW_NAME(vp));
		WARN(ERROR_STRING);
	}

	plot_vp = vp;

	if( plot_vp != NO_VIEWER ){
		/* this is just a default... */
		if( VW_XDEL(vp) != 0.0 ){	/* params already set */
			xp_fspace(VW_XMIN(vp),VW_YMIN(vp),
				VW_XMIN(vp)+VW_XDEL(vp),VW_YMIN(vp)+VW_YDEL(vp));
		} else {			/* use default */
			xp_space(0,0,VW_WIDTH(vp)-1,VW_HEIGHT(vp)-1);
		}
	}
}

void xp_bgselect(u_long color)
{
	if( plot_vp == NO_VIEWER ) {
		return;
	}

	_xp_bgselect(plot_vp,color);
}

void xp_select(u_long color)
{
	if( plot_vp == NO_VIEWER ) {
		return;
	}

	_xp_select(plot_vp,color);
}

void xp_erase(void)
{
	if( plot_vp != NO_VIEWER ){
		_xp_erase(plot_vp);
	}
}

#ifdef BUILD_FOR_IOS
void xp_update(void)
{
	if( plot_vp != NO_VIEWER ){
		_xp_update(plot_vp);
	}
}
#endif /* BUILD_FOR_IOS */


