#ifndef BELLESUB_HPP
#define BELLESUB_HPP

#include "makeconstiterator.hpp"

#include <concepts>
#include <ranges>
#include <cassert>

//*************************************************************
// class belleviews::sub_view
// 
// A C++ view
// with the following benefits compared to C++ standard views
// - Always propagates const
// Because
// - This view yields const iterators when it is const
// OPEN/TODO:
// - propgates const
// - borrowed?
//*************************************************************
namespace belleviews {

enum class subrange_kind : bool { unsized, sized };

template<std::input_or_output_iterator _It, std::sentinel_for<_It> _Sent = _It,
         subrange_kind _Kind = std::sized_sentinel_for<_Sent, _It> ? subrange_kind::sized
                                                              : subrange_kind::unsized>
requires (_Kind == subrange_kind::sized || !std::sized_sentinel_for<_Sent, _It>)
class sub_view : public std::ranges::view_interface<sub_view<_It, _Sent, _Kind>>
{
    private:
      static constexpr bool _S_store_size
        = _Kind == subrange_kind::sized && !std::sized_sentinel_for<_Sent, _It>;

      //friend struct views::_Drop; // Needs to inspect _S_store_size.

      _It _M_begin = _It();
      [[no_unique_address]] _Sent _M_end = _Sent();

      using __size_type
        = _intern::make_unsigned_like_t<std::iter_difference_t<_It>>;

      template<typename, bool = _S_store_size>
        struct _Size
        { };

      template<typename _Tp>
        struct _Size<_Tp, true>
        { _Tp _M_size; };

      [[no_unique_address]] _Size<__size_type> _M_size = {};

    public:
      sub_view() requires std::default_initializable<_It> = default;

      constexpr
      sub_view(_intern::convertible_to_non_slicing<_It> auto __i, _Sent __s)
      noexcept(std::is_nothrow_constructible_v<_It, decltype(__i)>
               && std::is_nothrow_constructible_v<_Sent, _Sent&>)
        requires (!_S_store_size)
      : _M_begin(std::move(__i)), _M_end(__s)
      { }

      constexpr
      sub_view(_intern::convertible_to_non_slicing<_It> auto __i, _Sent __s,
               __size_type __n)
      noexcept(std::is_nothrow_constructible_v<_It, decltype(__i)>
               && std::is_nothrow_constructible_v<_Sent, _Sent&>)
        requires (_Kind == subrange_kind::sized)
      : _M_begin(std::move(__i)), _M_end(__s)
      {
        if constexpr (_S_store_size)
          _M_size._M_size = __n;
      }

      template<_intern::different_from<sub_view> _Rng>
        requires std::ranges::borrowed_range<_Rng>
          && _intern::convertible_to_non_slicing<std::ranges::iterator_t<_Rng>, _It>
          && std::convertible_to<std::ranges::sentinel_t<_Rng>, _Sent>
        constexpr
        sub_view(_Rng&& __r)
        noexcept(noexcept(sub_view(__r, std::ranges::size(__r))))
        requires _S_store_size && std::ranges::sized_range<_Rng>
        : sub_view(__r, std::ranges::size(__r))
        { }

      template<_intern::different_from<sub_view> _Rng>
        requires std::ranges::borrowed_range<_Rng>
          && _intern::convertible_to_non_slicing<std::ranges::iterator_t<_Rng>, _It>
          && std::convertible_to<std::ranges::sentinel_t<_Rng>, _Sent>
        constexpr
        sub_view(_Rng&& __r)
        noexcept(noexcept(sub_view(std::ranges::begin(__r), std::ranges::end(__r))))
        requires (!_S_store_size)
        : sub_view(std::ranges::begin(__r), std::ranges::end(__r))
        { }

      template<std::ranges::borrowed_range _Rng>
        requires _intern::convertible_to_non_slicing<std::ranges::iterator_t<_Rng>, _It>
          && std::convertible_to<std::ranges::sentinel_t<_Rng>, _Sent>
        constexpr
        sub_view(_Rng&& __r, __size_type __n)
        noexcept(noexcept(sub_view(std::ranges::begin(__r), std::ranges::end(__r), __n)))
        requires (_Kind == subrange_kind::sized)
        : sub_view{std::ranges::begin(__r), std::ranges::end(__r), __n}
        { }

      template<_intern::different_from<sub_view> _PairLike>
        requires _intern::pair_like_convertible_from<_PairLike, const _It&,
                                                        const _Sent&>
        constexpr
        operator _PairLike() const
        { return _PairLike(_M_begin, _M_end); }

      // begin():
      constexpr _It begin() requires std::copyable<_It> {
        return _M_begin;
      }
      constexpr auto begin() const requires std::copyable<_It> {
        return make_const_iterator(_M_begin);
      }

      [[nodiscard]] constexpr _It begin() requires (!std::copyable<_It>) {
        return std::move(_M_begin); 
      }

      // end():
      constexpr _Sent end() {
        return _M_end;
      }
      constexpr auto end() const {
        return make_const_sentinel(_M_end); 
      }

      constexpr bool empty() const { return _M_begin == _M_end; }

      constexpr __size_type
      size() const requires (_Kind == subrange_kind::sized)
      {
        if constexpr (_S_store_size)
          return _M_size._M_size;
        else
          return _intern::to_unsigned_like(_M_end - _M_begin);
      }

      [[nodiscard]] constexpr sub_view
      next(std::iter_difference_t<_It> __n = 1) const &
        requires std::forward_iterator<_It>
      {
        auto __tmp = *this;
        __tmp.advance(__n);
        return __tmp;
      }

      [[nodiscard]] constexpr sub_view
      next(std::iter_difference_t<_It> __n = 1) &&
      {
        advance(__n);
        return std::move(*this);
      }

      [[nodiscard]] constexpr sub_view
      prev(std::iter_difference_t<_It> __n = 1) const
        requires std::bidirectional_iterator<_It>
      {
        auto __tmp = *this;
        __tmp.advance(-__n);
        return __tmp;
      }

      constexpr sub_view&
      advance(std::iter_difference_t<_It> __n)
      {
        // _GLIBCXX_RESOLVE_LIB_DEFECTS
        // 3433. sub_view::advance(n) has UB when n < 0
        if constexpr (std::bidirectional_iterator<_It>)
          if (__n < 0)
            {
              std::ranges::advance(_M_begin, __n);
              if constexpr (_S_store_size)
                _M_size._M_size += _intern::to_unsigned_like(-__n);
              return *this;
            }

        __glibcxx_assert(__n >= 0);
        auto __d = __n - std::ranges::advance(_M_begin, __n, _M_end);
        if constexpr (_S_store_size)
          _M_size._M_size -= _intern::to_unsigned_like(__d);
        return *this;
      }
};

template<std::input_or_output_iterator _It, std::sentinel_for<_It> _Sent>
  sub_view(_It, _Sent) -> sub_view<_It, _Sent>;

template<std::input_or_output_iterator _It, std::sentinel_for<_It> _Sent>
    sub_view(_It, _Sent,
             _intern::make_unsigned_like_t<std::iter_difference_t<_It>>)
  -> sub_view<_It, _Sent, subrange_kind::sized>;

template<std::ranges::borrowed_range _Rng>
    sub_view(_Rng&&)
  -> sub_view<std::ranges::iterator_t<_Rng>, std::ranges::sentinel_t<_Rng>,
              (std::ranges::sized_range<_Rng>
               || std::sized_sentinel_for<std::ranges::sentinel_t<_Rng>, std::ranges::iterator_t<_Rng>>)
              ? subrange_kind::sized : subrange_kind::unsized>;

template<std::ranges::borrowed_range _Rng>
    sub_view(_Rng&&,
             _intern::make_unsigned_like_t<std::ranges::range_difference_t<_Rng>>)
  -> sub_view<std::ranges::iterator_t<_Rng>, std::ranges::sentinel_t<_Rng>, subrange_kind::sized>;

template<size_t _Num, class _It, class _Sent, subrange_kind _Kind>
    requires (_Num < 2)
    constexpr auto
get(const sub_view<_It, _Sent, _Kind>& __r)
    {
      if constexpr (_Num == 0)
        return __r.begin();
      else
        return __r.end();
    }

template<size_t _Num, class _It, class _Sent, subrange_kind _Kind>
    requires (_Num < 2)
    constexpr auto
get(sub_view<_It, _Sent, _Kind>&& __r)
    {
      if constexpr (_Num == 0)
        return __r.begin();
      else
        return __r.end();
    }




} // namespace ranges


// ALWAYS BORROWED VIEW:
template<typename It, typename Sent, belleviews::subrange_kind Kind>
inline constexpr bool std::ranges::enable_borrowed_range<belleviews::sub_view<It, Sent, Kind>> = true;

//TODO:
//template<range _Range>
//  using borrowed_subrange_t = __conditional_t<borrowed_range<_Range>,
//                                                sub_view<std::ranges::iterator_t<_Range>>,
//                                                dangling>;


//*************************************************************
// belleviews::sub()
// bel::views::sub()
// 
// A C++ sub_view factory for the belleviews::sub_view
//*************************************************************
namespace belleviews {

namespace _intern {
  template<typename It, typename Sent>
  concept can_sub_view = requires { sub_view(std::declval<It>(), std::declval<Sent>()); };
}

struct _Sub {
   // for: bel::views::sub(beg, end)
   template<typename It, typename Sent>
   //requires ???
   constexpr auto
   operator() [[nodiscard]] (It&& it, Sent&& sent) const {
     return sub_view{std::forward<It>(it), std::forward<Sent>(sent)};
   }
};

inline constexpr _Sub sub;

} // namespace belleviews

namespace bel::views {
  inline constexpr belleviews::_Sub sub;
}

// allows to use bel::subrange instead of belleviews::sub_view
namespace bel {
  template<std::input_or_output_iterator _It, std::sentinel_for<_It> _Sent = _It,
           belleviews::subrange_kind _Kind = std::sized_sentinel_for<_Sent, _It>
                                              ? belleviews::subrange_kind::sized
                                              : belleviews::subrange_kind::unsized>
  requires (_Kind == belleviews::subrange_kind::sized || !std::sized_sentinel_for<_Sent, _It>)
  using subrange = belleviews::sub_view<_It, _Sent, _Kind>;
}

#endif // BELLESUB_HPP
