#ifndef BELLEALL_HPP
#define BELLEALL_HPP

#include "makeconstiterator.hpp"

#include <concepts>
#include <ranges>
#include <cassert>

//*************************************************************
// class belleviews::ref_view
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

template<std::ranges::range Rg>
requires std::is_object_v<Rg>
class ref_view : public std::ranges::view_interface<ref_view<Rg>>
{
 private:
  Rg* rgPtr;

  static void _S_fun(Rg&); // not defined
  static void _S_fun(Rg&&) = delete;

 public:
  template<_intern::different_from<ref_view> _Tp>
  requires std::convertible_to<_Tp, Rg&> && requires { _S_fun(std::declval<_Tp>()); }
  constexpr ref_view(_Tp&& __t) noexcept(noexcept(static_cast<Rg&>(std::declval<_Tp>())))
   : rgPtr(std::addressof(static_cast<Rg&>(std::forward<_Tp>(__t)))) {
  }

  constexpr Rg& base() const {
    return *rgPtr; 
  }

  // begin():
  constexpr std::ranges::iterator_t<Rg> begin() { 
    return std::ranges::begin(*rgPtr); 
  }
  constexpr auto begin() const { 
    return make_const_iterator(std::ranges::begin(*rgPtr)); 
  }

  // end():
  constexpr std::ranges::sentinel_t<Rg> end() { 
    return std::ranges::end(*rgPtr); 
  }
  constexpr auto end() const { 
    return make_const_iterator(std::ranges::end(*rgPtr)); 
  }

  constexpr bool empty() const requires requires { std::ranges::empty(*rgPtr); } { 
    return std::ranges::empty(*rgPtr); 
  }

  constexpr auto size() const requires std::ranges::sized_range<Rg> {
    return std::ranges::size(*rgPtr); 
  }

  constexpr auto data() const requires std::ranges::contiguous_range<Rg> {
    return std::ranges::data(*rgPtr);
  }
};

template<typename Rg>
ref_view(Rg&) -> ref_view<Rg>;

} // namespace belleviews

// always borrowed (as with std take_view):
template<typename Rg>
inline constexpr bool std::ranges::enable_borrowed_range<belleviews::ref_view<Rg>> = true;


//*************************************************************
// class belleviews::owning_view
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

template<std::ranges::range Rg>
requires std::movable<Rg> && (!_intern::is_initializer_list<std::remove_cv_t<Rg>>)
class owning_view : public std::ranges::view_interface<owning_view<Rg>>
{
 private:
   Rg rg = Rg();

 public:
   owning_view() requires std::default_initializable<Rg> = default;

   constexpr owning_view(Rg&& __t) noexcept(std::is_nothrow_move_constructible_v<Rg>)
    : rg(std::move(__t)) {
   }

   owning_view(owning_view&&) = default;
   owning_view& operator=(owning_view&&) = default;

   constexpr Rg& base() & noexcept { return rg; }
   constexpr const Rg& base() const& noexcept { return rg; }

   constexpr Rg&& base() && noexcept { return std::move(rg); }
   constexpr const Rg&& base() const&& noexcept { return std::move(rg); }

   // begin():
   constexpr std::ranges::iterator_t<Rg> begin() {
     return std::ranges::begin(rg);
   }

   constexpr auto begin() const requires std::ranges::range<const Rg> {
     return make_const_iterator(std::ranges::begin(rg)); 
   }

   // end():
   constexpr std::ranges::sentinel_t<Rg> end() {
     return std::ranges::end(rg);
   }
   constexpr auto end() const requires std::ranges::range<const Rg> {
     return make_const_iterator(std::ranges::end(rg));
   }

   constexpr bool empty() requires requires { std::ranges::empty(rg); } {
     return std::ranges::empty(rg);
   }

   // empty() and size():
   constexpr bool empty() const requires requires { std::ranges::empty(rg); } {
     return std::ranges::empty(rg);
   }

   constexpr auto size() requires std::ranges::sized_range<Rg> {
     return std::ranges::size(rg);
   }

   constexpr auto size() const requires std::ranges::sized_range<const Rg> {
     return std::ranges::size(rg);
   }

   // data():
   constexpr auto data() requires std::ranges::contiguous_range<Rg> {
     return std::ranges::data(rg);
   }

   constexpr auto data() const requires std::ranges::contiguous_range<const Rg> {
     return std::ranges::data(rg);
   }
};

} // namespace belleviews

template<typename Rg>
inline constexpr bool std::ranges::enable_borrowed_range<belleviews::owning_view<Rg>> = std::ranges::enable_borrowed_range<Rg>;
  


//*************************************************************
// belleviews::all()
// bel::views::all()
// bel::views::all_t
// 
// A C++ view adaptor for different belleviews views
//*************************************************************
namespace belleviews {

namespace _intern {
  template<typename Rg>
  concept can_ref_view = requires { ref_view(std::declval<Rg>()); };

  template<typename Rg>
  concept can_owning_view = requires { owning_view(std::declval<Rg>()); };
}

struct _All {
  // for:  bel::views::all(rg)
  template<std::ranges::viewable_range Rg>
  requires std::ranges::view<Rg> || _intern::can_ref_view<Rg> || _intern::can_owning_view<Rg>
  constexpr auto
  operator() [[nodiscard]] (Rg&& rg) const {
    if constexpr (std::ranges::view<std::decay_t<Rg>>) {
      return std::forward<Rg>(rg);
    }
    else if constexpr (_intern::can_ref_view<Rg>) {
      return ref_view{std::forward<Rg>(rg)};
    }
    else {
      return owning_view{std::forward<Rg>(rg)};
    }
  }
};


inline constexpr _All all;

template<std::ranges::viewable_range Rg>
  using all_t = decltype(all(std::declval<Rg>()));

} // namespace belleviews

namespace bel::views {
  inline constexpr belleviews::_All all;

  template<std::ranges::viewable_range Rg>
    using all_t = decltype(all(std::declval<Rg>()));
}

#endif // BELLEALL_HPP
