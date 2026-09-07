#include <IQCore/igQtAnimationFilterAdapters.h>
#include <IQCore/igQtAnimationFilterManager.h>

#include <Contour/iGameContourFilter.h>
#include <IsoVolume/iGameIsoVolumeFilter.h>
#include <iGameType.h>
#include <iGameUnstructuredMesh.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto ContourFilterId = "contour";
constexpr auto IsoVolumeFilterId = "isoVolume";
constexpr auto ScalarNameKey = "scalarName";
constexpr auto ScalarDimensionKey = "scalarDimension";
constexpr auto IsoValueKey = "isoValue";
constexpr auto LowerValueKey = "lowerValue";
constexpr auto UpperValueKey = "upperValue";

iGame::ArrayObject::Pointer findPointAttribute(
        iGame::DataObject::Pointer object, const QString& name) {
    if (!object || !object->GetAttributeSet()) return nullptr;
    auto attributes = object->GetAttributeSet()->GetAllPointAttributes();
    if (!attributes) return nullptr;

    for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
        auto array = attributes->GetElement(i).pointer;
        if (array && QString::fromStdString(array->GetName()) == name) {
            return array;
        }
    }
    return nullptr;
}

iGame::DataObject::Pointer parameterSource(iGame::DataObject::Pointer input) {
    if (!input) return nullptr;
    if (input->GetAttributeSet()) {
        auto attributes = input->GetAttributeSet()->GetAllPointAttributes();
        if (attributes && attributes->GetNumberOfElements() > 0) return input;
    }
    for (auto it = input->SubDataObjectIteratorBegin();
         it != input->SubDataObjectIteratorEnd(); ++it) {
        auto object = iGame::DynamicCast<iGame::DataObject>(it->second);
        auto attributes = object && object->GetAttributeSet()
                                  ? object->GetAttributeSet()->GetAllPointAttributes()
                                  : nullptr;
        if (attributes && attributes->GetNumberOfElements() > 0) return object;
    }
    return nullptr;
}

bool readContourParameters(const QVariantMap& parameters,
                           QString& scalarName,
                           int& dimension,
                           double& isoValue,
                           QString& error) {
    scalarName = parameters.value(QString::fromLatin1(ScalarNameKey)).toString();
    bool dimensionOk = false;
    dimension = parameters.value(QString::fromLatin1(ScalarDimensionKey))
                        .toInt(&dimensionOk);
    bool isoOk = false;
    isoValue = parameters.value(QString::fromLatin1(IsoValueKey)).toDouble(&isoOk);

    if (scalarName.trimmed().isEmpty()) {
        error = QStringLiteral("请选择等值面使用的标量属性。");
        return false;
    }
    if (!dimensionOk || dimension < 0) {
        error = QStringLiteral("等值面分量必须是非负整数。");
        return false;
    }
    if (!isoOk || !std::isfinite(isoValue)) {
        error = QStringLiteral("等值必须是有效数字。");
        return false;
    }
    return true;
}

bool readIsoVolumeParameters(const QVariantMap& parameters,
                             QString& scalarName,
                             int& dimension,
                             double& lowerValue,
                             double& upperValue,
                             QString& error) {
    scalarName = parameters.value(QString::fromLatin1(ScalarNameKey)).toString();
    bool dimensionOk = false;
    dimension = parameters.value(QString::fromLatin1(ScalarDimensionKey))
                        .toInt(&dimensionOk);
    bool lowerOk = false;
    lowerValue = parameters.value(QString::fromLatin1(LowerValueKey))
                         .toDouble(&lowerOk);
    bool upperOk = false;
    upperValue = parameters.value(QString::fromLatin1(UpperValueKey))
                         .toDouble(&upperOk);

    if (scalarName.trimmed().isEmpty()) {
        error = QStringLiteral("请选择等值体使用的标量属性。");
        return false;
    }
    if (!dimensionOk || dimension < 0) {
        error = QStringLiteral("等值体分量必须是非负整数。");
        return false;
    }
    if (!lowerOk || !upperOk || !std::isfinite(lowerValue) ||
        !std::isfinite(upperValue)) {
        error = QStringLiteral("等值体的下限和上限必须是有效数字。");
        return false;
    }
    if (lowerValue > upperValue) {
        error = QStringLiteral("等值体下限不能大于上限。");
        return false;
    }
    return true;
}

bool validateObject(iGame::DataObject::Pointer object,
                    const QString& scalarName,
                    int dimension,
                    QString& error) {
    auto scalar = findPointAttribute(object, scalarName);
    if (!scalar) {
        error = QStringLiteral("找不到点属性“%1”。").arg(scalarName);
        return false;
    }
    if (dimension >= scalar->GetDimension()) {
        error = QStringLiteral("分量 %1 超出属性“%2”的维度范围。")
                        .arg(dimension)
                        .arg(scalarName);
        return false;
    }
    return true;
}

bool validateInput(iGame::DataObject::Pointer input,
                   const QString& scalarName,
                   int dimension,
                   QString& error) {
    if (!input->HasSubDataObject()) {
        return validateObject(input, scalarName, dimension, error);
    }

    int subIndex = 0;
    for (auto it = input->SubDataObjectIteratorBegin();
         it != input->SubDataObjectIteratorEnd(); ++it, ++subIndex) {
        auto object = iGame::DynamicCast<iGame::DataObject>(it->second);
        if (!object) continue;
        QString localError;
        if (!validateObject(object, scalarName, dimension, localError)) {
            error = QStringLiteral("子对象 %1：%2")
                            .arg(subIndex + 1)
                            .arg(localError);
            return false;
        }
    }
    return true;
}

bool executeOne(iGame::DataObject::Pointer object,
                const QString& scalarName,
                int dimension,
                double isoValue,
                iGame::UnstructuredMesh::Pointer& output,
                QString& error) {
    auto scalar = findPointAttribute(object, scalarName);
    if (!scalar) {
        error = QStringLiteral("找不到点属性“%1”。").arg(scalarName);
        return false;
    }

    auto filter = iGame::ContourFilter::New();
    filter->SetInput(object);
    filter->SetIsoScalarData(scalar, isoValue, dimension);
    if (!filter->Execute()) {
        error = QStringLiteral("等值面执行失败。");
        return false;
    }

    output = filter->GetContourMesh();
    if (!output || output->GetNumberOfCells() == 0) output = nullptr;
    return true;
}

bool executeIsoVolumeOne(iGame::DataObject::Pointer object,
                         const QString& scalarName,
                         int dimension,
                         double lowerValue,
                         double upperValue,
                         iGame::UnstructuredMesh::Pointer& output,
                         QString& error) {
    auto scalar = findPointAttribute(object, scalarName);
    if (!scalar) {
        error = QStringLiteral("找不到点属性“%1”。").arg(scalarName);
        return false;
    }

    auto filter = iGame::IsoVolumeFilter::New();
    filter->SetInput(object);
    filter->SetIsoScalarData(scalar, lowerValue, upperValue, dimension);
    if (!filter->Execute()) {
        error = QStringLiteral("等值体执行失败。");
        return false;
    }

    output = filter->GetOutputMesh();
    if (!output || output->GetNumberOfCells() == 0) output = nullptr;
    return true;
}

void appendPointAttributeParameters(
        iGame::DataObject::Pointer input,
        igQtAnimationFilterParameterSchema& schema,
        double& defaultLower,
        double& defaultUpper) {
    auto source = parameterSource(input);
    auto attributes = source && source->GetAttributeSet()
                              ? source->GetAttributeSet()->GetAllPointAttributes()
                              : nullptr;

    QStringList scalarNames;
    int maximumDimension = 0;
    defaultLower = 0.0;
    defaultUpper = 1.0;
    if (attributes) {
        for (int i = 0; i < attributes->GetNumberOfElements(); ++i) {
            auto array = attributes->GetElement(i).pointer;
            if (!array) continue;
            scalarNames.push_back(QString::fromStdString(array->GetName()));
            maximumDimension = std::max(maximumDimension, array->GetDimension());
        }

        if (attributes->GetNumberOfElements() > 0) {
            auto& attribute = attributes->GetElement(0);
            auto range = attribute.GetDataRange();
            if (range) {
                if (range->GetNumberOfElements() >= 4) {
                    defaultLower = range->GetValue(2);
                    defaultUpper = range->GetValue(3);
                } else if (range->GetNumberOfElements() >= 2) {
                    defaultLower = range->GetValue(0);
                    defaultUpper = range->GetValue(1);
                }
            }
        }
    }

    QStringList dimensions;
    for (int i = 0; i < maximumDimension; ++i) {
        dimensions.push_back(QString::number(i));
    }

    schema.push_back({QString::fromLatin1(ScalarNameKey),
                      QStringLiteral("标量属性"),
                      igQtAnimationFilterParameterType::Choice,
                      scalarNames.value(0), {}, {}, scalarNames});
    schema.push_back({QString::fromLatin1(ScalarDimensionKey),
                      QStringLiteral("分量"),
                      igQtAnimationFilterParameterType::Choice,
                      QStringLiteral("0"), {}, {}, dimensions});
}

QString framePrefix(const igQtAnimationFrameContext& context) {
    return context.outputFrameIndex >= 0
                   ? QStringLiteral("第 %1 帧").arg(context.outputFrameIndex + 1)
                   : QStringLiteral("当前帧");
}

} // namespace

igQtAnimationFilterDescriptor igQtCreateContourAnimationFilterDescriptor() {
    igQtAnimationFilterDescriptor descriptor;
    descriptor.id = QString::fromLatin1(ContourFilterId);
    descriptor.displayName = QStringLiteral("等值面（Contour）");
    descriptor.outputPolicy = igQtAnimationFilterOutputPolicy::ReplaceFrame;

    descriptor.supports = [](iGame::DataObject::Pointer input, QString& error) {
        if (!parameterSource(input)) {
            error = QStringLiteral("当前动画帧没有可用于等值面的点属性。");
            return false;
        }
        return true;
    };

    descriptor.parameterSchema = [](iGame::DataObject::Pointer input) {
        igQtAnimationFilterParameterSchema schema;
        double unusedLower = 0.0;
        double unusedUpper = 0.0;
        appendPointAttributeParameters(input, schema, unusedLower, unusedUpper);
        schema.push_back({QString::fromLatin1(IsoValueKey),
                          QStringLiteral("等值"),
                          igQtAnimationFilterParameterType::Double,
                          0.0, {}, {}, {}});
        return schema;
    };

    descriptor.validateParameters = [](
            const QVariantMap& parameters,
            iGame::DataObject::Pointer input,
            QString& error) {
        QString scalarName;
        int dimension = 0;
        double isoValue = 0.0;
        if (!readContourParameters(parameters, scalarName, dimension,
                                   isoValue, error)) {
            return false;
        }
        return validateInput(input, scalarName, dimension, error);
    };

    descriptor.execute = [](
            const igQtAnimationFrameContext& context,
            const QVariantMap& parameters) {
        igQtAnimationFilterResult result;
        QString scalarName;
        int dimension = 0;
        double isoValue = 0.0;
        if (!readContourParameters(parameters, scalarName, dimension,
                                   isoValue, result.error)) {
            return result;
        }

        auto input = context.input;
        if (!input->HasSubDataObject()) {
            iGame::UnstructuredMesh::Pointer contour = nullptr;
            if (!executeOne(input, scalarName, dimension, isoValue,
                            contour, result.error)) {
                result.error = framePrefix(context) + QStringLiteral("：") + result.error;
                return result;
            }
            if (!contour) {
                result.error = QStringLiteral("%1的等值 %2 未与任何单元相交。")
                                       .arg(framePrefix(context))
                                       .arg(QString::number(isoValue, 'g', 12));
                return result;
            }
            contour->SetName(input->GetName() + "_AnimationContour");
            contour->SetShellRenderingOption(false);
            contour->SetViewStyle(IG_SURFACE);
            result.output = contour;
            result.success = true;
            return result;
        }

        auto container = iGame::UnstructuredMesh::New();
        container->SetName(input->GetName() + "_AnimationContour");
        container->SetAttributeSet(input->GetAttributeSet());
        container->SetShellRenderingOption(false);
        container->SetViewStyle(IG_SURFACE);

        int subIndex = 0;
        for (auto it = input->SubDataObjectIteratorBegin();
             it != input->SubDataObjectIteratorEnd(); ++it, ++subIndex) {
            auto object = iGame::DynamicCast<iGame::DataObject>(it->second);
            if (!object) continue;
            iGame::UnstructuredMesh::Pointer contour = nullptr;
            QString error;
            if (!executeOne(object, scalarName, dimension, isoValue,
                            contour, error)) {
                result.error = QStringLiteral("%1，子对象 %2：%3")
                                       .arg(framePrefix(context))
                                       .arg(subIndex + 1)
                                       .arg(error);
                return result;
            }
            if (contour) {
                contour->SetShellRenderingOption(false);
                contour->SetViewStyle(IG_SURFACE);
                container->AddSubDataObject(contour);
            }
        }

        if (!container->HasSubDataObject()) {
            result.error = QStringLiteral("%1的等值 %2 未与任何单元相交。")
                                   .arg(framePrefix(context))
                                   .arg(QString::number(isoValue, 'g', 12));
            return result;
        }

        result.output = container;
        result.success = true;
        return result;
    };

    return descriptor;
}

igQtAnimationFilterDescriptor
igQtCreateIsoVolumeAnimationFilterDescriptor() {
    igQtAnimationFilterDescriptor descriptor;
    descriptor.id = QString::fromLatin1(IsoVolumeFilterId);
    descriptor.displayName = QStringLiteral("等值体（IsoVolume）");
    descriptor.outputPolicy = igQtAnimationFilterOutputPolicy::ReplaceFrame;

    descriptor.supports = [](iGame::DataObject::Pointer input, QString& error) {
        if (!parameterSource(input)) {
            error = QStringLiteral("当前动画帧没有可用于等值体的点属性。");
            return false;
        }
        return true;
    };

    descriptor.parameterSchema = [](iGame::DataObject::Pointer input) {
        igQtAnimationFilterParameterSchema schema;
        double rangeMinimum = 0.0;
        double rangeMaximum = 1.0;
        appendPointAttributeParameters(input, schema, rangeMinimum, rangeMaximum);
        if (rangeMaximum < rangeMinimum) std::swap(rangeMinimum, rangeMaximum);
        const double span = rangeMaximum - rangeMinimum;
        const double lowerDefault = rangeMinimum + span / 3.0;
        const double upperDefault = rangeMinimum + span * 2.0 / 3.0;
        schema.push_back({QString::fromLatin1(LowerValueKey),
                          QStringLiteral("下限"),
                          igQtAnimationFilterParameterType::Double,
                          lowerDefault, {}, {}, {}});
        schema.push_back({QString::fromLatin1(UpperValueKey),
                          QStringLiteral("上限"),
                          igQtAnimationFilterParameterType::Double,
                          upperDefault, {}, {}, {}});
        return schema;
    };

    descriptor.validateParameters = [](
            const QVariantMap& parameters,
            iGame::DataObject::Pointer input,
            QString& error) {
        QString scalarName;
        int dimension = 0;
        double lowerValue = 0.0;
        double upperValue = 0.0;
        if (!readIsoVolumeParameters(parameters, scalarName, dimension,
                                     lowerValue, upperValue, error)) {
            return false;
        }
        return validateInput(input, scalarName, dimension, error);
    };

    descriptor.execute = [](
            const igQtAnimationFrameContext& context,
            const QVariantMap& parameters) {
        igQtAnimationFilterResult result;
        QString scalarName;
        int dimension = 0;
        double lowerValue = 0.0;
        double upperValue = 0.0;
        if (!readIsoVolumeParameters(parameters, scalarName, dimension,
                                     lowerValue, upperValue, result.error)) {
            return result;
        }

        auto input = context.input;
        if (!input->HasSubDataObject()) {
            iGame::UnstructuredMesh::Pointer output = nullptr;
            if (!executeIsoVolumeOne(input, scalarName, dimension,
                                     lowerValue, upperValue, output,
                                     result.error)) {
                result.error = framePrefix(context) + QStringLiteral("：") +
                               result.error;
                return result;
            }
            if (!output) {
                result.error = QStringLiteral("%1的区间 [%2, %3] 没有产生等值体。")
                                       .arg(framePrefix(context))
                                       .arg(QString::number(lowerValue, 'g', 12))
                                       .arg(QString::number(upperValue, 'g', 12));
                return result;
            }
            output->SetName(input->GetName() + "_AnimationIsoVolume");
            output->SetShellRenderingOption(false);
            output->SetViewStyle(IG_SURFACE);
            result.output = output;
            result.success = true;
            return result;
        }

        auto container = iGame::UnstructuredMesh::New();
        container->SetName(input->GetName() + "_AnimationIsoVolume");
        container->SetAttributeSet(input->GetAttributeSet());
        container->SetShellRenderingOption(false);
        container->SetViewStyle(IG_SURFACE);

        int subIndex = 0;
        for (auto it = input->SubDataObjectIteratorBegin();
             it != input->SubDataObjectIteratorEnd(); ++it, ++subIndex) {
            auto object = iGame::DynamicCast<iGame::DataObject>(it->second);
            if (!object) continue;
            iGame::UnstructuredMesh::Pointer output = nullptr;
            QString error;
            if (!executeIsoVolumeOne(object, scalarName, dimension,
                                     lowerValue, upperValue, output, error)) {
                result.error = QStringLiteral("%1，子对象 %2：%3")
                                       .arg(framePrefix(context))
                                       .arg(subIndex + 1)
                                       .arg(error);
                return result;
            }
            if (output) {
                output->SetShellRenderingOption(false);
                output->SetViewStyle(IG_SURFACE);
                container->AddSubDataObject(output);
            }
        }

        if (!container->HasSubDataObject()) {
            result.error = QStringLiteral("%1的区间 [%2, %3] 没有产生等值体。")
                                   .arg(framePrefix(context))
                                   .arg(QString::number(lowerValue, 'g', 12))
                                   .arg(QString::number(upperValue, 'g', 12));
            return result;
        }

        result.output = container;
        result.success = true;
        return result;
    };

    return descriptor;
}

bool igQtRegisterBuiltinAnimationFilters(
        igQtAnimationFilterManager& manager, QString* error) {
    if (!manager.registerFilter(
                igQtCreateContourAnimationFilterDescriptor(), error)) {
        return false;
    }
    return manager.registerFilter(
            igQtCreateIsoVolumeAnimationFilterDescriptor(), error);
}
