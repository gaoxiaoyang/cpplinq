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
#include <chrono>
// ----------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
// ----------------------------------------------------------------------------------------------
#include "../CppStream/cppstream.hpp"
// ----------------------------------------------------------------------------------------------
#define PERFSET_INNER_SIZE 100000U
#define PERFSET_OUTER_SIZE 10000U
// ----------------------------------------------------------------------------------------------
namespace
{
    template<typename TPredicate>
    __declspec(noinline) long long execute_testruns (
            char const *    name
        ,   uint32_t        test_runs
        ,   TPredicate      predicate
        )
    {
        printf ("Starting test run: %s\n", name);

        // dry run
        auto sum = predicate ();

        auto then = std::chrono::high_resolution_clock::now ();

        for (auto test_run = 0U; test_run < test_runs; ++test_run)
        {
            sum += predicate ();
        }

        auto now = std::chrono::high_resolution_clock::now ();

        auto diff = now - then;

        auto diff_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count ();

        printf ("Test run complete: %s - %u, %lu ms\n", name, sum, diff_in_ms);

        return diff_in_ms;
    }

    volatile uint32_t count = 0U;

    // Various trickery to disallow inline and call eliding
    __declspec(noinline) uint32_t sum_integers (uint32_t begin, uint32_t end)
    {
        ++count;
        auto result = 0U;

        for (auto iter = begin; iter != end; ++iter)
        {
            result += iter;
        }

        return result;
    }

}
// ----------------------------------------------------------------------------------------------
int main ()
{
    using namespace cppstream;

    char const * pi[] = { "3", "1", "4", "1", "5", "9", "2", "6", "5", "4", "Fraggel"};

    auto q =
            from_array (pi)
        >>  map ([] (auto s) { return atoi (s); })
        >>  filter ([] (auto i) { return i % 2 == 0; })
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

    if (true)
    {
        auto perf_test = [] ()
        {
            return 
                    from_range (0U, PERFSET_INNER_SIZE) 
//                >>  map ([] (auto x) { return x*x; }) 
                >>  to_sum ()
                ;
        };

        auto result = perf_test ();
        printf ("Sum: %u\n", result);

        execute_testruns ("Sum perf test", PERFSET_OUTER_SIZE, perf_test);
    }

    if (true)
    {
        auto perf_test = [] ()
        {
            return sum_integers (0U, PERFSET_INNER_SIZE);
        };

        auto result = perf_test ();
        printf ("Sum: %u\n", result);

        execute_testruns ("Sum perf test", PERFSET_OUTER_SIZE, perf_test);
    }


    return 0;
}
// ----------------------------------------------------------------------------------------------
