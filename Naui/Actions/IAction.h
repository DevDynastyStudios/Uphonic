#pragma once

namespace Naui
{
class IAction
{
public:
	virtual ~IAction() = default;
	virtual void Do() = 0;
	virtual void Undo() = 0;
	virtual const char* Name() const = 0;
};
}