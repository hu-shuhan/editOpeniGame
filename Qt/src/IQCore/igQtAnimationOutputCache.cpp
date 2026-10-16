#include <IQCore/igQtAnimationOutputCache.h>
#include <algorithm>

void igQtAnimationOutputCache::setCapacity(int frames) {
    m_Capacity = std::max(0, frames);
    while (size() > m_Capacity) m_Entries.pop_back();
}

igQtAnimationFilterResult igQtAnimationOutputCache::resolve(
        const igQtAnimationFrameRequest& request, const Producer& produce) {
    auto found = std::find_if(m_Entries.begin(), m_Entries.end(),
                             [&](const Entry& entry) { return entry.request == request; });
    if (found != m_Entries.end()) {
        m_Entries.splice(m_Entries.begin(), m_Entries, found);
        return m_Entries.front().result;
    }
    auto result = produce();
    if (m_Capacity > 0 && result.success && result.output) {
        m_Entries.push_front({request, result});
        while (size() > m_Capacity) m_Entries.pop_back();
    }
    return result;
}
