#include "String.h"

namespace Naui
{

String String::ToLower(void) const
{
    String result = *this;
    for (char &c : result)
        c = static_cast<char>(tolower(c));
    return result;
}

void String::ToLower(void)
{
    for (char &c : *this)
        c = static_cast<char>(tolower(c));
}

String String::ToUpper(void) const
{
    String result = *this;
    for (char &c : result)
        c = static_cast<char>(toupper(c));
    return result;
}

void String::ToUpper(void)
{
    for (char &c : *this)
        c = static_cast<char>(toupper(c));
}

}