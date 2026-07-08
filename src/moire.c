#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500

/* moire.c
 * Creates an animation of a rotation Moire pattern
 * To compile: gcc -o moire moire.c -lm -lX11
 * See also: https://en.wikipedia.org/wiki/Moire_pattern
*/

typedef struct {
	float x, y;
} MyPoint;

void rotate(MyPoint *p, int width, int height, float t)
{
	int x = p->x - width/2;
	int y = p->y - height/2;

	p->x = x*cosf(t) - y*sinf(t) + width/2;
	p->y = x*sinf(t) + y*cosf(t) + height/2;
}

void draw_lines(Display *dpy, Drawable d, GC gc, int width, int height, float t)
{
	int spaces = 50;
	int lines = spaces-1;
	XSegment segments[lines];

	int hs = width / spaces;
	int vs = height / spaces;

	for (int c = 1; c <= lines; c++)
	{
		MyPoint start = { .x = hs * c, .y = 0 };
		rotate(&start, width, height, t);

		segments[c-1].x1 = start.x;
		segments[c-1].y1 = start.y;

		MyPoint end = { .x = hs * c, .y = height };
		rotate(&end, width, height, t);

		segments[c-1].x2 = end.x;
		segments[c-1].y2 = end.y;
	}

	XDrawSegments(dpy, d, gc, segments, lines);
}

int main(void)
{
	/* ---------- Begin Set Up ---------- */
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy)
	{
		fprintf(stderr, "Error: can't connect to X server!\n");
		return EXIT_FAILURE;
	}

	int screen = XDefaultScreen(dpy);
	Window root = XRootWindow(dpy, screen);

	Window window = XCreateSimpleWindow(dpy, root,
		50, 50, SCREEN_WIDTH, SCREEN_HEIGHT,	// x, y, w, h
		2, XBlackPixel(dpy, screen),			// border size, color
		XWhitePixel(dpy, screen)				// background
	);

	XSelectInput(dpy, window, ExposureMask);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	XMapWindow(dpy, window);
	XStoreName(dpy, window, "I want Moire!");

	Pixmap pixmap = XCreatePixmap(dpy, window,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		XDefaultDepth(dpy, screen)
	);

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);
	/* ----------  End Set Up  ---------- */

	XSync(dpy, False);

	XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
	XFillRectangle(dpy, pixmap, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	XGCValues values;
	values.line_width = 4;
	values.line_style = LineSolid;
	XChangeGC(dpy, gc, GCLineWidth | GCLineStyle, &values);


	XEvent event;
	int running = 1;

	// theta/time: rotation angle (radians)
	float t = 0.0f;

	while (running)
	{
		while ( XPending(dpy) )
		{
			XNextEvent(dpy, &event);

			switch (event.type)
			{
				case Expose:
					if (event.xexpose.count == 0)
						XCopyArea(dpy, pixmap, window, gc, 0,0, SCREEN_WIDTH, SCREEN_HEIGHT, 0,0);
					break;
				case ClientMessage:
					if ((Atom)event.xclient.data.l[0] == delete_window)
						running = 0;
					break;
			}
		}

		XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
		XFillRectangle(dpy, pixmap, gc, 0,0, SCREEN_WIDTH, SCREEN_HEIGHT);

		XSetForeground(dpy, gc, 0x00FF0000L);
		draw_lines(dpy, pixmap, gc, SCREEN_WIDTH, SCREEN_HEIGHT, 0);

		XSetForeground(dpy, gc, 0x0000FF00L);
		draw_lines(dpy, pixmap, gc, SCREEN_WIDTH, SCREEN_HEIGHT, sinf(t) * M_PI/6);
		t = remainder(t + M_PI/600, 2*M_PI);

		XCopyArea(dpy, pixmap, window, gc, 0,0, SCREEN_WIDTH, SCREEN_HEIGHT, 0,0);

		struct timespec req = { .tv_sec = 0, .tv_nsec = 16666000 };
		nanosleep(&req, NULL);
	}

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XDestroyWindow(dpy, window);
	XCloseDisplay(dpy);
}
