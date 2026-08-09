#pragma once

#define NAUI_TOOLTIP(anchor_id) \
    for (bool naui__tt_ = naui_tooltip_begin((anchor_id)); \
         naui__tt_; \
         (naui_tooltip_end(naui__tt_), naui__tt_ = false))

bool naui_tooltip_begin(Leaf_ID anchor_id);
void naui_tooltip_end(bool state);
