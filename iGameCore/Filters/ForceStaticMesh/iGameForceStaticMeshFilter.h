#pragma once

#include "iGameFilter.h"
#include "iGameDataObject.h"
#include "iGameType.h"

#include <string>
#include <vector>

IGAME_NAMESPACE_BEGIN

/**
 * @class   iGameForceStaticMeshFilter
 * @brief
 *          静态网格缓存管理（Force Static Mesh）。
 *
 *          首次执行时深拷贝输入网格，建立「几何缓存」；之后只要几何规模
 *          （点数 / 单元数）不变，就只把新的属性数据搬运到缓存上，输出始终是
 *          同一份几何固定的网格。适用于「几何固定、属性随时间变化」的数据
 *          （如 ODB 逐帧结果、各时间步网格完全相同的多文件序列）。
 *
 *          两种执行入口：
 *          - Execute():交互式执行（GUI 菜单）。输入对象改变、块结构改变、
 *            规模变化或 ForceCacheComputation 为 true 时重建几何缓存。
 *          - ExecuteAtTimeStep(frameIndex, timeValue)：时序执行，由界面在时间步
 *            变化后调用。此时输入的每一帧对象虽然是新读入的对象，但只要规模一致
 *            就沿用已有几何缓存（几何固定来自建立缓存的那一帧），仅更新属性；
 *            规模变化时自动重建该块并给出提示。
 */
class ForceStaticMeshFilter : public Filter {
public:
    I_OBJECT(ForceStaticMeshFilter)
    static Pointer New() { return new ForceStaticMeshFilter; }

    /** 上一次执行时几何缓存的重建原因。 */
    enum RebuildReason {
        RR_NONE = 0,      // 未重建几何，仅更新属性
        RR_FIRST_BUILD,   // 首次建立缓存
        RR_INPUT_CHANGED, // 输入对象 / 块结构改变
        RR_FORCED,        // ForceCacheComputation 强制重建
        RR_SIZE_CHANGED,  // 点数 / 单元数变化，自动重建
    };

    /** 交互式执行：使用输入对象的当前状态更新 / 建立缓存。 */
    bool Execute() override;

    /**
     * @brief 时序执行：界面切换时间步后调用（输入已被更新到该时间步）。
     * @param frameIndex 当前时间步下标（仅用于记录「缓存几何来自第几帧」，可传 -1）
     * @param timeValue  当前时间步的时间值
     */
    bool ExecuteAtTimeStep(int frameIndex, float timeValue);

    /** 强制重建几何缓存（不复用已有几何）。 */
    void SetForceCacheComputation(bool on);
    bool GetForceCacheComputation() const;

    /**
     * @brief 关闭静态缓存：输出直接透传输入对象，几何随输入变化。
     *        用于在变形网格上对比「固定几何」与「跟随几何」两种表现。
     */
    void SetStaticCacheEnabled(bool on);
    bool GetStaticCacheEnabled() const;

    /* ---- 缓存状态查询（供界面显示） ---- */
    bool HasCache() const;
    int GetNumberOfCacheBlocks() const;
    /** 缓存几何建立时的时间步下标；-1 表示非时序（手动建立）。 */
    int GetCacheTimeStepIndex() const;
    /** 缓存几何建立时的时间值。 */
    float GetCacheTimeValue() const;
    IGsize GetCacheNumberOfPoints() const;
    IGsize GetCacheNumberOfCells() const;
    /** 上一次执行是否重建了几何缓存。 */
    bool WasCacheRebuilt() const;
    RebuildReason GetLastRebuildReason() const;
    /** 上一次执行的文字说明（界面提示用）。 */
    const std::string& GetStatusMessage() const;
    /** 例如「缓存几何来自 t=0（第 1 帧）：27 点 / 8 单元」。 */
    std::string GetCacheDescription() const;

protected:
    ForceStaticMeshFilter();
    ~ForceStaticMeshFilter() override = default;

    /** 缓存中的一个数据块：几何固定，属性随执行更新。 */
    struct CacheBlock {
        DataObject::Pointer Cache; // 缓存块（几何 + 当前属性）
        IGsize PointCount{0};      // 建立几何缓存时的点数
        IGsize CellCount{0};       // 建立几何缓存时的单元数
        int TimeStepIndex{-1};     // 该块几何来自哪个时间步
        float TimeValue{0.f};
    };

    bool ExecuteInternal(bool timeSeriesMode, int frameIndex, float timeValue);

    /* 输入的数据块：带子对象的容器返回各子块，否则返回输入自身 */
    static std::vector<DataObject::Pointer> CollectSourceBlocks(const DataObject::Pointer& input);
    static IGsize CountPoints(const DataObject::Pointer& obj);
    static IGsize CountCells(const DataObject::Pointer& obj);
    /* 深拷贝输入网格，生成独立的新网格作为静态缓存块 */
    static DataObject::Pointer CloneMesh(const DataObject::Pointer& input);
    /* 在已有缓存块上原地替换几何与属性（保持对象身份，界面无需替换模型）。
       类型不一致时返回 false，由调用方改为新建对象。 */
    static bool RebuildMeshInPlace(const DataObject::Pointer& cache, const DataObject::Pointer& src);
    /* 仅把源块的属性数据复制到缓存块，几何保持缓存不动 */
    static bool UpdateAttributes(const DataObject::Pointer& src, const DataObject::Pointer& cache);
    /* 属性 / 几何更新后标记对象与渲染数据已修改 */
    static void MarkObjectModified(const DataObject::Pointer& obj);

    /* 用 src 重建 block 的几何缓存（同时更新属性） */
    void RebuildBlock(CacheBlock& block, const DataObject::Pointer& src,
                      int timeStepIndex, float timeValue);
    /* 多块时维护输出的容器结构与缓存块一一对应 */
    bool SyncContainerChildren();
    /* 容器输出的属性集与首块属性对齐（模型树 / 云图按父容器属性下标寻址） */
    static void SyncContainerAttributes(const DataObject::Pointer& container);

protected:
    std::vector<CacheBlock> m_Blocks;
    DataObject::Pointer m_Cache;       // 输出对象（单块：块自身；多块：容器）
    DataObject::Pointer m_CachedInput; // 上次执行对应的输入对象（识别手动切换输入）
    bool m_CacheInitialized{false};
    bool m_ForceCacheComputation{false};
    bool m_StaticCacheEnabled{true};
    bool m_LastRebuilt{false};
    int m_LastRebuildReason{RR_NONE};
    std::string m_StatusMessage;
    /* 缓存几何是否带时间步上下文（用于「缓存几何来自 t=...」的措辞） */
    bool m_CacheHasTimeStep{false};
};

IGAME_NAMESPACE_END
