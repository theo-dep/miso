#include "miso.h"

#include <catch2/catch_test_macros.hpp>

namespace class_connect {
    struct d_class {
        int hh = 67;
    };

    struct a_class {
        miso::signal<d_class> m_s;

        void say_hello() {
            emit m_s(d_class());
        }
    };

    struct functor {
        static int s_aahh;
        void operator()(d_class aa) {
            s_aahh = aa.hh;
        }
    };

    int functor::s_aahh{ 0 };
}

TEST_CASE("Test with a functor and a class", "[functorClass]") {
    class_connect::a_class a;
    class_connect::functor f;

    miso::connect(a.m_s, f);

    a.say_hello();

    REQUIRE(class_connect::functor::s_aahh == 67);
}
