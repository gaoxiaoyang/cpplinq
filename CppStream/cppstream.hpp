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

// ----------------------------------------------------------------------------
#ifndef CPPSTREAM__HEADER_GUARD
#   define CPPSTREAM__HEADER_GUARD
// ----------------------------------------------------------------------------
#include <type_traits>
#include <utility>
#include <vector>
#// ----------------------------------------------------------------------------
#define CPPSTREAM_INLINE inline __declspec(noalias)
#define CPPSTREAM_SINK(type)                    \
    type (type &&)                  = default;  \
    type & operator= (type &&)      = default;  \
    type (type const &)             = delete;   \
    type & operator= (type const &) = delete
#define CPPSTREAM_SOURCE(type)                              \
    type (type &&)                  = default;              \
    type & operator= (type &&)      = default;              \
    type (type const &)             = default;              \
    type & operator= (type const &) = default;              \
    template<typename TSink>                                \
    CPPSTREAM_INLINE auto operator>> (TSink && sink) const  \
    {                                                       \
        return std::forward<TSink> (sink).bind (*this);     \
    }
// ----------------------------------------------------------------------------
namespace cppstream
{
    namespace details
    {
        template<typename T>
        struct cleanup_type
        {
            using type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
        };

        // --------------------------------------------------------------------

        template<typename TIterator>
        struct iterator_source
        {
            static TIterator get_iterator ();

            using value_type = decltype (*get_iterator ());

            TIterator begin ;
            TIterator end   ;

            CPPSTREAM_SOURCE (iterator_source);

            CPPSTREAM_INLINE iterator_source (TIterator begin, TIterator end) noexcept
                : begin (std::move (begin))
                , end   (std::move (end))
            {
            }

            template<bool is_cancellable, typename TReceiver>
            CPPSTREAM_INLINE void pull (TReceiver && receiver) const
            {
                if (is_cancellable)
                {
                    for (auto iter = begin; iter != end && receiver (*iter); ++iter)
                        ;
                }
                else
                {
                    for (auto iter = begin; iter != end; ++iter)
                    {
                        receiver (*iter);
                    }
                }
            }
        };

        // --------------------------------------------------------------------

        template<typename TValue>
        struct range_source
        {
            using value_type = TValue;

            TValue begin ;
            TValue end   ;

            CPPSTREAM_SOURCE (range_source);

            CPPSTREAM_INLINE range_source (TValue begin, TValue end) noexcept
                : begin (std::move (begin))
                , end   (std::move (end))
            {
            }

            template<bool is_cancellable, typename TReceiver>
            CPPSTREAM_INLINE void pull (TReceiver && receiver) const
            {
                if (is_cancellable)
                {
                    for (auto iter = begin; iter != end && receiver (iter); ++iter)
                        ;
                }
                else
                {
                    for (auto iter = begin; iter != end; ++iter)
                    {
                        receiver (iter);
                    }
                }
            }
        };

        // --------------------------------------------------------------------

        template<typename TSource, typename TMapPredicate>
        struct map_source
        {
            static TMapPredicate                get_map ()  ;
            static typename TSource::value_type get_value ();

            using value_type    = decltype (get_map () (get_value ()));

            TSource             source  ;
            TMapPredicate       map     ;

            CPPSTREAM_SOURCE (map_source);

            CPPSTREAM_INLINE map_source (TSource source, TMapPredicate map) noexcept
                : source (std::move (source))
                , map    (std::move (map))
            {
            }

            template<bool is_cancellable, typename TReceiver>
            CPPSTREAM_INLINE void pull (TReceiver && receiver) const
            {
                source.pull<is_cancellable> (
                        [this, r = std::forward<TReceiver> (receiver)] (auto v)
                        {
                            r (map (std::move (v)));

                            return true;
                        });
            }
        };

        template<typename TMapPredicate>
        struct map_sink
        {
            TMapPredicate   map ;

            CPPSTREAM_SINK (map_sink);

            CPPSTREAM_INLINE map_sink (TMapPredicate && map) noexcept
                : map (std::move (map))
            {
            }

            template<typename TSource>
            CPPSTREAM_INLINE auto bind (TSource && source) const
            {
                using source_type   = typename cleanup_type<TSource>::type;
                return map_source<source_type, TMapPredicate> (
                        std::forward<TSource> (source)
                    ,   map
                    );
            }
        };

        // --------------------------------------------------------------------

        template<typename TSource, typename TFilterPredicate>
        struct filter_source
        {
            using value_type    = typename TSource::value_type;

            TSource             source  ;
            TFilterPredicate    filter  ;

            CPPSTREAM_SOURCE (filter_source);

            CPPSTREAM_INLINE filter_source (TSource source, TFilterPredicate filter) noexcept
                : source (std::move (source))
                , filter (std::move (filter))
            {
            }

            template<bool is_cancellable, typename TReceiver>
            CPPSTREAM_INLINE void pull (TReceiver && receiver) const
            {
                source.pull<is_cancellable> (
                        [this, r = std::forward<TReceiver> (receiver)] (auto v)
                        {
                            if (filter (v))
                            {
                                r (std::move (v));
                            }

                            return true;
                        });
            }
        };

        template<typename TFilterPredicate>
        struct filter_sink
        {
            TFilterPredicate    filter  ;

            CPPSTREAM_SINK (filter_sink);

            CPPSTREAM_INLINE filter_sink (TFilterPredicate filter) noexcept
                : filter (std::move (filter))
            {
            }

            template<typename TSource>
            CPPSTREAM_INLINE auto bind (TSource && source) const
            {
                using source_type   = typename cleanup_type<TSource>::type;
                return filter_source<source_type, TFilterPredicate> (
                        std::forward<TSource> (source)
                    ,   filter
                    );
            }
        };

        // --------------------------------------------------------------------

        struct vector_sink
        {
            std::size_t capacity;

            CPPSTREAM_SINK (vector_sink);

            CPPSTREAM_INLINE vector_sink (std::size_t capacity)
                :   capacity (capacity)
            {
            }

            template<typename TSource>
            CPPSTREAM_INLINE auto bind (TSource && source) const
            {
                using source_type   = typename cleanup_type<TSource>::type                          ;
                using value_type    = typename cleanup_type<typename source_type::value_type>::type ;

                std::vector<value_type> result;
                result.reserve (capacity);

                std::forward<TSource> (source).pull<false> (
                        [&result] (auto v)
                        {
                            result.push_back (std::move (v));
                            return true;
                        });

                return result;
            }
        };

        // --------------------------------------------------------------------

        struct sum_sink
        {
            CPPSTREAM_SINK (sum_sink);

            sum_sink ()  = default;

            template<typename TSource>
            CPPSTREAM_INLINE auto bind (TSource && source) const
            {
                using source_type   = typename cleanup_type<TSource>::type                          ;
                using value_type    = typename cleanup_type<typename source_type::value_type>::type ;

                auto sum = value_type ();

                std::forward<TSource> (source).pull<false> (
                        [&sum] (auto v)
                        {
                            sum += v;
                            return true;
                        });

                return sum;
            }
        };

        // --------------------------------------------------------------------

        template<typename TForeachPredicate>
        struct foreach_sink
        {
            TForeachPredicate foreach;

            CPPSTREAM_SINK (foreach_sink);

            CPPSTREAM_INLINE foreach_sink (TForeachPredicate foreach) noexcept
                : foreach (std::move (foreach))
            {
            }


            template<typename TSource>
            CPPSTREAM_INLINE auto bind (TSource && source) const
            {
                using source_type   = typename cleanup_type<TSource>::type;

                std::forward<TSource> (source).pull<false> (
                        [this] (auto v)
                        {
                            foreach (v);

                            return true;
                        });
            }
        };

        // --------------------------------------------------------------------
    }

    template<typename TValue>
    CPPSTREAM_INLINE auto from_range (TValue begin, TValue end)
    {
        return details::range_source<TValue> (std::move (begin), std::move (end));
    }

    template<typename TIterator>
    CPPSTREAM_INLINE auto from_iterators (TIterator begin, TIterator end)
    {
        return details::iterator_source<TIterator> (std::move (begin), std::move (end));
    }

    template<typename TArray>
    CPPSTREAM_INLINE auto from_array (TArray & a)
    {
        auto rank           = std::extent<TArray>::value;
        auto begin          = a                         ;
        auto end            = begin + rank              ;

        return from_iterators (std::move (begin), std::move (end));
    }

    template<typename TMapPredicate>
    CPPSTREAM_INLINE auto map (TMapPredicate map)
    {
        return details::map_sink<TMapPredicate> (std::move (map));
    }

    template<typename TFilterPredicate>
    CPPSTREAM_INLINE auto filter (TFilterPredicate filter)
    {
        return details::filter_sink<TFilterPredicate> (std::move (filter));
    }

    CPPSTREAM_INLINE auto to_vector (std::size_t capacity = 16U)
    {
        return details::vector_sink (capacity);
    }

    CPPSTREAM_INLINE auto to_sum ()
    {
        return details::sum_sink ();
    }

    template<typename TForeachPredicate>
    CPPSTREAM_INLINE auto foreach (TForeachPredicate foreach)
    {
        return details::foreach_sink<TForeachPredicate> (std::move (foreach));
    }

}
// ----------------------------------------------------------------------------
#endif  // CPPSTREAM__HEADER_GUARD
// ----------------------------------------------------------------------------
