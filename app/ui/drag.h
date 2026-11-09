#define UPH_DRAG_PAYLOAD_MAX 4608

#define UPH_DRAG_TYPE_SAMPLE_FILE "uph_drag_sample_file"
#define UPH_DRAG_TYPE_RESOURCE "uph_drag_resource"

typedef void (*Uph_DragRenderFn)(const void *payload);

typedef struct
{
	Uph_ResourceType type;
	Uph_ResourceIndex index;
}
Uph_DragResource;

void uph_drag_begin(const char *type, const void *payload, size_t size, Uph_DragRenderFn render);
const void *uph_drag_hover(const char *type);
const void *uph_drag_drop(const char *type);
void uph_drag_flush(void);

void uph_drag_label(const Naui_Image *icon, const char *text, Naui_Color accent);
