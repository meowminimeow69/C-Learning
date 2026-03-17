#include <atomic>
#include <vector>
#include <optional>

template<typename T, size_t Capacity>
class SPSCQueue {
private:
    T buffer[Capacity];
    // Align to cache lines to prevent False Sharing (Critical for HFT)
    alignas(64) std::atomic<size_t> head{0}; 
    alignas(64) std::atomic<size_t> tail{0};

public:
    bool push(const T& data) {
        size_t t = tail.load(std::memory_order_relaxed);
        size_t next_tail = (t + 1) % Capacity;

        if (next_tail == head.load(std::memory_order_acquire)) {
            return false; // Queue full
        }

        buffer[t] = data;
        
        // RELEASE: Ensures 'buffer[t] = data' is visible to 
        // any thread that performs an ACQUIRE on this tail.
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t h = head.load(std::memory_order_relaxed);
        
        // ACQUIRE: Ensures that if we see the updated tail, 
        // we also see the data written to the buffer by the producer.
        if (h == tail.load(std::memory_order_acquire)) {
            return std::nullopt; // Queue empty
        }

        T data = buffer[h];
        
        // RELEASE: Ensures the consumer is done reading 'buffer[h]' 
        // before the head is updated, letting the producer reuse the slot.
        head.store((h + 1) % Capacity, std::memory_order_release);
        return data;
    }
};