#ifndef iGameProgressObserver_h
#define iGameProgressObserver_h

#include "iGameObject.h"
#include "iGameCommand.h"
#include <mutex>
#include <string>

IGAME_NAMESPACE_BEGIN
class ProgressObserver : public Object {
public:
    I_OBJECT(ProgressObserver)
    static ProgressObserver* Instance() { 
		static ProgressObserver ins;
        return &ins;
	}

	void UpdateProgress(double amount) {
		std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        this->m_Progress = amount;
		this->InvokeEvent(Command::ProgressEvent, static_cast<void*>(&amount));
	}

    // 用于 UI 展示“当前在做什么”（例如多帧/多块的 a/n 进度文案）
    void UpdateText(const std::string& text) {
		std::lock_guard<std::recursive_mutex> lock(m_Mutex);
        m_Text = text;
        const char* cstr = m_Text.c_str();
        this->InvokeEvent(Command::UpdateEvent, static_cast<void*>(const_cast<char*>(cstr)));
    }

protected:
    ProgressObserver(){};
    ~ProgressObserver() override = default;

	double m_Progress{ 0.0 };
    std::string m_Text{};
    std::recursive_mutex m_Mutex;
     
};
IGAME_NAMESPACE_END
#endif
