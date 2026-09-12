#define Naui_List(type) type *

#define naui_list_len(arr)              arrlen(arr)
#define naui_list_push(arr, data)       arrput(arr, data)
#define naui_list_pop(arr)              arrpop(arr)
#define naui_list_free(arr)             arrfree(arr)
#define naui_list_reserve(arr, len)     arrsetcap(arr, len)
#define naui_list_insert(arr, data, i)  arrins(arr, i, data)
#define naui_list_remove(arr, i)        arrdel(arr, i)
#define naui_list_uremove(arr, i)       arrdelswap(arr, i)
#define naui_list_clear(arr)            arrsetlen(arr, 0)

#define naui_list_clone(arr) \
    ((arr) ? ({ \
        __typeof__(arr) _new = NULL; \
        naui_list_reserve(_new, naui_list_len(arr)); \
        memcpy(_new, (arr), naui_list_len(arr) * sizeof(*(arr))); \
        arrsetlen(_new, naui_list_len(arr)); \
        _new; \
    }) : NULL)