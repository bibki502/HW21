#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
using namespace std;

class TaskQueue {
private:
    queue<int> tasks; //очередь из задач (int-чисел от 1 до 20)
    mutex mtx;
    condition_variable cv;
    bool stop_flag = false;

public:
    //добавление задачи в конец очереди
    void push(int task) {
        {
            lock_guard<mutex> lock(mtx); //mutex запирается
            tasks.push(task);
        } //mutex освобождается
        cv.notify_one(); //пробуждается один поток
    }

    //достаём первую в очереди задачу
    bool pop(int& task) {
        unique_lock<mutex> lock(mtx);
        
        //ждем, пока появится задача ИЛИ поступит сигнал об остановке
        cv.wait(lock, [this] { 
            return !tasks.empty() || stop_flag; 
        });

        if (tasks.empty()) {
            return false; // очередь пуста и работа завершена
        }

        task = tasks.front(); //посмотрел и запомнил первую в очереди задачу
        tasks.pop(); //удалил её из очереди
        return true;
    }

    // сигнал потокам, что пора завершаться
    void stop() {
        {
            lock_guard<mutex> lock(mtx);
            stop_flag = true;
        }
        cv.notify_all(); // будим все потоки, чтобы они вышли из wait
    }
};

//работа Worker-а
void worker_func(int id, TaskQueue& queue) {
    int task;
    while (queue.pop(task)) { //покаа очередь непустая

        this_thread::sleep_for(chrono::seconds(1)); //чтобы увидеть параллельную работу
        
        static mutex out_mtx;
        lock_guard<mutex> lock(out_mtx);
        cout << "[Worker-" << id << "] processed the task " << task <<endl;
    }
}

int main() {
    const int num_workers = 4;
    TaskQueue queue;
    vector<thread> workers;

    //создание рабочих потоков
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(worker_func, i, ref(queue));
    }

    //добавляем задачи
    for (int i = 1; i <= 20; ++i) {
        queue.push(i);
    }

    queue.stop();

    //join всех потоков
    for (auto& t : workers) { //проход по всем Worker-ам
        if (t.joinable()) { //если не был вызван join
            t.join();       //то вызвать join 
        }
    }

    cout << "Done" << endl;
    return 0;
}