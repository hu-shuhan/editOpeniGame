#ifndef iGameLsDynaReader_h
#define iGameLsDynaReader_h

#include "iGameFileReader.h"

IGAME_NAMESPACE_BEGIN

/**
 * @class   LsDynaReader
 * @brief   LS-DYNA d3plot 结果读取器。
 *
 * d3plot 是 LS-DYNA 的二进制结果格式，通常为一族无扩展名文件：
 *   d3plot, d3plot01, d3plot02, ...
 * 本读取器不直接解析二进制，而是后台调用外部转换器
 *   lsdyna_to_pvd_converter.exe --input <d3plot> --output <out.pvd>
 * 将其转换为 PVD + VTU 时序，再通过 iGamePVDReader 读回。
 */
class LsDynaReader : public FileReader {
public:
    I_OBJECT(LsDynaReader);
    static Pointer New() { return new LsDynaReader; }

    bool Parsing() override;

protected:
    LsDynaReader() = default;
    ~LsDynaReader() override = default;
};

IGAME_NAMESPACE_END
#endif
