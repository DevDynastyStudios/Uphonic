typedef struct
{
    char *key;
    Naui_Image value;
}
Naui_ImageHashEntry;

NAUI_API void naui_asset_manager_load_images(const char *const images_path);
NAUI_API void naui_asset_manager_free(void);
NAUI_API Naui_Image *naui_asset_image(const char *const name);
