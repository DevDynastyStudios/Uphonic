#include "Defer.h"

namespace Naui
{
std::vector<std::function<void()>> Defer::callbacks;

void Defer::Process()
{
    for (auto& cb : callbacks)
        cb();
    callbacks.clear();
}

void Defer::Flush()
{
    callbacks.clear();
}
}