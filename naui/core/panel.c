#define NAUI_PANEL_DEFAULT_WIDTH 1280
#define NAUI_PANEL_DEFAULT_HEIGHT 720

#define NAUI_PANEL_RESIZE_BORDER 6.0f

#define NAUI_MAIN_VIEWPORT_ID "__naui_main_viewport"

#define NAUI_ROOT_PANEL_ID "__naui_root_panel"
#define NAUI_CHILD_PANEL_ID "__naui_child_panel"
#define NAUI_PANEL_TAB_ID "__naui_panel_tab"
#define NAUI_PANEL_TITLEBAR_ID "__naui_panel_titlebar"

#define NAUI_CLOSE_BUTTON_ID "__naui_close_button"

#define NAUI_SPLIT_HANDLE_ID "__naui_split_handle"

#define NAUI_DOCK_GUIDE_LEFT_ID "__naui_dock_guide_left"
#define NAUI_DOCK_GUIDE_RIGHT_ID "__naui_dock_guide_right"
#define NAUI_DOCK_GUIDE_TOP_ID "__naui_dock_guide_top"
#define NAUI_DOCK_GUIDE_BOTTOM_ID "__naui_dock_guide_bottom"
#define NAUI_DOCK_GUIDE_CENTER_ID "__naui_dock_guide_center"
#define NAUI_DOCK_GUIDE_VIEWPORT_ID "__naui_dock_guide_viewport"

typedef uint8_t Naui_SplitAxis;
enum
{
    NAUI_SPLIT_AXIS_VERTICAL = LEAF_DIRECTION_VERTICAL,
    NAUI_SPLIT_AXIS_HORIZONTAL = LEAF_DIRECTION_HORIZONTAL
};

typedef struct Naui_PanelNode Naui_PanelNode;
struct Naui_PanelNode
{
    const char     *title;
    Naui_PanelType  type;
    Naui_PanelNode *children[2];
    Naui_PanelNode *parent;
    Naui_PanelNode *root;
    Naui_List(Naui_PanelNode*) tabs;
    void           *user_data;
    Naui_Vec2       position, size, min_size;
    Naui_PanelFlags flags;
    uint32_t        root_index;
    int32_t         active_tab;
    float           split_ratio;
    Naui_SplitAxis  split_axis;
    bool            occluded;
    bool            close_hovered;
};

typedef struct
{
    Naui_PanelNode *node;
}
Naui_PanelNodeWrapper;

typedef struct { char *key; Naui_PanelType value; } Naui_PanelTypeMapEntry;

typedef struct
{
    Naui_Map(Naui_PanelTypeMapEntry) panel_type_map;
    Naui_List(Naui_PanelNode*) root_nodes;

    Naui_PanelNode *main_viewport;

    Naui_PanelNode *dragging_node;
    Naui_PanelNode *resizing_node;
    Naui_PanelNode *split_resizing_node;

    Naui_PanelNode *current_panel;

    Leaf_BoundingBox dock_guide_area;
    bool any_panel_hovered;
}
Naui_PanelManager;
static Naui_PanelManager pm = { 0 };

static Naui_PanelNode *naui_alloc_panel_node(void) { return (Naui_PanelNode*)calloc(1, sizeof(Naui_PanelNode)); }
static void naui_free_panel_node(Naui_PanelNode *node) { free(node); }

void naui_register_panel_type(const char *name, Naui_PanelType type)
{
    naui_strmap_put(pm.panel_type_map, name, type);
}

Naui_PanelID naui_attach_panel(const char *type_name)
{
    ptrdiff_t type_index = naui_strmap_get_index(pm.panel_type_map, type_name);
    if (type_index < 0)
    {
        fprintf(stderr, "[Naui]: Panel of type `%s` not found!", type_name);
        return 0;
    }

    Naui_PanelNode *node = naui_alloc_panel_node();
    node->type = pm.panel_type_map[type_index].value;
    node->position = (Naui_Vec2) { 32.0f, 32.0f };
    node->size = (Naui_Vec2) { NAUI_PANEL_DEFAULT_WIDTH, NAUI_PANEL_DEFAULT_HEIGHT };
    node->min_size = (Naui_Vec2) { 100.0f, 100.0f };
    node->root = node;
    node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
    node->title = "\0";

    if (node->type.user_data_size)
        node->user_data = malloc(node->type.user_data_size);

    naui_list_push(pm.root_nodes, node);

    Naui_PanelNode *prev = pm.current_panel;
    pm.current_panel = node;
    if (node->type.on_attach)
        node->type.on_attach(node->user_data);
    pm.current_panel = prev;

    return (Naui_PanelID)node;
}

void naui_panel_set_title(Naui_PanelID panel_id, const char *title)
{
    ((Naui_PanelNode*)panel_id)->title = title;
}

void naui_panel_set_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->size = size;
}

void naui_panel_set_min_size(Naui_PanelID panel_id, Naui_Vec2 size)
{
    ((Naui_PanelNode*)panel_id)->min_size = size;
}

void naui_panel_enable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags |= flags;
}

void naui_panel_disable_flags(Naui_PanelID panel_id, Naui_PanelFlags flags)
{
    ((Naui_PanelNode*)panel_id)->flags &= ~flags;
}

static inline void naui_reset_root_indexes(uint32_t from)
{
    for (uint32_t i = from; i < naui_list_len(pm.root_nodes); i++)
        pm.root_nodes[i]->root_index = i;
}

void naui_set_main_viewport(Naui_PanelID id)
{
    pm.main_viewport = (Naui_PanelNode*)id;
    uint32_t removed_index = pm.main_viewport->root_index;
    naui_list_remove(pm.root_nodes, removed_index);
    naui_reset_root_indexes(removed_index);
}

Naui_PanelID naui_get_main_viewport(void)
{
    return (Naui_PanelID)pm.main_viewport;
}

static void naui_panel_bring_to_front_immediate(Naui_PanelNodeWrapper *wrapper)
{
    Naui_PanelNode *node = wrapper->node;
    Naui_PanelNode *root = node->root;

    if (root == pm.main_viewport)
        return;

    uint32_t last = (uint32_t)(naui_list_len(pm.root_nodes) - 1);
    if (root->root_index == last)
        return;

    for (uint32_t i = root->root_index; i < last; i++)
    {
        pm.root_nodes[i] = pm.root_nodes[i + 1];
        pm.root_nodes[i]->root_index = i;
    }

    pm.root_nodes[last] = root;
    root->root_index = last;
}

static void naui_panel_bring_to_front(Naui_PanelNode *node)
{
    Naui_PanelNodeWrapper node_wrapper = { node };
    naui_defer((Naui_DeferredEvent)naui_panel_bring_to_front_immediate, &node_wrapper, sizeof(Naui_PanelNodeWrapper));
}

static void naui_set_root_recursive(Naui_PanelNode *node, Naui_PanelNode *new_root)
{
    if (!node)
        return;
    node->root = new_root;
    naui_set_root_recursive(node->children[0], new_root);
    naui_set_root_recursive(node->children[1], new_root);
}

static inline bool naui_can_dock_center(Naui_PanelNode *node)
{
    return !pm.dragging_node->children[0];
}

Naui_PanelID naui_dock_panel(Naui_PanelID target_id, Naui_PanelID guest_id, Naui_DockDirection direction, float split_ratio)
{
    Naui_PanelNode *target = (Naui_PanelNode*)target_id;
    Naui_PanelNode *guest  = (Naui_PanelNode*)guest_id;

    bool target_is_viewport = (target == pm.main_viewport);

    if (direction == NAUI_DOCK_DIRECTION_CENTER)
    {
        naui_list_remove(pm.root_nodes, guest->root_index);
        naui_reset_root_indexes(guest->root_index);

        Naui_List(Naui_PanelNode*) target_tabs;
        if (target->parent && target->parent->tabs)
            target_tabs = target->parent->tabs;
        else if (!target->tabs)
        {
            Naui_PanelNode *target_copy = naui_alloc_panel_node();
            *target_copy = *target;
            target_copy->parent = target;
            naui_list_push(target->tabs, target_copy);
            target_tabs = target->tabs;
        }
        else
            target_tabs = target->tabs;

        if (guest->tabs)
        {
            for (int32_t i = 0; i < naui_list_len(guest->tabs); i++)
            {
                Naui_PanelNode *guest_tab = guest->tabs[i];
                guest_tab->parent = target;
                guest_tab->root = target->root;
                naui_list_push(target_tabs, guest_tab);
            }
            naui_list_free(guest->tabs);
            naui_free_panel_node(guest);
        }
        else
        {
            guest->parent = target;
            guest->root = target->root;
            naui_list_push(target_tabs, guest);
        }

        if (target->parent && target->parent->tabs)
            target->parent->tabs = target_tabs;
        else
            target->tabs = target_tabs;

        return (Naui_PanelID)target;
    }

    naui_list_remove(pm.root_nodes, guest->root_index);
    naui_reset_root_indexes(guest->root_index);

    Naui_PanelNode *dock_node = naui_alloc_panel_node();

    if (target_is_viewport)
    {
        pm.main_viewport = dock_node;
        dock_node->root = dock_node;
    }
    else if (target->parent)
    {
        dock_node->parent = target->parent;
        target->parent->children[target->parent->children[0] == target ? 0 : 1] = dock_node;
        dock_node->root = target->root;
    }
    else
    {
        dock_node->root_index = target->root_index;
        pm.root_nodes[target->root_index] = dock_node;
        dock_node->root = dock_node;
    }

    target->parent = dock_node;

    if (direction == NAUI_DOCK_DIRECTION_RIGHT || direction == NAUI_DOCK_DIRECTION_BOTTOM)
    {
        dock_node->children[0] = target;
        dock_node->children[1] = guest;
    }
    else
    {
        dock_node->children[0] = guest;
        dock_node->children[1] = target;
    }

    dock_node->split_ratio = split_ratio;
    dock_node->split_axis =
        (direction == NAUI_DOCK_DIRECTION_LEFT || direction == NAUI_DOCK_DIRECTION_RIGHT) ?
        NAUI_SPLIT_AXIS_HORIZONTAL : NAUI_SPLIT_AXIS_VERTICAL;

    naui_set_root_recursive(target, dock_node->root);
    naui_set_root_recursive(guest, dock_node->root);

    guest->parent = dock_node;

    dock_node->size = target->size;
    dock_node->position = target->position;

    return (Naui_PanelID)dock_node;
}

static void naui_undock_panel_immediate(Naui_PanelNodeWrapper *wrapper)
{
    Naui_PanelNode *node = wrapper->node;

    if (node == pm.main_viewport)
    {
        pm.main_viewport = NULL;

        node->parent = NULL;
        node->root = node;
        node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
        naui_list_push(pm.root_nodes, node);
        return;
    }

    Naui_PanelNode *dock_node = node->parent;

    if (!dock_node)
        return;

    if (dock_node->tabs)
    {
        Naui_PanelNode *group = dock_node;
        int32_t count = naui_list_len(group->tabs);
        int32_t found = -1;

        for (int32_t i = 0; i < count; i++)
        {
            if (group->tabs[i] == node)
            {
                found = i;
                break;
            }
        }

        if (found < 0)
            return;

        naui_list_remove(group->tabs, found);
        count--;

        if (group->active_tab >= count)
            group->active_tab = count - 1;
        else if (found < group->active_tab)
            group->active_tab--;

        if (count == 1)
        {
            Naui_PanelNode *remaining = group->tabs[0];

            naui_list_free(group->tabs);
            remaining->tabs = NULL;
            remaining->active_tab = 0;
            remaining->parent = group->parent;
            remaining->root = group->root;
            remaining->position = group->position;
            remaining->size = group->size;

            if (group->parent)
            {
                int slot = group->parent->children[0] == group ? 0 : 1;
                group->parent->children[slot] = remaining;
            }
            else if (group == pm.main_viewport)
            {
                pm.main_viewport = remaining;
                naui_set_root_recursive(remaining, remaining);
            }
            else
            {
                remaining->root_index = group->root_index;
                pm.root_nodes[group->root_index] = remaining;
                naui_set_root_recursive(remaining, remaining);
            }

            naui_free_panel_node(group);
        }

        node->parent = NULL;
        node->root = node;
        node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
        naui_list_push(pm.root_nodes, node);
        return;
    }

    Naui_PanelNode *sibling = dock_node->children[dock_node->children[0] == node ? 1 : 0];
    sibling->position = node->root->position;
    sibling->size = node->root->size;

    if (dock_node->parent)
    {
        int slot = dock_node->parent->children[0] == dock_node ? 0 : 1;
        dock_node->parent->children[slot] = sibling;
        sibling->parent = dock_node->parent;
    }
    else if (dock_node == pm.main_viewport)
    {
        pm.main_viewport = sibling;
        sibling->parent = NULL;
        sibling->root = sibling;
        naui_set_root_recursive(sibling, sibling);
    }
    else
    {
        sibling->root_index = dock_node->root_index;
        pm.root_nodes[dock_node->root_index] = sibling;
        sibling->parent = NULL;
        naui_set_root_recursive(sibling, sibling);
    }

    naui_free_panel_node(dock_node);

    node->parent = NULL;
    node->root = node;
    node->root_index = (uint32_t)naui_list_len(pm.root_nodes);
    naui_list_push(pm.root_nodes, node);
}

void naui_undock_panel(Naui_PanelID id)
{
    Naui_PanelNodeWrapper node_wrapper = { (Naui_PanelNode*)id };
    naui_defer((Naui_DeferredEvent)naui_undock_panel_immediate, &node_wrapper, sizeof(Naui_PanelNodeWrapper));
}

static void naui_detach_panel_immediate(Naui_PanelNodeWrapper *wrapper)
{
    Naui_PanelNode *node = wrapper->node;
    naui_undock_panel_immediate(wrapper);

    Naui_PanelNode *prev = pm.current_panel;
    pm.current_panel = node;
    if (node->type.on_detach)
        node->type.on_detach(node->user_data);
    pm.current_panel = prev;

    if (node->user_data)
        free(node->user_data);

    naui_list_remove(pm.root_nodes, node->root_index);
    naui_free_panel_node(node);
}

void naui_detach_panel(Naui_PanelID id)
{
    Naui_PanelNodeWrapper node_wrapper = { (Naui_PanelNode*)id };
    naui_defer((Naui_DeferredEvent)naui_detach_panel_immediate, &node_wrapper, sizeof(Naui_PanelNodeWrapper));
}

static bool naui_range_occludes_point(float mx, float my, uint32_t from, Naui_PanelNode *skip)
{
    for (uint32_t i = from; i < naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *other = pm.root_nodes[i];
        if (skip && (other == skip || other == skip->root))
            continue;
        Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)other));
        if (mx >= b.x && mx <= b.x + b.width && my >= b.y && my <= b.y + b.height)
            return true;
    }
    return false;
}

static bool naui_point_occluded_by_higher_panel(Naui_PanelNode *node)
{
    Naui_PanelNode *root = node->root;
    uint32_t from = (root == pm.main_viewport) ? 0 : root->root_index + 1;
    return naui_range_occludes_point((float)naui_mouse_x(), (float)naui_mouse_y(), from, pm.dragging_node);
}

static bool naui_point_occluded_above(float mx, float my, Naui_PanelNode *root)
{
    uint32_t from = (root == pm.main_viewport) ? 0 : root->root_index + 1;
    return naui_range_occludes_point(mx, my, from, NULL);
}

bool naui_panel_hovered(Naui_PanelID panel_id)
{
    return !((Naui_PanelNode*)panel_id)->occluded;
}

bool naui_any_panel_hovered(void)
{
    return pm.any_panel_hovered;
}

Naui_PanelID naui_current_panel(void)
{
    return (Naui_PanelID)pm.current_panel;
}

void *naui_current_panel_data(void)
{
    return pm.current_panel->user_data;
}

static Naui_PanelNode *naui_find_panel_of_type_recursive(Naui_PanelNode *node, const char *type_name)
{
    if (!node)
        return NULL;

    if (node->children[0])
    {
        Naui_PanelNode *found = naui_find_panel_of_type_recursive(node->children[0], type_name);
        if (found)
            return found;
        return naui_find_panel_of_type_recursive(node->children[1], type_name);
    }

    if (node->tabs)
    {
        for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
        {
            Naui_PanelNode *tab = node->tabs[i];
            if (tab->type.type_name && !strcmp(tab->type.type_name, type_name))
                return tab;
        }
        return NULL;
    }

    if (node->type.type_name && !strcmp(node->type.type_name, type_name))
        return node;

    return NULL;
}

Naui_PanelID naui_find_panel_of_type(const char *type_name)
{
    if (pm.main_viewport)
    {
        Naui_PanelNode *found = naui_find_panel_of_type_recursive(pm.main_viewport, type_name);
        if (found)
            return (Naui_PanelID)found;
    }

    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *found = naui_find_panel_of_type_recursive(pm.root_nodes[i], type_name);
        if (found)
            return (Naui_PanelID)found;
    }

    return 0;
}

static inline bool naui_can_show_dock_guides(Naui_PanelNode *node)
{
    return
        !(node->flags & NAUI_PANEL_FLAG_NO_DOCK_FROM_OTHER) &&
        pm.dragging_node &&
        pm.dragging_node != node &&
        node->root != pm.dragging_node->root &&
        !(pm.dragging_node->flags & NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER);
}

static inline void naui_render_dock_guide_slot(const char *label, Naui_PanelNode *node, Leaf_BoundingBox bb, Naui_DockDirection direciton, bool horizontal, bool occluded)
{
    Leaf_ID id = occluded ? (Leaf_ID){0}: leaf_id_indexed(label, (Naui_PanelID)node);
    bool hovered = occluded ? 0 : leaf_hovered(id);
    leaf({
        .id = id,
        .size = horizontal ?
            (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_DERIVED} :
            (Leaf_Size){LEAF_SIZE_DERIVED, LEAF_SIZE_GROW},
        .color = hovered ?
            naui_theme_color(NAUI_DOCK_GUIDE_HOVERED_COLOR_TAG) :
            naui_theme_color(NAUI_DOCK_GUIDE_COLOR_TAG),
        .rounding = {
            .value = NAUI_DPI(8.0f),
            .corners = LEAF_CORNER_ALL
        },
        .border = {
            .width = 1.0f,
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color(NAUI_DOCK_GUIDE_OUTLINE_COLOR_TAG)
        },
        .aspect_ratio = 1.0f
    });

    if (!hovered)
        return;

    switch (direciton)
    {
        case NAUI_DOCK_DIRECTION_TOP:       bb.height *= 0.5f; break;
        case NAUI_DOCK_DIRECTION_BOTTOM:    bb.height *= 0.5f; bb.y += bb.height; break;
        case NAUI_DOCK_DIRECTION_LEFT:      bb.width  *= 0.5f; break;
        case NAUI_DOCK_DIRECTION_RIGHT:     bb.width  *= 0.5f; bb.x  += bb.width; break;
    }

    pm.dock_guide_area = bb;
}

static void naui_render_dock_guides(Naui_PanelNode *node)
{
    const bool occluded = naui_point_occluded_by_higher_panel(node);
    const Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));

    const float scale = 0.5f;

    const Leaf_Size size = box.width > box.height ?
        (Leaf_Size){LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(scale)} :
        (Leaf_Size){LEAF_SIZE_PERCENT(scale), LEAF_SIZE_DERIVED};

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .size = size,
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .floating = {
            .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        },
        .aspect_ratio = 1.0f,
        .child_gap = 8.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_TOP_ID, node, box, NAUI_DOCK_DIRECTION_TOP, false, occluded);
        if (naui_can_dock_center(pm.dragging_node))
            naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_CENTER_ID, node, box, NAUI_DOCK_DIRECTION_CENTER, false, occluded);
        else leaf({.size = {LEAF_SIZE_DERIVED, LEAF_SIZE_GROW}, .aspect_ratio = 1.0f});
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_BOTTOM_ID, node, box, NAUI_DOCK_DIRECTION_BOTTOM, false, occluded);
    }

    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = size,
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .floating = {
            .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
        },
        .aspect_ratio = 1.0f,
        .child_gap = 8.0f
    })
    {
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_LEFT_ID, node, box, NAUI_DOCK_DIRECTION_LEFT, true, occluded);
        leaf({.size = {LEAF_SIZE_GROW, LEAF_SIZE_DERIVED}, .aspect_ratio = 1.0f});
        naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_RIGHT_ID, node, box, NAUI_DOCK_DIRECTION_RIGHT, true, occluded);
    }
}

static void naui_render_close_button(Naui_PanelNode *node, Naui_PanelNode *occlusion_node, float size)
{
    Leaf_ID id = leaf_id_indexed(NAUI_CLOSE_BUTTON_ID, (Naui_PanelID)node);

    bool hovered = pm.resizing_node ? false : leaf_hovered(id);
    node->close_hovered = hovered;

    if (hovered && naui_mouse_clicked(NAUI_MOUSE_LEFT) && naui_panel_hovered((Naui_PanelID)occlusion_node))
    {
        naui_detach_panel((Naui_PanelID)node);
        pm.dragging_node = NULL;
    }

    leaf({
        .id = id,
        .size = {LEAF_SIZE_FIXED(size), LEAF_SIZE_FIXED(size)},
        .padding = LEAF_PADDING_AXES(3.0f, 3.0f),
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .color = hovered ?
            naui_theme_color(NAUI_PANEL_CLOSE_HOVERED_BG_COLOR_TAG) :
            LEAF_COLOR_TRANSPARENT,
        .rounding = LEAF_ROUNDING_FIXED(NAUI_DPI(8.0f), LEAF_CORNER_ALL)
    })
    {
        Naui_Image *icon = naui_get_image(NAUI_CLOSE_ICON_TAG);
        leaf({
            .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_PERCENT(0.8f)},
            .color = naui_theme_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG),
            .image = icon,
            .aspect_ratio = 1.0f
        });
    }
}

static inline void naui_render_basic_panel_titlebar(Naui_PanelNode *node)
{
    const float font_size = NAUI_DPI(naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG));
    const Leaf_Color text_color = naui_theme_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Vec2 padding = naui_vec2_scale(naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG), naui_app_dpi_scale());

    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIXED(font_size)},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y)
    })
    {
        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
            .floating = {
                .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
            }
        })
        leaf_text(node->title, {
            .font_size = font_size,
            .color = text_color,
            .alignment = LEAF_TEXT_ALIGN_CENTER
        });

        leaf({
            .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
            .floating = {
                .parent_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER},
                .self_alignment = {LEAF_ALIGN_X_RIGHT, LEAF_ALIGN_Y_CENTER}
            },
            .padding = LEAF_PADDING_AXES(padding.x * 0.5f, padding.y)
        })

        if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE))
            naui_render_close_button(node, node, font_size);
    }
}

static inline void naui_render_docked_panel_tab(Naui_PanelNode *node, Naui_PanelNode *group, bool is_active, Leaf_ID id)
{
    const float font_size = NAUI_DPI(naui_theme_float(NAUI_PANEL_FONT_SIZE_TAG));
    const float rounding = naui_theme_float(NAUI_PANEL_ROUNDING_TAG);
    const Leaf_Color text_color = naui_theme_color(NAUI_PANEL_TITLEBAR_TEXT_COLOR_TAG);
    const Naui_Vec2 padding = naui_vec2_scale(naui_theme_vec2(NAUI_PANEL_TITLEBAR_PADDING_TAG), naui_app_dpi_scale());

    const Leaf_Color bg_color = is_active
        ? naui_theme_color(NAUI_PANEL_BODY_BG_COLOR_TAG)
        : naui_theme_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);

    leaf({
        .id = id,
        .direction = LEAF_DIRECTION_HORIZONTAL,
        .size = {LEAF_SIZE_FIT, LEAF_SIZE_FIXED(font_size)},
        .padding = LEAF_PADDING_AXES(padding.x, padding.y),
        .rounding = { NAUI_DPI(rounding), LEAF_CORNER_TL | LEAF_CORNER_TR },
        .color = bg_color,
        .child_gap = NAUI_DPI(2.0f)
    })
    {
        leaf_text(node->title, { .font_size = font_size, .color = text_color });
        if (!(node->flags & NAUI_PANEL_FLAG_NO_CLOSE) && !pm.resizing_node && leaf_hovered(id))
            naui_render_close_button(node, group, font_size);
        else
        {
            leaf({
                .size = {LEAF_SIZE_FIXED(font_size), LEAF_SIZE_FIXED(font_size)},
                .padding = LEAF_PADDING_AXES(3.0f, 3.0f)
            });
            node->close_hovered = false;
        }
    }
}

static inline void naui_render_docked_panel_titlebar(Naui_PanelNode *node)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .child_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
    })
    {
        leaf({
            .direction = LEAF_DIRECTION_HORIZONTAL,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
            .child_alignment = {LEAF_ALIGN_X_LEFT, LEAF_ALIGN_Y_CENTER},
            .child_gap = NAUI_DPI(2.0f)
        })
        {
            if (node->tabs)
            {
                for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
                {
                    Naui_PanelNode *tab = node->tabs[i];
                    naui_render_docked_panel_tab(tab, node, i == node->active_tab, leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)tab));
                }
            }
            else naui_render_docked_panel_tab(node, node, true, leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node));
        }
    }
}

static inline void naui_render_panel_titlebar(Naui_PanelNode *node)
{
    const Leaf_Color bg_color = naui_theme_color(NAUI_PANEL_TITLEBAR_BG_COLOR_TAG);

    leaf({
        .id = leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FIT},
        .color = bg_color,
        .rounding = {
            NAUI_DPI(naui_theme_float(NAUI_PANEL_ROUNDING_TAG)),
            LEAF_CORNER_TL | LEAF_CORNER_TR
        }
    })
    {
        if (node->parent || node->root == pm.main_viewport || node->tabs)
            naui_render_docked_panel_titlebar(node);
        else naui_render_basic_panel_titlebar(node);
    }
}

static inline void naui_render_panel_body(Naui_PanelNode *node)
{
    leaf({
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_GROW},
        .color = naui_theme_color(NAUI_PANEL_BODY_BG_COLOR_TAG),
        .rounding = {
            NAUI_DPI(naui_theme_float(NAUI_PANEL_ROUNDING_TAG)),
            LEAF_CORNER_BL | LEAF_CORNER_BR
        },
        .clip_children = true
    })
    {
        Naui_PanelNode *tab = node->tabs ? node->tabs[node->active_tab] : node;
        pm.current_panel = tab;
        if (tab->type.on_update)
            tab->type.on_update(tab->user_data);
        if (naui_can_show_dock_guides(node))
            naui_render_dock_guides(node);
    }
}

static void naui_render_next_panel_child(Naui_PanelNode *node)
{
    if (node->children[0])
    {
        Leaf_Color border_color = naui_theme_color(NAUI_PANEL_BORDER_COLOR_TAG);
        float border_width = naui_theme_float(NAUI_PANEL_BORDER_WIDTH_TAG);

        leaf({
            .direction = (Leaf_LayoutDirection)node->split_axis,
            .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
        })
        {
            leaf({
                .size = node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ?
                (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_PERCENT(node->split_ratio)} :
                (Leaf_Size){LEAF_SIZE_PERCENT(node->split_ratio), LEAF_SIZE_FULL}
            })
            naui_render_next_panel_child(node->children[0]);

            leaf({
                .id = leaf_id_indexed(NAUI_SPLIT_HANDLE_ID, (Naui_PanelID)node),
                .size = node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ?
                (Leaf_Size){LEAF_SIZE_FULL, LEAF_SIZE_GROW} :
                (Leaf_Size){LEAF_SIZE_GROW, LEAF_SIZE_FULL},
                .border = {
                    .width = border_width,
                    .sides = LEAF_SIDE_ALL,
                    .color = border_color
                }
            })
            naui_render_next_panel_child(node->children[1]);
        }
        return;
    }

    leaf({
        .id = leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node),
        .size = {LEAF_SIZE_FULL, LEAF_SIZE_FULL}
    })
    {
        if (!(node->flags & NAUI_PANEL_FLAG_NO_TITLE) || node->parent)
            naui_render_panel_titlebar(node);
        naui_render_panel_body(node);
    }
}

static void naui_render_panel(Naui_PanelNode *node)
{
    leaf({
        .id = leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)node),
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(node->size.x), LEAF_SIZE_FIXED(node->size.y)},
        .floating.offset = {node->position.x, node->position.y},
        .border = {
            .width = naui_theme_float(NAUI_PANEL_BORDER_WIDTH_TAG),
            .sides = LEAF_SIDE_ALL,
            .color = naui_theme_color(NAUI_PANEL_BORDER_COLOR_TAG)
        },
        .shadow = {
            .blur_radius = 32.0f,
            .color = naui_theme_color(NAUI_PANEL_SHADOW_COLOR_TAG)
        },
        .rounding = {
            NAUI_DPI(naui_theme_float(NAUI_PANEL_ROUNDING_TAG)),
            LEAF_CORNER_ALL
        },
        .color = naui_theme_color(NAUI_PANEL_BORDER_COLOR_TAG),
        .clip_children = true
    })
    {
        naui_render_next_panel_child(node);
    }
}

static void naui_render_main_viewport(void)
{
    Naui_PanelNode *node = pm.main_viewport;
    Leaf_ID id = leaf_id(NAUI_MAIN_VIEWPORT_ID);
    leaf({
        .id = id,
        .size = {LEAF_SIZE_GROW, LEAF_SIZE_GROW},
        .color = node ?
            naui_theme_color(NAUI_PANEL_BORDER_COLOR_TAG) :
            naui_theme_color(NAUI_VIEWPORT_BG_COLOR_TAG)
    })
    {
        if (node)
            naui_render_next_panel_child(node);
        else
        {
            if (pm.dragging_node && !(pm.dragging_node->flags & NAUI_PANEL_FLAG_NO_DOCK_TO_OTHER))
            leaf({
                .positioning = LEAF_POSITIONING_FLOATING_TO_PARENT,
                .size = {LEAF_SIZE_DERIVED, LEAF_SIZE_FIXED(128.0f)},
                .floating = {
                    .parent_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER},
                    .self_alignment = {LEAF_ALIGN_X_CENTER, LEAF_ALIGN_Y_CENTER}
                },
                .aspect_ratio = 1.0f
            })
            {
                naui_render_dock_guide_slot(NAUI_DOCK_GUIDE_VIEWPORT_ID, NULL,
                    leaf_get_bounding_box(id), NAUI_DOCK_DIRECTION_CENTER, false,
                    naui_range_occludes_point((float)naui_mouse_x(), (float)naui_mouse_y(), 0, pm.dragging_node));
            }
        }
    }
}

static void naui_render_dock_guide_area(void)
{
    if (pm.dock_guide_area.width <= 0 || pm.dock_guide_area.height <= 0)
        return;
    leaf({
        .positioning = LEAF_POSITIONING_FLOATING_TO_ROOT,
        .size = {LEAF_SIZE_FIXED(pm.dock_guide_area.width), LEAF_SIZE_FIXED(pm.dock_guide_area.height)},
        .floating.offset = {pm.dock_guide_area.x, pm.dock_guide_area.y},
        .color = naui_theme_color(NAUI_DOCK_GUIDE_HOVERED_COLOR_TAG)
    });
}

static bool naui_subtree_has_close_hovered(Naui_PanelNode *node)
{
    if (!node) return false;
    if (!node->children[0])
        return node->close_hovered;
    return naui_subtree_has_close_hovered(node->children[0]) ||
           naui_subtree_has_close_hovered(node->children[1]);
}

static Naui_PanelNode *naui_find_hovered_tab(Naui_PanelNode *node, Naui_PanelNode *active_tab, Leaf_ID *out_id)
{
    if (node->flags & NAUI_PANEL_FLAG_NO_UNDOCK)
        return NULL;

    if (node->tabs)
    {
        for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
        {
            Leaf_ID id = leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node->tabs[i]);
            if (leaf_hovered(id))
            {
                if (out_id) *out_id = id;
                return node->tabs[i];
            }
        }
        return NULL;
    }

    Leaf_ID id = leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)active_tab);
    if (leaf_hovered(id))
    {
        if (out_id) *out_id = id;
        return active_tab;
    }
    return NULL;
}


static inline bool naui_try_begin_panel_move(Naui_PanelNode *node, Naui_Vec2 *drag_offset)
{
    Naui_PanelNode *root = node->root;

    if (pm.dragging_node || node == pm.main_viewport)
    {
        if (leaf_hovered(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root)))
            naui_panel_bring_to_front(root);
        return false;
    }

    if (leaf_hovered(leaf_id_indexed(NAUI_PANEL_TITLEBAR_ID, (Naui_PanelID)node)))
    {
        pm.dragging_node = root;
        drag_offset->x = naui_mouse_x() - root->position.x;
        drag_offset->y = naui_mouse_y() - root->position.y;
        naui_panel_bring_to_front(root);
        return true;
    }

    if (leaf_hovered(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root)))
        naui_panel_bring_to_front(root);

    return false;
}

static inline void naui_handle_panel_click(Naui_PanelNode *node, Naui_PanelNode *active_tab, Naui_Vec2 *drag_offset, Naui_PanelNode **pending_undock_node)
{
    Naui_PanelNode *root = node->root;

    if (node->occluded)
        return;
    if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return;
    if (pm.dragging_node && pm.dragging_node->root != root)
        return;
    if (naui_subtree_has_close_hovered(root))
        return;

    Naui_PanelNode *hovered_tab = naui_find_hovered_tab(node, active_tab, NULL);
    if (hovered_tab)
    {
        *pending_undock_node = hovered_tab;
        return;
    }

    if (!pm.dragging_node && node != pm.main_viewport)
        naui_try_begin_panel_move(node, drag_offset);
}

static inline void naui_apply_pending_undock(Naui_PanelNode *node, Naui_PanelNode *active_tab, Naui_Vec2 *drag_offset, Naui_PanelNode **pending_undock_node)
{
    if (*pending_undock_node != active_tab)
        return;

    if (naui_mouse_released(NAUI_MOUSE_LEFT))
    {
        *pending_undock_node = NULL;
        return;
    }

    if (!naui_mouse_dragging(NAUI_MOUSE_LEFT))
        return;

    naui_undock_panel((Naui_PanelID)active_tab);

    Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));
    box.width  = fmaxf(box.width,  active_tab->min_size.x);
    box.height = fmaxf(box.height, active_tab->min_size.y);

    active_tab->position = (Naui_Vec2){ box.x, box.y };
    active_tab->size = (node == pm.main_viewport)
        ? (Naui_Vec2){ box.width * 0.5f, box.height * 0.5f }
        : (Naui_Vec2){ box.width, box.height };

    pm.dragging_node = active_tab;
    drag_offset->x = naui_mouse_x() - active_tab->position.x;
    drag_offset->y = naui_mouse_y() - active_tab->position.y;
    naui_panel_bring_to_front(active_tab);

    *pending_undock_node = NULL;
}

static inline void naui_apply_panel_move(Naui_PanelNode *root, Naui_Vec2 drag_offset)
{
    if (root == pm.main_viewport || root->flags & NAUI_PANEL_FLAG_NO_MOVE)
        return;
    if (pm.dragging_node != root)
        return;

    root->position.x = naui_mouse_x() - drag_offset.x;
    root->position.y = naui_mouse_y() - drag_offset.y;
}

static void naui_update_panel_dragging(Naui_PanelNode *node)
{
    static Naui_Vec2 drag_offset;
    static Naui_PanelNode *pending_undock_node;

    if (pm.resizing_node || pm.split_resizing_node)
        return;

    Naui_PanelNode *root = node->root;
    Naui_PanelNode *active_tab = node->tabs ? node->tabs[node->active_tab] : node;

    naui_handle_panel_click(node, active_tab, &drag_offset, &pending_undock_node);
    naui_apply_pending_undock(node, active_tab, &drag_offset, &pending_undock_node);
    naui_apply_panel_move(root, drag_offset);
}

static inline Naui_Cursor naui_resize_cursor(int8_t ex, int8_t ey)
{
    if (ex && ey) return (ex == ey) ? NAUI_CURSOR_RESIZE_NWSE : NAUI_CURSOR_RESIZE_NESW;
    return ex ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS;
}

static void naui_update_panel_resizing(Naui_PanelNode *node)
{
    static int8_t drag_x, drag_y;
    static Naui_Vec2 drag_mouse, drag_pos, drag_size;

    if (node->flags & NAUI_PANEL_FLAG_NO_RESIZE || pm.split_resizing_node)
        return;

    Naui_PanelNode *root = node->root;
    if (root == pm.main_viewport)
        return;

    if (pm.resizing_node)
    {
        if (pm.resizing_node != root)
            return;

        float dx = naui_mouse_x() - drag_mouse.x;
        float dy = naui_mouse_y() - drag_mouse.y;

        if (drag_x < 0) dx = fminf(dx, drag_size.x - node->min_size.x);
        if (drag_y < 0) dy = fminf(dy, drag_size.y - node->min_size.y);

        if (drag_x < 0) { root->position.x = drag_pos.x + dx; root->size.x = drag_size.x - dx; }
        else if (drag_x > 0) root->size.x = fmaxf(drag_size.x + dx, node->min_size.x);

        if (drag_y < 0) { root->position.y = drag_pos.y + dy; root->size.y = drag_size.y - dy; }
        else if (drag_y > 0) root->size.y = fmaxf(drag_size.y + dy, node->min_size.y);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            pm.resizing_node = NULL;

        naui_set_cursor(naui_resize_cursor(drag_x, drag_y));
        return;
    }

    if (pm.dragging_node)
        return;

    Leaf_BoundingBox b = leaf_get_bounding_box(leaf_id_indexed(NAUI_ROOT_PANEL_ID, (Naui_PanelID)root));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();

    int8_t ex = (mx < b.x + NAUI_PANEL_RESIZE_BORDER) ? -1 : (mx > b.x + b.width - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;
    int8_t ey = (my < b.y + NAUI_PANEL_RESIZE_BORDER) ? -1 : (my > b.y + b.height - NAUI_PANEL_RESIZE_BORDER) ? 1 : 0;

    bool inside = mx > b.x - NAUI_PANEL_RESIZE_BORDER && mx < b.x + b.width + NAUI_PANEL_RESIZE_BORDER
               && my > b.y - NAUI_PANEL_RESIZE_BORDER && my < b.y + b.height + NAUI_PANEL_RESIZE_BORDER;

    if (!inside || (!ex && !ey))
        return;

    if (naui_point_occluded_above(mx, my, root))
        return;

    if (naui_mouse_pressed(NAUI_MOUSE_LEFT))
    {
        pm.resizing_node = root;
        drag_x = ex; drag_y = ey;
        drag_mouse = (Naui_Vec2){ mx, my };
        drag_pos = root->position;
        drag_size = root->size;
        naui_panel_bring_to_front(root);
        naui_set_cursor(naui_resize_cursor(ex, ey));
        return;
    }

    naui_set_cursor(naui_resize_cursor(ex, ey));
}

static void naui_update_split_resizing(Naui_PanelNode *node)
{
    static Naui_Vec2 drag_start_mouse;
    static float drag_start_ratio;
    static float drag_total_size;

    if (pm.split_resizing_node)
    {
        if (pm.split_resizing_node != node)
            return;

        float delta = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
            ? naui_mouse_x() - drag_start_mouse.x
            : naui_mouse_y() - drag_start_mouse.y;

        node->split_ratio = fmaxf(0.05f, fminf(0.95f,
            drag_start_ratio + delta / drag_total_size));

        naui_set_cursor(node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
            ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS);

        if (naui_mouse_released(NAUI_MOUSE_LEFT))
            pm.split_resizing_node = NULL;
        return;
    }

    if (pm.dragging_node || pm.resizing_node)
        return;

    Leaf_BoundingBox b = leaf_get_bounding_box(
        leaf_id_indexed(NAUI_SPLIT_HANDLE_ID, (Naui_PanelID)node));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();

    bool near_divider = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? (fabsf(mx - b.x) < NAUI_PANEL_RESIZE_BORDER && my >= b.y && my <= b.y + b.height)
        : (fabsf(my - b.y) < NAUI_PANEL_RESIZE_BORDER && mx >= b.x && mx <= b.x + b.width);

    if (!near_divider)
        return;

    if (naui_point_occluded_above(mx, my, node->root))
        return;

    naui_set_cursor(node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? NAUI_CURSOR_RESIZE_EW : NAUI_CURSOR_RESIZE_NS);

    if (!naui_mouse_pressed(NAUI_MOUSE_LEFT))
        return;

    pm.split_resizing_node = node;
    drag_start_mouse = (Naui_Vec2){ mx, my };
    drag_start_ratio = node->split_ratio;
    drag_total_size  = node->split_axis == NAUI_SPLIT_AXIS_HORIZONTAL
        ? b.width  / fmaxf(1.0f - drag_start_ratio, 0.001f)
        : b.height / fmaxf(1.0f - drag_start_ratio, 0.001f);
}

static void naui_update_panel_dock_guides(Naui_PanelNode *node)
{
    if (!naui_mouse_released(NAUI_MOUSE_LEFT))
        return;
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_LEFT_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_LEFT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_RIGHT_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_RIGHT, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_TOP_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_TOP, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_BOTTOM_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_BOTTOM, 0.5f);
    else if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_CENTER_ID, (Naui_PanelID)node)))
        naui_dock_panel((Naui_PanelID)node, (Naui_PanelID)pm.dragging_node, NAUI_DOCK_DIRECTION_CENTER, 0.0f);
}

static void naui_update_splits_only(Naui_PanelNode *node)
{
    if (!node->children[0]) return;
    naui_update_split_resizing(node);
    naui_update_splits_only(node->children[0]);
    naui_update_splits_only(node->children[1]);
}

static inline void naui_calculate_and_cache_panel_occlusion(Naui_PanelNode *node)
{
    Leaf_BoundingBox box = leaf_get_bounding_box(leaf_id_indexed(NAUI_CHILD_PANEL_ID, (Naui_PanelID)node));
    float mx = (float)naui_mouse_x(), my = (float)naui_mouse_y();
    bool in_bounds = mx >= box.x && mx <= box.x + box.width && my >= box.y && my <= box.y + box.height;
    node->occluded = !in_bounds || naui_point_occluded_above(mx, my, node->root);
    if (!node->occluded)
        pm.any_panel_hovered = true;
}

static void naui_update_panel_tabs(Naui_PanelNode *node)
{
    if (!node->tabs)
        return;
    for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
    {
        if (naui_mouse_pressed(NAUI_MOUSE_LEFT) && !node->tabs[i]->close_hovered && leaf_hovered(leaf_id_indexed(NAUI_PANEL_TAB_ID, (uintptr_t)node->tabs[i])))
        {
            node->active_tab = i;
            break;
        }
    }
}

static void naui_update_panel(Naui_PanelNode *node)
{
    if (node->children[0])
    {
        naui_update_panel(node->children[0]);
        naui_update_panel(node->children[1]);
        return;
    }

    naui_calculate_and_cache_panel_occlusion(node);

    naui_update_panel_tabs(node);
    naui_update_panel_resizing(node);
    naui_update_panel_dragging(node);

    node->position.x = fminf(node->position.x, naui_app_width() - 24.0f);
    node->position.y = fminf(node->position.y, naui_app_height() - 24.0f);

    if (naui_can_show_dock_guides(node))
        naui_update_panel_dock_guides(node);
}

static void naui_update_main_viewport(void)
{
    Naui_PanelNode *node = pm.main_viewport;
    if (node)
    {
        naui_update_panel(node);
        return;
    }

    if (!naui_mouse_released(NAUI_MOUSE_LEFT) || !pm.dragging_node)
        return;
    if (leaf_hovered(leaf_id_indexed(NAUI_DOCK_GUIDE_VIEWPORT_ID, 0)))
        naui_set_main_viewport((Naui_PanelID)pm.dragging_node);
}

void naui_render_panels_and_viewport(void)
{
    pm.any_panel_hovered = false;

    for (int32_t i = (int32_t)naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_splits_only(pm.root_nodes[i]);
    if (pm.main_viewport)
        naui_update_splits_only(pm.main_viewport);

    for (int32_t i = (int32_t)naui_list_len(pm.root_nodes); i-- > 0;)
        naui_update_panel(pm.root_nodes[i]);
    naui_update_main_viewport();

    pm.dock_guide_area = (Leaf_BoundingBox){0};
    if (naui_mouse_released(NAUI_MOUSE_LEFT))
    {
        pm.dragging_node = NULL;
        pm.resizing_node = NULL;
        pm.split_resizing_node = NULL;
    }

    naui_render_main_viewport();
    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
        naui_render_panel(pm.root_nodes[i]);

    naui_render_dock_guide_area();
    pm.current_panel = NULL;
}

#pragma region Serialization
static inline const char *naui_get_panel_type(Naui_PanelNode *node)
{
    return node->type.type_name;
}

static inline Naui_PanelNode *naui_find_root_panel_of_type(const char *type_name)
{
    for (int32_t i = 0; i < (int32_t)naui_list_len(pm.root_nodes); i++)
    {
        Naui_PanelNode *n = pm.root_nodes[i];
        if (n->children[0] || !n->type.type_name)
            continue;
        if (!strcmp(n->type.type_name, type_name))
            return n;
    }
    return NULL;
}

static bool naui_serialize_panel_node(Naui_Json *json, Naui_JsonValue *out, Naui_PanelNode *node)
{
    if (!node)
        return false;

    if (node->children[0])
    {
        Naui_JsonValue *child0 = naui_json_set_object(json, out, "child0");
        Naui_JsonValue *child1 = naui_json_set_object(json, out, "child1");

        bool ok0 = naui_serialize_panel_node(json, child0, node->children[0]);
        bool ok1 = naui_serialize_panel_node(json, child1, node->children[1]);

        if (!ok0 && !ok1)
            return false;

        naui_json_set_string(json, out, "kind", "split");
        naui_json_set_string(json, out, "axis",
            node->split_axis == NAUI_SPLIT_AXIS_VERTICAL ? "vertical" : "horizontal");
        naui_json_set_number(json, out, "ratio", node->split_ratio);
        return true;
    }

    if (node->tabs)
    {
        Naui_JsonValue *tabs_arr = naui_json_set_array(json, out, "tabs");
        bool any = false;
        for (int32_t i = 0; i < naui_list_len(node->tabs); i++)
        {
            Naui_PanelNode *tab = node->tabs[i];
            if (!(tab->flags & NAUI_PANEL_FLAG_SERIALIZABLE))
                continue;
            const char *name = naui_get_panel_type(tab);
            if (!name)
                continue;
            naui_json_push_string(json, tabs_arr, name);
            any = true;
        }
        if (!any)
            return false;

        naui_json_set_string(json, out, "kind", "tabs");
        naui_json_set_int(json, out, "active_tab", node->active_tab);
        return true;
    }

    if (!(node->flags & NAUI_PANEL_FLAG_SERIALIZABLE))
        return false;

    const char *name = naui_get_panel_type(node);
    if (!name)
        return false;

    naui_json_set_string(json, out, "kind", "panel");
    naui_json_set_string(json, out, "type", name);

    return true;
}

bool naui_serialize_viewport(const char *file_path)
{
    if (!pm.main_viewport)
        return false;

    Naui_Json json = naui_json_result_create();
    Naui_JsonValue *root = naui_json_object(&json);
    naui_serialize_panel_node(&json, root, pm.main_viewport);
    naui_json_write_file(root, NAUI_PATH(file_path), true);
    naui_json_free(&json);
    return true;
}

static Naui_PanelNode *naui_deserialize_panel_node(Naui_JsonValue *json_parent)
{
    Naui_JsonValue *kind = naui_json_object_get(json_parent, "kind");
    char kind_name[8];
    naui_json_copy_string(kind, kind_name, sizeof(kind_name));

    if (!strncmp(kind_name, "panel", sizeof(kind_name)))
    {
        Naui_JsonValue *type = naui_json_object_get(json_parent, "type");
        char type_name[64];
        naui_json_copy_string(type, type_name, sizeof(type_name));
        return naui_find_root_panel_of_type(type_name);
    }
    else if (!strncmp(kind_name, "split", sizeof(kind_name)))
    {
        Naui_JsonValue *axis = naui_json_object_get(json_parent, "axis");
        char axis_name[16];
        naui_json_copy_string(axis, axis_name, sizeof(axis_name));

        float split_ratio = (float)naui_json_get_number(naui_json_object_get(json_parent, "ratio"), 0.5f);

        Naui_JsonValue *child0 = naui_json_object_get(json_parent, "child0");
        Naui_PanelNode *child0_node = naui_deserialize_panel_node(child0);

        Naui_JsonValue *child1 = naui_json_object_get(json_parent, "child1");
        Naui_PanelNode *child1_node = naui_deserialize_panel_node(child1);

        Naui_PanelNode *dock_node = (Naui_PanelNode*)naui_dock_panel(
            (Naui_PanelID)child0_node, (Naui_PanelID)child1_node,
            strncmp(axis_name, "horizontal", sizeof(axis_name)) ?
            NAUI_DOCK_DIRECTION_BOTTOM : NAUI_DOCK_DIRECTION_RIGHT,
            split_ratio
        );
        return dock_node;
    }
    else if (!strncmp(kind_name, "tabs", sizeof(kind_name)))
    {
        Naui_JsonValue *tabs_arr = naui_json_object_get(json_parent, "tabs");
        size_t tab_count = (tabs_arr && tabs_arr->type == NAUI_JSON_ARRAY) ? tabs_arr->array.count : 0;

        char tab_type_name[64] = {0};
        naui_json_copy_string(naui_json_array_get(tabs_arr, 0), tab_type_name, sizeof(tab_type_name));
        Naui_PanelNode *main_tab = naui_find_root_panel_of_type(tab_type_name);

        for (size_t i = 1; i < tab_count; i++)
        {
            naui_json_copy_string(naui_json_array_get(tabs_arr, i), tab_type_name, sizeof(tab_type_name));
            naui_dock_panel(
                (Naui_PanelID)main_tab, (Naui_PanelID)naui_find_root_panel_of_type(tab_type_name),
                NAUI_DOCK_DIRECTION_CENTER, 0.0f);
        }

        int32_t active_tab = naui_json_get_int(naui_json_object_get(json_parent, "active_tab"), 0);
        if (main_tab->tabs && active_tab >= 0 && active_tab < naui_list_len(main_tab->tabs))
            main_tab->active_tab = active_tab;

        return main_tab;
    }

    return NULL;
}

bool naui_deserialize_viewport(const char *file_path)
{
    Naui_Json json = naui_json_parse_file(NAUI_PATH(file_path));

    Naui_PanelNode *root_node = naui_deserialize_panel_node(json.root);
    if (root_node)
        naui_set_main_viewport((Naui_PanelID)root_node);

    naui_json_free(&json);
    return true;
}

#pragma endregion Serialization