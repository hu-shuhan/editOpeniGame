#ifndef IGAMEVIS_IGAMEICODECARCHIVE_H
#define IGAMEVIS_IGAMEICODECARCHIVE_H

#include "iGameMacro.h"
#include <type_traits>

IGAME_NAMESPACE_BEGIN

template<typename Derived>
class ICodecArchive {
public:
    //region Archive 方法检测 -----------------------------------
    template<typename T, typename Archive, typename = void>
    struct HasArchiveMethod : std::false_type {};

    template<typename T, typename Archive>
    struct HasArchiveMethod<T, Archive,
        std::void_t<decltype(std::declval<T&>().Archive(std::declval<Archive&>()))>> : std::true_type {};
    //endregion -----------------------------------

    //region 重载 () 运算符 -----------------------------------
    template<typename T>
    std::enable_if_t<HasArchiveMethod<T, Derived>::value, void>
    operator()(T& target) {
        target.Archive(static_cast<Derived&>(*this));
    }

    template<typename T, typename... Args>
    void operator()(T& first, Args&... rest) {
        static_assert(
            HasArchiveMethod<T, Derived>::value &&
            (... && HasArchiveMethod<Args, Derived>::value),
            "All types must implement Archive method");
        this->operator()(first);
        this->operator()(rest...);
    }
    //endregion -----------------------------------
};

IGAME_NAMESPACE_END

#endif
