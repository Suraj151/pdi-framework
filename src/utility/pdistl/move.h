/***************************** PDI STD File ***********************************
This file is taken from external sources. this is taken from open source uClibc++ 
library available on github. below is reference link. Thanking to author for
providing this .

This is free software. you can redistribute it and/or modify it but without any
warranty.

referred from   : https://github.com/mike-matera/ArduinoSTL
added Date      : 1st Dec 2024
added by        : Suraj I.
******************************************************************************/

#ifndef _PDISTD_MOVE_H
#define _PDISTD_MOVE_H 1

#include "type_traits"

namespace pdistd
{

  // Used, in C++03 mode too, by allocators, etc.
  /**
   *  @brief Same as C++11 pdistd::addressof
   *  @ingroup utilities
   */
  template <typename _Tp>
  inline constexpr _Tp *
  __addressof(_Tp &__r) noexcept
  {
    return __builtin_addressof(__r);
  }

} // namespace

namespace pdistd
{

  /**
   *  @addtogroup utilities
   *  @{
   */

  /**
   *  @brief  Forward an lvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template <typename _Tp>
  constexpr _Tp &&
  forward(typename pdistd::remove_reference<_Tp>::type &__t) _UCXX_NOEXCEPT
  {
    return static_cast<_Tp &&>(__t);
  }

  /**
   *  @brief  Forward an rvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template <typename _Tp>
  constexpr _Tp &&
  forward(typename pdistd::remove_reference<_Tp>::type &&__t) _UCXX_NOEXCEPT
  {
    static_assert(!pdistd::is_lvalue_reference<_Tp>::value, "template argument"
                                                            " substituting _Tp is an lvalue reference type");
    return static_cast<_Tp &&>(__t);
  }

  /**
   *  @brief  Convert a value to an rvalue.
   *  @param  __t  A thing of arbitrary type.
   *  @return The parameter cast to an rvalue-reference to allow moving it.
   */
  template <typename _Tp>
  constexpr typename pdistd::remove_reference<_Tp>::type &&
  move(_Tp &&__t) _UCXX_NOEXCEPT
  {
    return static_cast<typename pdistd::remove_reference<_Tp>::type &&>(__t);
  }

  // template <typename _Tp>
  // struct __move_if_noexcept_cond
  //     : public __and_<__not_<is_nothrow_move_constructible<_Tp>>,
  //                     is_copy_constructible<_Tp>>::type
  // {
  // };

  /**
   *  @brief  Conditionally convert a value to an rvalue.
   *  @param  __x  A thing of arbitrary type.
   *  @return The parameter, possibly cast to an rvalue-reference.
   *
   *  Same as std::move unless the type's move constructor could throw and the
   *  type is copyable, in which case an lvalue-reference is returned instead.
   */
  // template<typename _Tp>
  //   constexpr typename
  //   conditional<__move_if_noexcept_cond<_Tp>::value, const _Tp&, _Tp&&>::type
  //   move_if_noexcept(_Tp& __x) _UCXX_NOEXCEPT
  //   { return pdistd::move(__x); }

  // declval, from type_traits.

  /**
   *  @brief Returns the actual address of the object or function
   *         referenced by r, even in the presence of an overloaded
   *         operator&.
   *  @param  __r  Reference to an object or function.
   *  @return   The actual address.
   */
  template <typename _Tp>
  inline constexpr _Tp *
  addressof(_Tp &__r) noexcept
  {
    return pdistd::__addressof(__r);
  }

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // 2598. addressof works on temporaries
  template <typename _Tp>
  const _Tp *addressof(const _Tp &&) = delete;

  // C++11 version of pdistd::exchange for internal use.
  template <typename _Tp, typename _Up = _Tp>
  constexpr inline _Tp
  __exchange(_Tp &__obj, _Up &&__new_val)
  {
    _Tp __old_val = pdistd::move(__obj);
    __obj = pdistd::forward<_Up>(__new_val);
    return __old_val;
  }

  /// @} group utilities

#define _ULIBCXX_MOVE(__val) pdistd::move(__val)
#define _ULIBCXX_FORWARD(_Tp, __val) pdistd::forward<_Tp>(__val)

  /**
   *  @addtogroup utilities
   *  @{
   */

  /**
   *  @brief Swaps two values.
   *  @param  __a  A thing of arbitrary type.
   *  @param  __b  Another thing of arbitrary type.
   *  @return   Nothing.
   */
  // template <typename _Tp>
  // constexpr inline
  //     // typename enable_if<__and_<__not_<__is_tuple_like<_Tp>>,
  //     // 	      is_move_constructible<_Tp>,
  //     // 	      is_move_assignable<_Tp>>::value>::type
  // void
  //     swap(_Tp &__a, _Tp &__b)
  // _UCXX_USE_NOEXCEPT
  // // _UCXX_NOEXCEPT_IF(__and_<is_nothrow_move_constructible<_Tp>,
  // //                          is_nothrow_move_assignable<_Tp>>::value)
  // {
  //   _Tp __tmp = _ULIBCXX_MOVE(__a);
  //   __a = _ULIBCXX_MOVE(__b);
  //   __b = _ULIBCXX_MOVE(__tmp);
  // }

  // _ULIBCXX_RESOLVE_LIB_DEFECTS
  // DR 809. std::swap should be overloaded for array types.
  /// Swap the contents of two arrays.
  // template <typename _Tp, size_t _Nm>
  // constexpr inline 
  // void
  // swap(_Tp (&__a)[_Nm], _Tp (&__b)[_Nm])
  // _UCXX_USE_NOEXCEPT
  // // _UCXX_NOEXCEPT_IF(__is_nothrow_swappable<_Tp>::value)
  // {
  //   for (size_t __n = 0; __n < _Nm; ++__n)
  //     swap(__a[__n], __b[__n]);
  // }

} // namespace

#endif /* _PDISTD_MOVE_H */
