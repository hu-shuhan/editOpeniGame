#include "iGameObject.h"
#include "iGameCommand.h"

IGAME_NAMESPACE_BEGIN

class Observer {
public:
	Observer() {}
	~Observer() {}

	Command::Pointer m_Command{};
	unsigned long m_Event{ 0 };
	unsigned long m_Tag{ 0 };
	Observer* m_Next{ nullptr };
	float m_Priority{ 0.0 };
};

class ObserverInternal {
public:
	ObserverInternal() {}
	~ObserverInternal() { this->RemoveAllObservers(); }

	unsigned long AddObserver(unsigned long event, Command::Pointer cmd, float p) {
		Observer* elem = new Observer;
		elem->m_Priority = p;
		elem->m_Next = nullptr;
		elem->m_Event = event;
		elem->m_Command = cmd;
		elem->m_Tag = this->m_Count;
		this->m_Count++;


		if (!this->m_Start)
		{
			this->m_Start = elem;
		}
		else
		{
			// Insert sort, from highest to lowest priority
			Observer* prev = nullptr;
			Observer* pos = this->m_Start;
			while (pos->m_Priority >= elem->m_Priority && pos->m_Next)
			{
				prev = pos;
				pos = pos->m_Next;
			}

			if (pos->m_Priority > elem->m_Priority)
			{
				pos->m_Next = elem;
			}
			else
			{
				if (prev)
				{
					prev->m_Next = elem;
				}
				elem->m_Next = pos;

				if (pos == this->m_Start)
				{
					this->m_Start = elem;
				}
			}
		}
		return elem->m_Tag;
	}

	void RemoveObserver(unsigned long tag)
	{
		Observer* elem;
		Observer* prev;
		Observer* next;

		elem = this->m_Start;
		prev = nullptr;
		while (elem)
		{
			if (elem->m_Tag == tag)
			{
				if (prev)
				{
					prev->m_Next = elem->m_Next;
					next = prev->m_Next;
				}
				else
				{
					this->m_Start = elem->m_Next;
					next = this->m_Start;
				}
				delete elem;
				elem = next;
			}
			else
			{
				prev = elem;
				elem = elem->m_Next;
			}
		}
	}
	void RemoveObservers(unsigned long event)
	{
		Observer* elem;
		Observer* prev;
		Observer* next;

		elem = this->m_Start;
		prev = nullptr;
		while (elem)
		{
			if (elem->m_Event == event)
			{
				if (prev)
				{
					prev->m_Next = elem->m_Next;
					next = prev->m_Next;
				}
				else
				{
					this->m_Start = elem->m_Next;
					next = this->m_Start;
				}
				delete elem;
				elem = next;
			}
			else
			{
				prev = elem;
				elem = elem->m_Next;
			}
		}
	}
	void RemoveObservers(unsigned long event, Command::Pointer cmd)
	{
		Observer* elem;
		Observer* prev;
		Observer* next;

		elem = this->m_Start;
		prev = nullptr;
		while (elem)
		{
			if (elem->m_Event == event && elem->m_Command == cmd)
			{
				if (prev)
				{
					prev->m_Next = elem->m_Next;
					next = prev->m_Next;
				}
				else
				{
					this->m_Start = elem->m_Next;
					next = this->m_Start;
				}
				delete elem;
				elem = next;
			}
			else
			{
				prev = elem;
				elem = elem->m_Next;
			}
		}
	}
	void RemoveAllObservers()
	{
		Observer* elem = this->m_Start;
		Observer* next;
		while (elem)
		{
			next = elem->m_Next;
			delete elem;
			elem = next;
		}
		this->m_Start = nullptr;
	}

	bool InvokeEvent(unsigned long event, void* callData, Object* self)
	{
		Observer* elem = this->m_Start;
		Observer* next;

		while (elem)
		{
			next = elem->m_Next;
			if (elem->m_Event == event && elem->m_Tag < this->m_Count)
			{
				Command::Pointer command = elem->m_Command;
				elem->m_Command->Execute(self, event, callData);
			}

			elem = next;
		}

		return 0;
	}

	Command::Pointer GetCommand(unsigned long tag)
	{
		Observer* elem = this->m_Start;
		while (elem)
		{
			if (elem->m_Tag == tag)
			{
				return elem->m_Command;
			}
			elem = elem->m_Next;
		}
		return nullptr;
	}

	unsigned long GetTag(Command::Pointer cmd)
	{
		Observer* elem = this->m_Start;
		while (elem)
		{
			if (elem->m_Command == cmd)
			{
				return elem->m_Tag;
			}
			elem = elem->m_Next;
		}
		return 0;
	}

	bool HasObserver(unsigned long event)
	{
		Observer* elem = this->m_Start;
		while (elem)
		{
			if (elem->m_Event == event)
			{
				return true;
			}
			elem = elem->m_Next;
		}
		return false;
	}

	bool HasObserver(unsigned long event, Command::Pointer cmd) {
		Observer* elem = this->m_Start;
		while (elem)
		{
			if (elem->m_Event == event && elem->m_Command == cmd)
			{
				return true;
			}
			elem = elem->m_Next;
		}
		return false;
	}

protected:
	Observer* m_Start{ nullptr };
	unsigned long m_Count{ 0 };
};

class CallbackBase {
public:
    CallbackBase(){}
    ~CallbackBase(){}
	virtual bool operator()(Object*, unsigned long, void*) = 0;
};

class LambdaCallback : public CallbackBase {
public:
	std::function<void()> Method1;
	std::function<void(Object*, unsigned long, void*)> Method2;
	//std::function<bool(Object*, unsigned long, void*)> Method3;

	LambdaCallback(std::function<void()> method)
	{
		this->Method1 = method;
		this->Method2 = nullptr;
		//this->Method3 = nullptr;
	}

	LambdaCallback(std::function<void(Object*, unsigned long, void*)> method)
	{
		this->Method1 = nullptr;
		this->Method2 = method;
		//this->Method3 = nullptr;
	}

	//iGameLambdaCallback(std::function<bool(Object*, unsigned long, void*)> method)
	//{
	//    this->Method1 = nullptr;
	//    this->Method2 = nullptr;
	//    this->Method3 = method;
	//}

	bool operator()(Object* caller, unsigned long event, void* calldata) override
	{
		if (this->Method1)
		{
			Method1();
		}
		else if (this->Method2)
		{
			Method2(caller, event, calldata);
		}
		//else if (this->Method3)
		//{
		//    return Method3(caller, event, calldata);
		//}
		return false;
	}
};

class LambdaCommand : public Command {
public:
	I_OBJECT(LambdaCommand);
	static Pointer New() { return new LambdaCommand; }

	virtual void Execute(Object* caller, unsigned long eventId, void* callData) override {
		if (this->Callable)
		{
			(*this->Callable)(caller, eventId, callData);
		}
	};

	void SetCallable(CallbackBase* callable)
	{
		//delete this->Callable;
		this->Callable = callable;
	}

private:
	LambdaCommand() {}
	~LambdaCommand() override {
		delete this->Callable;
	}

	CallbackBase* Callable{ nullptr };
};

unsigned long Object::AddObserver(unsigned long event, Command::Pointer cmd, float priority)
{
	if (!this->m_Internal)
	{
		this->m_Internal = new ObserverInternal;
	}
	return this->m_Internal->AddObserver(event, cmd, priority);
}

unsigned long Object::AddObserver(const char* event, Command::Pointer cmd, float priority)
{
	return this->AddObserver(Command::GetEventIdFromString(event), cmd, priority);
}

unsigned long Object::AddObserver(unsigned long event, std::function<void()> callback, float priority)
{
	LambdaCallback* callable = new LambdaCallback(callback);
	LambdaCommand::Pointer command = LambdaCommand::New();
	command->SetCallable(callable);
	return this->AddObserver(event, command, priority);
}

unsigned long Object::AddObserver(unsigned long event, std::function<void(Object*, unsigned long, void*)> callback, float priority)
{
	LambdaCallback* callable = new LambdaCallback(callback);
	LambdaCommand::Pointer command = LambdaCommand::New();
	command->SetCallable(callable);
	return this->AddObserver(event, command, priority);
}

void Object::RemoveObserver(unsigned long tag)
{
	if (this->m_Internal)
	{
		this->m_Internal->RemoveObserver(tag);
	}
}

void Object::RemoveObservers(unsigned long event)
{
	if (this->m_Internal)
	{
		this->m_Internal->RemoveObserver(event);
	}
}

void Object::RemoveObservers(const char* event)
{
	this->RemoveObservers(Command::GetEventIdFromString(event));
}

void Object::RemoveAllObservers()
{
	if (this->m_Internal)
	{
		this->m_Internal->RemoveAllObservers();
	}
}

bool Object::HasObserver(unsigned long event)
{
	if (this->m_Internal)
	{
		return this->m_Internal->HasObserver(event);
	}
	return false;
}

bool Object::HasObserver(const char* event)
{
	return this->HasObserver(Command::GetEventIdFromString(event));
}

Command::Pointer Object::GetCommand(unsigned long tag)
{
	if (this->m_Internal)
	{
		return this->m_Internal->GetCommand(tag);
	}
	return nullptr;
}

void Object::RemoveObserver(Command::Pointer cmd)
{
	if (this->m_Internal)
	{
		unsigned long tag = this->m_Internal->GetTag(cmd);
		while (tag)
		{
			this->m_Internal->RemoveObserver(tag);
			tag = this->m_Internal->GetTag(cmd);
		}
	}
}

void Object::RemoveObservers(unsigned long event, Command::Pointer cmd)
{
	if (this->m_Internal)
	{
		this->m_Internal->RemoveObservers(event, cmd);
	}
}

void Object::RemoveObservers(const char* event, Command::Pointer cmd)
{
	this->RemoveObservers(Command::GetEventIdFromString(event), cmd);
}

bool Object::HasObserver(unsigned long event, Command::Pointer cmd)
{
	if (this->m_Internal)
	{
		return this->m_Internal->HasObserver(event, cmd);
	}
	return false;
}

bool Object::HasObserver(const char* event, Command::Pointer cmd)
{
	return this->HasObserver(Command::GetEventIdFromString(event), cmd);
}

bool Object::InvokeEvent(unsigned long event, void* callData)
{
	if (this->m_Internal)
	{
		return this->m_Internal->InvokeEvent(event, callData, this);
	}
	return false;
}

bool Object::InvokeEvent(const char* event, void* callData)
{
	return this->InvokeEvent(Command::GetEventIdFromString(event), callData);
}

void Object::Modified()
{
	m_MTime.Modified();
	//this->InvokeEvent(Command::ModifiedEvent);
}

Object::~Object()  {
    delete m_Internal;
}


IGAME_NAMESPACE_END
