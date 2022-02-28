//
// Created by antonio on 31/07/21.
//

#ifndef OHBOI_UTIL_H
#define OHBOI_UTIL_H

#include <type_traits>
#include <cstdint>
#include <cstddef>


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
    struct Mem_range {
        T start_;
        S size_;
    };

    template <typename T, size_t index, size_t length = 1> requires std::is_integral_v<T>
    class Bit_field {
    public:
        template <typename U>
        Bit_field& operator = (U val) {
            value_ = (value_ & ~(mask << index)) | ((val & mask) << index);
            return *this;
        }
        operator T() const {
            return (value_ >> index) & mask;
        }
        explicit operator bool() const {
            return value_ & (mask << index);
        }


        Bit_field& operator ++ () {
            return *this = *this + 1;
        }

        T operator ++ (int) {
            T ret = *this;
            ++*this;
            return ret;
        }

        Bit_field& operator -- () {
            return *this = *this - 1;
        }

        T operator -- (int) {
            T ret = *this;
            --*this;
            return ret;
        }

    private:
        static constexpr uint64_t mask = (1ULL << length) - 1;
        T value_;
    };

    template <typename T, size_t index>
    class Bit_field<T, index, 1> {
    public:
        Bit_field& operator = (bool val) {
            value_ = (value_ & (1 << index)) | (val << index);
            return *this;
        }
        operator T() const {
            return (value_ >> index) & 1;
        }
        explicit operator bool() const {
            return value_ & (1 << index);
        }
    private:
        T value_;
    };
}

#endif //OHBOI_UTIL_H
