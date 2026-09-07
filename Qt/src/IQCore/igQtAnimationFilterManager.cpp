#include <IQCore/igQtAnimationFilterManager.h>

#include <utility>

bool igQtAnimationFilterManager::registerFilter(
        igQtAnimationFilterDescriptor descriptor,
        QString* error) {
    if (descriptor.id.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Filter ID 不能为空。");
        return false;
    }
    if (descriptor.displayName.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Filter 显示名称不能为空。");
        return false;
    }
    if (!descriptor.execute) {
        if (error) *error = QStringLiteral("Filter 必须提供 execute 回调。");
        return false;
    }
    if (m_Filters.contains(descriptor.id)) {
        if (error) {
            *error = QStringLiteral("Filter ID“%1”已经注册。")
                             .arg(descriptor.id);
        }
        return false;
    }

    m_Filters.insert(descriptor.id, std::move(descriptor));
    return true;
}

bool igQtAnimationFilterManager::unregisterFilter(const QString& id) {
    return m_Filters.remove(id) > 0;
}

void igQtAnimationFilterManager::clear() {
    m_Filters.clear();
}

bool igQtAnimationFilterManager::contains(const QString& id) const {
    return m_Filters.contains(id);
}

QList<QString> igQtAnimationFilterManager::filterIds() const {
    return m_Filters.keys();
}

QList<QString> igQtAnimationFilterManager::filterDisplayNames() const {
    QList<QString> names;
    names.reserve(m_Filters.size());
    for (auto it = m_Filters.cbegin(); it != m_Filters.cend(); ++it) {
        names.push_back(it.value().displayName);
    }
    return names;
}

const igQtAnimationFilterDescriptor*
igQtAnimationFilterManager::descriptor(const QString& id) const {
    auto it = m_Filters.constFind(id);
    return it == m_Filters.cend() ? nullptr : &it.value();
}

igQtAnimationFilterParameterSchema
igQtAnimationFilterManager::parameterSchema(
        const QString& id,
        iGame::DataObject::Pointer input,
        QString* error) const {
    const auto* filter = descriptor(id);
    if (!filter) {
        if (error) {
            *error = QStringLiteral("未注册动画 Filter“%1”。").arg(id);
        }
        return {};
    }
    if (!input) {
        if (error) *error = QStringLiteral("动画 Filter 没有输入对象。");
        return {};
    }
    QString supportsError;
    if (filter->supports && !filter->supports(input, supportsError)) {
        if (error) *error = supportsError;
        return {};
    }
    if (!filter->parameterSchema) return {};
    return filter->parameterSchema(input);
}

bool igQtAnimationFilterManager::validateParameters(
        const QString& id,
        const QVariantMap& parameters,
        iGame::DataObject::Pointer input,
        QString& error) const {
    error.clear();
    const auto* filter = descriptor(id);
    if (!filter) {
        error = QStringLiteral("未注册动画 Filter“%1”。").arg(id);
        return false;
    }
    if (!input) {
        error = QStringLiteral("动画 Filter 没有输入对象。");
        return false;
    }
    if (filter->supports && !filter->supports(input, error)) return false;
    if (filter->validateParameters &&
        !filter->validateParameters(parameters, input, error)) {
        return false;
    }
    return true;
}

igQtAnimationFilterResult igQtAnimationFilterManager::execute(
        const QString& id,
        const igQtAnimationFrameContext& context,
        const QVariantMap& parameterSnapshot) const {
    igQtAnimationFilterResult result;
    QString error;
    if (!validateParameters(id, parameterSnapshot, context.input, error)) {
        result.error = error;
        return result;
    }

    const auto* filter = descriptor(id);
    result = filter->execute(context, parameterSnapshot);
    if (result.success && !result.output) {
        result.success = false;
        result.error = QStringLiteral("动画 Filter 执行成功但没有返回输出对象。");
    }
    return result;
}
