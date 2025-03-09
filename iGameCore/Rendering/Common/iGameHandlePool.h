/**
 * @class    HandlePool
 * @brief    HandlePool类是一个对象管理的工具类，支持动态分配和释放对象的句柄。
 * @par      Copyright(c): Hangzhou Dianzi University, iGame-Lab
 */

#pragma once

#include "iGameObject.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>

IGAME_NAMESPACE_BEGIN

template<typename T>
struct is_smart_pointer : std::false_type {};

template<typename T>
struct is_smart_pointer<SmartPointer<T>> : std::true_type {};

template<typename ObjectType>
class HandlePool : public Object {
public:
    I_OBJECT(HandlePool); ///< 声明对象类型的宏。

    /**
     * @brief 创建一个新的 HandlePool 实例。
     * @return 指向新 HandlePool 实例的指针。
     */
    static Pointer New() { return new HandlePool; }

    /// 定义句柄类型。
    using HandleType = IGuint;

    /// 定义存储句柄到对象的映射类型。
    using MapType = std::unordered_map<HandleType, ObjectType>;

    /// 定义映射的迭代器类型。
    using Iterator = typename MapType::iterator;

    /// 定义映射的常量迭代器类型。
    using ConstIterator = typename MapType::const_iterator;

    // 根据ObjectType类型推导返回类型
    using ReturnType = typename std::conditional<
            is_smart_pointer<ObjectType>::value,
            ObjectType, // 如果是智能指针，返回ObjectType
            ObjectType* // 否则返回指针
            >::type;

    /**
     * @brief 为一个新对象分配句柄。
     * @param object 要分配的对象。
     * @return 分配的句柄。
     */
    HandleType AllocateObject(const ObjectType& object);

    /**
     * @brief 释放指定的句柄及其关联的对象。
     * @param handle 要释放的句柄。
     */
    void ReleaseHandle(HandleType handle);

    /**
     * @brief 检查句柄是否有效。
     * @param handle 要检查的句柄。
     * @return 如果句柄有效返回 true，否则返回 false。
     */
    bool CheckHandle(HandleType handle) const;

    ///**
    // * @brief 获取与句柄关联的对象。
    // * @param handle 要查询的句柄。
    // * @return 指向对象的指针，如果句柄无效则返回 nullptr。
    // */
    //ObjectType* GetObjectByHandle(HandleType handle);

    /**
     * @brief 获取与句柄关联的对象。
     * @param handle 要查询的句柄。
     * @return 返回智能指针（当ObjectType为SmartPointer时）或原生指针。
     */
    ReturnType GetObjectByHandle(HandleType handle);

    /**
     * @brief 返回当前对象池中的对象个数。
     * @return 对象个数。
     */
    HandleType GetObjectCount() const;

    /**
     * @brief 清空句柄池，包括所有分配的句柄和对象。
     */
    void Clear();

    /**
     * @brief 获取对象映射的开始迭代器。
     * @return 开始迭代器。
     */
    Iterator Begin();

    /**
     * @brief 获取对象映射的结束迭代器。
     * @return 结束迭代器。
     */
    Iterator End();

    /**
     * @brief 获取对象映射的常量开始迭代器。
     * @return 常量开始迭代器。
     */
    ConstIterator Begin() const;

    /**
     * @brief 获取对象映射的常量结束迭代器。
     * @return 常量结束迭代器。
     */
    ConstIterator End() const;

protected:
    /**
     * @brief 构造一个 HandlePool 对象。
     */
    HandlePool();

    /**
     * @brief 销毁 HandlePool 对象。
     */
    ~HandlePool() override;

    HandleType m_CurrentHandle;
    std::queue<HandleType> m_FreeHandles;
    std::unordered_set<HandleType> m_ActiveHandles;
    std::unordered_map<HandleType, ObjectType> m_HandleToObject;
};

IGAME_NAMESPACE_END

#include "iGameHandlePool.inl"
