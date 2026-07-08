#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* bounce.c
 * Animated Bouncing Box
 * To compile: gcc -o bounce bounce.c -lX11
*/

typedef struct {
	int w, h, x, y, vx, vy;
} MyBox;

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

	int width = 500;
	int height = 500;

	Window window = XCreateSimpleWindow(dpy, root,
		50, 50, width, height,	// x, y, w, h
		2, XBlackPixel(dpy, screen), // border size, color
		XWhitePixel(dpy, screen)     // background
	);

	XSelectInput(dpy, window,
		ExposureMask | StructureNotifyMask
	);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	XMapWindow(dpy, window);
	XStoreName(dpy, window, "Bounce!");

	Pixmap pixmap = XCreatePixmap(dpy, window,
		width, height,
		XDefaultDepth(dpy, screen)
	);

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);
	MyBox box = { .w = 50, .h = 50, .x = 0, .y = 0, .vx = 3, .vy = 2 };

	/* ----------  End Set Up  ---------- */
	XSync(dpy, False);
//	printf("Setup successful!\n");

	XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
	XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);

	XEvent event;
	int running = 1;
	while (running)
	{
		while ( XPending(dpy) )
		{
			XNextEvent(dpy, &event);

			switch (event.type)
			{
				case Expose:
					if (event.xexpose.count == 0)
						XCopyArea(dpy, pixmap, window, gc, 0,0, width,height, 0,0);
					break;
				case ConfigureNotify:
					XConfigureEvent xconfig = event.xconfigure;

					if (xconfig.width != width || xconfig.height != height)
						{
						width = xconfig.width;
						height = xconfig.height;

						XFreePixmap(dpy, pixmap);
						XFreeGC(dpy, gc);

						pixmap = XCreatePixmap(dpy, window,
							width, height,
							DefaultDepth(dpy, screen)
						);

						gc = XCreateGC(dpy, pixmap, 0, NULL);

						XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
						XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);
					}
					break;
				case ClientMessage:
					if ((Atom)event.xclient.data.l[0] == delete_window)
						running = 0;
					break;
			}
		}

		int old_x = box.x;
		int old_y = box.y;

		box.x += box.vx;
		if (box.x > width - box.w)
		{
			box.x = width - box.w;
			box.vx *= -1;
		}
		if (box.x < 0)
		{
			box.x = 0;
			box.vx *= -1;
		}

		box.y += box.vy;
		if (box.y >= height - box.h)
		{
			box.y = height - box.h;
			box.vy *= -1;
		}
		if (box.y <= 0)
		{
			box.y = 0;
			box.vy *= -1;
		}

		XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
		XFillRectangle(dpy, pixmap, gc, old_x, old_y, box.w, box.h);

		XSetForeground(dpy, gc, XBlackPixel(dpy, screen));
		XFillRectangle(dpy, pixmap, gc, box.x, box.y, box.w, box.h);

		XCopyArea(dpy, pixmap, window, gc, old_x, old_y, box.w, box.h, old_x, old_y);
		XCopyArea(dpy, pixmap, window, gc, box.x, box.y, box.w, box.h, box.x, box.y);

		struct timespec req = { .tv_sec = 0, .tv_nsec = 16666000 };
		nanosleep(&req, NULL);
	}

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XCloseDisplay(dpy);
}
