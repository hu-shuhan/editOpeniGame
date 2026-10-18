#ifndef iGameProgressObserver_h
#define iGameProgressObserver_h

#include "iGameObject.h"
#include "iGameCommand.h"
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
        this->m_Progress = amount;
		this->InvokeEvent(Command::ProgressEvent, static_cast<void*>(&amount));
	}

    // 用于 UI 展示“当前在做什么”（例如多帧/多块的 a/n 进度文案）
    void UpdateText(const std::string& text) {
        m_Text = text;
        const char* cstr = m_Text.c_str();
        this->InvokeEvent(Command::UpdateEvent, static_cast<void*>(const_cast<char*>(cstr)));
    }

    /**
     * 「本次操作正阻塞在 GUI 线程上」的标记。
     *
     * 写文件是同步执行的：调用期间 Qt 事件循环停摆，进度条控件里的 show()/setValue()
     * 要等整个操作结束才会被绘制，用户看到的就是「界面卡死、没有任何反馈」。
     *
     * 进度条控件据此在收到进度事件后主动泵一次事件（排除用户输入），让进度真正可见。
     * 目前只有 FileWriter 的写盘流程会置位，其余进度上报路径保持原行为
     * （避免在渲染/滤镜等调用栈里重入事件循环）。
     */
    void SetBlockingGuiThread(bool blocking) { s_BlockingGuiThread = blocking; }
    bool IsBlockingGuiThread() const { return s_BlockingGuiThread; }

protected:
    ProgressObserver(){};
    ~ProgressObserver() override = default;

	double m_Progress{ 0.0 };
    std::string m_Text{};

private:
    // 按线程保存：写文件也可能跑在工作线程（如报告生成），此时 GUI 线程并未被阻塞，
    // 不应该因为它而让 GUI 线程上的进度上报去泵事件。
    inline static thread_local bool s_BlockingGuiThread{ false };
};
IGAME_NAMESPACE_END
#endif
