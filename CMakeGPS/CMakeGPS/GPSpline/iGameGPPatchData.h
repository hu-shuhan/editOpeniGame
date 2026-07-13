#pragma once

#include "Util/iGameGP_Macros.h"

GPSTART

    struct GPPatchData
    {
        real_t *m_HHprime_p = nullptr;

        real_t *m_H_q_extended = nullptr;
        real_t *m_Hprime_q_extended = nullptr;

        real_t *m_HHprime_p_C_x = nullptr;
        real_t *m_HHprime_p_C_y = nullptr;
        real_t *m_HHprime_p_C_z = nullptr;

        real_t *m_result_SSv_x = nullptr;
        real_t *m_result_SSv_y = nullptr;
        real_t *m_result_SSv_z = nullptr;

        real_t *m_result_Su0_x = nullptr;
        real_t *m_result_Su0_y = nullptr;
        real_t *m_result_Su0_z = nullptr;

        real_t * m_position_ptr = nullptr;
        uint32_t m_position_offset = INVALID32;
        uint32_t m_position_size = INVALID32;

        real_t * m_normal_ptr = nullptr;
        uint32_t m_normal_offset = INVALID32;
        uint32_t m_normal_size = INVALID32;


        real_t* m_HHprime_p_s = nullptr;

        real_t* m_H_q_extended_s = nullptr;
        real_t* m_Hprime_q_extended_s = nullptr;

        real_t* m_HHprime_p_C_x_s = nullptr;
        real_t* m_HHprime_p_C_y_s = nullptr;
        real_t* m_HHprime_p_C_z_s = nullptr;

        real_t* m_result_SSv_x_s = nullptr;
        real_t* m_result_SSv_y_s = nullptr;
        real_t* m_result_SSv_z_s = nullptr;

        real_t* m_result_Su0_x_s = nullptr;
        real_t* m_result_Su0_y_s = nullptr;
        real_t* m_result_Su0_z_s = nullptr;
    };

GPEND