#ifndef BELLEDROP_HPP
#define BELLEDROP_HPP

#include "makeconstiterator.hpp"

#include <concepts>
#include <ranges>
#include <cassert>

//*************************************************************
// class belleviews::drop_view
// 
// A C++ view
// with the following benefits compared to C++ standard views
// - Always propagates const
// Because
// - This view yields const iterators when it is const
// OPEN/TODO:
// - ...
//*************************************************************
namespace belleviews {

template<std::ranges::view V>
class drop_view : public std::ranges::view_interface<drop_view<V>>
{
 private:
  V base_ = V();
  std::ranges::range_difference_t<V> count_ = 0;
 public:
  drop_view() requires std::default_initializable<V> = default;

  constexpr drop_view(V v, std::ranges::range_difference_t<V> c)
   : base_(std::move(v)), count_{c} {
      assert(c >= 0);
  }

  constexpr V base() const& requires std::copy_constructible<V> { return base_; }
  constexpr V base() && { return std::move(base_); }

  constexpr auto begin()
  { //requires (!(_intern::simple_view<V> && std::ranges::random_access_range<const V> && std::ranges::sized_range<const V>)) {
    return std::ranges::next(std::ranges::begin(base_), count_,
                             std::ranges::end(base_));
  }
  constexpr auto begin() const
  { //requires std::ranges::random_access_range<const V> && std::ranges::sized_range<const V> {
    return make_const_iterator(std::ranges::next(std::ranges::begin(base_), count_,
                                                 std::ranges::end(base_)));
  }
  constexpr auto end()
  //requires (!_intern::simple_view<V>) {
  {
    return std::ranges::end(base_);
  }
  constexpr auto end() const
  //requires std::ranges::range<const V> {
  {
    return make_const_sentinel(std::ranges::end(base_));
  }

  constexpr auto size()
  requires std::ranges::sized_range<V> {
    const auto s = std::ranges::size(base_);
    const auto c = static_cast<decltype(s)>(count_);
    return s < c ? 0 : s - c;
  }
  constexpr auto size() const
  requires std::ranges::sized_range<const V> {
    const auto s = std::ranges::size(base_);
    const auto c = static_cast<decltype(s)>(count_);
    return s < c ? 0 : s - c;
  }
};

template<typename R>
drop_view(R&&, std::ranges::range_difference_t<R>) -> drop_view<std::views::all_t<R>>;

} // namespace belleviews

// borrowed if underlying range is borrowed (as with std drop_view):
template<typename Rg>
inline constexpr bool std::ranges::enable_borrowed_range<belleviews::drop_view<Rg>> = std::ranges::enable_borrowed_range<Rg>;


//*************************************************************
// belleviews::drop()
// bel::views::drop()
// 
// A C++ drop_view adaptor for the belleviews::drop_view
//*************************************************************
namespace belleviews {

namespace _intern {
  template<typename Rg, typename DiffT>
  concept can_drop_view = requires { drop_view(std::declval<Rg>(), std::declval<DiffT>()); };
}

struct _Drop {
   // for:  bel::views::drop(rg, 2)
   template<std::ranges::viewable_range Rg, typename DiffT = std::ranges::range_difference_t<Rg>>
   requires _intern::can_drop_view<Rg, DiffT>
   constexpr auto
   operator() [[nodiscard]] (Rg&& rg, DiffT diff) const {
     return drop_view{std::forward<Rg>(rg), diff};
   }

   // for:  rg | bel::views::drop(2)
   template<typename T>
   struct PartialDrop {
     T diff;
   };

   template<typename DiffT>
   constexpr auto
   operator() [[nodiscard]] (DiffT diff) const {
     return PartialDrop<DiffT>{diff};
   }

   template<typename Rg, typename DiffT>
   friend constexpr auto
   operator| (Rg&& rg, PartialDrop<DiffT> pd) {
     return drop_view{std::forward<Rg>(rg), pd.diff};
   }
};

inline constexpr _Drop drop;

} // namespace belleviews

namespace bel::views {
  inline constexpr belleviews::_Drop drop;
}

#endif // BELLEDROP_HPP
