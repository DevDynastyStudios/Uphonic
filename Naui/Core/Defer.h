#pragma warning(push)
#pragma warning(disable : 4251)
#pragma once

#include "Base.h"

#include <functional>
#include <vector>
#include <utility>

namespace Naui
{

class NAUI_API Defer
{
public:
	template<typename Fn, typename... Args>
	static void Add(Fn&& fn, Args&&... args)
	{
		callbacks.emplace_back(
			[fn = std::forward<Fn>(fn),
			 ...args = std::forward<Args>(args)]() mutable
			{
				fn(args...);
			}
		);
	}

	static void Process();
	static void Flush();

private:
	static std::vector<std::function<void()>> callbacks;
};

}
#pragma warning(pop)