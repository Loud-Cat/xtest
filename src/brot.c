#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <complex.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Default Real bounds
#define MIN_R -1.95
#define MAX_R 0.55

// Default Imaginary Bounds
#define MIN_I -1.25
#define MAX_I 1.25

/* brot.c
 * An interactive Mandelbrot Set fractal visualizer
 * To compile: gcc -o brot brot.c -lm -lX11
 *
 * Controls:
 *   Use the mouse to move the mini viewport inside the window
 *   Press number keys 1-9 to set the size of the mini viewport
 *   Press 0 to remove mini viewport (for screenshots)
 *   Press SPACE to reset zoom and viewport
*/

int brot(double complex c, int attempts);
double map_value(double value, double s1, double e1, double s2, double e2);

unsigned long get_color(Display *dpy, int screen, int rgb) {
    Colormap cmap = DefaultColormap(dpy, screen);

    XColor color;
    color.red = rgb << 8; color.green = rgb << 8; color.blue = rgb << 8;
    color.flags = DoRed | DoGreen | DoBlue;

    return XAllocColor(dpy, cmap, &color) ? color.pixel : BlackPixel(dpy, screen);
}


int main()
{
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy)
	{
		fprintf(stderr, "Error: can't connect to X server\n");
		return EXIT_FAILURE;
	}

	Window root = XDefaultRootWindow(dpy);
	int screen = XDefaultScreen(dpy);

	int width = 500;
	int height = 500;

	int white = XWhitePixel(dpy, screen);
	int black = XBlackPixel(dpy, screen);

	Window window = XCreateSimpleWindow(
		dpy, root,
		50,50, width,height,
		2, black,
		white
	);

	XSelectInput(dpy, window,
		ExposureMask | PointerMotionMask | ButtonPressMask | KeyPressMask
	);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	XMapWindow(dpy, window);
	XStoreName(dpy, window, "Mandelbrot!");

	Pixmap pixmap = XCreatePixmap(dpy, window,
		width,height, XDefaultDepth(dpy, screen)
	);

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);

	XSetForeground(dpy, gc, white);
	XFillRectangle(dpy, pixmap, gc, 0,0, width,height);

	unsigned long grays[256];
	for (int i = 0; i < 256; i++)
		grays[i] = get_color(dpy, screen, i);

	XSync(dpy, False);

	int attempts = 100;

	// viewport preview
	int view_x = 0, view_y = 0;
	double scale = 0.5;
	double view_width = width * scale;
	double view_height = height * scale;

	// The coordinates of the viewport, in the Complex Plane.
	// These default values show a square "birds eye" view of the fractal.
	double min_r = MIN_R, max_r = MAX_R;
	double min_i = MIN_I, max_i = MAX_I;

	XEvent event;
	int running = 1;

	bool drawBrot = true;
	while (running)
	{
		XNextEvent(dpy, &event);

		switch (event.type)
		{
			case Expose:
				if (event.xexpose.count == 0)
					XCopyArea(dpy, pixmap, window, gc, 0,0, width,height, 0,0);
				break;
			case ClientMessage:
				if ((Atom)event.xclient.data.l[0] == delete_window)
					running = 0;
				break;
			case MotionNotify:
				XMotionEvent xmotion = event.xmotion;

				// To prevent viewports at a ratio other than the window,
				// limit the bounds of the viewport to maintain the size.

				int cap_x = width - view_width;
				int cap_y = height - view_height;

				XCopyArea(dpy, pixmap, window, gc,
					view_x-5, view_y-5, view_width+10, view_height+10, view_x-5, view_y-5
				);

				int new_x = xmotion.x - view_width/2;
				int new_y = xmotion.y - view_height/2;

				if (new_x < 0) view_x = 0;
				else if (new_x > cap_x) view_x = cap_x;
				else view_x = new_x;

				if (new_y < 0) view_y = 0;
				else if (new_y > cap_y) view_y = cap_y;
				else view_y = new_y;

				break;
			case ButtonPress:
				if (scale == 0) break;

				double min_r2 = map_value(view_x, 0, width, min_r, max_r);
				max_r = map_value(view_x + view_width, 0,width, min_r, max_r);
				min_r = min_r2;

				double min_i2 = map_value(view_y, 0, height, min_i, max_i);
				max_i = map_value(view_y + view_height, 0,height, min_i, max_i);
				min_i = min_i2;

				drawBrot = true;
				break;
			case KeyPress:
				char buffer[32];
				KeySym keysym = NoSymbol;
				XComposeStatus comp;

				int len = XLookupString(
					&event.xkey, buffer, sizeof(buffer), &keysym, &comp
				);

				if (len > 0)
					buffer[len] = '\0';

				if (keysym == XK_space)
				{
					view_x = 0;
					view_y = 0;
					scale = 0.5;
					view_width = width*scale;
					view_height = height*scale;

					min_r = MIN_R;
					max_r = MAX_R;
					min_i = MIN_I;
					max_i = MAX_I;

					drawBrot = true;
					break;
				}

				char key = buffer[0];
				if (key >= '0' && key <= '9')
				{
					XCopyArea(dpy, pixmap, window, gc,
						view_x-5, view_y-5, view_width+10, view_height+10, view_x-5, view_y-5
					);

					double mx = view_x + view_width/2;
					double my = view_y + view_height/2;

					key -= '0';
					scale = key / 10.0;
					view_width = width * scale;
					view_height = height * scale;

					view_x = mx - view_width/2;
					view_y = my - view_height/2;
				}
				break;
		}

		if (drawBrot)
		{
			for (int y = 0; y < height; y++)
			{
				double cy = map_value(y, 0, height, min_i, max_i);

				for (int x = 0; x < width; x++)
				{
					double cx = map_value(x, 0, width, min_r, max_r);

					double complex c = CMPLX(cx, cy);
					int gray = brot(c, attempts) % 256;

					XSetForeground(dpy, gc, grays[gray]);
					XDrawPoint(dpy, pixmap, gc, x, y);
				}
			}

			XCopyArea(dpy, pixmap, window, gc, 0,0, width,height, 0,0);
			drawBrot = false;
		}

		XSetForeground(dpy, gc, black);

		if (scale > 0)
			XDrawRectangle(dpy, window, gc,
				view_x, view_y, view_width, view_height
			);
	}

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XDestroyWindow(dpy, window);
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

double map_value(double value, double s1, double e1, double s2, double e2)
{
	double t = (value - s1) / (e1 - s1);
	return s2 + (e2 - s2)*t;
}

int brot(double complex c, int attempts)
{
	double complex z = c;

	for (int i = 0; i < attempts; i++)
	{
		z = (z * z) + c;
		double dist = creal( cabs(z) );

		if (dist > 2)
			return 255 * ((attempts - i) / (double)attempts);
	}

	return 0;
}
