/**
 * @class   iGameMessageManager
 * @brief   消息管理
 * @author  XSong
 * @note    消息管理器，用于负责接收消息以及发送消息，使用iGameSocketConnection进行进程间通信
 */
#ifndef iGameMessageManager_h
#define iGameMessageManager_h

#include "iGameObject.h"
#include <functional>
#include <string>

IGAME_NAMESPACE_BEGIN

class iGameSocketConnection;

class iGameMessageManager : public Object {
public:
    I_OBJECT(iGameMessageManager);

    static Pointer New() { return new iGameMessageManager; }

    iGameMessageManager();
    ~iGameMessageManager() override;

    bool start(const std::string& host, int port);
    void stop();
    bool isConnected() const;
    bool isRunning() const;
    bool sendString(const std::string& data);

    void setMessageHandler(std::function<void(const std::string&)> handler);
    void setConnectionHandler(std::function<void(bool)> handler);

protected:
    virtual void onMessage(const std::string& message);
    virtual void onConnectionChanged(bool connected);

protected:
    iGameSocketConnection* m_connection;
    bool m_connected;
};

IGAME_NAMESPACE_END

#endif

