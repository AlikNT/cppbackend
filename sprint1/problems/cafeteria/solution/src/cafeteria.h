#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using Timer = net::steady_timer;

using namespace std::literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        auto strand = net::make_strand(io_);
        // Create a new HotDog object
        std::shared_ptr<Bread> bread = store_.GetBread();
        std::shared_ptr<Sausage> sausage = store_.GetSausage();

        // Create a lambda function to handle the completion of baking and frying
        auto on_completion = [handler, bread, sausage, this]() {
            if (bread->IsCooked() && sausage->IsCooked()) {
                try {
                    // Create a hot dog from the baked bread and fried sausage
                    HotDog hot_dog(++next_hotdog_id_, sausage, bread);
                    // Call the handler with the result
                    handler(Result<HotDog>{std::move(hot_dog)});
                } catch (const std::exception &e) {
                    // Call the handler with the error
                    handler(Result<HotDog>{std::current_exception()});
                }
            }
        };

//        net::strand<net::io_context::executor_type> strand_{net::make_strand(io_)};
        net::post(strand, [bread, sausage, this, on_completion]() {
            bread->StartBake(*gas_cooker_, [bread, sausage, this, on_completion]() {
                auto bake_timer = std::make_shared<Timer>(io_, HotDog::MIN_BREAD_COOK_DURATION);
                bake_timer->async_wait([bake_timer, bread, this, sausage, on_completion](const sys::error_code& ec) {
                    bread->StopBaking();
                    on_completion();
/*                    if (sausage->IsCooked()) {
                        handler(Result<HotDog>({++next_hotdog_id_, sausage, bread}));
                    }*/
                });
            });
        });
        net::post(strand, [bread, sausage, this, on_completion]() {
            sausage->StartFry(*gas_cooker_, [bread, sausage, this, on_completion]() {
                auto fry_timer = std::make_shared<Timer>(io_, HotDog::MIN_SAUSAGE_COOK_DURATION);
                fry_timer->async_wait([fry_timer, bread, sausage, this, on_completion](const sys::error_code& ec) {
                    sausage->StopFry();
                    on_completion();
                    /*if (bread->IsCooked()) {
                        handler(Result<HotDog>({++next_hotdog_id_, sausage, bread}));
                    }*/
                });
            });
        });
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    std::atomic<int> next_hotdog_id_{0};
};
