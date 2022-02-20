//
// Created by antonio on 31/07/21.
//

#ifndef OHBOI_UTIL_H
#define OHBOI_UTIL_H

#include <type_traits>

namespace gb::util {
    template<typename T>
    struct is_unsigned_integral : std::integral_constant<bool, std::is_integral_v<T> && std::is_unsigned_v<T>> {};

    template<typename T>
    inline constexpr auto is_unsigned_integral_v =
            is_unsigned_integral<T>::value;

    template <typename, typename = void>
    struct is_address : std::false_type {};

    template<typename T>
    struct is_address<T, std::enable_if_t<is_unsigned_integral_v<T>> > : std::true_type {};

    template <typename T>
    struct is_address<T,std::enable_if_t<std::is_enum_v<T> >> : is_address<std::underlying_type_t<T> > {};

    template<typename T>
    inline constexpr auto is_address_v = is_address<T>::value;

    template <typename T, typename S = T, std::enable_if_t<is_address_v<T> && is_address_v<S>, bool> = true>
    struct span {
        T start_;
        S size_;
    };
}

#endif //OHBOI_UTIL_H
