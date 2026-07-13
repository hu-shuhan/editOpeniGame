#pragma once
#include <tuple>
namespace gpmesh {
namespace detail {

template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())>
{
};

template <typename ClassType, typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType (ClassType::*)(Args...) const>
{
    enum
    {
        arity = sizeof...(Args)
    };

    typedef ReturnType result_type;

    template <size_t i>
    struct arg
    {
        using type_rc =
            typename std::tuple_element<i, std::tuple<Args...>>::type;
        using type_c = std::conditional_t<std::is_reference_v<type_rc>,
                                          std::remove_reference_t<type_rc>,
                                          type_rc>;
        using type   = std::conditional_t<std::is_const_v<type_c>,
                                        std::remove_const_t<type_c>,
                                        type_c>;
    };
};

}    
}    
