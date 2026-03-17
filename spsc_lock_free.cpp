// Single-Producer Single-Consumer lock-free queue
// Used for: market data feed -> order engine
template<typename T, size_t N>
struct SPSCQueue {
    struct Slot {
        std::atomic<uint64_t> sequence;   // the flag
        T data;
    };
    
    alignas(64) Slot slots[N];            // cache-line aligned
    alignas(64) std::atomic<uint64_t> write_pos{0};
    alignas(64) std::atomic<uint64_t> read_pos{0};

    bool push(const T& val) {
        uint64_t pos = write_pos.load(memory_order_relaxed);
        Slot& slot = slots[pos % N];
        
        // Is slot free? Relaxed — we just need the number
        if (slot.sequence.load(memory_order_relaxed) != pos)
            return false;  // full
        
        slot.data = val;  // write payload
        
        // RELEASE: makes all writes above visible to consumer
        slot.sequence.store(pos + 1, memory_order_release);
        
        write_pos.store(pos + 1, memory_order_relaxed);
        return true;
    }

    bool pop(T& out) {
        uint64_t pos = read_pos.load(memory_order_relaxed);
        Slot& slot = slots[pos % N];
        
        // ACQUIRE: see everything written before the release store
        if (slot.sequence.load(memory_order_acquire) != pos + 1)
            return false;  // empty
        
        out = slot.data;  // safe: acquire guarantees visibility
        
        // Mark slot as free — release so producer sees it
        slot.sequence.store(pos + N, memory_order_release);
        read_pos.store(pos + 1, memory_order_relaxed);
        return true;
    }
};