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
    void SetAutoCompute(bool enable_auto_compute){m_enable_auto_compute = enable_auto_compute;}
    void SetEnableDeformation(bool enable_enable_deformation){m_enable_dsf = enable_enable_deformation;}


    void SetAttributeName(const std::string& name){ m_deformation_attribute_name = name;}

    float GetScaleFactorX(){return m_deformation_scale_factor_x;}
    float GetScaleFactorY(){return m_deformation_scale_factor_y;}
    float GetScaleFactorZ(){return m_deformation_scale_factor_z;}

    bool GetEnableStatus(){return m_enable_dsf;}
    bool GetAutoComputeStatus(){return m_enable_auto_compute;}
    const std::string& GetDeformationAttributeName() {return m_deformation_attribute_name;}
protected:
    float m_deformation_scale_factor_x = {0.f};
    float m_deformation_scale_factor_y = {0.f};
    float m_deformation_scale_factor_z = {0.f};

    bool m_enable_dsf         {false};
    bool m_enable_auto_compute{false};
    std::string m_deformation_attribute_name;


};
IGAME_NAMESPACE_END