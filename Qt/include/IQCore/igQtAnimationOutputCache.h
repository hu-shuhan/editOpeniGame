#pragma once

#include <IQCore/igQtAnimationFilterTypes.h>
#include <list>

struct igQtAnimationFrameRequest {
    int sourceFrame{0};
    bool interpolate{false};
    float weight{0};
    int outputFrame{-1};

    bool operator==(const igQtAnimationFrameRequest& other) const {
        return sourceFrame == other.sourceFrame && outputFrame == other.outputFrame && interpolate == other.interpolate &&
               (!interpolate || weight == other.weight);
    }
};

// Owns successful final outputs only. The producer includes loading/interpolation
// and the whole pipeline, so neither disk IO nor filters run on a cache hit.
// The owner clears this cache when the source or pipeline configuration changes.
class IG_QT_MODULE_EXPORT igQtAnimationOutputCache {
public:
    using Producer = std::function<igQtAnimationFilterResult()>;
    void setCapacity(int frames);
    int capacity() const { return m_Capacity; }
    int size() const { return static_cast<int>(m_Entries.size()); }
    void clear() { m_Entries.clear(); }
    igQtAnimationFilterResult resolve(const igQtAnimationFrameRequest& request,
                                      const Producer& produce);

private:
    struct Entry {
        igQtAnimationFrameRequest request;
        igQtAnimationFilterResult result;
    };
    int m_Capacity{0};
    std::list<Entry> m_Entries;
};
