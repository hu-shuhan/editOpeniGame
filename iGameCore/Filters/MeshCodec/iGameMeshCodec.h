#ifndef MeshCodec_h
#define MeshCodec_h

#include "MeshCodec/Utils/iGameMeshCodecParamSet.h"
#include "iGameFilter.h"
#include "iGameMacro.h"

IGAME_NAMESPACE_BEGIN

// 在 Utils/iGameMeshCodecThread.h 中启用 IGAME_CODEC_SINGLE_THREAD 以启用单线程模式
class MeshCodec : public Filter {
public:
    I_OBJECT(MeshCodec);
    bool Execute() override { return true; }

protected:
    MeshCodec() = default;
    CodecParameters m_codecParams;
    bool m_enableMultiThread = true;

};
IGAME_NAMESPACE_END
#endif