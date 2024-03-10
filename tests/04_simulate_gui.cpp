#include "miso.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace simulate_gui {
    struct more_class {
        more_class() = default;

        void run(int v, const std::string& s = "Hurra") {
            emit ms(v);
            emit os(v, s);
        }

        miso::signal<int> ms;
        miso::signal<int, std::string> os;
        miso::signal<float, int> float_int_sig;
    };

    void global_int_method(int s) {
    	more_class* x = miso::sender<more_class>();
    	if (s == 7) x->run(1234, "Hu");
    }

    void other_global_int_method(int i) {
      	more_class* x = miso::sender<more_class>();
        if (i == 1234) x->run(4321, "Ha");
    }

    void global_with_two_parameters(int i, std::string s) {
        more_class* x = miso::sender<more_class>();
        if (i == 4321) x->run(8, "HipHip!");
    }

    class other_class {
    public:
        other_class() = default;
        other_class(int v) : nr(v) {}

        bool clickedIsCalled{ false };
        void clicked() {
            clickedIsCalled = true;
        }

        int clickedAgain{ 0 };
        void clicked_again(int v) {
            clickedAgain = v;
        }

    private:
        int nr = 23;
    };

    class my_class {
    public:
    	my_class(int f) : mf(f) {}

        void some_method() {
            emit click();
        }

        miso::signal<> click;
    	int mf = 0;
    };

    void global_void_method() {
    	auto x = miso::sender<my_class>();
    	x->mf++;
    }

    struct functor {
        static bool s_functionVoidIsCalled;
    	void operator()() {
    		s_functionVoidIsCalled = true;
    	}

        static int s_functionIntIsCalled;
    	void operator()(int aa) {
    		s_functionIntIsCalled = aa;
    	}
    };

    bool functor::s_functionVoidIsCalled{ false };
    int functor::s_functionIntIsCalled{ 0 };
}

SCENARIO("Simulate connect and disconnect with click", "[simulateGui]" ) {

    GIVEN("A source class and an other class") {

        simulate_gui::my_class src(56);
        simulate_gui::other_class dst(4);
        simulate_gui::functor f;

        WHEN("lambdas are connected") {
            bool lambdvIsCalled{ false };
            auto lambdv = [&lambdvIsCalled]() { lambdvIsCalled = true; };

            miso::connect(src.click, lambdv);
            miso::connect(src.click, std::bind(&simulate_gui::other_class::clicked, std::ref(dst)));
            miso::connect(src.click, simulate_gui::global_void_method);
            miso::connect(src.click, f);

            THEN("clicked is emitted") {
                src.some_method();

                REQUIRE(lambdvIsCalled);
                REQUIRE(dst.clickedIsCalled);
                REQUIRE(src.mf == 56 + 1);
                REQUIRE(simulate_gui::functor::s_functionVoidIsCalled);
            }
        }

        AND_WHEN("lambdas are disconnected") {
            bool lambdvIsCalled{ false };
            auto lambdv = [&lambdvIsCalled]() { lambdvIsCalled = true; };

            miso::connect(src.click, lambdv);
            miso::disconnect(src.click, lambdv);

            THEN("clicked is not emitted") {
                src.some_method();

                REQUIRE(lambdvIsCalled == false);
            }
        }
    }

    AND_GIVEN("More class which simulate an app and chained signals") {

        simulate_gui::more_class mc;
        simulate_gui::other_class dst(4);
        simulate_gui::functor f;

        WHEN("lambdas are connected") {

            miso::connect(mc.ms, simulate_gui::global_int_method);
            miso::connect(mc.ms, simulate_gui::other_global_int_method);
            miso::connect(mc.ms, f);

            miso::connect(mc.os, simulate_gui::global_with_two_parameters);

            int lambdiIsCalled{ 0 };
            auto lambdi = [&lambdiIsCalled](int c) { lambdiIsCalled = c; };
            miso::connect(mc.ms, lambdi);

            std::pair<int, std::string> lambdisIsCalled{ 0, "" };
            auto lambdis = [&lambdisIsCalled](int c, std::string s) { lambdisIsCalled = std::make_pair(c, s); };
            miso::connect(mc.os, lambdis);

            miso::connect(mc.ms, std::bind(&simulate_gui::other_class::clicked_again, std::ref(dst), std::placeholders::_1));

            THEN("run is called") {
                mc.run(7);

                REQUIRE(lambdiIsCalled == 8);
                REQUIRE(lambdisIsCalled.first == 7); // decreasing
                REQUIRE(lambdisIsCalled.second == "Hurra"); // decreasing
                REQUIRE(simulate_gui::functor::s_functionIntIsCalled == 8);
                REQUIRE(dst.clickedAgain == 8);
            }

            AND_THEN("lambdas are disconnected") {
                miso::disconnect(mc.ms, simulate_gui::global_int_method);
            	miso::disconnect(mc.ms, simulate_gui::other_global_int_method);
            	miso::disconnect(mc.ms, lambdi);
            	miso::disconnect(mc.os, lambdis);

            	mc.run(8);

            	REQUIRE(lambdiIsCalled == 0);
                REQUIRE(lambdisIsCalled.first == 0);
                REQUIRE(lambdisIsCalled.second == "");
                REQUIRE(simulate_gui::functor::s_functionIntIsCalled == 8);
                REQUIRE(dst.clickedAgain == 8);
            }
        }
    }

}
