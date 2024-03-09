#include "miso.h"

#include <catch2/catch_test_macros.hpp>

namespace simple_connect {
    struct a_class {
        miso::signal<int> m_s;

        void say_hello() {
            emit m_s(42);
        }
    };

    struct functor {
        static int s_aa;
        void operator()(int aa) {
            s_aa += aa;
        }
    };

    int functor::s_aa{ 0 };
}

TEST_CASE("Test with a functor", "[functor]" ) {
    simple_connect::a_class a;
    simple_connect::functor f;

    SECTION("Call the functor directly") {
        miso::connect(a.m_s, f);

        a.say_hello();

        REQUIRE(simple_connect::functor::s_aa == 42);
    }

    SECTION("Call the functor inside a return function") {
        auto retFun = []() {
            return []() -> simple_connect::functor { return simple_connect::functor(); };
        };

        miso::connect(a.m_s, retFun()());

        a.say_hello();

        REQUIRE(simple_connect::functor::s_aa == 2 * 42);
    }
}
