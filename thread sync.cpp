#include "queue"
#include "list"
#include <mutex>
#include <thread>
#include <random>
#include <memory>
#include <string>
#include <iostream>
#include <future>
#include <condition_variable>
using namespace std;

#define _default

#ifdef _default
	typedef struct Part {
		int part_id;
		float volume;
		typedef shared_ptr<struct Part> PartPtr;
	} Part;

	static bool done = false;
	queue<Part::PartPtr> shared_queue;
	mutex lock_queue;
	mutex lock_cout;
	static bool done2 = false;
	queue<Part::PartPtr> shared_queue2;
	mutex lock_queue2;

	condition_variable event_holder;
	condition_variable event_holder2;

	void locked_output(const std::string& str) {
		lock_guard<mutex> raii(lock_cout);
		cout << str << endl;
	}

	void threadCwork(Part::PartPtr & part) {
		part->volume -= 0.5;
		this_thread::sleep_for(chrono::milliseconds(500 + rand() % 6000));
	}

	void threadAwork(Part::PartPtr & part) {
		part->volume -= 2;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadAwork finished with part " + to_string(part->part_id));
	}
	void threadBwork(Part::PartPtr & part) {
		part->volume -= 1;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadBwork finished with part " + to_string(part->part_id));
	}

	void threadA(list<Part::PartPtr>&input) {
		srand(7777777);
		size_t size = input.size();
		for (size_t i = 0; i < size; i++) {
			threadAwork(*input.begin());
			{
				lock_guard<mutex> raii_obj(lock_queue);
				shared_queue.push(Part::PartPtr(*input.begin()));
				input.remove(*input.begin());
				locked_output("Part was added to queue");
			}
		}
		done = true;
	}

	void threadB() {
		srand(100000);
		while (true) {
			Part::PartPtr part_for_work;
			{
				lock_queue.lock();
				if (shared_queue.empty()) {
					lock_queue.unlock();
					if (done) break;
					locked_output("threadB useless check, queue is empty. Going to bed");
					this_thread::sleep_for(chrono::milliseconds(1000));
					continue;
				}
				else {
					part_for_work = shared_queue.front();
					shared_queue.pop();
					lock_queue.unlock();
					locked_output("Part was removed from queue");
				}
			}
			threadBwork(part_for_work);
			lock_guard<mutex> raii_obj(lock_queue2);
			shared_queue2.push(part_for_work);
		}
		done2 = true;
	}

	void threadC() {
		srand(5555555);
		while (true) {
			Part::PartPtr part_for_work;
			{
				lock_queue2.lock();
				if (shared_queue2.empty()) {
					lock_queue2.unlock();
					if (done2) break;
					locked_output("threadC useless check, queue is empty. Going to bed");
					this_thread::sleep_for(chrono::milliseconds(1000));
					continue;
				}
				else {
					part_for_work = shared_queue2.front();
					shared_queue2.pop();
					lock_queue2.unlock();
				}
			}
			threadCwork(part_for_work);
		}
	}

	int main(int argc, char* argv[])
	{
		list<Part::PartPtr> spare_parts;
		for (int i = 1; i <= 5; i++) {
			spare_parts.push_back(Part::PartPtr(new Part{ i, 10.0 }));
		}

		thread ta(threadA, ref(spare_parts));
		thread tb(threadB);
		thread tc(threadC);

		ta.join();
		tb.join();
		tc.join();

		cout << "done" << endl;
		return 0;
	}
#endif;

#ifdef future_class
	typedef struct Part {
		int part_id;
		float volume;
		typedef shared_ptr<struct Part> PartPtr;
	} Part;

	static bool done = false;
	queue<Part::PartPtr> shared_queue;
	mutex lock_queue;
	mutex lock_cout;
	static bool done2 = false;
	queue<Part::PartPtr> shared_queue2;
	mutex lock_queue2;

	condition_variable event_holder;
	condition_variable event_holder2;

	void locked_output(const std::string& str) {
		lock_guard<mutex> raii(lock_cout);
		std::cout << str << endl;
	}
	void threadCwork(Part::PartPtr& part) {
		part->volume -= 0.5;
		this_thread::sleep_for(chrono::milliseconds(500 + rand() % 6000));
	}

	void threadAwork(Part::PartPtr& part) {
		part->volume -= 2;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadAwork finished with part " + to_string(part->part_id));
	}

	void threadBwork(Part::PartPtr& part) {
		part->volume -= 1;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadBwork finished with part " + to_string(part->part_id));
	}

	void threadA(list<Part::PartPtr>& input) {
		srand(77777777);
		size_t size = input.size();
		for (size_t i = 0; i < size; i++) {
			// обрабатываем деталь
			threadAwork(*input.begin());
			// кладем в очередь
			{
				lock_guard<mutex> raii_obj(lock_queue);
				shared_queue.push(Part::PartPtr(*input.begin()));
				input.remove(*input.begin());
				locked_output("Part was added to queue");
				event_holder.notify_one();
			}
		}
	}

	void threadB(future<void>& a_res) {
		srand(1000000);
		while (true) {
			list<Part::PartPtr> parts_for_work;
			{
				unique_lock<mutex> m_holder(lock_queue);

				if (a_res.wait_for(chrono::seconds(0)) == future_status::ready
					&& shared_queue.empty()) {
					break;
				}

				if (shared_queue.empty()) {
					event_holder.wait(m_holder, [&a_res]() {
						return !shared_queue.empty() ||
							a_res.wait_for(chrono::seconds(0)) == future_status::ready;
						});
				}
				for (size_t i = 0; i < shared_queue.size(); i++)
				{
					parts_for_work.push_back(shared_queue.front());
					shared_queue.pop();
				}
				locked_output("Parts were removed from queue");
			}
			for (auto& p : parts_for_work)
			{
				threadBwork(p);
				lock_guard<mutex> raii_obj(lock_queue2);
				shared_queue2.push(p);
				event_holder2.notify_one();
			}
		}
		event_holder2.notify_one();
	}

	void threadC(future<void>& b_res) {
		srand(5555555);
		while (true) {
			list<Part::PartPtr> parts_for_work;
			{
				unique_lock<mutex> m_holder(lock_queue2);

				if (b_res.wait_for(chrono::seconds(0)) == future_status::ready
					&& shared_queue2.empty()) break;

				if (shared_queue2.empty()) {
					event_holder2.wait(m_holder, [&b_res]() {
						return !shared_queue2.empty() ||
							b_res.wait_for(chrono::seconds(0)) == future_status::ready;
						});
				}
				for (size_t i = 0; i < shared_queue2.size(); i++)
				{
					parts_for_work.push_back(shared_queue2.front());
					shared_queue2.pop();
				}
			}
			for (auto& p : parts_for_work)
				threadCwork(p);
		}
	}

	int main(int argc, char* argv[])
	{

		list<Part::PartPtr> spare_parts;
		for (int i = 0; i < 5; i++) {
			spare_parts.push_back(Part::PartPtr(new Part{ i + 1, 10.0 }));
		}

		future<void> a_res = async(launch::async, threadA, std::ref(spare_parts));
		future<void> b_res = async(launch::async, threadB, std::ref(a_res));
		future<void> c_res = async(launch::async, threadC, std::ref(b_res));

		a_res.wait();
		event_holder.notify_one();
		b_res.wait();
		event_holder2.notify_one();
		c_res.wait();

		cout << "done";
		return 0;
	}
#endif

#ifdef promise_class

	typedef struct Part {
		int part_id;
		float volume;
		typedef shared_ptr<struct Part> PartPtr;
	} Part;

	static bool done = false;
	queue<Part::PartPtr> shared_queue;
	mutex lock_queue;
	mutex lock_cout;
	static bool done2 = false;
	queue<Part::PartPtr> shared_queue2;
	mutex lock_queue2;

	condition_variable event_holder;
	condition_variable event_holder2;

	void locked_output(const std::string& str) {
		lock_guard<mutex> raii(lock_cout);
		cout << str << endl;
	}
	void threadCwork(Part::PartPtr& part) {
		part->volume -= 0.5;
		this_thread::sleep_for(chrono::milliseconds(500 + rand() % 6000));
	}

	void threadAwork(Part::PartPtr& part) {
		part->volume -= 2;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadAwork finished with part " + to_string(part->part_id));
	}

	void threadBwork(Part::PartPtr& part) {
		part->volume -= 1;
		std::this_thread::sleep_for(std::chrono::milliseconds(500 + rand() % 6000));

		locked_output("threadBwork finished with part " + to_string(part->part_id));
	}

	void threadA(list<Part::PartPtr>& input, promise<void>& a_p) {
		srand(77777777);
		size_t size = input.size();
		for (size_t i = 0; i < size; i++) {
			threadAwork(*input.begin());
			{
				lock_guard<mutex> raii_obj(lock_queue);
				shared_queue.push(Part::PartPtr(*input.begin()));
				input.remove(*input.begin());
				locked_output("Part was added to queue");
				event_holder.notify_one();
			}
		}
		a_p.set_value();
		event_holder.notify_one();
	}

	void threadB(promise<void>& a_p, promise<void>& b_p) {
		srand(1000000);
		auto f = a_p.get_future();
		while (true) {
			list<Part::PartPtr> parts_for_work;
			{
				unique_lock<mutex> m_holder(lock_queue);

				if (f.wait_for(chrono::seconds(0)) == future_status::ready
					&& shared_queue.empty()) {
					break;
				}

				if (shared_queue.empty()) {
					event_holder.wait(m_holder, [&f]() {
						return !shared_queue.empty() ||
							f.wait_for(chrono::seconds(0)) == future_status::ready;
						});
				}
				for (size_t i = 0; i < shared_queue.size(); i++)
				{
					parts_for_work.push_back(shared_queue.front());
					shared_queue.pop();
				}
				locked_output("Parts were removed from queue");
			}
			for (auto& p : parts_for_work)
			{
				threadBwork(p);
				lock_guard<mutex> raii_obj(lock_queue2);
				shared_queue2.push(p);
				event_holder2.notify_one();
			}
		}
		b_p.set_value();
		event_holder2.notify_one();
	}

	void threadC(promise<void>& b_p, promise<void>& c_p) {
		srand(5555555);
		auto f = b_p.get_future();
		while (true) {
			list<Part::PartPtr> parts_for_work;
			{
				unique_lock<mutex> m_holder(lock_queue2);

				if (f.wait_for(chrono::seconds(0)) == future_status::ready
					&& shared_queue2.empty()) break;

				if (shared_queue2.empty()) {
					event_holder2.wait(m_holder, [&f]() {
						return !shared_queue2.empty() ||
							f.wait_for(chrono::seconds(0)) == future_status::ready;
						});
				}
				for (size_t i = 0; i < shared_queue2.size(); i++)
				{
					parts_for_work.push_back(shared_queue2.front());
					shared_queue2.pop();
				}
			}
			for (auto& p : parts_for_work)
				threadCwork(p);
		}
		c_p.set_value();
	}

	int main(int argc, char* argv[])
	{
		list<Part::PartPtr> spare_parts;
		for (int i = 0; i < 5; i++) {
			spare_parts.push_back(Part::PartPtr(new Part{ i + 1, 10.0 }));
		}
		promise<void> a_p;
		thread ta(threadA, std::ref(spare_parts), std::ref(a_p));
		promise<void> b_p;
		thread tb(threadB, std::ref(a_p), std::ref(b_p));
		promise<void> c_p;
		auto f = c_p.get_future();
		thread tc(threadC, std::ref(b_p), std::ref(c_p));

		ta.detach();
		tb.detach();
		tc.detach();

		f.wait();

		cout << "done";
		return 0;
	}

#endif
