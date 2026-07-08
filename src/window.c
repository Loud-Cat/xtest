#include <X11/Xlib.h>	// For X11
#include <stdlib.h>		// For exit macros
#include <stdio.h>		// For fprintf()

/* window.c
 * My first X11 Window! Creates a Pixmap and stores a rectangle on it.
 * To compile: gcc -o window window.c -lX11
*/

void print_pfv(XPixmapFormatValues *pfv);

XPixmapFormatValues max_pfv(XPixmapFormatValues *arr, int count);
int handle_error(Display *display, XErrorEvent *event);
unsigned long get_rgb_pixel(Display *display, int screen, int r, int g, int b);

int gError = 0;

int main(void)
{
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy)
	{
		fprintf(stderr, "Error: can't open display\n");
		return EXIT_FAILURE;
	}

	printf("Connected to display: %s\n", XDisplayString(dpy));
	XSetErrorHandler(handle_error);

	Screen *screen = XDefaultScreenOfDisplay(dpy);
	if (!screen)
	{
		fprintf(stderr, "Error: can't find screen\n");
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	int screenNumber = XScreenNumberOfScreen(screen);
	printf("Connected to screen %d: width = %dpx (%dmm), height = %dpx (%dmm)\n",
		screenNumber,
		XWidthOfScreen(screen), XWidthMMOfScreen(screen),
		XHeightOfScreen(screen), XHeightMMOfScreen(screen)
	);

	Window root = XRootWindow(dpy, screenNumber);
	int black = XBlackPixel(dpy, screenNumber);
	int white = XWhitePixel(dpy, screenNumber);

	int count;
	XPixmapFormatValues* pfvs = XListPixmapFormats(dpy, &count);

	if (!pfvs)
	{
		fprintf(stderr, "Error: no pixmap formats available\n");
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	XPixmapFormatValues maxPFV = max_pfv(pfvs, count);
	printf("Max Pixmap Format Values: ");
	print_pfv(&maxPFV);
	XFree(pfvs);

	int byteOrder = XImageByteOrder(dpy);
	printf("Byte order: %s\n",
		(byteOrder == LSBFirst) ? "LSBFirst" :
		(byteOrder == MSBFirst) ? "MSBFirst" : "Other"
	);

	Window window = XCreateSimpleWindow(
		dpy, root,	// display, parent
		50, 50,		// x, y
		500, 500,	// width, height
		4,			// border width
		black,		// border
		get_rgb_pixel(dpy, screenNumber, 255, 128, 64) // background
	);

	XSelectInput(dpy, window, ExposureMask);
	XMapWindow(dpy, window);

	if (gError != 0) {
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	XStoreName(dpy, window, "My First Window");
	printf("Created a window!\n");

	/* "Clients should include the atom WM_DELETE_WINDOW in the WM_PROTOCOLS property
	 * on each window. They will receive a ClientMessage event as described above
	 * whose data[0] field is WM_DELETE_WINDOW" */

	Atom deleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &deleteWindow, 1);

	if (gError != 0)
	{
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	Pixmap pixmap = XCreatePixmap(dpy, window, 250, 250,
		DefaultDepth(dpy, screenNumber)
	);

	if (gError != 0)
	{
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	printf("Created a pixmap!\n");

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);

    if (gError != 0)
    {
		XFreePixmap(dpy, pixmap);
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }

    printf("Created a Graphics Context!!\n");

	XSetForeground(dpy, gc,
		get_rgb_pixel(dpy, screenNumber, 50, 50, 200)
	);

	XSetBackground(dpy, gc,
		get_rgb_pixel(dpy, screenNumber, 50, 50, 200)
	);

	XFillRectangle(dpy, pixmap, gc, 0, 0, 250, 250);

	if (gError != 0)
	{
		XFreeGC(dpy, gc);
		XFreePixmap(dpy, pixmap);
		XDestroyWindow(dpy, window);
		XCloseDisplay(dpy);

		return EXIT_FAILURE;
	}

	int running = 1;
	XEvent event;

	while (running)
	{
		XNextEvent(dpy, &event);

		if (event.type == ClientMessage)
		{
			if ( (Atom)event.xclient.data.l[0] == deleteWindow )
			{
				printf("Deleting Window!\n");
				running = 0;
			}
		} else if (event.type == Expose)
		{
			printf("I'm exposed, how indecent!\n");

			if (event.xexpose.count == 0)
			{
				XCopyArea(dpy, pixmap, window, gc,
					0, 0, 250, 250,
					0, 0
				);
			}
		}
	}

	printf("Done!\n");
	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XDestroyWindow(dpy, window);
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}

void print_pfv(XPixmapFormatValues *pfv)
{
	printf("XPixmapFormatValues{depth=%d, bits_per_pixel=%d, scanline_pad=%d}\n",
		pfv->depth, pfv->bits_per_pixel, pfv->scanline_pad
	);
}

XPixmapFormatValues max_pfv(XPixmapFormatValues *arr, int count)
{
	XPixmapFormatValues out = arr[0];

	for (int i = 1; i < count; i++)
	{
		if ((arr + i)->depth > out.depth)
			out = arr[i];
	}

	return out;
}

int handle_error(Display *display, XErrorEvent *event)
{
	char error_text[256];
	XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
	fprintf(stderr, "%s\n", error_text);

	gError = event->error_code;
	return 0;
}

unsigned long get_rgb_pixel(Display *display, int screen, int r, int g, int b) {
	Colormap colormap = DefaultColormap(display, screen);

	XColor color;
	color.red   = r << 8;
	color.green = g << 8;
	color.blue  = b << 8;
	color.flags = DoRed | DoGreen | DoBlue;

	if (XAllocColor(display, colormap, &color)) {
		return color.pixel;
	}

	return BlackPixel(display, screen);
}
