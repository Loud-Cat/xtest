#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <stdio.h>
#include <stdlib.h>

/* gradient.c
 * This program uses the XRender extension to xlib for creating a gradient
 * To compile: gcc -o gradient gradient.c $(pkg-config --cflags --libs xrender x11)
 *
 * See also: https://refspecs.linuxbase.org/LSB_5.0.0/LSB-Desktop-generic/LSB-Desktop-generic/libxrender-ddefs.html
*/

int main() {
	// Normal xlib stuff...
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    Visual *visual = DefaultVisual(dpy, screen);

    Window window = XCreateSimpleWindow(dpy, root, 10, 10, 400, 300, 1, 0, 0);
    XSelectInput(dpy, window, ExposureMask | KeyPressMask);
    XMapWindow(dpy, window);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	// XRender Picture: fancy pixmaps??
    XRenderPictFormat *format = XRenderFindVisualFormat(dpy, visual);
    XRenderPictureAttributes attr;
    Picture dest_pic = XRenderCreatePicture(dpy, window, format, 0, &attr);

    XLinearGradient gradient;
    gradient.p1.x = XDoubleToFixed(0.0);
    gradient.p1.y = XDoubleToFixed(0.0);
    gradient.p2.x = XDoubleToFixed(400.0);
    gradient.p2.y = XDoubleToFixed(0.0);

    // color stops (0.0 is start, 1.0 is end)
    // Note: XRenderColor values range from 0 to 65535 (0xFFFF)
    XFixed stops[2] = { XDoubleToFixed(0.0), XDoubleToFixed(1.0) };
    XRenderColor colors[2] = {
        { .red = 0xFFFF, .green = 0x0000, .blue = 0x0000, .alpha = 0xFFFF },
        { .red = 0x0000, .green = 0x0000, .blue = 0xFFFF, .alpha = 0xFFFF }
    };

    // Create the gradient! Yay!
    Picture grad_pic = XRenderCreateLinearGradient(dpy, &gradient, stops, colors, 2);

    // Event Loop
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        if (ev.type == Expose) {
			// Note: XRender uses Compositing rather than XCopyData()
            // PictOpSrc means overwrite the window area
            XRenderComposite(dpy, PictOpSrc, grad_pic, None, dest_pic,
                             0, 0, 0, 0, 0, 0, 400, 300);
        }

		if (ev.type == ClientMessage)
			if ((Atom)ev.xclient.data.l[0] == delete_window)
				break;

        if (ev.type == KeyPress) break;
    }

    // Clean up
    XRenderFreePicture(dpy, grad_pic);
    XRenderFreePicture(dpy, dest_pic);
    XCloseDisplay(dpy);
    return 0;
}
