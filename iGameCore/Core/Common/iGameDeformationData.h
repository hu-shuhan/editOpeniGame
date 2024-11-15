//
// Created by m_ky on 2024/10/16.
//

/**
 * @class   iGameDeformationData
 * @brief   iGameDeformationData's brief
 */
#pragma once

#include <iGameObject.h>

IGAME_NAMESPACE_BEGIN
class DeformationData : public Object{
public:
    I_OBJECT(DeformationData)

    DeformationData() = default;
    ~DeformationData() = default;

    static DeformationData::Pointer New(){return new DeformationData;}
public:
    void SetScaleFactors(float dsf){ m_deformation_scale_factor_x = m_deformation_scale_factor_y = m_deformation_scale_factor_z = dsf; }
    void SetScaleFactorX(float dsf){ m_deformation_scale_factor_x = dsf;}
    void SetScaleFactorY(float dsf){ m_deformation_scale_factor_y = dsf;}
    void SetScaleFactorZ(float dsf){ m_deformation_scale_factor_z = dsf;}

    void SetAttributeName(const std::string& name){ m_deformation_attribute_name = name;}

public:
    float m_deformation_scale_factor_x = {0.f};
    float m_deformation_scale_factor_y = {0.f};
    float m_deformation_scale_factor_z = {0.f};

    bool m_enable_dsf         {false};
    bool m_enable_auto_compute{false};
    std::string m_deformation_attribute_name;


};
IGAME_NAMESPACE_END