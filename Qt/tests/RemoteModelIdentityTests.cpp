#include <IQCore/igQtRemoteModelIdentity.h>
#include <QCoreApplication>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const QByteArray sha(32, 'a');
    const auto key = igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v1", sha, 5000000000ULL);
    auto require = [](bool ok) { if (!ok) { std::cerr << "Identity validation failed\n"; std::exit(1); } };
    require(key.size() == 64);
    require(key == igQtRemoteModelIdentity(" 127.0.0.1 ", 34569, "p", "p.tar.zst", "v1", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.2", 34569, "p", "p.tar.zst", "v1", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34570, "p", "p.tar.zst", "v1", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34569, "q", "p.tar.zst", "v1", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "q.tar.zst", "v1", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v2", sha, 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v1", QByteArray(32, 'b'), 5000000000ULL));
    require(key != igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v1", sha, 5000000001ULL));
    require(igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v1", {}, 1).isEmpty());
    require(igQtRemoteModelIdentity("127.0.0.1", 0, "p", "p.tar.zst", "v1", sha, 1).isEmpty());
    require(igQtRemoteModelIdentity("", 34569, "p", "p.tar.zst", "v1", sha, 1).isEmpty());
    require(igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "", sha, 1).isEmpty());
    require(igQtRemoteModelIdentity("127.0.0.1", 34569, "p", "p.tar.zst", "v1", sha, 0).isEmpty());
    std::cout << "Remote model identity validation passed\n";
}
