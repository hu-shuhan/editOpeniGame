#ifndef iGamePriorityQueue_h
#define iGamePriorityQueue_h

#include "iGameVector.h"
#include "iGameObject.h"

#include <queue>
#include <algorithm>
#include <unordered_map>

IGAME_NAMESPACE_BEGIN
class PriorityQueue : public Object{
public:
	I_OBJECT(PriorityQueue);
	static Pointer New() { return new PriorityQueue; }

	struct Item {
		double priority, rest;
		int handle, uniqueId;
		Item() : priority(static_cast<double>(1.0e+299)), handle(-1), uniqueId(-1) {};
        Item(double priority, int handle, double rest)
            : priority(priority), handle(handle), uniqueId(-1), rest(rest){};
        Item(double priority, int handle, int uniqueId, double rest)
            : priority(priority), handle(handle), uniqueId(uniqueId),
              rest(rest){};
		bool operator<(const Item& other) const {
			return priority > other.priority;
		}
	};

	void push(double priority, int id, double rest = 0.0) {
		Item item(priority, id, currentId, rest);
		queue.push(item);
		uniqueId_map.push_back(uint8_t(1));
		handle_to_uniqueId_map[item.handle] = currentId++;
	}
	//void push(Item& item) {
	//	item.uniqueId = currentId;
	//	queue.push(item);
	//	uniqueId_map.push_back(uint8_t(1));
	//	handle_to_uniqueId_map[item.handle] = currentId++;
	//}
	//void push(Item item) {
	//	item.uniqueId = currentId;
	//	queue.push(item);
	//	uniqueId_map.push_back(uint8_t(1));
	//	handle_to_uniqueId_map[item.handle] = currentId++;
	//}
	const Item& top() {
		return queue.top();
	}
	void update() {
		while (!empty() && uniqueId_map[queue.top().uniqueId] == uint8_t(0)) {
			queue.pop();
		}
	}
	void pop() {
		queue.pop();
	}
	void remove(int id) {
		//if (0 <= id && id < handle_to_uniqueId_map.size()) {
		uniqueId_map[handle_to_uniqueId_map[id]] = uint8_t(0);
		//}
	}
	bool empty() const {
		return queue.empty();
	}
	void clear() {
		currentId = 0;
		uniqueId_map.clear();
		handle_to_uniqueId_map.clear();
		std::priority_queue<Item> newQueue;
		queue.swap(newQueue);
	}
	size_t size() const {
		return queue.size();
	}

private:
	PriorityQueue() {}
	~PriorityQueue() override = default;

	int currentId = 0;
	std::priority_queue<Item> queue;
	std::vector<uint8_t> uniqueId_map;
	//std::vector<int> handle_to_uniqueId_map;
	std::unordered_map<int, int> handle_to_uniqueId_map;
};
IGAME_NAMESPACE_END
#endif