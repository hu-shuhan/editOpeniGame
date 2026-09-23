#include "iGameVolumeDistributor.h"

#include "iGameFileIO.h"
#include "iGameParallelContext.h"
#include <tinyxml2.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

IGAME_NAMESPACE_BEGIN

namespace {

// ---------------------------------------------------------------------------
// PVD 轻量解析：提取某个 timestep 的 <DataSet timestep= part= file=/>。
// 对齐 UnifiedVersion/DataDistribution.cpp::ParsePvd —— 只解析 XML，不读分块数据。
// ---------------------------------------------------------------------------
bool ParsePvdPieces(const std::string& pvdPath, int timestep,
                    std::vector<std::pair<int, std::string>>& pieces,
                    std::string& err) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(pvdPath.c_str()) != tinyxml2::XML_SUCCESS) {
        err = "cannot open pvd: " + pvdPath;
        return false;
    }
    tinyxml2::XMLElement* root = doc.RootElement(); // <VTKFile type="Collection">
    if (!root) { err = "pvd has no root element"; return false; }
    tinyxml2::XMLElement* collection = root->FirstChildElement("Collection");
    if (!collection) { err = "pvd has no <Collection>"; return false; }

    const size_t slash = pvdPath.find_last_of("/\\");
    const std::string dir =
            (slash == std::string::npos) ? std::string() : pvdPath.substr(0, slash + 1);

    std::map<int, std::string> byPart;
    for (tinyxml2::XMLElement* ds = collection->FirstChildElement("DataSet"); ds;
         ds = ds->NextSiblingElement("DataSet")) {
        const int t = ds->IntAttribute("timestep", 0);
        if (t != timestep) continue;
        const char* file = ds->Attribute("file");
        if (!file || !*file) continue;
        const int part = ds->IntAttribute("part", static_cast<int>(byPart.size()));
        byPart[part] = dir + file;
    }
    if (byPart.empty()) {
        err = "pvd has no <DataSet> for timestep " + std::to_string(timestep);
        return false;
    }
    pieces.clear();
    pieces.reserve(byPart.size());
    for (auto& kv : byPart) { pieces.emplace_back(kv.first, kv.second); }
    return true;
}

// ---------------------------------------------------------------------------
// 目录模式：枚举 figure_N.{igc,vtr,vts,vtu}，按优先级 igc > vtr > vts > vtu 每编号取一个。
// 对齐 UnifiedVersion/DataDistribution.cpp::EnumerateDirectory。
// ---------------------------------------------------------------------------
bool EnumerateDirPieces(const std::string& dirPath,
                        std::vector<std::pair<int, std::string>>& pieces,
                        std::string& err) {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::map<int, std::pair<int, std::string>> best; // index -> (priority, path)

    auto prio = [](const std::string& ext) {
        if (ext == ".igc") return 4;
        if (ext == ".vtr") return 3;
        if (ext == ".vts") return 2;
        if (ext == ".vtu") return 1;
        return 0;
    };

    for (fs::directory_iterator it(dirPath, ec), end; !ec && it != end; it.increment(ec)) {
        if (ec) break;
        if (!it->is_regular_file(ec)) continue;
        const std::string fn = it->path().filename().string();
        if (fn.rfind("figure_", 0) != 0) continue;

        const size_t dot = fn.find_last_of('.');
        const std::string ext = (dot == std::string::npos) ? std::string() : fn.substr(dot);
        const int p = prio(ext);
        if (p == 0) continue;

        std::string numPart = fn.substr(7);
        const size_t d2 = numPart.find_last_of('.');
        if (d2 != std::string::npos) numPart = numPart.substr(0, d2);
        if (numPart.empty()) continue;
        const bool numeric = std::all_of(numPart.begin(), numPart.end(),
                                         [](unsigned char c) { return std::isdigit(c); });
        if (!numeric) continue;

        const int idx = std::atoi(numPart.c_str());
        auto it2 = best.find(idx);
        if (it2 == best.end() || p > it2->second.first) {
            best[idx] = {p, it->path().string()};
        }
    }

    for (auto& kv : best) { pieces.emplace_back(kv.first, kv.second.second); }
    if (pieces.empty()) {
        err = "no figure_N.{igc,vtr,vts,vtu} files in " + dirPath;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 把分块文件列表序列化成字节流广播给所有 rank。
// 对齐 UnifiedVersion/DataDistribution.cpp::BroadcastPieces。
// ---------------------------------------------------------------------------
void BroadcastRawPieces(std::vector<std::pair<int, std::string>>& pieces, int root) {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int rank = ctx->Rank();

    std::string buf;
    int n = 0;
    if (rank == root) {
        std::ostringstream os(std::ios::binary);
        n = static_cast<int>(pieces.size());
        os.write(reinterpret_cast<const char*>(&n), sizeof(n));
        for (const auto& p : pieces) {
            const int part = p.first;
            const int len = static_cast<int>(p.second.size());
            os.write(reinterpret_cast<const char*>(&part), sizeof(part));
            os.write(reinterpret_cast<const char*>(&len), sizeof(len));
            os.write(p.second.data(), len);
        }
        buf = os.str();
    }

    int bufLen = static_cast<int>(buf.size());
    ctx->Broadcast(&bufLen, 1, root);
    if (rank != root) buf.resize(static_cast<size_t>(bufLen));
    if (bufLen > 0) ctx->Broadcast(buf.data(), bufLen, root);

    if (rank != root) {
        pieces.clear();
        std::istringstream is(buf, std::ios::binary);
        is.read(reinterpret_cast<char*>(&n), sizeof(n));
        pieces.resize(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i) {
            int part = 0, len = 0;
            is.read(reinterpret_cast<char*>(&part), sizeof(part));
            is.read(reinterpret_cast<char*>(&len), sizeof(len));
            pieces[i].first = part;
            pieces[i].second.resize(static_cast<size_t>(len));
            is.read(pieces[i].second.data(), len);
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// 空间超块 k-d 二分（严格对齐 UnifiedVersion/DataDistribution.cpp::BisectGrid）：
//   把 nx×ny×nz 的块网格按体素开销切成 target 个轴对齐连续超块，尽量均衡，结果确定一致。
// ---------------------------------------------------------------------------
namespace {

struct GBBlock {
    int ix0{0}, ix1{0}, iy0{0}, iy1{0}, iz0{0}, iz1{0};
    double cost{0.0};
};

std::vector<GBBlock> BisectGrid(int nx, int ny, int nz,
                                const std::vector<double>& cost, int target) {
    auto cellCost = [&](int ix, int iy, int iz) -> double {
        return cost[static_cast<size_t>(iz) * ny * nx + iy * nx + ix];
    };
    auto blockCost = [&](const GBBlock& b) -> double {
        double c = 0;
        for (int iz = b.iz0; iz <= b.iz1; ++iz)
            for (int iy = b.iy0; iy <= b.iy1; ++iy)
                for (int ix = b.ix0; ix <= b.ix1; ++ix)
                    c += cellCost(ix, iy, iz);
        return c;
    };

    struct Cmp {
        bool operator()(const GBBlock& a, const GBBlock& b) const {
            if (a.cost != b.cost) return a.cost < b.cost; // 大顶堆
            if (a.iz0 != b.iz0) return a.iz0 > b.iz0;
            if (a.iy0 != b.iy0) return a.iy0 > b.iy0;
            if (a.ix0 != b.ix0) return a.ix0 > b.ix0;
            if (a.iz1 != b.iz1) return a.iz1 > b.iz1;
            if (a.iy1 != b.iy1) return a.iy1 > b.iy1;
            return a.ix1 > b.ix1;
        }
    };

    GBBlock whole{0, nx - 1, 0, ny - 1, 0, nz - 1, 0.0};
    whole.cost = blockCost(whole);
    std::priority_queue<GBBlock, std::vector<GBBlock>, Cmp> pq;
    pq.push(whole);

    while (static_cast<int>(pq.size()) < target) {
        GBBlock b = pq.top();
        pq.pop();

        const int lx = b.ix1 - b.ix0 + 1;
        const int ly = b.iy1 - b.iy0 + 1;
        const int lz = b.iz1 - b.iz0 + 1;
        if (lx <= 1 && ly <= 1 && lz <= 1) { pq.push(b); break; }

        // 选最长轴（tie 优先 z > y > x）。
        int axis = 0;
        if (ly >= lx && ly >= lz) axis = 1;
        if (lz >= lx && lz >= ly) axis = 2;

        const double half = b.cost * 0.5;

        if (axis == 0) {
            double acc = 0;
            int s = b.ix0;
            for (int ix = b.ix0; ix <= b.ix1; ++ix) {
                for (int iz = b.iz0; iz <= b.iz1; ++iz)
                    for (int iy = b.iy0; iy <= b.iy1; ++iy)
                        acc += cellCost(ix, iy, iz);
                if (acc >= half) { s = ix; break; }
            }
            if (s >= b.ix1) s = b.ix1 - 1; // 保证右半非空
            GBBlock l{b.ix0, s, b.iy0, b.iy1, b.iz0, b.iz1, 0.0};
            GBBlock r{s + 1, b.ix1, b.iy0, b.iy1, b.iz0, b.iz1, 0.0};
            l.cost = blockCost(l);
            r.cost = blockCost(r);
            pq.push(l);
            pq.push(r);
        } else if (axis == 1) {
            double acc = 0;
            int s = b.iy0;
            for (int iy = b.iy0; iy <= b.iy1; ++iy) {
                for (int iz = b.iz0; iz <= b.iz1; ++iz)
                    for (int ix = b.ix0; ix <= b.ix1; ++ix)
                        acc += cellCost(ix, iy, iz);
                if (acc >= half) { s = iy; break; }
            }
            if (s >= b.iy1) s = b.iy1 - 1;
            GBBlock l{b.ix0, b.ix1, b.iy0, s, b.iz0, b.iz1, 0.0};
            GBBlock r{b.ix0, b.ix1, s + 1, b.iy1, b.iz0, b.iz1, 0.0};
            l.cost = blockCost(l);
            r.cost = blockCost(r);
            pq.push(l);
            pq.push(r);
        } else {
            double acc = 0;
            int s = b.iz0;
            for (int iz = b.iz0; iz <= b.iz1; ++iz) {
                for (int iy = b.iy0; iy <= b.iy1; ++iy)
                    for (int ix = b.ix0; ix <= b.ix1; ++ix)
                        acc += cellCost(ix, iy, iz);
                if (acc >= half) { s = iz; break; }
            }
            if (s >= b.iz1) s = b.iz1 - 1;
            GBBlock l{b.ix0, b.ix1, b.iy0, b.iy1, b.iz0, s, 0.0};
            GBBlock r{b.ix0, b.ix1, b.iy0, b.iy1, s + 1, b.iz1, 0.0};
            l.cost = blockCost(l);
            r.cost = blockCost(r);
            pq.push(l);
            pq.push(r);
        }
    }

    std::vector<GBBlock> blocks;
    blocks.reserve(pq.size());
    while (!pq.empty()) { blocks.push_back(pq.top()); pq.pop(); }
    std::sort(blocks.begin(), blocks.end(), [](const GBBlock& a, const GBBlock& b) {
        if (a.iz0 != b.iz0) return a.iz0 < b.iz0;
        if (a.iy0 != b.iy0) return a.iy0 < b.iy0;
        if (a.ix0 != b.ix0) return a.ix0 < b.ix0;
        if (a.iz1 != b.iz1) return a.iz1 < b.iz1;
        if (a.iy1 != b.iy1) return a.iy1 < b.iy1;
        return a.ix1 < b.ix1;
    });
    return blocks;
}

} // namespace

void iGameVolumeDistributor::SetInput(DataObject::Pointer root) {
    m_Input = root;
    m_Pieces.clear();
    m_LocalIndices.clear();
    m_LocalBounds.reset();
    if (m_Input) { CollectPieces(m_Input); }
}

void iGameVolumeDistributor::CollectPieces(DataObject::Pointer obj) {
    if (!obj) { return; }

    if (obj->HasSubDataObject()) {
        for (auto it = obj->SubDataObjectIteratorBegin();
             it != obj->SubDataObjectIteratorEnd(); ++it) {
            CollectPieces(it->second);
        }
        return;
    }

    // 叶子分块：必须有几何（点集）才参与体绘制分配；纯容器/元数据对象跳过。
    if (obj->GetPoints() == nullptr) { return; }

    Piece piece;
    piece.object = obj;
    piece.originalIndex = static_cast<int>(m_Pieces.size());
    piece.bounds = obj->GetBoundingBox();
    m_Pieces.push_back(piece);
}

bool iGameVolumeDistributor::ComputeDistribution() {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();

    // 按左下角坐标确定性排序（x 优先，其次 y、z）。所有 rank 用同一输入树 + 同一
    // 比较器，因此得到完全一致的全局顺序，无需跨进程交换。
    std::sort(m_Pieces.begin(), m_Pieces.end(), [](const Piece& a, const Piece& b) {
        const Vector3d& amin = a.bounds.min;
        const Vector3d& bmin = b.bounds.min;
        if (amin[0] != bmin[0]) { return amin[0] < bmin[0]; }
        if (amin[1] != bmin[1]) { return amin[1] < bmin[1]; }
        return amin[2] < bmin[2];
    });

    const int n = static_cast<int>(m_Pieces.size());
    if (n < 1) { return false; }

    // 约束（对标 TestPVolumeRender）：进程数不能超过分块数，否则部分 rank 空跑，
    // 后续集合通信/合成会死锁。单进程 size==1 恒满足（-n 1 回退为整块）。
    if (size > n) { return false; }

    // 连续切块：把有序分块按数量切成 size 段，rank 取第 rank 段。
    const int base = n / size;
    const int rem = n % size;
    int start = 0;
    for (int r = 0; r < rank; ++r) {
        start += base + (r < rem ? 1 : 0);
    }
    const int count = base + (rank < rem ? 1 : 0);

    m_LocalIndices.clear();
    m_LocalBounds.reset();
    for (int i = start; i < start + count; ++i) {
        m_LocalIndices.push_back(i);
        m_LocalBounds.add(m_Pieces[i].bounds);
    }
    return true;
}

DataObject::Pointer iGameVolumeDistributor::GetPiece(int i) const {
    if (i < 0 || i >= static_cast<int>(m_Pieces.size())) { return nullptr; }
    return m_Pieces[i].object;
}

DataObject::Pointer iGameVolumeDistributor::GetLocalPiece(int i) const {
    if (i < 0 || i >= static_cast<int>(m_LocalIndices.size())) { return nullptr; }
    return m_Pieces[m_LocalIndices[i]].object;
}

DataObject::Pointer iGameVolumeDistributor::GetLocalComposite() const {
    DataObject::Pointer composite = DataObject::New();
    for (int idx : m_LocalIndices) {
        composite->AddSubDataObject(m_Pieces[idx].object);
    }
    return composite;
}

// ---------------------------------------------------------------------------
// 文件级分发（阶段 7，严格对齐 UnifiedVersion/DataDistribution.cpp::ComputeDistribution）：
//   枚举分块文件列表 → 各 rank 只扫 part%Size==rank 的分块包围盒 → AllReduce 汇总 →
//   确定性排序 + 数量连续切块 → 得到本 rank 要读的分块文件列表（不整读全量数据）。
// ---------------------------------------------------------------------------
bool iGameVolumeDistributor::ComputeFileDistribution(const std::string& inputPath,
                                                     int timestep) {
    auto* ctx = ParallelContext::Instance().GetPointer();
    const int size = ctx->Size();
    const int rank = ctx->Rank();

    // 1) 枚举分块文件列表（rank0 解析，广播给所有 rank）。
    std::vector<std::pair<int, std::string>> raw;
    std::string err;
    std::error_code ec;
    const bool isDir = std::filesystem::is_directory(inputPath, ec);
    if (rank == 0) {
        if (isDir) {
            EnumerateDirPieces(inputPath, raw, err);
        } else {
            std::string lower = inputPath;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            const bool isPvd =
                    lower.size() >= 4 && lower.substr(lower.size() - 4) == ".pvd";
            if (isPvd) {
                ParsePvdPieces(inputPath, timestep, raw, err);
            } else {
                raw.emplace_back(0, inputPath); // 单文件 = 一个分块
            }
        }
        if (raw.empty() && err.empty()) { err = "no pieces to render"; }
        if (!err.empty()) { std::cerr << "[distributor] " << err << '\n'; }
    }
    BroadcastRawPieces(raw, 0);

    const int n = static_cast<int>(raw.size());
    if (n < 1) {
        if (rank == 0) { std::cerr << "Distribution failed: no pieces.\n"; }
        return false;
    }
    // 约束（对标 TestPVolumeRender）：进程数不能超过分块数，否则部分 rank 空跑导致死锁。
    if (size > n) {
        if (rank == 0) {
            std::cerr << "Distribution failed: process count (" << size
                      << ") > piece count (" << n << ").\n";
        }
        return false;
    }

    // 2) 并行扫包围盒：每个 rank 只读 part % size == rank 的分块，AllReduce(SUM) 汇总成
    //    全局 7×n 表 [valid, minX, minY, minZ, maxX, maxY, maxZ]。
    std::vector<FilePiece> pieces(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        pieces[i].part = raw[i].first;
        pieces[i].file = std::move(raw[i].second);
    }

    std::vector<double> send(static_cast<size_t>(7) * n, 0.0);
    for (int p = rank; p < n; p += size) {
        DataObject::Pointer obj = FileIO::ReadFile(pieces[p].file);
        if (!obj) { continue; }
        const BoundingBox& b = obj->GetBoundingBox();
        const size_t base = static_cast<size_t>(7) * p;
        send[base + 0] = 1.0;
        send[base + 1] = b.min[0];
        send[base + 2] = b.min[1];
        send[base + 3] = b.min[2];
        send[base + 4] = b.max[0];
        send[base + 5] = b.max[1];
        send[base + 6] = b.max[2];
    }

    std::vector<double> recv(static_cast<size_t>(7) * n, 0.0);
    if (size > 1) {
        ctx->AllReduce(send.data(), recv.data(), 7 * n, ParallelContext::ReduceOp::Sum);
    } else {
        recv = send;
    }

    for (int p = 0; p < n; ++p) {
        const size_t base = static_cast<size_t>(7) * p;
        if (recv[base + 0] <= 0.5) {
            if (rank == 0) {
                std::cerr << "Distribution failed: cannot read bounds of piece "
                          << pieces[p].file << ".\n";
            }
            return false;
        }
        const double mn[3] = {recv[base + 1], recv[base + 2], recv[base + 3]};
        const double mx[3] = {recv[base + 4], recv[base + 5], recv[base + 6]};
        pieces[p].bounds = BoundingBox(mn, mx);
    }

    // 3) 计算体素开销（物理体积近似）并推导规则网格 (ix,iy,iz)。
    std::vector<double> xs, ys, zs;
    double maxAbs = 0.0;
    for (int p = 0; p < n; ++p) {
        const double sx = pieces[p].bounds.max[0] - pieces[p].bounds.min[0];
        const double sy = pieces[p].bounds.max[1] - pieces[p].bounds.min[1];
        const double sz = pieces[p].bounds.max[2] - pieces[p].bounds.min[2];
        pieces[p].cost = std::max(sx * sy * sz, 1e-30);
        xs.push_back(pieces[p].bounds.min[0]);
        ys.push_back(pieces[p].bounds.min[1]);
        zs.push_back(pieces[p].bounds.min[2]);
        maxAbs = std::max(maxAbs, std::abs(pieces[p].bounds.min[0]));
        maxAbs = std::max(maxAbs, std::abs(pieces[p].bounds.min[1]));
        maxAbs = std::max(maxAbs, std::abs(pieces[p].bounds.min[2]));
    }

    auto uniq = [](std::vector<double>& v, double eps) {
        std::sort(v.begin(), v.end());
        std::vector<double> u;
        for (double x : v)
            if (u.empty() || x - u.back() > eps) u.push_back(x);
        v.swap(u);
    };
    const double eps = std::max(1.0, maxAbs) * 1e-6;
    uniq(xs, eps);
    uniq(ys, eps);
    uniq(zs, eps);
    const int nx = static_cast<int>(xs.size());
    const int ny = static_cast<int>(ys.size());
    const int nz = static_cast<int>(zs.size());

    for (int p = 0; p < n; ++p) {
        pieces[p].ix = static_cast<int>(
                std::lower_bound(xs.begin(), xs.end(), pieces[p].bounds.min[0] - eps) - xs.begin());
        pieces[p].iy = static_cast<int>(
                std::lower_bound(ys.begin(), ys.end(), pieces[p].bounds.min[1] - eps) - ys.begin());
        pieces[p].iz = static_cast<int>(
                std::lower_bound(zs.begin(), zs.end(), pieces[p].bounds.min[2] - eps) - zs.begin());
    }

    m_LocalFiles.clear();
    m_LocalBounds.reset();
    m_LocalBlock = Block{};

    // 4) 分配：分块构成完整规则网格（nx*ny*nz == n）时用 k-d 二分（对齐 BisectGrid，
    //    保证每 rank 一个连通、互不重叠、负载均衡的超块）；否则退回数量连续切块。
    const bool completeGrid = (static_cast<long long>(nx) * ny * nz == n);
    if (completeGrid) {
        std::vector<double> cost(static_cast<size_t>(nx) * ny * nz, 0.0);
        for (int p = 0; p < n; ++p) {
            const size_t idx = static_cast<size_t>(pieces[p].iz) * ny * nx +
                               pieces[p].iy * nx + pieces[p].ix;
            cost[idx] = pieces[p].cost;
        }

        const int target = std::min(size, n);
        const std::vector<GBBlock> blocks = BisectGrid(nx, ny, nz, cost, target);
        for (const GBBlock& blk : blocks) {
            if (blk.cost <= 0.0) {
                if (rank == 0) {
                    std::cerr << "Distribution failed: empty super-block "
                                 "(incomplete spatial grid).\n";
                }
                return false;
            }
        }

        if (rank < static_cast<int>(blocks.size())) {
            const GBBlock& blk = blocks[rank];
            m_LocalBlock = Block{blk.ix0, blk.ix1, blk.iy0, blk.iy1, blk.iz0, blk.iz1};
            for (int p = 0; p < n; ++p) {
                if (pieces[p].ix >= blk.ix0 && pieces[p].ix <= blk.ix1 &&
                    pieces[p].iy >= blk.iy0 && pieces[p].iy <= blk.iy1 &&
                    pieces[p].iz >= blk.iz0 && pieces[p].iz <= blk.iz1) {
                    m_LocalFiles.push_back(pieces[p]);
                    m_LocalBounds.add(pieces[p].bounds);
                }
            }
        }
    } else {
        // 退回：按包围盒左下角确定性排序 + 数量连续切块。
        std::sort(pieces.begin(), pieces.end(), [](const FilePiece& a, const FilePiece& b) {
            const Vector3d& amin = a.bounds.min;
            const Vector3d& bmin = b.bounds.min;
            if (amin[0] != bmin[0]) { return amin[0] < bmin[0]; }
            if (amin[1] != bmin[1]) { return amin[1] < bmin[1]; }
            return amin[2] < bmin[2];
        });
        const int base = n / size;
        const int rem = n % size;
        int start = 0;
        for (int r = 0; r < rank; ++r) { start += base + (r < rem ? 1 : 0); }
        const int count = base + (rank < rem ? 1 : 0);
        for (int i = start; i < start + count; ++i) {
            m_LocalFiles.push_back(pieces[i]);
            m_LocalBounds.add(pieces[i].bounds);
        }
    }
    return true;
}

IGAME_NAMESPACE_END
