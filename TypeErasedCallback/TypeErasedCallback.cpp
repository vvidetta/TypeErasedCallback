// TypeErasedCallback.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <functional>
#include <numeric>

using namespace std::chrono;
using namespace std::chrono_literals;

template <typename EventData>
class EventHandler
{
    void* m_context;
    void (*m_fp)(EventData const&, void*);
    void (*m_invoke) (void(* const)(EventData const&, void*), EventData const&, void*);

    template <typename T>
    static void invokePMF(void (*fp)(EventData const&, void*), EventData const& event_data, void* context)
    {
        auto pmf = reinterpret_cast<void(T::*)(EventData const&)>(fp);
        auto& obj = *reinterpret_cast<T*>(context);
        (obj.*pmf)(event_data);
    }

    static void invokeFree(void (*fp)(EventData const&, void*), EventData const& event_data, void* context)
    {
        reinterpret_cast<void(*)(EventData const&, void*)>(fp)(context, event_data);
    }

public:

    using EventDataType = EventData;

    // VV 2023-09-03 PMFs are tricky; need to work out how to cast it to function pointer (Is that even possible?)
    //template <typename T>
    //EventHandler(void (T::* pmf)(EventData const&), T& context)
    //    : m_context{ &context },
    //    m_fp{ reinterpret_cast<void(*)(EventData const&, void *)>(const_cast<T const&>(context).*pmf)},
    //    m_invoke{ &EventHandler<EventData>::template invokePMF<T> }
    //{ }

    EventHandler(void (*fp)(EventData const&, void*), void* priv)
        : m_context{ priv },
        m_fp{ fp },
        m_invoke{ &EventHandler<EventData>::invokeFree }
    { }

    void operator()(EventDataType const& event_data) const
    {
        m_invoke(m_fp, event_data, m_context);
    }

};

using Event_Data = int;

struct S
{
    void onEvent(Event_Data const& event_data)
    {
        std::cout << "Event raised on " << this << " with data " << event_data << std::endl;
    }

    static void staticEventHandler(Event_Data const& event_data, void* priv)
    {
        auto& self = *reinterpret_cast<S*>(priv);
        self.onEvent(event_data);
    }
};

template <typename EventData>
struct Callback
{
    void (*fp)(EventData const&, void*);
    void* priv;
};

int main()
{
    auto s = S{};

    //auto const event_handler = EventHandler<Event_Data>{ &S::onEvent, s };
    auto const callback = Callback<Event_Data>{ &S::staticEventHandler, &s };

    auto const event_data = Event_Data{ 42 };

    using Clock = system_clock;
    auto constexpr trials = 10000;
    auto t = std::array<Clock::duration, trials>{};

    auto const average = [&t, trials]
    {
        return duration_cast<microseconds>(std::accumulate(std::begin(t), std::end(t), Clock::duration{ 0 })) / trials;
    };

    for (auto i = 0; i < trials; ++i)
    {
        auto const start = Clock::now();
        auto const end = Clock::now();
        t[i] = end - start;
    }
    auto const overhead = average();

    for (auto i = 0; i < trials; ++i)
    {
        auto const start = Clock::now();
        s.onEvent(event_data);
        auto const end = Clock::now();
        t[i] = end - start;
    }
    auto const direct = average();

    //for (auto i = 0; i < trials; ++i)
    //{
    //    auto const start = Clock::now();
    //    event_handler(event_data);
    //    auto const end = Clock::now();
    //    t[i] = end - start;
    //}
    //auto const eventHandlerTime = average();

    for (auto i = 0; i < trials; ++i)
    {
        auto const start = Clock::now();
        callback.fp(event_data, callback.priv);
        auto const end = Clock::now();
        t[i] = end - start;
    }
    auto const callbackTime = average();

    std::cout << "Overhead = " << overhead.count() << "\n"
        << "direct = " << direct.count() << "\n"
        //<< "eventHandlerTime = " << eventHandlerTime.count() << "\n"
        << "callbackTime = " << callbackTime.count() << "\n"
        << "Sizeof EventHandler = " << sizeof(EventHandler<Event_Data>) << "\n"
        << "Sizeof callback = " << sizeof(Callback<Event_Data>) << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
