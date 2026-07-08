#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500

/* sketch.c
 * Simulates an "Etch-a-Sketch" display. Click and drag to draw lines.
 * To compile: gcc -o sketch sketch.c -lX11
*/

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

	XSelectInput(dpy, window,
		ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
	);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	XMapWindow(dpy, window);
	XStoreName(dpy, window, "Sketch!");

	Pixmap pixmap = XCreatePixmap(dpy, window,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		XDefaultDepth(dpy, screen)
	);

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);
	/* ----------  End Set Up  ---------- */

	XSync(dpy, False);
	printf("Setup successful!\n");

	XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
	XFillRectangle(dpy, pixmap, gc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	XSetForeground(dpy, gc, XBlackPixel(dpy, screen));

	XEvent event;
	int running = 1;

	int px = -1, py = -1;

	while (running) {
		XNextEvent(dpy, &event);

		switch (event.type) {
			case Expose:
//				printf("#exposed!\n");

				if (event.xexpose.count == 0)
				{
					XCopyArea(
						dpy, pixmap, window, gc,
						0,0, SCREEN_WIDTH,SCREEN_HEIGHT, 0,0
					);
				}
				break;
			case MotionNotify:
//				printf("Motion!\n");
				XMotionEvent xmotion = event.xmotion;

				if ((xmotion.state & Button1Mask) && px != -1)
				{
//					printf("Drawing line...\n");
					XDrawLine(dpy, pixmap, gc,
						px, py, xmotion.x, xmotion.y);

					XCopyArea(
						dpy, pixmap, window, gc,
						0,0, SCREEN_WIDTH,SCREEN_HEIGHT, 0,0
					);
				}

				px = xmotion.x;
				py = xmotion.y;
				break;
			case ButtonRelease:
				px = -1;
				py = -1;
				break;
			case ClientMessage:
				if ((Atom)event.xclient.data.l[0] == delete_window)
				{
					printf("Deleting window!\n");
					running = 0;
				}
				break;
		}
	}

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XCloseDisplay(dpy);
	printf("Done!\n");
}
