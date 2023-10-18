#include "vector.h"
#include <iostream>

using namespace std;

constexpr std::size_t SIZE = 8u;
constexpr int MAGIC = 42;
constexpr uint32_t DEFAULT_COOKIE = 0xdeadbeef;

template<bool MoveNoexcept>
struct WithCopy {
    WithCopy() noexcept {
        ++def_ctor;
    }

    WithCopy(const int&) noexcept {
        ++copy_with_val;
    }

    WithCopy(int&&) noexcept {
        ++move_with_val;
    }

    WithCopy(const WithCopy& /*other*/) noexcept {
        ++copy_ctor;
    }

    WithCopy(WithCopy&& /*other*/) noexcept(MoveNoexcept) {
        ++move_ctor;
    }

    WithCopy& operator=(const WithCopy& other) noexcept {
        if (this != &other) {
            ++copy_assign;
        }
        return *this;
    }

    WithCopy& operator=(WithCopy&& /*other*/) noexcept {
        ++move_assign;
        return *this;
    }

    ~WithCopy() {
        ++dtor;
    }

    static size_t InstanceCount() {
        return def_ctor + copy_ctor + move_ctor - dtor;
    }

    static void Reset() {
        def_ctor = 0;
        copy_ctor = 0;
        move_ctor = 0;
        copy_assign = 0;
        move_assign = 0;
        dtor = 0;
        copy_with_val = 0;
        move_with_val = 0;
    }

    inline static size_t def_ctor = 0;
    inline static size_t copy_ctor = 0;
    inline static size_t move_ctor = 0;
    inline static size_t copy_assign = 0;
    inline static size_t move_assign = 0;
    inline static size_t dtor = 0;
    inline static size_t copy_with_val = 0;
    inline static size_t move_with_val = 0;

};

template<bool MoveNoexcept>
struct WithoutCopy {
    WithoutCopy() noexcept {
        ++def_ctor;
    }

    WithoutCopy(const int&) noexcept {
        ++copy_with_val;
    }

    WithoutCopy(int&&) noexcept {
        ++move_with_val;
    }

    WithoutCopy(const WithoutCopy& /*other*/) = delete;

    WithoutCopy(WithoutCopy&& /*other*/) noexcept(MoveNoexcept) {
        ++move_ctor;
    }

    WithoutCopy& operator=(const WithoutCopy& other) noexcept {
        if (this != &other) {
            ++copy_assign;
        }
        return *this;
    }

    WithoutCopy& operator=(WithoutCopy&& /*other*/) noexcept {
        ++move_assign;
        return *this;
    }

    ~WithoutCopy() {
        ++dtor;
    }

    static size_t InstanceCount() {
        return def_ctor + copy_ctor + move_ctor - dtor;
    }

    static void Reset() {
        def_ctor = 0;
        copy_ctor = 0;
        move_ctor = 0;
        copy_assign = 0;
        move_assign = 0;
        dtor = 0;
        copy_with_val = 0;
        move_with_val = 0;
    }

    inline static size_t def_ctor = 0;
    inline static size_t copy_ctor = 0;
    inline static size_t move_ctor = 0;
    inline static size_t copy_assign = 0;
    inline static size_t move_assign = 0;
    inline static size_t dtor = 0;
    inline static size_t copy_with_val = 0;
    inline static size_t move_with_val = 0;
};

using C = WithCopy<true>;
using move_without_noexcept = WithCopy<false>;

using move_noexcept_no_copy = WithoutCopy<true>;
using move_without_noexcept_no_copy = WithoutCopy<false>;

template<typename OBJ>
void TestEmplaceAdditionalCopyImpl(size_t copy, size_t move) {
    {
        int a{ MAGIC };
        Vector <OBJ> v(SIZE);
        OBJ::Reset();
        v.Emplace(v.begin(), a);
        assert(OBJ::def_ctor == 0u);
        assert(OBJ::copy_ctor == copy);
        assert(OBJ::move_ctor == move);
        assert(OBJ::copy_assign == 0u);
        assert(OBJ::move_assign == 0u);
        assert(OBJ::dtor == SIZE);
        assert(OBJ::copy_with_val == 1u);
        assert(OBJ::move_with_val == 0u);
    }
    {
        int a{ MAGIC };
        Vector <OBJ> v(SIZE);
        v.Reserve(2 * SIZE);
        OBJ::Reset();
        v.Emplace(v.begin(), a);
        assert(OBJ::def_ctor == 0u);
        assert(OBJ::copy_ctor == 0u);
        assert(OBJ::move_ctor == 1u);
        assert(OBJ::copy_assign == 0u);
        assert(OBJ::move_assign == SIZE);
        assert(OBJ::dtor == 1u);
        assert(OBJ::copy_with_val == 1u);
        assert(OBJ::move_with_val == 0u);
    }
}
template<typename OBJ>
void TestEmplaceAdditionalMoveImpl(size_t copy, size_t move) {
    {
        int a{ MAGIC };
        Vector<OBJ> v(SIZE);
        OBJ::Reset();
        v.Emplace(v.begin(), std::move(a));
        assert(OBJ::def_ctor == 0u);
        assert(OBJ::copy_ctor == copy);
        assert(OBJ::move_ctor == move);
        assert(OBJ::copy_assign == 0u);
        assert(OBJ::move_assign == 0u);
        assert(OBJ::dtor == SIZE);
        assert(OBJ::copy_with_val == 0u);
        assert(OBJ::move_with_val == 1u);
    } {
        int a{ MAGIC };
        Vector<OBJ> v(SIZE);
        v.Reserve(2 * SIZE);
        OBJ::Reset();
        v.Emplace(v.begin(), std::move(a));
        assert(OBJ::def_ctor == 0u);
        assert(OBJ::copy_ctor == 0u);
        assert(OBJ::move_ctor == 1u);
        assert(OBJ::copy_assign == 0u);
        assert(OBJ::move_assign == SIZE);
        assert(OBJ::dtor == 1u);
        assert(OBJ::copy_with_val == 0u);
        assert(OBJ::move_with_val == 1u);
    }
}

void TestEmplaceAdditional_move_noexcept_copy() {
    TestEmplaceAdditionalCopyImpl<C>(0u, SIZE);
    TestEmplaceAdditionalMoveImpl<C>(0u, SIZE);
}
void TestEmplaceAdditional_move_without_noexcept_copy() {
    TestEmplaceAdditionalCopyImpl<move_without_noexcept>(SIZE, 0u);
    TestEmplaceAdditionalMoveImpl<move_without_noexcept>(SIZE, 0u);
}

int main() {
    TestEmplaceAdditional_move_noexcept_copy();
    TestEmplaceAdditional_move_without_noexcept_copy();
    cerr << "Tests passed"s << endl;
    return 0;
}