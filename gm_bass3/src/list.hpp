#include <mutex>

#ifndef TSyncList_H
#define TSyncList_H

template<class T> 
class SyncList
{
	struct node {
		T value;
		node *next;	
	};
	node *head;
	node *tail;
	int size;
	std::mutex MutexLock;
public:
	SyncList(): size(0), head(NULL), tail(NULL) {};
	~SyncList() {}
	int getSize()
	{
		std::lock_guard<std::mutex> Lock(MutexLock);
		return size;
	}

	void add(T data)
	{
		node *b = new node;
		b->next = NULL;
		b->value = data;

		std::lock_guard<std::mutex> Lock(MutexLock);
		if((head == NULL) || (head == nullptr))
		{
			head = b;
			tail = b;
		} else {
			tail->next = b;
			tail = b;
		}
		size++;
	}

	T remove()
	{
		T result;
		node *tmp;

		{ // Scope
			std::lock_guard<std::mutex> Lock(MutexLock);
			if(head == NULL) return NULL;
			if(head == nullptr) return NULL;

			tmp = head;
			head = tmp->next;
			size--;
			if(size == 0) head = NULL;
		}

		result = tmp->value;
		delete tmp;
		return result;
	}
};

#endif