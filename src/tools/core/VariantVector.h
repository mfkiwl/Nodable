#pragma once
#include <typeindex>
#include <list>
#include <vector>
#include <deque>
#include <unordered_set>
#include <variant>
#include <iterator> // for forward_iterator, see append
#include "Signals.h"
#include "Hash.h"
#include "Variant.h"

namespace tools
{
    template<typename ...Args>
    struct VariantVector
    {
        using element_t = Variant<Args...>;

        enum EventT
        {
            EventT_Append,
            EventT_Remove,
        };

        SIGNAL(on_change, EventT, element_t);

        bool empty() const
        {
            return _unique_elem.empty();
        }

        bool contains(const element_t& elem) const // Constant on average, worst case linear in the size of the container.
        {
            return _unique_elem.contains( elem.hash() );
        }

        const std::list<element_t>& data() const
        {
            return _ordered_elem;
        }

        void clear() // O(n)
        {
            if ( _unique_elem.empty() )
            {
                return;
            }

            for( const element_t& elem : _ordered_elem )
            {
                on_change.emit( EventT_Remove, elem );
            }
            _unique_elem.clear();
            _ordered_elem.clear();
            _count_by_index.clear();
        }

        template<class Iterator> size_t append( Iterator begin, Iterator end )
        {
            size_t count = 0;

            for(auto it = begin; it != end; ++it)
                if ( append( *it ) )
                    ++count;

            return count;
        }

        template<class T> bool contains() const // O(1), read from cache.
        {
            constexpr size_t index = element_t::template index_of<T>();
            return _count_by_index.contains( index );
        }

        template<class T> size_t count() const // O(1), read from cache.
        {
            constexpr size_t index = element_t::template index_of<T>();
            if ( _count_by_index.contains( index ) )
            {
                return _count_by_index.at( index );
            }
            return 0;
        }

        template<typename T>
        bool append(T* ptr)
        {
            element_t elem{ptr};
            return append( elem );
        }

        template<typename T>
        bool append(T& data)
        {
            return append(element_t{data});
        }

        bool append(element_t& elem)  // Constant on average, worst case linear in the size of the container.
        {
            const auto& [_, inserted] = _unique_elem.insert( elem.hash() );
            if ( inserted )
            {
                _ordered_elem.push_back( elem );
                on_change.emit( EventT_Append, elem );
                _count_by_index[elem.index()]++;

                ASSERT( _unique_elem.contains( elem.hash() ) );
                return true;
            }
            return false;
        }

        bool remove(const element_t& elem)// Constant on average, worst case linear in the size of the container.
        {
            if ( _unique_elem.erase( elem.hash() ) )
            {
                auto it = std::find( _ordered_elem.begin(), _ordered_elem.end(), elem );
                on_change.emit( EventT_Remove, elem );
                _ordered_elem.erase( it );
                _count_by_index[it->index()]--;
                ASSERT( !contains(elem) );
                return true;
            }
            return false;
        }

        template<class T>
        T first_of() const // O(n), I suggest you to use contains() once first
        {
            const size_t _count = count<T>();
            if ( _count == 0 )
                return {};

            for ( const element_t& elem : _ordered_elem )
                if ( T ptr = elem.template get_if<T>() )
                    return ptr;

            return nullptr;
        }

        template<class T>
        std::vector<T> collect() const // O(n), do only a single allocation when necessary
        {
            const size_t _count = count<T>();
            if ( _count == 0 )
                return {};

            std::vector<T> result;
            const std::type_index type_index{ typeid(T) };
            result.reserve( _count ); // 1 allocation max :)

            // OPTIM: we could use a cache per type_index if necessary ( type_index => list<T*> )
            for ( const element_t& elem : _ordered_elem )
                if ( T data = elem.template get_if<T>() )
                    result.push_back( data );

            return result;
        }
    private:
        struct NoHash
        {
            // simply pass the value as-is, our u32_t is already a hash
            constexpr u32_t operator()(u32_t u) const
            { return u; }
        };

        std::unordered_set<u32_t, NoHash>  _unique_elem{};  // unordered_set because we need uniqueness, furthermore unordered_set::contains is faster than std::find on a list
        std::list<element_t>               _ordered_elem{}; // list, because it " supports constant time insertion and removal of elements from anywhere in the container." (see https://devdocs.io/cpp/container/list)
        std::unordered_map<size_t, size_t> _count_by_index{};
    };
}