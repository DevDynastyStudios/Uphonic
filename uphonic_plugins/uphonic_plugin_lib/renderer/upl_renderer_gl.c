#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#define UPL_GL_MAX_QUADS		4096
#define UPL_GL_IMAGE_SLOTS		16

#define UPL_GL_KIND_FILL		0.0f
#define UPL_GL_KIND_STROKE		1.0f
#define UPL_GL_KIND_SHADOW		2.0f
#define UPL_GL_KIND_LINE		3.0f
#define UPL_GL_KIND_ARC			4.0f
#define UPL_GL_KIND_GLYPH		5.0f
#define UPL_GL_KIND_IMAGE		6.0f

#define UPL_GL_FUNCTIONS(X) \
	X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
	X(PFNGLATTACHSHADERPROC, glAttachShader) \
	X(PFNGLBINDBUFFERPROC, glBindBuffer) \
	X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
	X(PFNGLBUFFERDATAPROC, glBufferData) \
	X(PFNGLCOMPILESHADERPROC, glCompileShader) \
	X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
	X(PFNGLCREATESHADERPROC, glCreateShader) \
	X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
	X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
	X(PFNGLDELETESHADERPROC, glDeleteShader) \
	X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
	X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
	X(PFNGLGENBUFFERSPROC, glGenBuffers) \
	X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
	X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
	X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
	X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
	X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
	X(PFNGLSHADERSOURCEPROC, glShaderSource) \
	X(PFNGLUNIFORM1IPROC, glUniform1i) \
	X(PFNGLUNIFORM2FPROC, glUniform2f) \
	X(PFNGLUSEPROGRAMPROC, glUseProgram) \
	X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)

#define UPL_GL_DECLARE(type, name) type name;
#define UPL_GL_LOAD(type, name) api->name = (type)glXGetProcAddressARB((const GLubyte *)#name); loaded = loaded && api->name;

typedef struct
{
	UPL_GL_FUNCTIONS(UPL_GL_DECLARE)
}
Upl_GlApi;

typedef struct
{
	float x;
	float y;
	float u;
	float v;
	float shape[4];
	float radii[4];
	float params[4];
	uint8_t color[4];
}
Upl_GlVertex;

typedef struct
{
	uint64_t serial;
	uint64_t last_used;
	GLuint texture;
}
Upl_GlImageSlot;

typedef struct
{
	Display *display;
	Window window;
	GLXContext context;
	GLuint program;
	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLuint atlas;
	GLint viewport_uniform;
	uint32_t atlas_generation;
	uint64_t frame;
	Upl_GlImageSlot images[UPL_GL_IMAGE_SLOTS];
}
Upl_GlSurface;

typedef struct
{
	Display *display;
	GLXDrawable draw;
	GLXDrawable read;
	GLXContext context;
}
Upl_GlBinding;

typedef struct
{
	Upl_GlApi api;
	bool api_loaded;
	bool context_error;
	Upl_GlSurface *current;
	Upl_GlBinding previous;
	const Upl_Atlas *atlas;
	GLuint bound_image;
	int32_t height;
	uint32_t quad_count;
	Upl_GlVertex vertices[UPL_GL_MAX_QUADS * 4];
}
Upl_GlState;

static Upl_GlState upl__gl_state;

static const char *const upl__gl_vertex_source =
	"#version 330 core\n"
	"layout(location = 0) in vec2 a_position;\n"
	"layout(location = 1) in vec2 a_uv;\n"
	"layout(location = 2) in vec4 a_shape;\n"
	"layout(location = 3) in vec4 a_radii;\n"
	"layout(location = 4) in vec4 a_params;\n"
	"layout(location = 5) in vec4 a_color;\n"
	"uniform vec2 u_viewport;\n"
	"out vec2 v_position;\n"
	"out vec2 v_uv;\n"
	"flat out vec4 v_shape;\n"
	"flat out vec4 v_radii;\n"
	"flat out vec4 v_params;\n"
	"flat out vec4 v_color;\n"
	"void main()\n"
	"{\n"
	"	v_position = a_position;\n"
	"	v_uv = a_uv;\n"
	"	v_shape = a_shape;\n"
	"	v_radii = a_radii;\n"
	"	v_params = a_params;\n"
	"	v_color = a_color;\n"
	"	gl_Position = vec4(a_position / u_viewport * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);\n"
	"}\n";

static const char *const upl__gl_fragment_source =
	"#version 330 core\n"
	"in vec2 v_position;\n"
	"in vec2 v_uv;\n"
	"flat in vec4 v_shape;\n"
	"flat in vec4 v_radii;\n"
	"flat in vec4 v_params;\n"
	"flat in vec4 v_color;\n"
	"uniform sampler2D u_atlas;\n"
	"uniform sampler2D u_image;\n"
	"out vec4 o_color;\n"
	"float sd_box(vec2 p)\n"
	"{\n"
	"	p -= v_shape.xy;\n"
	"	float r = p.x < 0.0 ? (p.y < 0.0 ? v_radii.x : v_radii.w) : (p.y < 0.0 ? v_radii.y : v_radii.z);\n"
	"	vec2 q = abs(p) - v_shape.zw + r;\n"
	"	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;\n"
	"}\n"
	"float sd_segment(vec2 p, vec2 a, vec2 b)\n"
	"{\n"
	"	vec2 pa = p - a;\n"
	"	vec2 ba = b - a;\n"
	"	float l = dot(ba, ba);\n"
	"	float h = l > 0.0 ? clamp(dot(pa, ba) / l, 0.0, 1.0) : 0.0;\n"
	"	return length(pa - ba * h);\n"
	"}\n"
	"float sd_arc(vec2 p)\n"
	"{\n"
	"	vec2 d = p - v_shape.xy;\n"
	"	float radius = v_shape.z;\n"
	"	float start = v_params.z;\n"
	"	float sweep = v_params.w;\n"
	"	float angle = mod(atan(d.y, d.x) - start, 6.28318530718);\n"
	"	if (angle <= sweep)\n"
	"		return abs(length(d) - radius);\n"
	"	vec2 s = v_shape.xy + vec2(cos(start), sin(start)) * radius;\n"
	"	vec2 e = v_shape.xy + vec2(cos(start + sweep), sin(start + sweep)) * radius;\n"
	"	return min(length(p - s), length(p - e));\n"
	"}\n"
	"void main()\n"
	"{\n"
	"	int kind = int(v_params.x + 0.5);\n"
	"	vec4 color = v_color;\n"
	"	float coverage = 1.0;\n"
	"	if (kind == 0)\n"
	"		coverage = clamp(0.5 - sd_box(v_position), 0.0, 1.0);\n"
	"	else if (kind == 1)\n"
	"		coverage = clamp(0.5 - (abs(sd_box(v_position) + v_params.y * 0.5) - v_params.y * 0.5), 0.0, 1.0);\n"
	"	else if (kind == 2)\n"
	"	{\n"
	"		float t = clamp((sd_box(v_position) + v_params.y) / (2.0 * v_params.y), 0.0, 1.0);\n"
	"		coverage = 1.0 - t * t * (3.0 - 2.0 * t);\n"
	"	}\n"
	"	else if (kind == 3)\n"
	"		coverage = clamp(v_params.y + 0.5 - sd_segment(v_position, v_shape.xy, v_shape.zw), 0.0, 1.0);\n"
	"	else if (kind == 4)\n"
	"		coverage = clamp(v_params.y + 0.5 - sd_arc(v_position), 0.0, 1.0);\n"
	"	else if (kind == 5)\n"
	"		coverage = texture(u_atlas, v_uv).r;\n"
	"	else\n"
	"	{\n"
	"		color *= texture(u_image, v_uv);\n"
	"		coverage = clamp(0.5 - sd_box(v_position), 0.0, 1.0);\n"
	"	}\n"
	"	float alpha = color.a * coverage;\n"
	"	o_color = vec4(color.rgb * alpha, alpha);\n"
	"}\n";

#pragma region Context

static int upl__gl_error_handler(Display *display, XErrorEvent *event)
{
	(void)display;
	(void)event;
	upl__gl_state.context_error = true;
	return 0;
}

static bool upl__gl_has_extension(Display *display, int screen, const char *name)
{
	const char *extensions = glXQueryExtensionsString(display, screen);
	const size_t length = strlen(name);
	for (const char *cursor = extensions; cursor && (cursor = strstr(cursor, name)); cursor += length)
	{
		const bool starts = cursor == extensions || cursor[-1] == ' ';
		const bool ends = cursor[length] == ' ' || cursor[length] == '\0';
		if (starts && ends)
			return true;
	}

	return false;
}

static bool upl__gl_choose_config(Display *display, int screen, GLXFBConfig *config, XVisualInfo **visual)
{
	static const int attributes[] = {
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		None
	};

	int count = 0;
	GLXFBConfig *configs = glXChooseFBConfig(display, screen, attributes, &count);
	if (!configs)
		return false;

	if (count > 0)
	{
		*config = configs[0];
		*visual = glXGetVisualFromFBConfig(display, *config);
	}

	XFree(configs);
	return count > 0 && *visual;
}

static void upl__gl_bind(Display *display, GLXDrawable drawable, GLXContext context)
{
	Upl_GlState *state = &upl__gl_state;
	state->previous = (Upl_GlBinding) {
		.display = glXGetCurrentDisplay(),
		.draw = glXGetCurrentDrawable(),
		.read = glXGetCurrentReadDrawable(),
		.context = glXGetCurrentContext()
	};

	glXMakeCurrent(display, drawable, context);
}

static void upl__gl_unbind(Display *display)
{
	const Upl_GlBinding *previous = &upl__gl_state.previous;
	if (previous->context)
		glXMakeContextCurrent(previous->display, previous->draw, previous->read, previous->context);
	else
		glXMakeCurrent(display, None, NULL);
}

static bool upl__gl_load_api(void)
{
	Upl_GlState *state = &upl__gl_state;
	if (state->api_loaded)
		return true;

	Upl_GlApi *api = &state->api;
	bool loaded = true;
	UPL_GL_FUNCTIONS(UPL_GL_LOAD)
	state->api_loaded = loaded;
	return loaded;
}

static GLuint upl__gl_compile(GLenum type, const char *source)
{
	Upl_GlApi *api = &upl__gl_state.api;
	const GLuint shader = api->glCreateShader(type);
	api->glShaderSource(shader, 1, &source, NULL);
	api->glCompileShader(shader);

	GLint ok = GL_FALSE;
	api->glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (ok)
		return shader;

	api->glDeleteShader(shader);
	return 0;
}

static bool upl__gl_create_resources(Upl_GlSurface *surface)
{
	Upl_GlApi *api = &upl__gl_state.api;
	const GLuint vertex = upl__gl_compile(GL_VERTEX_SHADER, upl__gl_vertex_source);
	const GLuint fragment = upl__gl_compile(GL_FRAGMENT_SHADER, upl__gl_fragment_source);
	if (!vertex || !fragment)
	{
		if (vertex)
			api->glDeleteShader(vertex);
		if (fragment)
			api->glDeleteShader(fragment);
		return false;
	}

	surface->program = api->glCreateProgram();
	api->glAttachShader(surface->program, vertex);
	api->glAttachShader(surface->program, fragment);
	api->glLinkProgram(surface->program);
	api->glDeleteShader(vertex);
	api->glDeleteShader(fragment);

	GLint ok = GL_FALSE;
	api->glGetProgramiv(surface->program, GL_LINK_STATUS, &ok);
	if (!ok)
		return false;

	api->glUseProgram(surface->program);
	api->glUniform1i(api->glGetUniformLocation(surface->program, "u_atlas"), 0);
	api->glUniform1i(api->glGetUniformLocation(surface->program, "u_image"), 1);
	surface->viewport_uniform = api->glGetUniformLocation(surface->program, "u_viewport");

	api->glGenVertexArrays(1, &surface->vao);
	api->glBindVertexArray(surface->vao);
	api->glGenBuffers(1, &surface->vbo);
	api->glBindBuffer(GL_ARRAY_BUFFER, surface->vbo);
	api->glBufferData(GL_ARRAY_BUFFER, sizeof(upl__gl_state.vertices), NULL, GL_STREAM_DRAW);

	const GLsizei stride = (GLsizei)sizeof(Upl_GlVertex);
	api->glEnableVertexAttribArray(0);
	api->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offsetof(Upl_GlVertex, x));
	api->glEnableVertexAttribArray(1);
	api->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offsetof(Upl_GlVertex, u));
	api->glEnableVertexAttribArray(2);
	api->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (const void *)offsetof(Upl_GlVertex, shape));
	api->glEnableVertexAttribArray(3);
	api->glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (const void *)offsetof(Upl_GlVertex, radii));
	api->glEnableVertexAttribArray(4);
	api->glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (const void *)offsetof(Upl_GlVertex, params));
	api->glEnableVertexAttribArray(5);
	api->glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (const void *)offsetof(Upl_GlVertex, color));

	uint32_t *indices = (uint32_t *)malloc(UPL_GL_MAX_QUADS * 6 * sizeof(uint32_t));
	if (!indices)
		return false;

	for (uint32_t i = 0; i < UPL_GL_MAX_QUADS; i++)
	{
		const uint32_t base = i * 4;
		uint32_t *quad = indices + i * 6;
		quad[0] = base;
		quad[1] = base + 1;
		quad[2] = base + 2;
		quad[3] = base;
		quad[4] = base + 2;
		quad[5] = base + 3;
	}

	api->glGenBuffers(1, &surface->ibo);
	api->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surface->ibo);
	api->glBufferData(GL_ELEMENT_ARRAY_BUFFER, UPL_GL_MAX_QUADS * 6 * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	free(indices);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &surface->atlas);
	api->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surface->atlas);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, UPL_ATLAS_SIZE, UPL_ATLAS_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	surface->atlas_generation = UINT32_MAX;
	return true;
}

static void upl__gl_release_resources(Upl_GlSurface *surface)
{
	Upl_GlApi *api = &upl__gl_state.api;
	for (uint32_t i = 0; i < UPL_GL_IMAGE_SLOTS; i++)
	{
		if (surface->images[i].texture)
			glDeleteTextures(1, &surface->images[i].texture);
	}

	if (surface->atlas)
		glDeleteTextures(1, &surface->atlas);
	if (surface->ibo)
		api->glDeleteBuffers(1, &surface->ibo);
	if (surface->vbo)
		api->glDeleteBuffers(1, &surface->vbo);
	if (surface->vao)
		api->glDeleteVertexArrays(1, &surface->vao);
	if (surface->program)
		api->glDeleteProgram(surface->program);
}

static void upl__gl_set_swap_interval(Upl_GlSurface *surface, int screen)
{
	if (upl__gl_has_extension(surface->display, screen, "GLX_EXT_swap_control"))
	{
		PFNGLXSWAPINTERVALEXTPROC swap_interval = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
		if (swap_interval)
			swap_interval(surface->display, surface->window, 0);
	}
	else if (upl__gl_has_extension(surface->display, screen, "GLX_MESA_swap_control"))
	{
		PFNGLXSWAPINTERVALMESAPROC swap_interval = (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalMESA");
		if (swap_interval)
			swap_interval(0);
	}
}

static Upl_GlSurface *upl__gl_surface_create(Display *display, int screen, Window window, GLXFBConfig config)
{
	Upl_GlState *state = &upl__gl_state;
	if (!upl__gl_has_extension(display, screen, "GLX_ARB_create_context_profile"))
		return NULL;

	PFNGLXCREATECONTEXTATTRIBSARBPROC create_context = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
	if (!create_context)
		return NULL;

	static const int attributes[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None
	};

	XSync(display, False);
	int (*previous_handler)(Display *, XErrorEvent *) = XSetErrorHandler(upl__gl_error_handler);
	state->context_error = false;
	GLXContext context = create_context(display, config, NULL, True, attributes);
	XSync(display, False);
	XSetErrorHandler(previous_handler);

	if (!context || state->context_error)
	{
		if (context)
			glXDestroyContext(display, context);
		return NULL;
	}

	Upl_GlSurface *surface = (Upl_GlSurface *)calloc(1, sizeof(Upl_GlSurface));
	surface->display = display;
	surface->window = window;
	surface->context = context;

	upl__gl_bind(display, window, context);
	const bool ready = upl__gl_load_api() && upl__gl_create_resources(surface);
	if (ready)
		upl__gl_set_swap_interval(surface, screen);
	else
		upl__gl_release_resources(surface);
	upl__gl_unbind(display);

	if (ready)
		return surface;

	glXDestroyContext(display, context);
	free(surface);
	return NULL;
}

static void upl__gl_surface_destroy(Upl_GlSurface *surface)
{
	if (!surface)
		return;

	upl__gl_bind(surface->display, surface->window, surface->context);
	upl__gl_release_resources(surface);
	upl__gl_unbind(surface->display);
	glXDestroyContext(surface->display, surface->context);
	free(surface);
}

#pragma endregion

#pragma region Batch

static void upl__gl_flush(void)
{
	Upl_GlState *state = &upl__gl_state;
	Upl_GlSurface *surface = state->current;
	if (!surface || state->quad_count == 0)
		return;

	Upl_GlApi *api = &state->api;
	if (state->atlas && surface->atlas_generation != state->atlas->generation)
	{
		api->glActiveTexture(GL_TEXTURE0);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, UPL_ATLAS_SIZE, UPL_ATLAS_SIZE, GL_RED, GL_UNSIGNED_BYTE, state->atlas->pixels);
		surface->atlas_generation = state->atlas->generation;
	}

	api->glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(state->quad_count * 4 * sizeof(Upl_GlVertex)), state->vertices, GL_STREAM_DRAW);
	glDrawElements(GL_TRIANGLES, (GLsizei)(state->quad_count * 6), GL_UNSIGNED_INT, NULL);
	state->quad_count = 0;
}

static void upl__gl_push(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, const float shape[4], const float radii[4], const float params[4], Leaf_Color color)
{
	Upl_GlState *state = &upl__gl_state;
	if (!state->current)
		return;

	if (state->quad_count >= UPL_GL_MAX_QUADS)
		upl__gl_flush();

	Upl_GlVertex *quad = &state->vertices[state->quad_count++ * 4];
	const float xs[4] = { x0, x1, x1, x0 };
	const float ys[4] = { y0, y0, y1, y1 };
	const float us[4] = { u0, u1, u1, u0 };
	const float vs[4] = { v0, v0, v1, v1 };

	for (uint32_t i = 0; i < 4; i++)
	{
		Upl_GlVertex *vertex = &quad[i];
		vertex->x = xs[i];
		vertex->y = ys[i];
		vertex->u = us[i];
		vertex->v = vs[i];
		memcpy(vertex->shape, shape, sizeof(vertex->shape));
		memcpy(vertex->radii, radii, sizeof(vertex->radii));
		memcpy(vertex->params, params, sizeof(vertex->params));
		vertex->color[0] = color.r;
		vertex->color[1] = color.g;
		vertex->color[2] = color.b;
		vertex->color[3] = color.a;
	}
}

static void upl__gl_push_box(float x, float y, float width, float height, float margin, const float radii[4], const float params[4], Leaf_Color color)
{
	const float shape[4] = { x + width * 0.5f, y + height * 0.5f, width * 0.5f, height * 0.5f };
	upl__gl_push(x - margin, y - margin, x + width + margin, y + height + margin, 0.0f, 0.0f, 1.0f, 1.0f, shape, radii, params, color);
}

static bool upl__gl_begin(Upl_GlSurface *surface, int32_t width, int32_t height)
{
	Upl_GlState *state = &upl__gl_state;
	upl__gl_bind(surface->display, surface->window, surface->context);
	if (glXGetCurrentContext() != surface->context)
	{
		upl__gl_unbind(surface->display);
		return false;
	}

	Upl_GlApi *api = &state->api;
	state->current = surface;
	state->height = height;
	state->bound_image = 0;
	state->quad_count = 0;
	surface->frame++;

	glViewport(0, 0, width, height);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	api->glUseProgram(surface->program);
	api->glBindVertexArray(surface->vao);
	api->glBindBuffer(GL_ARRAY_BUFFER, surface->vbo);
	api->glUniform2f(surface->viewport_uniform, (float)width, (float)height);
	api->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, surface->atlas);
	return true;
}

static void upl__gl_end(Upl_GlSurface *surface)
{
	Upl_GlState *state = &upl__gl_state;
	upl__gl_flush();
	glXSwapBuffers(surface->display, surface->window);
	state->current = NULL;
	upl__gl_unbind(surface->display);
}

static GLuint upl__gl_image_texture(Upl_GlSurface *surface, Upl_Image *image)
{
	Upl_GlImageSlot *slot = &surface->images[0];
	for (uint32_t i = 0; i < UPL_GL_IMAGE_SLOTS; i++)
	{
		Upl_GlImageSlot *candidate = &surface->images[i];
		if (candidate->serial == image->serial)
		{
			candidate->last_used = surface->frame;
			return candidate->texture;
		}

		if (candidate->last_used < slot->last_used)
			slot = candidate;
	}

	upl__gl_flush();
	if (slot->texture)
		glDeleteTextures(1, &slot->texture);

	Upl_GlApi *api = &upl__gl_state.api;
	glGenTextures(1, &slot->texture);
	api->glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, slot->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	upl__gl_state.bound_image = slot->texture;

	slot->serial = image->serial;
	slot->last_used = surface->frame;
	return slot->texture;
}

#pragma endregion

#pragma region Backend

static void upl__gfx_clear(Leaf_Color color)
{
	if (!upl__gl_state.current)
		return;

	upl__gl_flush();
	glClearColor((float)color.r / 255.0f, (float)color.g / 255.0f, (float)color.b / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

static void upl__gfx_fill_rect(float x, float y, float width, float height, const float radii[4], Leaf_Color color)
{
	const float params[4] = { UPL_GL_KIND_FILL, 0.0f, 0.0f, 0.0f };
	upl__gl_push_box(x, y, width, height, 1.0f, radii, params, color);
}

static void upl__gfx_stroke_rect(float x, float y, float width, float height, const float radii[4], float line_width, Leaf_Color color)
{
	const float params[4] = { UPL_GL_KIND_STROKE, line_width, 0.0f, 0.0f };
	upl__gl_push_box(x, y, width, height, 1.0f, radii, params, color);
}

static void upl__gfx_shadow(float x, float y, float width, float height, const float radii[4], float blur, Leaf_Color color)
{
	const float spread = fmaxf(blur, 0.5f);
	const float params[4] = { UPL_GL_KIND_SHADOW, spread, 0.0f, 0.0f };
	upl__gl_push_box(x, y, width, height, spread + 1.0f, radii, params, color);
}

static void upl__gfx_line(float x0, float y0, float x1, float y1, float width, Leaf_Color color)
{
	const float half = width * 0.5f;
	const float margin = half + 1.0f;
	const float shape[4] = { x0, y0, x1, y1 };
	const float radii[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float params[4] = { UPL_GL_KIND_LINE, half, 0.0f, 0.0f };
	upl__gl_push(fminf(x0, x1) - margin, fminf(y0, y1) - margin, fmaxf(x0, x1) + margin, fmaxf(y0, y1) + margin, 0.0f, 0.0f, 1.0f, 1.0f, shape, radii, params, color);
}

static void upl__gfx_fill_circle(float cx, float cy, float radius, Leaf_Color color)
{
	const float radii[4] = { radius, radius, radius, radius };
	upl__gfx_fill_rect(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, radii, color);
}

static void upl__gfx_stroke_arc(float cx, float cy, float radius, float start_angle, float sweep, float width, Leaf_Color color)
{
	const float half = width * 0.5f;
	const float extent = radius + half + 1.0f;
	const float shape[4] = { cx, cy, radius, 0.0f };
	const float radii[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float params[4] = { UPL_GL_KIND_ARC, half, start_angle, sweep };
	upl__gl_push(cx - extent, cy - extent, cx + extent, cy + extent, 0.0f, 0.0f, 1.0f, 1.0f, shape, radii, params, color);
}

static void upl__gfx_glyph(const Upl_Atlas *atlas, Upl_Glyph *glyph, float x, float y, Leaf_Color color)
{
	upl__gl_state.atlas = atlas;
	const float texel = 1.0f / (float)UPL_ATLAS_SIZE;
	const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float params[4] = { UPL_GL_KIND_GLYPH, 0.0f, 0.0f, 0.0f };
	upl__gl_push(
		x, y, x + (float)glyph->width, y + (float)glyph->height,
		(float)glyph->atlas_x * texel, (float)glyph->atlas_y * texel,
		(float)(glyph->atlas_x + glyph->width) * texel, (float)(glyph->atlas_y + glyph->height) * texel,
		zero, zero, params, color
	);
}

static void upl__gfx_image(Upl_Image *image, float x, float y, float width, float height, const float radii[4], Leaf_Color tint)
{
	Upl_GlState *state = &upl__gl_state;
	if (!state->current)
		return;

	const GLuint texture = upl__gl_image_texture(state->current, image);
	if (texture != state->bound_image)
	{
		upl__gl_flush();
		state->api.glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture);
		state->bound_image = texture;
	}

	const float params[4] = { UPL_GL_KIND_IMAGE, 0.0f, 0.0f, 0.0f };
	upl__gl_push_box(x, y, width, height, 0.0f, radii, params, tint);
}

static void upl__gfx_set_clip(const Upl_Clip *clip)
{
	Upl_GlState *state = &upl__gl_state;
	if (!state->current)
		return;

	upl__gl_flush();
	if (!clip)
	{
		glDisable(GL_SCISSOR_TEST);
		return;
	}

	const int32_t width = clip->x1 > clip->x0 ? clip->x1 - clip->x0 : 0;
	const int32_t height = clip->y1 > clip->y0 ? clip->y1 - clip->y0 : 0;
	glEnable(GL_SCISSOR_TEST);
	glScissor(clip->x0, state->height - clip->y0 - height, width, height);
}

static void upl__gfx_atlas_reset(void)
{
	upl__gl_flush();
}

static void upl__gfx_glyph_release(Upl_Glyph *glyph)
{
	glyph->native = NULL;
}

static void upl__gfx_image_release(Upl_Image *image)
{
	image->native = NULL;
}

#pragma endregion
