#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>
#include <string>
 
// ─────────────────────────────────────────────
//  Configuration
// ─────────────────────────────────────────────
constexpr int NUM_PHILOSOPHERS = 5;
constexpr int MEAL_COUNT       = 3;   // each philosopher eats this many times
 
// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
std::mutex print_mtx;
 
void log(int id, const std::string& action) {
    std::lock_guard<std::mutex> lk(print_mtx);
    std::cout << "[Philosopher " << id << "] " << action << "\n";
}
 
int random_ms(int lo, int hi) {
    thread_local std::mt19937 rng{std::random_device{}()};
    return std::uniform_int_distribution<int>{lo, hi}(rng);
}
 
// ─────────────────────────────────────────────
//  Fork (chopstick)
// ─────────────────────────────────────────────
struct Fork {
    std::mutex mtx;
    int        id;
    explicit Fork(int i) : id(i) {}
};
 
// ─────────────────────────────────────────────
//  Philosopher
//
//  Deadlock prevention strategy:
//    • Every philosopher picks up the lower-id fork first.
//      This breaks the circular-wait condition.
// ─────────────────────────────────────────────
class Philosopher {
public:
    Philosopher(int id, Fork& left, Fork& right)
        : id_(id), left_(left), right_(right) {}
 
    void run() {
        for (int meal = 0; meal < MEAL_COUNT; ++meal) {
            think();
            dine();
        }
        log(id_, "is done dining.");
    }
 
private:
    void think() {
        log(id_, "is thinking...");
        std::this_thread::sleep_for(
            std::chrono::milliseconds(random_ms(100, 500)));
    }
 
    void dine() {
        // Always lock the lower-id fork first to prevent deadlock
        Fork& first  = (left_.id < right_.id) ? left_  : right_;
        Fork& second = (left_.id < right_.id) ? right_ : left_;
 
        log(id_, "is hungry, waiting for fork " + std::to_string(first.id));
        std::unique_lock<std::mutex> lk1(first.mtx);
        log(id_, "picked up fork " + std::to_string(first.id)
                 + ", waiting for fork " + std::to_string(second.id));
 
        std::unique_lock<std::mutex> lk2(second.mtx);
        log(id_, "picked up fork " + std::to_string(second.id) + " — eating!");
 
        std::this_thread::sleep_for(
            std::chrono::milliseconds(random_ms(200, 600)));
 
        log(id_, "finished eating, putting down forks.");
        // lk1 and lk2 are released automatically (RAII)
    }
 
    int    id_;
    Fork&  left_;
    Fork&  right_;
};
 
// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "=== Dining Philosophers (C++17) ===\n"
              << NUM_PHILOSOPHERS << " philosophers, "
              << MEAL_COUNT       << " meals each\n\n";
 
    // Create forks — use unique_ptr because std::mutex is not movable/copyable
    std::vector<std::unique_ptr<Fork>> forks;
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        forks.push_back(std::make_unique<Fork>(i));
 
    // Create philosopher objects — same reason: store by unique_ptr
    std::vector<std::unique_ptr<Philosopher>> philosophers;
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
        philosophers.push_back(std::make_unique<Philosopher>(i,
            *forks[i],
            *forks[(i + 1) % NUM_PHILOSOPHERS]));
 
    // Launch threads
    std::vector<std::thread> threads;
    threads.reserve(NUM_PHILOSOPHERS);
    for (auto& p : philosophers)
        threads.emplace_back(&Philosopher::run, p.get());
 
    // Join threads
    for (auto& t : threads)
        t.join();
 
    std::cout << "\nAll philosophers have finished dining.\n";
    return 0;
}