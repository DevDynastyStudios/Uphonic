#define Naui_Map(type)                      type*

#define naui_strmap_get(map, key)           shget(map, key)
#define naui_strmap_get_index(map, key)     shgeti(map, key)
#define naui_strmap_put(map, key, value)    shput(map, key, value)
#define naui_strmap_puts(map, entry)		shputs(map, entry)
#define naui_strmap_del(map, key)           shdel(map, key)
#define naui_strmap_len(map)                shlen(map)
#define naui_strmap_free(map)               shfree(map)
#define naui_strmap_default(map, value)     shdefault(map, value)

#define naui_intmap_get(map, key)           hmget(map, key)
#define naui_intmap_get_index(map, key)     hmgeti(map, key)
#define naui_intmap_put(map, key, value)    hmput(map, key, value)
#define naui_intmap_del(map, key)           hmdel(map, key)
#define naui_intmap_len(map)                hmlen(map)
#define naui_intmap_free(map)               hmfree(map)
#define naui_intmap_default(map, value)     hmdefault(map, value)
