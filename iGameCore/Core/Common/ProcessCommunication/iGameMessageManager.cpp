#include "iGameMessageManager.h"
#include "ProcessCommunication/iGameSocketConnection.h"

IGAME_NAMESPACE_BEGIN

iGameMessageManager::iGameMessageManager()
    : m_connection(nullptr)
    , m_connected(false)
{
}

iGameMessageManager::~iGameMessageManager()
{
    stop();
}

bool iGameMessageManager::start(const std::string& host, int port)
{
    if (m_connection) { return true; }
    m_connection = new iGameSocketConnection(host, port);
    m_connection->setMessageCallback([this](const std::string& msg) { this->onMessage(msg); });
    m_connection->setConnectionCallback([this](bool c){ this->onConnectionChanged(c); });
    if (!m_connection->start()) {
        delete m_connection; m_connection = nullptr; return false;
    }
    return true;
}

void iGameMessageManager::stop()
{
    if (!m_connection) { return; }
    m_connection->stop();
    delete m_connection;
    m_connection = nullptr;
    m_connected = false;
}

bool iGameMessageManager::isConnected() const
{
    return m_connection && m_connection->isClientConnected();
}

bool iGameMessageManager::isRunning() const
{
    return m_connection && m_connection->isRunning();
}

bool iGameMessageManager::sendString(const std::string& data)
{
    return m_connection && m_connection->sendResponse(data);
}

void iGameMessageManager::setMessageHandler(std::function<void(const std::string&)> handler)
{
    if (m_connection) { m_connection->setMessageCallback(std::move(handler)); }
}

void iGameMessageManager::setConnectionHandler(std::function<void(bool)> handler)
{
    if (m_connection) { m_connection->setConnectionCallback(std::move(handler)); }
}

void iGameMessageManager::onMessage(const std::string& message) { (void)message; }
void iGameMessageManager::onConnectionChanged(bool connected) { m_connected = connected; }

IGAME_NAMESPACE_END


