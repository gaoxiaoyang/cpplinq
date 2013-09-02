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
#include <utility>
// ----------------------------------------------------------------------------------------------
#define CPPOBS_METHOD 
#define CPPOBS_INLINEMETHOD inline
// ----------------------------------------------------------------------------------------------
namespace cppobs
{
    namespace detail
    {
        struct base_observer
        {

        };

        struct base_observer_builder
        {

        };

        template<typename TObserver, typename TPredicate>
        struct select_observer : base_observer
        {

            typedef     select_observer<TObserver, TPredicate>  this_type       ;
            typedef     TObserver                               observer_type   ;
            typedef     TPredicate                              predicate_type  ;

            observer_type   observer    ;
            predicate_type  predicate   ;

            CPPOBS_INLINEMETHOD select_observer (
                    observer_type observer
                ,   predicate_type predicate
                )
                :   observer    (std::move (observer))
                ,   predicate   (std::move (predicate))
            {
            }

            template<typename TValue>
            CPPOBS_INLINEMETHOD void accept (TValue&& value)
            {
                observer.accept (predicate (std::forward<TValue> (value)));
            }

            CPPOBS_INLINEMETHOD void close ()
            {
                observer.close ();
            }

        };

        template<typename TPredicate>
        struct select_observer_builder : base_observer_builder
        {
            typedef     select_observer_builder<TPredicate> this_type       ;
            typedef     TPredicate                          predicate_type  ;

            predicate_type  predicate   ;

            CPPOBS_INLINEMETHOD select_observer_builder (predicate_type predicate)
                :   predicate (std::move (predicate))
            {
            }

            template<typename TObserver>
            CPPOBS_INLINEMETHOD select_observer<TObserver, TPredicate> operator>>= (TObserver&& observer) const
            {
                return select_observer<TObserver, TPredicate> (std::forward<TObserver> (observer), predicate);
            }
        };

        template<typename TObserver, typename TPredicate>
        struct where_observer : base_observer
        {

            typedef     where_observer<TObserver, TPredicate>   this_type       ;
            typedef     TObserver                               observer_type   ;
            typedef     TPredicate                              predicate_type  ;

            observer_type   observer    ;
            predicate_type  predicate   ;

            CPPOBS_INLINEMETHOD where_observer (
                    observer_type observer
                ,   predicate_type predicate
                )
                :   observer    (std::move (observer))
                ,   predicate   (std::move (predicate))
            {
            }

            template<typename TValue>
            CPPOBS_INLINEMETHOD void accept (TValue&& value)
            {
                if (predicate(value))
                {
                    observer.accept (std::forward<TValue> (value));
                }
            }

            CPPOBS_INLINEMETHOD void close ()
            {
                observer.close ();
            }

        };

        template<typename TPredicate>
        struct where_observer_builder : base_observer_builder
        {
            typedef     where_observer_builder<TPredicate>  this_type       ;
            typedef     TPredicate                          predicate_type  ;

            predicate_type  predicate   ;

            CPPOBS_INLINEMETHOD where_observer_builder (predicate_type predicate)
                :   predicate (std::move (predicate))
            {
            }

            template<typename TObserver>
            CPPOBS_INLINEMETHOD where_observer<TObserver, TPredicate> operator>>= (TObserver&& observer) const
            {
                return where_observer<TObserver, TPredicate> (std::forward<TObserver> (observer), predicate);
            }
        };

        template<typename TAcceptPredicate, typename TClosePredicate>
        struct terminate_observer : base_observer
        {

            typedef     terminate_observer<TAcceptPredicate, TClosePredicate>   this_type               ;
            typedef     TAcceptPredicate                                        accept_predicate_type   ;
            typedef     TClosePredicate                                         close_predicate_type    ;

            accept_predicate_type   accept_predicate    ;
            close_predicate_type    close_predicate     ;

            CPPOBS_INLINEMETHOD terminate_observer (
                    accept_predicate_type   accept_predicate
                ,   close_predicate_type    close_predicate
                )
                :   accept_predicate    (std::move (accept_predicate))
                ,   close_predicate     (std::move (close_predicate))
            {
            }

            template<typename TValue>
            CPPOBS_INLINEMETHOD void accept (TValue&& value)
            {
                accept_predicate (std::forward<TValue> (value));
            }

            CPPOBS_INLINEMETHOD void close ()
            {
                close_predicate ();
            }
        };

       
    }

    template<typename TPredicate>
    CPPOBS_INLINEMETHOD detail::select_observer_builder<TPredicate> select (TPredicate&& predicate)
    {
        return detail::select_observer_builder<TPredicate> (std::forward<TPredicate> (predicate));
    }
        
    template<typename TPredicate>
    CPPOBS_INLINEMETHOD detail::where_observer_builder<TPredicate> where (TPredicate&& predicate)
    {
        return detail::where_observer_builder<TPredicate> (std::forward<TPredicate> (predicate));
    }
        
    template<typename TAcceptPredicate, typename TClosePredicate>
    CPPOBS_INLINEMETHOD detail::terminate_observer<TAcceptPredicate, TClosePredicate> terminate (
            TAcceptPredicate&&  accept_predicate
        ,   TClosePredicate&&   close_predicate
        )
    {
        return detail::terminate_observer<TAcceptPredicate, TClosePredicate> (
                std::forward<TAcceptPredicate>  (accept_predicate)
            ,   std::forward<TClosePredicate>   (close_predicate)
            );
    }
}
// ----------------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------
int main()
{
    using namespace cppobs;

    auto observer = 
            select ([](int i) {return i + 100;})
        >>= where ([](int i) {return i % 2 == 0;})
        >>= terminate (
                [] (int i) {printf("Accepted:%d\r\n", i);},
                [] () {printf("Done!\r\n");}
                );

    observer.accept (3);
    observer.accept (1);
    observer.accept (4);
    observer.accept (1);
    observer.accept (5);
    observer.close ();

    return 0;
}
// ----------------------------------------------------------------------------------------------
