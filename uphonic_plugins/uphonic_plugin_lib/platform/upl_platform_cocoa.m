#import <Cocoa/Cocoa.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <mach/mach_time.h>

#define UPL_WINDOW_API CLAP_WINDOW_API_COCOA
#define UPL_USE_HOST_TIMER 0
#define UPL_WINDOW_LOGICAL_SIZE 1

typedef struct
{
	NSView *view;
	CFRunLoopTimerRef timer;
	NSTrackingArea *tracking_area;
}
Upl_CocoaWindow;

typedef struct
{
	Class view_class;
	Ivar plugin_ivar;
	CGColorSpaceRef color_space;
}
Upl_CocoaState;

typedef struct
{
	Upl_Plugin *plugin;
	NSOpenPanel *panel;
}
Upl_CocoaFileDialog;

static Upl_CocoaState upl__cocoa_state;

static double upl__time(void)
{
	static mach_timebase_info_data_t timebase;
	if (timebase.denom == 0)
		mach_timebase_info(&timebase);

	return (double)mach_absolute_time() * (double)timebase.numer / (double)timebase.denom * 1e-9;
}

static Upl_Plugin *upl__cocoa_plugin(id view)
{
	return *(Upl_Plugin **)((uint8_t *)(__bridge void *)view + ivar_getOffset(upl__cocoa_state.plugin_ivar));
}

static void upl__cocoa_set_plugin(id view, Upl_Plugin *plugin)
{
	*(Upl_Plugin **)((uint8_t *)(__bridge void *)view + ivar_getOffset(upl__cocoa_state.plugin_ivar)) = plugin;
}

static void upl__cocoa_mouse(id view, NSEvent *event)
{
	Upl_Plugin *plugin = upl__cocoa_plugin(view);
	if (!plugin)
		return;

	const NSPoint point = [(NSView *)view convertPoint:[event locationInWindow] fromView:nil];
	const float scale = plugin->gui.scale;
	upl__input_set_shift(plugin, ([event modifierFlags] & NSEventModifierFlagShift) != 0);
	upl__input_mouse_move(plugin, (int32_t)(point.x * scale), (int32_t)(point.y * scale));
}

static void upl__cocoa_button(id view, NSEvent *event, Upl_MouseButton button, bool down)
{
	upl__cocoa_mouse(view, event);
	Upl_Plugin *plugin = upl__cocoa_plugin(view);
	if (plugin)
		upl__input_mouse_button(plugin, button, down);
}

static BOOL upl__cocoa_is_flipped(id self, SEL cmd)
{
	(void)self;
	(void)cmd;
	return YES;
}

static BOOL upl__cocoa_accepts_first_mouse(id self, SEL cmd, NSEvent *event)
{
	(void)self;
	(void)cmd;
	(void)event;
	return YES;
}

static void upl__cocoa_draw_rect(id self, SEL cmd, NSRect rect)
{
	(void)cmd;
	(void)rect;
	Upl_Plugin *plugin = upl__cocoa_plugin(self);
	if (!plugin || !plugin->gui.pixels)
		return;

	const Upl_Gui *gui = &plugin->gui;
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, gui->pixels, (size_t)gui->width * (size_t)gui->height * 4, NULL);
	CGImageRef image = CGImageCreate(
		(size_t)gui->width, (size_t)gui->height,
		8, 32, (size_t)gui->width * 4,
		upl__cocoa_state.color_space,
		kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaNoneSkipFirst,
		provider, NULL, false, kCGRenderingIntentDefault
	);

	const NSRect bounds = [(NSView *)self bounds];
	CGContextSaveGState(context);
	CGContextTranslateCTM(context, 0.0, bounds.size.height);
	CGContextScaleCTM(context, 1.0, -1.0);
	CGContextSetInterpolationQuality(context, kCGInterpolationNone);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, bounds.size.width, bounds.size.height), image);
	CGContextRestoreGState(context);

	CGImageRelease(image);
	CGDataProviderRelease(provider);
}

static void upl__cocoa_mouse_moved(id self, SEL cmd, NSEvent *event)
{
	(void)cmd;
	upl__cocoa_mouse(self, event);
}

static void upl__cocoa_mouse_exited(id self, SEL cmd, NSEvent *event)
{
	(void)cmd;
	(void)event;
	Upl_Plugin *plugin = upl__cocoa_plugin(self);
	if (plugin)
		upl__input_mouse_leave(plugin);
}

static void upl__cocoa_left_down(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_LEFT, true); }
static void upl__cocoa_left_up(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_LEFT, false); }
static void upl__cocoa_right_down(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_RIGHT, true); }
static void upl__cocoa_right_up(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_RIGHT, false); }
static void upl__cocoa_other_down(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_MIDDLE, true); }
static void upl__cocoa_other_up(id self, SEL cmd, NSEvent *event) { (void)cmd; upl__cocoa_button(self, event, UPL_MOUSE_MIDDLE, false); }

static void upl__cocoa_scroll(id self, SEL cmd, NSEvent *event)
{
	(void)cmd;
	upl__cocoa_mouse(self, event);
	Upl_Plugin *plugin = upl__cocoa_plugin(self);
	if (!plugin)
		return;

	const float delta = (float)[event scrollingDeltaY];
	upl__input_mouse_wheel(plugin, [event hasPreciseScrollingDeltas] ? delta * 0.1f : delta);
}

static void upl__cocoa_update_tracking_areas(id self, SEL cmd)
{
	Upl_Plugin *plugin = upl__cocoa_plugin(self);
	struct objc_super super_info = { self, [NSView class] };
	((void (*)(struct objc_super *, SEL))objc_msgSendSuper)(&super_info, cmd);

	if (!plugin || !plugin->gui.window)
		return;

	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	NSView *view = (NSView *)self;
	if (window->tracking_area)
	{
		[view removeTrackingArea:window->tracking_area];
		[window->tracking_area release];
	}

	window->tracking_area = [[NSTrackingArea alloc] initWithRect:[view bounds] options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect owner:view userInfo:nil];
	[view addTrackingArea:window->tracking_area];
}

static bool upl__cocoa_register_class(void)
{
	Upl_CocoaState *state = &upl__cocoa_state;
	if (state->view_class)
		return true;

	char name[64];
	snprintf(name, sizeof(name), "UplView_%p", (void *)state);
	Class view_class = objc_allocateClassPair([NSView class], name, 0);
	if (!view_class)
		return false;

	class_addIvar(view_class, "upl_plugin", sizeof(void *), (uint8_t)__builtin_ctz(sizeof(void *)), "^v");
	class_addMethod(view_class, @selector(isFlipped), (IMP)upl__cocoa_is_flipped, "c@:");
	class_addMethod(view_class, @selector(acceptsFirstMouse:), (IMP)upl__cocoa_accepts_first_mouse, "c@:@");
	class_addMethod(view_class, @selector(drawRect:), (IMP)upl__cocoa_draw_rect, "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
	class_addMethod(view_class, @selector(mouseMoved:), (IMP)upl__cocoa_mouse_moved, "v@:@");
	class_addMethod(view_class, @selector(mouseDragged:), (IMP)upl__cocoa_mouse_moved, "v@:@");
	class_addMethod(view_class, @selector(rightMouseDragged:), (IMP)upl__cocoa_mouse_moved, "v@:@");
	class_addMethod(view_class, @selector(otherMouseDragged:), (IMP)upl__cocoa_mouse_moved, "v@:@");
	class_addMethod(view_class, @selector(mouseEntered:), (IMP)upl__cocoa_mouse_moved, "v@:@");
	class_addMethod(view_class, @selector(mouseExited:), (IMP)upl__cocoa_mouse_exited, "v@:@");
	class_addMethod(view_class, @selector(mouseDown:), (IMP)upl__cocoa_left_down, "v@:@");
	class_addMethod(view_class, @selector(mouseUp:), (IMP)upl__cocoa_left_up, "v@:@");
	class_addMethod(view_class, @selector(rightMouseDown:), (IMP)upl__cocoa_right_down, "v@:@");
	class_addMethod(view_class, @selector(rightMouseUp:), (IMP)upl__cocoa_right_up, "v@:@");
	class_addMethod(view_class, @selector(otherMouseDown:), (IMP)upl__cocoa_other_down, "v@:@");
	class_addMethod(view_class, @selector(otherMouseUp:), (IMP)upl__cocoa_other_up, "v@:@");
	class_addMethod(view_class, @selector(scrollWheel:), (IMP)upl__cocoa_scroll, "v@:@");
	class_addMethod(view_class, @selector(updateTrackingAreas), (IMP)upl__cocoa_update_tracking_areas, "v@:");
	objc_registerClassPair(view_class);

	state->view_class = view_class;
	state->plugin_ivar = class_getInstanceVariable(view_class, "upl_plugin");
	state->color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
	return true;
}

static void upl__cocoa_timer(CFRunLoopTimerRef timer, void *info)
{
	(void)timer;
	upl__gui_tick((Upl_Plugin *)info);
}

static void upl__cocoa_apply_scale(Upl_Plugin *plugin)
{
	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	NSWindow *host_window = [window->view window];
	const float scale = host_window ? (float)[host_window backingScaleFactor] : 1.0f;
	if (scale == plugin->gui.scale && plugin->gui.pixels)
		return;

	plugin->gui.scale = scale;
	upl__gui_resize_buffer(plugin);
}

static bool upl__window_create(Upl_Plugin *plugin, const clap_window_t *parent)
{
	if (strcmp(parent->api, CLAP_WINDOW_API_COCOA) != 0 || !upl__cocoa_register_class())
		return false;

	NSView *parent_view = (NSView *)parent->cocoa;
	const NSRect frame = NSMakeRect(0.0, 0.0, (CGFloat)upl_descriptor.gui_width, (CGFloat)upl_descriptor.gui_height);
	NSView *view = [[upl__cocoa_state.view_class alloc] initWithFrame:frame];
	if (!view)
		return false;

	Upl_CocoaWindow *window = (Upl_CocoaWindow *)calloc(1, sizeof(Upl_CocoaWindow));
	window->view = view;
	plugin->gui.window = window;
	upl__cocoa_set_plugin(view, plugin);

	[parent_view addSubview:view];
	[view setHidden:!plugin->gui.visible];
	[view updateTrackingAreas];
	upl__cocoa_apply_scale(plugin);

	CFRunLoopTimerContext context = { 0, plugin, NULL, NULL, NULL };
	const CFTimeInterval interval = (CFTimeInterval)UPL_FRAME_INTERVAL_MS / 1000.0;
	window->timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + interval, interval, 0, 0, upl__cocoa_timer, &context);
	CFRunLoopAddTimer(CFRunLoopGetMain(), window->timer, kCFRunLoopCommonModes);
	return true;
}

static void upl__window_destroy(Upl_Plugin *plugin)
{
	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	if (!window)
		return;

	CFRunLoopTimerInvalidate(window->timer);
	CFRelease(window->timer);

	if (window->tracking_area)
	{
		[window->view removeTrackingArea:window->tracking_area];
		[window->tracking_area release];
	}

	upl__cocoa_set_plugin(window->view, NULL);
	[window->view removeFromSuperview];
	[window->view release];
	free(window);
	plugin->gui.window = NULL;
}

static void upl__window_resize(Upl_Plugin *plugin)
{
	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	[window->view setFrameSize:NSMakeSize((CGFloat)upl_descriptor.gui_width, (CGFloat)upl_descriptor.gui_height)];
}

static void upl__window_set_visible(Upl_Plugin *plugin, bool visible)
{
	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	[window->view setHidden:!visible];
}

static void upl__window_present(Upl_Plugin *plugin)
{
	Upl_CocoaWindow *window = (Upl_CocoaWindow *)plugin->gui.window;
	[window->view setNeedsDisplay:YES];
}

static void upl__window_poll(Upl_Plugin *plugin)
{
	upl__cocoa_apply_scale(plugin);
}

static void upl__cocoa_file_dialog_done(Upl_CocoaFileDialog *state, NSModalResponse response)
{
	Upl_Plugin *plugin = state->plugin;
	NSURL *url = response == NSModalResponseOK ? [[state->panel URLs] firstObject] : nil;
	const char *path = url ? [url fileSystemRepresentation] : NULL;

	if (plugin)
	{
		plugin->file_dialog.platform = NULL;
		upl__file_dialog_finish(plugin, path);
	}

	[state->panel release];
	free(state);
}

static bool upl__file_dialog_begin(Upl_Plugin *plugin, const Upl_FileDialog *dialog)
{
	Upl_CocoaFileDialog *state = (Upl_CocoaFileDialog *)calloc(1, sizeof(Upl_CocoaFileDialog));
	if (!state)
		return false;

	NSOpenPanel *panel = [[NSOpenPanel openPanel] retain];
	[panel setCanChooseFiles:YES];
	[panel setCanChooseDirectories:NO];
	[panel setAllowsMultipleSelection:NO];

	if (dialog->title)
	{
		NSString *title = [NSString stringWithUTF8String:dialog->title];
		[panel setTitle:title];
		[panel setMessage:title];
	}

	if (dialog->extensions && dialog->extensions[0])
	{
		NSMutableArray *types = [NSMutableArray array];
		for (const char *const *extension = dialog->extensions; *extension; extension++)
			[types addObject:[NSString stringWithUTF8String:*extension]];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		[panel setAllowedFileTypes:types];
#pragma clang diagnostic pop
	}

	state->plugin = plugin;
	state->panel = panel;
	plugin->file_dialog.platform = state;

	[panel beginWithCompletionHandler:^(NSModalResponse response) {
		upl__cocoa_file_dialog_done(state, response);
	}];

	return true;
}

static void upl__file_dialog_poll(Upl_Plugin *plugin)
{
	(void)plugin;
}

static void upl__file_dialog_cancel(Upl_Plugin *plugin)
{
	Upl_CocoaFileDialog *state = (Upl_CocoaFileDialog *)plugin->file_dialog.platform;
	plugin->file_dialog.platform = NULL;
	upl__file_dialog_finish(plugin, NULL);

	if (!state)
		return;

	state->plugin = NULL;
	[state->panel cancel:nil];
}
