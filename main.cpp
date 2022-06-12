#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <vector>

// a simple function to generate a vector containing a given number of
// random doubles; uses mersenne twister
std::vector<double> generate_n_random_numbers(size_t number_of_randoms) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, 100.0);

    std::vector<double> randoms;
    randoms.reserve(number_of_randoms);
    for(auto i=0; i < number_of_randoms; i++) {
        randoms.push_back(dist(mt));
    }

    return randoms;
}

class VectorOps {
public:
    double vector_sum(std::vector<double>&& inputs) {
        std::cout << "started vector_sum execution\n";
        auto sum = std::accumulate(inputs.begin(), inputs.end(), 0.0);
        return sum;
    }

    void consume() {
        while(!m_term) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "consumer is awake...\n";
            double res = 0.0;
            bool evaluated = false;

            {
                std::lock_guard<std::mutex> lock_guard(m_mut);
                if (m_queue.size() > 0) {
                    auto fut = std::move(m_queue.front());
                    m_queue.pop();
                    res = fut.get();
                    evaluated = true;
                }
            }

            if (evaluated) {
                std::cout << "got a future from the queue, got " << res << '\n';
            }
        }
    }

    void start_consumer_thread() {
        m_consumer_thread = std::thread(&VectorOps::consume, this);
    }

    void stop_consumer_thread() {
        m_term = true;
    }

    void append_future_to_queue(std::future<double>&& fut) {
        std::lock_guard<std::mutex> lock_guard(m_mut);
        m_queue.push(std::move(fut));
    }

    ~VectorOps() {
        stop_consumer_thread();
        if (m_consumer_thread.joinable()) {
            m_consumer_thread.join();
        }

        {
            std::lock_guard<std::mutex> lock_guard(m_mut);
            auto queue_size = m_queue.size();
            for (auto i=0; i < queue_size; i++) {
                auto fut = std::move(m_queue.front());
                m_queue.pop();
                double res = fut.get();
                std::cout << "drained " << res << " from the queue, from the destructor\n";
            }
        }
    }

private:
    std::atomic<bool> m_term{false};
    std::mutex m_mut;
    std::queue<std::future<double>> m_queue;
    std::thread m_consumer_thread;
};


int main() {
    VectorOps vo{};
    vo.start_consumer_thread();

    // create and queue up fifty futures
    for(auto i=0; i < 50; i++) {
        std::vector<double> vec = generate_n_random_numbers(10);
        auto fut = std::async(std::launch::async, &VectorOps::vector_sum, &vo, std::move(vec));
        vo.append_future_to_queue(std::move(fut));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    vo.stop_consumer_thread();

    return 0;
}
