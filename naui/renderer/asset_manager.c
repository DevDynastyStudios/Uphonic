#define NAUI_IMAGE_ATLAS_MIN_SIZE 256
#define NAUI_IMAGE_ATLAS_MAX_SIZE 4096
#define NAUI_IMAGE_ATLAS_PADDING 1

static Naui_ImageHashEntry *naui__image_hm = NULL;

void naui_asset_manager_load_images(const char *const images_path)
{
    typedef struct
    {
        int32_t width, height;
        uint8_t *pixels;
    }
    Naui_TempImageData;

    Naui_List(Naui_TempImageData) images = NULL;
    Naui_Arena temp_arena = { 0 };

    // looping thru the images directory.
    {
        struct dirent *dp;
        DIR *dir = opendir(images_path);
        if (!dir)
        {
            fprintf(stderr, "[Naui]: failed to open %s directory for images\n", images_path);
            exit(1);
        }

        char path[256] = {0};
        char cwd[512];
#if NAUI_WINDOWS
        _getcwd(cwd, sizeof(cwd));
#else
        getcwd(cwd, sizeof(cwd));
#endif

        while ((dp = readdir(dir)))
        {
            if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;

            // this is a hack, use getcwd instead.
            snprintf(path, sizeof(path), "%s/%s", images_path, dp->d_name);
            char name[128];
            strncpy(name, dp->d_name, sizeof(name));
            name[sizeof(name) - 1] = '\0';
            char *image_name = strtok(name, "."); // this is also a hack, should iterate backwards thru str instead.

            Naui_TempImageData image;
            {
                Naui_FileHandle file_handle;
                naui_file_open(&file_handle, NAUI_PATH(path), NAUI_FILE_READ);
                const size_t file_len = naui_file_size(NAUI_PATH(path));
                uint8_t *file_data = (uint8_t*)naui_arena_alloc(&temp_arena, file_len);
                naui_file_read(&file_handle, file_data, file_len);
                naui_file_close(&file_handle);

                int32_t temp_channels;
                image.pixels = stbi_load_from_memory(file_data, file_len, &image.width, &image.height, &temp_channels, 4);
                if (!image.pixels) naui_log(NAUI_LOG_FUCKED, "failed to load image: %s\n", path);
            }

            const Naui_Image sprite = (Naui_Image){ .width = (uint32_t)image.width, .height = (uint32_t)image.height };
            naui_strmap_put(naui__image_hm, strdup(image_name), sprite);
            naui_list_push(images, image);
        }
    }
    
    if (naui_list_len(images) == 0)
        return;

    naui_arena_reset(&temp_arena);

    {
        const size_t image_count = naui_list_len(images);
        stbrp_node *nodes = (stbrp_node*)naui_arena_alloc(&temp_arena, sizeof(*nodes) * NAUI_IMAGE_ATLAS_MAX_SIZE);
        stbrp_rect *rects = (stbrp_rect*)naui_arena_alloc(&temp_arena, sizeof(*rects) * image_count);

        for (int i = 0; i < image_count; ++i)
        {
            stbrp_rect *r = &rects[i];
            const Naui_TempImageData *img = &images[i];
            r->w = img->width  + NAUI_IMAGE_ATLAS_PADDING;
            r->h = img->height + NAUI_IMAGE_ATLAS_PADDING;
            r->id = i;
        }

        int atlas_size = NAUI_IMAGE_ATLAS_MIN_SIZE;
        for (;; atlas_size *= 2)
        {
            if (atlas_size > NAUI_IMAGE_ATLAS_MAX_SIZE)
            {
                fprintf(stderr, "[Naui]: image atlas size is not enough\n");
                exit(1);
            }

            stbrp_context ctx;
            stbrp_init_target(&ctx, atlas_size, atlas_size, nodes, atlas_size);
            if (stbrp_pack_rects(&ctx, rects, image_count))
                break;
        }

        const size_t atlas_bytes = (size_t)atlas_size * atlas_size * 4;
        uint8_t *pixels = (uint8_t*)naui_arena_alloc(&temp_arena, atlas_bytes);
        memset(pixels, 0, atlas_bytes);

        const float inv_atlas_size = 1.0f / (float)atlas_size;
        for (int i = 0; i < image_count; ++i)
        {
            const stbrp_rect r = rects[i];
            const Naui_TempImageData *img = &images[i];
            const int w = img->width, h = img->height;
            Naui_Image *sprite = &naui__image_hm[i].value;

            for (int y = 0; y < h; ++y)
            {
                uint8_t *dst_row = pixels + ((r.y + y) * atlas_size + r.x) * 4;
                const uint8_t *src_row = img->pixels + (y * w) * 4;
                memcpy(dst_row, src_row, w * 4);
            }
            stbi_image_free(img->pixels);

            sprite->texture_area[0] = (float)r.x * inv_atlas_size;
            sprite->texture_area[1] = (float)r.y * inv_atlas_size;
            sprite->texture_area[2] = (float)w   * inv_atlas_size;
            sprite->texture_area[3] = (float)h   * inv_atlas_size;
        }

        extern void naui_renderer_build_atlas(uint32_t width, uint32_t height, void *data);
        naui_renderer_build_atlas(atlas_size, atlas_size, pixels);
    }

    naui_arena_free(&temp_arena);
    naui_list_free(images);
}

void naui_asset_manager_free(void)
{
    naui_strmap_free(naui__image_hm);
}

Naui_Image *naui_asset_image(const char *const name)
{
    const ptrdiff_t index = naui_strmap_get_index(naui__image_hm, name);
    if (index < 0)
    {
        fprintf(stderr, "[Naui]: image not found: %s\n", name);
        return NULL;
    }
    return &naui__image_hm[index].value;
}