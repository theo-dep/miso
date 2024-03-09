#include "miso.h"

#include <catch2/catch_test_macros.hpp>

namespace sender {
    struct a_class {
        int m_aa{ 0 };
        miso::signal<int> m_s;

        void say_hello() {
            emit m_s(42);
        }
    };

    struct functor {
        void operator()(int aa) {
            auto ap = miso::sender<a_class>();
            ap->m_aa = aa;
        }
    };
}

TEST_CASE("Test with sender", "[sender]" ) {
    sender::a_class a;
    sender::functor f;

    miso::connect(a.m_s, f);

    a.say_hello();

    REQUIRE(a.m_aa == 42);
}
