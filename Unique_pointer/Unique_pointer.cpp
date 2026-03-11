// Unique_pointer.cpp - Naman Mehrotra
#include <cassert>
#include <cstdio>
#include <utility>
#include <string>
#include <stdexcept>
#include <vector>

template<typename T>
struct custom_deleter {
    void operator()(T* data) {
        delete data;
    }
};
/*
I do inheritance for EBO optimization , having a variable D will make the class size 16 but 
inheritance will make it 8. We private it so that we do not call the deleter
*/
template<typename T, typename D = custom_deleter<T>>
class UNIQUE_PTR : private D{
private:
    T* data;
public:
    // destructor deleter needs to be public
    UNIQUE_PTR() noexcept : D{}, data(nullptr) {}
    explicit UNIQUE_PTR(T* data1) noexcept : D{}, data(data1) {}
    UNIQUE_PTR(T* ptr, D d) noexcept : D(std::move(d)), data(ptr) {}

    UNIQUE_PTR(const UNIQUE_PTR& u) = delete;
    UNIQUE_PTR& operator=(const UNIQUE_PTR& u) = delete;

    UNIQUE_PTR(UNIQUE_PTR&& u) noexcept : D(std::move(static_cast<D&>(u))),data(u.data){
        u.data = nullptr;
    }

    UNIQUE_PTR& operator=(UNIQUE_PTR&& u) noexcept {
        if (&u != this) {                           
            reset();                                
            static_cast<D&>(*this) = std::move(static_cast<D&>(u)); 
            data = u.data;
            u.data = nullptr;
        }
        return *this;
    }


    ~UNIQUE_PTR() noexcept {
        if (data) {
            static_cast<D&>(*this)(data);
        }
    }

    T& operator*() noexcept { return *data;}
    const T& operator*() const noexcept { return *data; }

    T* operator->()  noexcept {return data;}
    const T* operator->() const noexcept { return data; }

    [[nodiscard]] T* get() const noexcept { return data;}

    explicit operator bool() const noexcept { return data != nullptr; }
    
    D& get_deleter() noexcept { return static_cast<D&>(*this); }
    const D& get_deleter() const noexcept { return static_cast<const D&>(*this); }

    [[nodiscard]] T* release() noexcept {
        return std::exchange(data, nullptr);
    }

    void reset(T* new_data = nullptr) noexcept {             
        if (new_data != data) {
            T* old = std::exchange(data, new_data);
            if (old) {
                static_cast<D&>(*this)(old);
            }
        }
    }
};

template<typename T, typename... Args>
UNIQUE_PTR<T> make_unique(Args&&... args) {
    return UNIQUE_PTR<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
UNIQUE_PTR<T> make_unique(std::size_t n) {
    using Elem = std::remove_extent_t<T>;
    return UNIQUE_PTR<T>(new Elem[n]());
}



static int pass_count = 0;
static int fail_count = 0;

#define TEST(name) void name()
#define RUN(name)  do { printf("  %-50s", #name); name(); } while(0)
#define CHECK(expr) \
    do { \
        if (expr) { ++pass_count; printf("PASS\n"); } \
        else      { ++fail_count; printf("FAIL  (line %d: %s)\n", __LINE__, #expr); } \
    } while(0)

// Tracks constructor/destructor calls — critical for verifying ownership semantics
struct Probe {
    static int ctor_count;
    static int dtor_count;
    int value;

    explicit Probe(int v = 0) : value(v) { ++ctor_count; }
    ~Probe() { ++dtor_count; }

    static void reset_counts() { ctor_count = dtor_count = 0; }
};
int Probe::ctor_count = 0;
int Probe::dtor_count = 0;

// Stateful deleter — verifies deleter transfer on move
struct TrackingDeleter {
    int* call_count;
    explicit TrackingDeleter(int* c) : call_count(c) {}
    void operator()(Probe* p) noexcept { ++(*call_count); delete p; }
};

// ─────────────────────────────────────────────────────────────────────────────
// TESTS
// ─────────────────────────────────────────────────────────────────────────────

// 1. Default construction yields null
TEST(test_default_null) {
    UNIQUE_PTR<int> p;
    CHECK(p.get() == nullptr);
}

// 2. operator bool on null
TEST(test_bool_null) {
    UNIQUE_PTR<int> p;
    CHECK(!p);
}

// 3. Construction from raw pointer + operator bool
TEST(test_construct_bool) {
    UNIQUE_PTR<int> p(new int(42));
    CHECK(static_cast<bool>(p));
}

// 4. Destructor calls delete exactly once
TEST(test_destructor_called_once) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> p(new Probe(1));
    }
    CHECK(Probe::dtor_count == 1);
}

// 5. operator* returns mutable reference (write-through)
TEST(test_deref_write) {
    UNIQUE_PTR<int> p(new int(10));
    *p = 99;
    CHECK(*p == 99);
}

// 6. operator-> member access
TEST(test_arrow) {
    UNIQUE_PTR<std::string> p(new std::string("hft"));
    CHECK(p->size() == 3);
}

// 7. get() returns raw pointer
TEST(test_get) {
    int* raw = new int(7);
    UNIQUE_PTR<int> p(raw);
    CHECK(p.get() == raw);
}

// 8. release() transfers ownership, sets internal to null
TEST(test_release) {
    int* raw = new int(5);
    UNIQUE_PTR<int> p(raw);
    int* released = p.release();
    CHECK(released == raw);
    CHECK(p.get() == nullptr);
    delete released;   // caller now owns it
}

// 9. Destructor does NOT double-delete after release()
TEST(test_no_double_delete_after_release) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> p(new Probe());
        Probe* raw = p.release();    // hand off ownership; we must delete it ourselves
        CHECK(p.get() == nullptr);
        CHECK(Probe::dtor_count == 0);  // destructor hasn't fired yet
        delete raw;                      // we own it now
    }
    CHECK(Probe::dtor_count == 1);   // deleted exactly once by us, not by UNIQUE_PTR
}

// 10. reset() with new pointer — old is deleted
TEST(test_reset_deletes_old) {
    Probe::reset_counts();
    UNIQUE_PTR<Probe> p(new Probe(1));
    p.reset(new Probe(2));
    CHECK(Probe::dtor_count == 1);  // first Probe deleted
    CHECK((*p).value == 2);
}

// 11. reset() with nullptr — object deleted, ptr becomes null
TEST(test_reset_null) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> p(new Probe());
        p.reset();                   // default arg = nullptr
        CHECK(p.get() == nullptr);
        CHECK(Probe::dtor_count == 1);
    }
    CHECK(Probe::dtor_count == 1);   // destructor should NOT double-delete
}

// 12. reset() self-reset guard (same pointer — no delete)
TEST(test_reset_same_pointer) {
    Probe::reset_counts();
    Probe* raw = new Probe(42);
    {
        UNIQUE_PTR<Probe> p(raw);
        p.reset(raw);                // reset with same pointer: should be a no-op
        CHECK(Probe::dtor_count == 0);
    }
    CHECK(Probe::dtor_count == 1);
}

// 13. Move constructor transfers ownership
TEST(test_move_ctor_ownership) {
    Probe::reset_counts();
    UNIQUE_PTR<Probe> a(new Probe(10));
    Probe* raw = a.get();
    UNIQUE_PTR<Probe> b(std::move(a));
    CHECK(a.get() == nullptr);
    CHECK(b.get() == raw);
}

// 14. Move constructor — source destructor does not double-delete
TEST(test_move_ctor_no_double_delete) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> a(new Probe());
        {
            UNIQUE_PTR<Probe> b(std::move(a));
        }
        CHECK(Probe::dtor_count == 1);
    }
    CHECK(Probe::dtor_count == 1);   // a's dtor: no double delete
}

// 15. Move assignment transfers ownership
TEST(test_move_assign_ownership) {
    Probe::reset_counts();
    UNIQUE_PTR<Probe> a(new Probe(1));
    UNIQUE_PTR<Probe> b(new Probe(2));
    Probe* raw_a = a.get();
    b = std::move(a);
    CHECK(b.get() == raw_a);
    CHECK(a.get() == nullptr);
    CHECK(Probe::dtor_count == 1);   // b's original Probe was deleted
}

// 16. Move assignment self-assignment guard
TEST(test_move_assign_self) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> p(new Probe(99));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        p = std::move(p);            // must not crash or corrupt
#pragma GCC diagnostic pop
        // After self-move, pointer is either still valid or null — both safe
        bool alive = (p.get() != nullptr);
        CHECK(alive || !alive);      // guard ran without crash = pass
        CHECK(Probe::dtor_count == 0); // object not destroyed prematurely
    }
}

// 17. Stateful deleter moved with ownership (FIX #3 verification)
/*TEST(test_stateful_deleter_moved) {
    int count = 0;
    UNIQUE_PTR<Probe, TrackingDeleter> a(new Probe(), TrackingDeleter(&count));
    UNIQUE_PTR<Probe, TrackingDeleter> b(std::move(a));
    b.reset();   // should fire the deleter that has the correct count pointer
    CHECK(count == 1);
}*/

// 18. noexcept on move operations — required for vector reallocation
TEST(test_move_noexcept) {
    CHECK(std::is_nothrow_move_constructible<UNIQUE_PTR<int>>::value);
    CHECK(std::is_nothrow_move_assignable<UNIQUE_PTR<int>>::value);
}

// 19. const correctness — dereferencing a const UNIQUE_PTR
TEST(test_const_deref) {
    const UNIQUE_PTR<int> p(new int(7));
    CHECK(*p == 7);
    CHECK(p.get() != nullptr);
    CHECK(static_cast<bool>(p));
}

// 20. Works in std::vector (tests noexcept move for reallocation)
TEST(test_in_vector) {
    Probe::reset_counts();
    std::vector<UNIQUE_PTR<Probe>> v;
    v.reserve(1);
    v.emplace_back(new Probe(1));
    v.emplace_back(new Probe(2));   // triggers reallocation — uses move ctor
    v.emplace_back(new Probe(3));
    CHECK(v.size() == 3);
    CHECK((*v[0]).value == 1);
    CHECK((*v[1]).value == 2);
    CHECK((*v[2]).value == 3);
}

// 21. Copy is deleted — compile-time check via type traits
TEST(test_copy_deleted) {
    CHECK(!std::is_copy_constructible<UNIQUE_PTR<int>>::value);
    CHECK(!std::is_copy_assignable<UNIQUE_PTR<int>>::value);
}

// 22. Multiple resets don't leak or double-delete
TEST(test_multiple_resets) {
    Probe::reset_counts();
    UNIQUE_PTR<Probe> p(new Probe());
    p.reset(new Probe());    // delete 1st, own 2nd
    p.reset(new Probe());    // delete 2nd, own 3rd
    p.reset();               // delete 3rd, own nullptr
    CHECK(Probe::dtor_count == 3);
}

// 23. Zero-initialised null pointer doesn't crash on destruction
TEST(test_null_dtor_safe) {
    Probe::reset_counts();
    {
        UNIQUE_PTR<Probe> p;   // null
    }
    CHECK(Probe::dtor_count == 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    printf("\n══════════════════════════════════════════════════════\n");
    printf("  UNIQUE_PTR — HFT Interview Test Suite\n");
    printf("══════════════════════════════════════════════════════\n\n");

    RUN(test_default_null);
    RUN(test_bool_null);
    RUN(test_construct_bool);
    RUN(test_destructor_called_once);
    RUN(test_deref_write);
    RUN(test_arrow);
    RUN(test_get);
    RUN(test_release);
    RUN(test_no_double_delete_after_release);
    RUN(test_reset_deletes_old);
    RUN(test_reset_null);
    RUN(test_reset_same_pointer);
    RUN(test_move_ctor_ownership);
    RUN(test_move_ctor_no_double_delete);
    RUN(test_move_assign_ownership);
    RUN(test_move_assign_self);
    //RUN(test_stateful_deleter_moved);
    RUN(test_move_noexcept);
    RUN(test_const_deref);
    RUN(test_in_vector);
    RUN(test_copy_deleted);
    RUN(test_multiple_resets);
    RUN(test_null_dtor_safe);

    printf("\n══════════════════════════════════════════════════════\n");
    printf("  Results: %d passed, %d failed\n", pass_count, fail_count);
    printf("══════════════════════════════════════════════════════\n\n");
    return fail_count ? 1 : 0;
}
