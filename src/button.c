#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* button.c
 * Xlib Project 4: Simple GUI Button
*/

typedef struct {
	unsigned long idle_color, hover_color, pressed_color;
	int x, y, w, h;
	char *text;
} MyButton;

typedef enum {
	STATE_IDLE,
	STATE_HOVER,
	STATE_PRESSED
} ButtonState;

void center_button(MyButton *button, int width, int height)
{
	button->x = width/2 - button->w/2;
	button->y = height/2 - button->h/2;
}

Bool is_within(int x, int y, MyButton *button)
{
	if (x > button->x && x < button->x + button->w)
		if (y > button->y && y < button->y + button->h)
			return True;

	return False;
}

void draw_button(Display *dpy, Drawable draw, GC gc, MyButton *button, ButtonState state)
{
	switch (state) {
		case STATE_IDLE:
			XSetForeground(dpy, gc, button->idle_color);
			break;
		case STATE_HOVER:
			XSetForeground(dpy, gc, button->hover_color);
			break;
		case STATE_PRESSED:
			XSetForeground(dpy, gc, button->pressed_color);
			break;
	}

	XFillRectangle(dpy, draw, gc, button->x, button->y, button->w, button->h);

	XSetForeground(dpy, gc, XWhitePixel(dpy, 0));
	XDrawString(dpy, draw, gc, button->x + 25, button->y + 25, button->text, strlen(button->text));
}

// Helper to quickly generate a color pixel
unsigned long get_color(Display *dpy, int screen, int r, int g, int b) {
    Colormap cmap = DefaultColormap(dpy, screen);

    XColor color;
    color.red = r << 8; color.green = g << 8; color.blue = b << 8;
    color.flags = DoRed | DoGreen | DoBlue;

    return XAllocColor(dpy, cmap, &color) ?
		color.pixel : BlackPixel(dpy, screen);
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

	int width = 500;
	int height = 500;

	Window window = XCreateSimpleWindow(dpy, root,
		50, 50, width, height,			// x, y, w, h
		2, XBlackPixel(dpy, screen),	// border size, color
		XWhitePixel(dpy, screen)		// background
	);

	XSelectInput(dpy, window,
		ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | LeaveWindowMask
	);

	Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, window, &delete_window, 1);

	XMapWindow(dpy, window);
	XStoreName(dpy, window, "Button!");

	Pixmap pixmap = XCreatePixmap(dpy, window,
		width, height,
		XDefaultDepth(dpy, screen)
	);

	GC gc = XCreateGC(dpy, pixmap, 0, NULL);

	MyButton button;
	button.idle_color = get_color(dpy, screen, 200,200,200);
	button.hover_color = get_color(dpy, screen, 75,75,75);
	button.pressed_color = get_color(dpy, screen, 50,50,200);
	button.text = "Click Me!";

	int font_count = 0;
	char **fonts = XListFonts(dpy, "*mono*", 10, &font_count);

	if (fonts == NULL)
	{
		fprintf(stderr, "No fonts??\n");
		XFreePixmap(dpy, pixmap);
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	Font font = XLoadFont(dpy, fonts[0]);
	XFontStruct *xfs = XQueryFont(dpy, font);
	XFreeFontNames(fonts);

	int text_width = 0;
	if (xfs != NULL) {
		text_width = XTextWidth(xfs, button.text, strlen(button.text));

		button.w = text_width + 50;
		button.h = xfs->ascent + xfs->descent + 50;
		center_button(&button, width, height);

		XSetFont(dpy, gc, font);
		XFreeFontInfo(NULL, xfs, 1);
	}
	else
	{
		fprintf(stderr, "Can't use font...\n");
		XFreePixmap(dpy, pixmap);
		XCloseDisplay(dpy);
		return EXIT_FAILURE;
	}

	/* ----------  End Set Up  ---------- */

	XSync(dpy, False);
//	printf("Setup successful!\n");

	XSetForeground(dpy, gc, XWhitePixel(dpy, screen));
	XFillRectangle(dpy, pixmap, gc, 0, 0, width, height);

	XEvent event;
	int running = 1;

	ButtonState state = STATE_IDLE;
	Bool pWithin = False;

	draw_button(dpy, pixmap, gc, &button, state);
	XCopyArea(dpy, pixmap, window, gc, button.x, button.y, button.w, button.h, 0,0);

	while (running)
	{
		XNextEvent(dpy, &event);

		switch (event.type)
		{
			case ClientMessage:
				if ((Atom)event.xclient.data.l[0] == delete_window)
					running = 0;
				break;
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
			case MotionNotify:
				XMotionEvent xmotion = event.xmotion;
				Bool within = is_within(xmotion.x, xmotion.y, &button);

				if (within != pWithin)
				{
					state = within ? STATE_HOVER : STATE_IDLE;
					draw_button(dpy, pixmap, gc, &button, state);
					XCopyArea(dpy, pixmap, window, gc, button.x, button.y, button.w, button.h, button.x, button.y);
				}

				pWithin = within;
				break;
			case ButtonPress:
			{
				XButtonEvent xbutton = event.xbutton;

				if ( is_within(xbutton.x, xbutton.y, &button) )
				{
					state = STATE_PRESSED;
					draw_button(dpy, pixmap, gc, &button, state);
					XCopyArea(dpy, pixmap, window, gc, button.x, button.y, button.w, button.h, button.x, button.y);
				}
				break;
			}
			case ButtonRelease:
			{
				XButtonEvent xbutton = event.xbutton;

				state = is_within(xbutton.x, xbutton.y, &button) ?
					STATE_HOVER : STATE_IDLE;

				if (state == STATE_HOVER)
					printf("Click!\n");

				draw_button(dpy, pixmap, gc, &button, state);
				XCopyArea(dpy, pixmap, window, gc, button.x, button.y, button.w, button.h, button.x, button.y);
				break;
			}
			case LeaveNotify:
				break;
		}
	}

	XUnloadFont(dpy, font);
	XFreeGC(dpy, gc);
	XFreePixmap(dpy, pixmap);
	XDestroyWindow(dpy, window);
	XCloseDisplay(dpy);
}
