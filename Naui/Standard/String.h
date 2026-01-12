#pragma once

#include "Base.h"
#include <string>

namespace Naui
{

class NAUI_API String : public std::string
{
public:
    using std::string::string;
    
    String ToLower(void) const;
    void ToLower(void);
    String ToUpper(void) const;
    void ToUpper(void);
};

}