#ifndef MISO_H
#define MISO_H

#include <unordered_map>
#include <sstream>
#include <string>

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <tuple>
#include <stack>

namespace miso
{
    template <class... Args> class signal;

    namespace internal {

        template<int ...> struct sequence {};
        template<int N, int ...S> struct sequence_generator : sequence_generator<N - 1, N - 1, S...> {};
        template<int ...S> struct sequence_generator<0, S...> { typedef sequence<S...> type; };

        template<typename FT>
        struct func_and_bool final {
            std::shared_ptr<FT> ft;
            void *addr;
        };

        struct common_slot_base {
            virtual ~common_slot_base() = default;
        };

        struct connecter {
            using slot_holder_hash = std::unordered_map<std::string, std::shared_ptr<common_slot_base>>;
            slot_holder_hash sh_hash;

        template<class T, class FT, class SHT>
            void connect_i(T &&f, std::vector<common_slot_base *> &sholders, bool active) {
            std::string sh_key = typeid(T).name() + std::string(typeid(FT).name()) + static_cast<std::ostringstream&>(
                                    std::ostringstream().flush() << reinterpret_cast<uintptr_t>(&f)
                                 ).str();

                slot_holder_hash::const_iterator sh_it = sh_hash.find(sh_key);
                if (sh_it == sh_hash.cend()) {
                    sh_it = sh_hash.insert(sh_it, std::make_pair(sh_key, std::make_shared<SHT>()));
                }

                std::shared_ptr<SHT> sh = std::static_pointer_cast<SHT>(sh_it->second);

            func_and_bool<FT> fb{ std::make_shared<FT>(std::forward<T>(f)), reinterpret_cast<void *>(&f) };

            if (active) {
                    if (std::find_if(sh->slots.cbegin(), sh->slots.cend(), [&](const func_and_bool<FT> &s) { return (s.addr == fb.addr); }) == sh->slots.cend()) {
                        sh->slots.emplace_back(fb);
                }
                    if (std::find(sholders.cbegin(), sholders.cend(), static_cast<common_slot_base *>(sh.get())) == sholders.cend()) {
                        sholders.push_back(sh.get());
                }
            } else {
                    sh->slots.erase(std::remove_if(sh->slots.begin(), sh->slots.end(), [&](const func_and_bool<FT> &s) { return (s.addr == fb.addr); }), sh->slots.end());
                    if (sh->slots.empty()) {
                        sholders.erase(std::remove(sholders.begin(), sholders.end(), static_cast<common_slot_base *>(sh.get())), sholders.end());
                    }
                }
            }
        };

        template<class T>
        struct emitter final {
            explicit emitter(T* emtr) : sender_obj(emtr) {
                minstances.push(this);
            }

            ~emitter() {
                minstances.pop();
            }

            static T *sender() {
                if (!instance()) return nullptr;
                return instance()->sender_obj;
            }

            static emitter<T> *instance() {
                if (minstances.empty()) return nullptr;
                return minstances.top();
            }

        private:
            T* sender_obj;
            static std::stack<emitter<T>*> minstances;
        };

        template<class T> std::stack<emitter<T>*> emitter<T>::minstances;

        template<class T, class... Args>
        emitter<T> &&operator <<(emitter<T> &&e, signal<Args...> &s) {
            s.delayed_dispatch();
            return std::forward<emitter<T>>(e);
        }
    }

	template <class... Args>
	class signal final
	{
        struct slot_holder_base : public internal::common_slot_base {
            virtual void run_slots(const Args&... args) = 0;
        };

        template<class T>
        struct slot_holder : public slot_holder_base {
            using FT = std::function<typename std::result_of<T(const Args&...)>::type(const Args&...)>;
            using slot_vec_type = std::vector<internal::func_and_bool<FT>>;
            slot_vec_type slots;

            void run_slots(const Args&... args) override {
                std::for_each(slots.rbegin(), slots.rend(), [&](internal::func_and_bool<FT>& s)
                              { (*(s.ft.get()))(args...); }
                );
            }
        };

        std::vector<internal::common_slot_base*> slot_holders;
        std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...> call_args;
        internal::connecter connecter;

        void emit_signal(const Args&... args) {
            std::for_each(slot_holders.rbegin(), slot_holders.rend(), [&](internal::common_slot_base* sh)
                          { (dynamic_cast<slot_holder_base*>(sh))->run_slots(args...); }
                );
        }

        template<int ...S>
        void delayed_call(const internal::sequence<S...>&)	{
            emit_signal(std::get<S>(call_args) ...);
        }

        void delayed_dispatch()	{
            delayed_call(typename internal::sequence_generator<sizeof...(Args)>::type());
        }

    public:
        template<class T, class... Brgs> friend
        internal::emitter<T> && internal::operator <<(internal::emitter<T> &&e, signal<Brgs...> &s);

		explicit signal() = default;
		~signal() noexcept = default;

		template<class T>
		void connect(T&& f) {
			connecter.connect_i<T, typename slot_holder<T>::FT,
			          slot_holder<T>> (std::forward<T>(f), slot_holders, true);
		}

		template<class T>
		void disconnect(T&& f) {
			connecter.connect_i<T, typename slot_holder<T>::FT,
			          slot_holder<T>> (std::forward<T>(f), slot_holders, false);
		}

        signal<Args...>& operator()(const Args&... args) {
            call_args = std::tuple<Args...>(args...);
            return *this;
        }
	};

    template<class Si, class So>
    void connect(Si &&sig, So &&slo) {
        static_assert(!std::is_same<So, std::nullptr_t>::value, "cannot use nullptr as slot");
        std::forward<Si>(sig).connect(std::forward<So>(slo));
    }

    template<class Si, class So>
    void disconnect(Si &&sig, So &&slo) {
        static_assert(!std::is_same<So, std::nullptr_t>::value, "cannot use nullptr as slot");
        std::forward<Si>(sig).disconnect(std::forward<So>(slo));
    }

    template<class T>
    T *sender() {
        if(internal::emitter<T>::instance()) {
            return internal::emitter<T>::instance()->sender();
        }
        throw std::runtime_error("not in an emit call");
    }

    #define emit miso::internal::emitter<std::remove_pointer<decltype(this)>::type>(this) <<

}

#endif
