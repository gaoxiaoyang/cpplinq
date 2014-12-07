// ----------------------------------------------------------------------------------------------
// Copyright (c) Mårten Rånge.
// ----------------------------------------------------------------------------------------------
// This source code is subject to terms and conditions of the Microsoft Public License. A
// copy of the license can be found in the License.html file at the root of this distribution.
// If you cannot locate the  Microsoft Public License, please send an email to
// dlr@microsoft.com. By using this source code in any fashion, you are agreeing to be bound
//  by the terms of the Microsoft Public License.
// ----------------------------------------------------------------------------------------------
// You must not remove this notice, or any other, from this software.
// ----------------------------------------------------------------------------------------------
#include "stdafx.h"
// ----------------------------------------------------------------------------------------------
#include "../CppStream/cppstream.hpp"
// ----------------------------------------------------------------------------------------------
int main ()
{
    using namespace cppstream;

    char const * pi[] = { "3", "1", "4", "1", "5", "9", "2", "6", "5", "4", "Fraggel"};

    auto q =
            from_array (pi)
        >>  map ([] (const char * s) { return atoi (s); })
        >>  filter ([] (int i) { return i % 2 == 0; })
        ;

    {
        auto result = q >> to_vector ();

        for (auto x : result)
        {
            printf ("%d\n", x);
        }
    }

    {
        auto result = q >> to_sum ();

        printf ("Sum: %d\n", result);
    }

    {
        auto result = from_range (0, 10000) >> to_sum ();

        printf ("Sum: %d\n", result);
    }

    return 0;
}
// ----------------------------------------------------------------------------------------------
