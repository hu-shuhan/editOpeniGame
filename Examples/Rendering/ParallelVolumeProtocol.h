#pragma once
// ParallelVolumeProtocol.h — 并行体绘制 C/S 模式（阶段 6）共享协议与跨平台 socket 工具。
//
// 对标 UnifiedVersion 的 MiniPVServer.cpp / MiniPVClient.cpp：渲染后端（MPI 多 rank，
// 跑在超算计算节点）开放 TCP 端口，前端（客户端，跑在登录节点/本地机器）连接后拿到
// 合成图并回传交互命令。与参考实现不同的是，这里不依赖 Qt/VTK，客户端复用阶段 5 的
// GLFW 交互窗口（ParallelVolumeInteractive.h 里的显示辅助代码）。
//
// 协议（字节序统一按小端，两侧均为 x86_64）：
//   1) 握手元数据（连接建立后，server -> client，固定长度）：
//        uint32 magic   = 0x50564D31 ("PVM1")
//        double scalarMin
//        double scalarMax
//        uint8  colorbar[256*4]      // RGBA8，传输函数颜色 ramp（仅颜色，alpha 恒 255）
//      客户端用它重建 colorbar 渐变纹理与 min/mid/max 刻度标签。
//   2) 控制命令（client -> server，UTF-8 文本，'\n' 结尾）：
//        INTERACT <seq> <dAzimRad> <dElevRad> <zoomFactor> <interactiveFlag>\n
//          增量式：seq 为客户端单调递增的命令序号（用于 RTT 测量，服务端在回帧里原样
//          带回）；dAzimRad/dElevRad 为本次帧间轨道旋转增量（弧度），zoomFactor 为距离
//          缩放因子（0.9=拉近，1.1=拉远），interactiveFlag 0=松手/高清，1=拖动/低清。
//        NEXT\n / PREV\n
//          多帧播放切帧（N 键下一帧 / P 键上一帧，越界自动回绕）；服务端切帧后渲染一帧回传。
//        EXIT\n
//   3) 帧（server -> client，二进制，每次合成后一帧）：
//        uint32 magic    = 0x50564631 ("PVF1")
//        int32  width
//        int32  height
//        int32  codec    = 0（raw RGBA8）/ 1（zlib 压缩 RGBA8）
//        uint32 seq      = 触发本帧的 INTERACT 命令 seq（初始帧/无命令 = 0）
//        int32  payloadSize = width*height*4
//        uint8  payload[payloadSize]   // 不透明 RGBA8（合成结果，已叠背景色）
//
// 说明：codec=0 走 raw RGBA8（实现最小）；codec=1 走 zlib(compress2) 压缩的 RGBA8，服务端
// 压缩后更小才用 codec=1，否则退回 codec=0；客户端按 codec 用 uncompress 解压。帧头布局
// 保持不变。若将来需要更高压缩比，可再引入 PNG（server 用 stb_image_write、client 用
// stb_image），协议仅需新增 codec 值即可。

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 跨平台 socket（Windows: Winsock2；Linux: POSIX）。参考 iGameSocketConnection.h 的
// 平台分支写法，仅做纯函数封装，不引入类/线程，便于在示例里内联使用。
// ---------------------------------------------------------------------------
#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
using PVSocket = SOCKET;
#  define PV_INVALID_SOCKET INVALID_SOCKET
#  define PV_SOCKET_ERROR SOCKET_ERROR
#  define PV_EWOULDBLOCK WSAEWOULDBLOCK
#  define PV_EINTR WSAEINTR
#else
#  include <arpa/inet.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
using PVSocket = int;
#  define PV_INVALID_SOCKET (-1)
#  define PV_SOCKET_ERROR (-1)
#  define PV_EWOULDBLOCK EWOULDBLOCK
#  define PV_EINTR EINTR
#endif

namespace iGamePVNet {

// ---------------------------------------------------------------------------
// 协议常量
// ---------------------------------------------------------------------------
inline constexpr std::uint32_t kMetadataMagic = 0x50564D31u; // "PVM1"
inline constexpr std::uint32_t kFrameMagic = 0x50564631u;     // "PVF1"
inline constexpr int kColorbarSize = 256;                    // colorbar ramp 的像素数
inline constexpr int kColorbarBytes = kColorbarSize * 4;      // RGBA8
inline constexpr int kFieldNameMaxLen = 256;                  // 字段名最大字节数（握手元数据）
inline constexpr std::uint32_t kCodecRawRGBA = 0;             // 帧编码：raw RGBA8
inline constexpr std::uint32_t kCodecZlib = 1;                // 帧编码：zlib(compress2) 压缩的 RGBA8

// 元数据（握手）二进制长度：magic + width + height + numFrames + scalarMin/Max +
// colorbar + fieldName。
inline constexpr std::size_t kMetadataBytes =
        sizeof(std::uint32_t) + 3 * sizeof(std::int32_t) + 2 * sizeof(double) +
        kColorbarBytes + kFieldNameMaxLen;
// 帧头长度：magic + width + height + codec + seq + frameIndex + payloadSize + roiX + roiY + roiW + roiH。
inline constexpr std::size_t kFrameHeaderBytes =
        2 * sizeof(std::uint32_t) + 9 * sizeof(std::int32_t);

// ---------------------------------------------------------------------------
// 平台初始化 / 错误码
// ---------------------------------------------------------------------------
inline int LastError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

// 幂等初始化；Windows 需要 WSAStartup，Linux 无需。可重复调用。
inline bool Startup() {
#ifdef _WIN32
    static bool s_started = []() {
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();
    return s_started;
#else
    return true;
#endif
}

inline void Cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

inline void CloseSocket(PVSocket s) {
    if (s == PV_INVALID_SOCKET) { return; }
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

// 建立到 <host>:<port> 的 TCP 连接。host 支持主机名（如超算计算节点 "cn78021"）或
// 数字 IP（如 "127.0.0.1"）。对标 MiniPVClient 的 QTcpSocket::connectToHost：内部用
// getaddrinfo 做主机名解析（inet_pton 只能解析数字 IP，无法解析 cn 节点名）。
// 成功返回已连接的 socket，失败返回 PV_INVALID_SOCKET。
inline PVSocket ConnectTo(const char* host, int port) {
    const std::string portStr = std::to_string(port);

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;      // 同时支持 IPv4 / IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* res = nullptr;
    if (getaddrinfo(host, portStr.c_str(), &hints, &res) != 0 || res == nullptr) {
        return PV_INVALID_SOCKET;
    }

    PVSocket sock = PV_INVALID_SOCKET;
    for (const addrinfo* ai = res; ai != nullptr; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock == PV_INVALID_SOCKET) { continue; }
#ifdef _WIN32
        const int addrLen = static_cast<int>(ai->ai_addrlen);
#else
        const socklen_t addrLen = ai->ai_addrlen;
#endif
        if (connect(sock, ai->ai_addr, addrLen) == 0) {
            break; // 连接成功
        }
        CloseSocket(sock);
        sock = PV_INVALID_SOCKET;
    }
    freeaddrinfo(res);
    return sock;
}

// ---------------------------------------------------------------------------
// 阻塞收发原语
// ---------------------------------------------------------------------------
// 循环 send 直到发完所有字节；失败返回 false。
inline bool SendAll(PVSocket s, const void* buf, std::size_t bytes) {
    const char* p = static_cast<const char*>(buf);
    std::size_t left = bytes;
    while (left > 0) {
#ifdef _WIN32
        const int n = send(s, p, static_cast<int>(left), 0);
#else
        const ssize_t n = send(s, p, left, MSG_NOSIGNAL);
#endif
        if (n == PV_SOCKET_ERROR) {
            const int e = LastError();
            if (e == PV_EINTR) { continue; }
            return false;
        }
        p += n;
        left -= static_cast<std::size_t>(n);
    }
    return true;
}

// 循环 recv 直到收满 bytes 字节；失败（含对端关闭）返回 false。
inline bool RecvFull(PVSocket s, void* buf, std::size_t bytes) {
    char* p = static_cast<char*>(buf);
    std::size_t left = bytes;
    while (left > 0) {
#ifdef _WIN32
        const int n = recv(s, p, static_cast<int>(left), 0);
#else
        const ssize_t n = recv(s, p, left, 0);
#endif
        if (n == 0) { return false; } // 对端关闭
        if (n == PV_SOCKET_ERROR) {
            const int e = LastError();
            if (e == PV_EINTR) { continue; }
            return false;
        }
        p += n;
        left -= static_cast<std::size_t>(n);
    }
    return true;
}

// 阻塞读一行（直到遇到 '\n'）；返回 false 表示断开/出错。
inline bool RecvLine(PVSocket s, std::string& line) {
    line.clear();
    char c;
    while (true) {
        if (!RecvFull(s, &c, 1)) { return false; }
        line.push_back(c);
        if (c == '\n') { break; }
    }
    return true;
}

// 非阻塞排空当前 socket 里可读的数据到 buf（用于批量合并命令，避免积压）。
// 用 select(0 超时) 探测可读性，保证 Windows/Linux 行为一致、不阻塞。
// 返回 false 表示断开；无更多可读数据（超时/其它错误）视为排空完成返回 true。
inline bool DrainNonBlock(PVSocket s, std::string& buf) {
    char tmp[8192];
    while (true) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);
        timeval tv{0, 0};
        const int ready = select(static_cast<int>(s) + 1, &rfds, nullptr,
                                 nullptr, &tv);
        if (ready <= 0) { return true; } // 超时或出错：视为无更多数据
#ifdef _WIN32
        const int n = recv(s, tmp, sizeof(tmp), 0);
#else
        const ssize_t n = recv(s, tmp, sizeof(tmp), 0);
#endif
        if (n > 0) {
            buf.append(tmp, static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0) { return false; } // 对端关闭
        const int e = LastError();
        if (e == PV_EINTR) { continue; }
        return true; // 其它错误：视为排空完成
    }
}

// 从 buf 中弹出首行（含 '\n' 之前的内容）；无完整行返回 false。
inline bool PopLine(std::string& buf, std::string& line) {
    const std::size_t pos = buf.find('\n');
    if (pos == std::string::npos) { return false; }
    line = buf.substr(0, pos);
    buf.erase(0, pos + 1);
    return true;
}

// ---------------------------------------------------------------------------
// 协议数据包收发
// ---------------------------------------------------------------------------

// 握手元数据：渲染分辨率 + 帧数（多帧播放用）+ 标量范围 + colorbar ramp + 字段名。
struct Metadata {
    int width{0};
    int height{0};
    int numFrames{1};
    double scalarMin{0.0};
    double scalarMax{1.0};
    unsigned char colorbar[kColorbarBytes]{};
    char fieldName[kFieldNameMaxLen]{};
};

// 发送握手元数据（server -> client，连接建立后调用一次）。
inline bool SendMetadata(PVSocket s, const Metadata& meta) {
    // 小端序按字段逐一发送（x86_64 原生即小端，两侧一致）。
    std::uint32_t magic = kMetadataMagic;
    std::int32_t w = meta.width;
    std::int32_t h = meta.height;
    std::int32_t nf = meta.numFrames;
    if (!SendAll(s, &magic, sizeof(magic))) { return false; }
    if (!SendAll(s, &w, sizeof(w))) { return false; }
    if (!SendAll(s, &h, sizeof(h))) { return false; }
    if (!SendAll(s, &nf, sizeof(nf))) { return false; }
    if (!SendAll(s, &meta.scalarMin, sizeof(meta.scalarMin))) { return false; }
    if (!SendAll(s, &meta.scalarMax, sizeof(meta.scalarMax))) { return false; }
    if (!SendAll(s, meta.colorbar, kColorbarBytes)) { return false; }
    return SendAll(s, meta.fieldName, kFieldNameMaxLen);
}

// 接收握手元数据（client 端）。返回 false 表示断开或 magic 不匹配。
inline bool RecvMetadata(PVSocket s, Metadata& meta) {
    std::uint32_t magic = 0;
    std::int32_t w = 0, h = 0, nf = 1;
    if (!RecvFull(s, &magic, sizeof(magic))) { return false; }
    if (magic != kMetadataMagic) { return false; }
    if (!RecvFull(s, &w, sizeof(w))) { return false; }
    if (!RecvFull(s, &h, sizeof(h))) { return false; }
    if (!RecvFull(s, &nf, sizeof(nf))) { return false; }
    if (!RecvFull(s, &meta.scalarMin, sizeof(meta.scalarMin))) { return false; }
    if (!RecvFull(s, &meta.scalarMax, sizeof(meta.scalarMax))) { return false; }
    if (!RecvFull(s, meta.colorbar, kColorbarBytes)) { return false; }
    if (!RecvFull(s, meta.fieldName, kFieldNameMaxLen)) { return false; }
    meta.fieldName[kFieldNameMaxLen - 1] = '\0'; // 防御：保证以 '\0' 结尾
    meta.width = w;
    meta.height = h;
    meta.numFrames = nf;
    return true;
}

// 发送一帧（通用）：codec 指明 payload 编码，payload 为「ROI 子矩形」的已编码字节。
// roi* 为 payload 在全帧中的位置（codec=0 时 payload 为 roiW*roiH*4 字节的 raw RGBA8）。
// frameIndex 为当前帧序号（多帧播放时显示用，0 起始）。
inline bool SendFramePayload(PVSocket s, int width, int height,
                             std::uint32_t codec,
                             std::int32_t roiX, std::int32_t roiY,
                             std::int32_t roiW, std::int32_t roiH,
                             const void* payload, std::int32_t payloadSize,
                             std::uint32_t seq, std::int32_t frameIndex = 0) {
    const std::uint32_t magic = kFrameMagic;
    const std::int32_t w = width;
    const std::int32_t h = height;
    const std::int32_t c = static_cast<std::int32_t>(codec);
    const std::int32_t ps = payloadSize;

    if (!SendAll(s, &magic, sizeof(magic))) { return false; }
    if (!SendAll(s, &w, sizeof(w))) { return false; }
    if (!SendAll(s, &h, sizeof(h))) { return false; }
    if (!SendAll(s, &c, sizeof(c))) { return false; }
    if (!SendAll(s, &seq, sizeof(seq))) { return false; }
    if (!SendAll(s, &frameIndex, sizeof(frameIndex))) { return false; }
    if (!SendAll(s, &ps, sizeof(ps))) { return false; }
    if (!SendAll(s, &roiX, sizeof(roiX))) { return false; }
    if (!SendAll(s, &roiY, sizeof(roiY))) { return false; }
    if (!SendAll(s, &roiW, sizeof(roiW))) { return false; }
    if (!SendAll(s, &roiH, sizeof(roiH))) { return false; }
    if (ps <= 0) { return true; }
    return SendAll(s, payload, static_cast<std::size_t>(ps));
}

// 发送一帧 raw RGBA8（codec=0，整帧无 ROI 裁剪）。
inline bool SendFrame(PVSocket s, int width, int height,
                      const unsigned char* rgba, std::uint32_t seq,
                      std::int32_t frameIndex = 0) {
    return SendFramePayload(s, width, height, kCodecRawRGBA, 0, 0, width, height,
                            rgba, static_cast<std::int32_t>(width) * height * 4,
                            seq, frameIndex);
}

// 帧（client 端解析结果）。
struct Frame {
    int width{0};
    int height{0};
    std::uint32_t seq{0};               // 触发本帧的 INTERACT 命令 seq（0 = 无命令/初始帧）
    std::uint32_t codec{kCodecRawRGBA}; // 帧编码（0=raw RGBA8，1=zlib 压缩）
    int roiX{0}, roiY{0}, roiW{0}, roiH{0}; // payload 在全帧中的子矩形
    int frameIndex{0};                  // 当前帧序号（0 起始，多帧播放显示用）
    std::vector<unsigned char> payload; // 原始 payload 字节（ROI 子矩形）
    std::vector<unsigned char> rgba;    // 解码后的全帧 RGBA8（width*height*4，客户端重建后填充）
};

// 从字节流解析一帧；返回 0=成功，1=缓冲不足（需更多数据），-1=格式错误需重对齐。
// 与 MiniPVClient::onReadyRead 的健壮性口径一致：magic 不匹配时丢弃 1 字节重新对齐。
inline int ParseFrame(const std::vector<char>& buf, std::size_t offset,
                      Frame& out, std::size_t& consumed) {
    consumed = 0;
    if (buf.size() - offset < kFrameHeaderBytes) { return 1; }

    const char* p = buf.data() + offset;
    std::uint32_t magic = 0;
    std::int32_t w = 0, h = 0, codec = 0, payloadSize = 0;
    std::uint32_t seq = 0;
    std::int32_t frameIndex = 0;
    std::int32_t roiX = 0, roiY = 0, roiW = 0, roiH = 0;
    std::size_t o = 0;
    std::memcpy(&magic, p + o, sizeof(magic)); o += sizeof(magic);
    std::memcpy(&w, p + o, sizeof(w)); o += sizeof(w);
    std::memcpy(&h, p + o, sizeof(h)); o += sizeof(h);
    std::memcpy(&codec, p + o, sizeof(codec)); o += sizeof(codec);
    std::memcpy(&seq, p + o, sizeof(seq)); o += sizeof(seq);
    std::memcpy(&frameIndex, p + o, sizeof(frameIndex)); o += sizeof(frameIndex);
    std::memcpy(&payloadSize, p + o, sizeof(payloadSize)); o += sizeof(payloadSize);
    std::memcpy(&roiX, p + o, sizeof(roiX)); o += sizeof(roiX);
    std::memcpy(&roiY, p + o, sizeof(roiY)); o += sizeof(roiY);
    std::memcpy(&roiW, p + o, sizeof(roiW)); o += sizeof(roiW);
    std::memcpy(&roiH, p + o, sizeof(roiH)); o += sizeof(roiH);

    if (magic != kFrameMagic) { return -1; }
    // 基本健壮性检查（与 MiniPVClient 一致）。
    if (w <= 0 || h <= 0 || w > 10000 || h > 10000 ||
        payloadSize < 0 || payloadSize > 512 * 1024 * 1024) {
        return -1;
    }
    // ROI 校验：子矩形必须在全帧内；空 ROI（roiW==0 || roiH==0）要求 payloadSize==0。
    if (roiX < 0 || roiY < 0 || roiW < 0 || roiH < 0 ||
        roiX + roiW > w || roiY + roiH > h) {
        return -1;
    }
    if ((roiW == 0 || roiH == 0) && payloadSize != 0) { return -1; }
    if (codec == static_cast<std::int32_t>(kCodecRawRGBA)) {
        // raw RGBA8：payload 长度必须严格等于 ROI 子矩形字节数。
        if (payloadSize != roiW * roiH * 4) { return -1; }
    } else if (codec != static_cast<std::int32_t>(kCodecZlib)) {
        return -1; // 未知 codec
    }
    if (buf.size() - offset - kFrameHeaderBytes <
        static_cast<std::size_t>(payloadSize)) {
        return 1;
    }

    out.width = w;
    out.height = h;
    out.seq = seq;
    out.codec = static_cast<std::uint32_t>(codec);
    out.frameIndex = frameIndex;
    out.roiX = roiX;
    out.roiY = roiY;
    out.roiW = roiW;
    out.roiH = roiH;
    out.payload.assign(buf.data() + offset + kFrameHeaderBytes,
                       buf.data() + offset + kFrameHeaderBytes + payloadSize);
    out.rgba.clear();
    consumed = kFrameHeaderBytes + static_cast<std::size_t>(payloadSize);
    return 0;
}

// 拼接一条 INTERACT 命令（client -> server）。
inline std::string MakeInteractCommand(std::uint32_t seq, double dAzimRad,
                                       double dElevRad, double zoomFactor,
                                       int interactiveFlag) {
    std::ostringstream oss;
    oss.precision(6);
    oss << "INTERACT " << seq << ' ' << dAzimRad << ' ' << dElevRad << ' '
        << zoomFactor << ' ' << interactiveFlag << '\n';
    return oss.str();
}

// 解析 INTERACT 命令（server 端），返回是否成功；结果写入 out 参数。
// 解析 INTERACT 命令（server 端），返回是否成功；结果写入 out 参数。
inline bool ParseInteractCommand(const std::string& line, std::uint32_t& seq,
                                 double& dAzimRad, double& dElevRad,
                                 double& zoomFactor, int& interactiveFlag) {
    std::istringstream iss(line);
    std::string op;
    iss >> op;
    if (op != "INTERACT") { return false; }
    if (!(iss >> seq >> dAzimRad >> dElevRad >> zoomFactor)) { return false; }
    // interactiveFlag 可选，缺省 0（高清）。
    interactiveFlag = 0;
    int flag = 0;
    if (iss >> flag) { interactiveFlag = flag; }
    return true;
}

// 拼接一条切帧命令（client -> server）：step = +1 下一帧（NEXT）、-1 上一帧（PREV）。
inline std::string MakeFrameStepCommand(int step) {
    return (step > 0) ? std::string("NEXT\n") : std::string("PREV\n");
}

} // namespace iGamePVNet
