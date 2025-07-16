#include "server/Server.hpp"
#include "Database.hpp"
#include "Logger.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <csignal>

int main() {
    Logger::init_thread_pool();  // Инициализировать thread pool до первого лога!
    auto log = Logger::get();

    try {
        // Получить количество доступных ядер процессора
        unsigned int network_threads = std::thread::hardware_concurrency();
        if (network_threads > 2) {
            network_threads -= 1; // оставим 1 поток под систему и логи, если допустимых потоков мало
        } else if (network_threads == 0) {
            network_threads = 1; // безопасное значение по умолчанию
        }
        int port = 6112;

        // 🟢 Разделяем io_context и thread_pool
        boost::asio::io_context io_context;
        boost::asio::thread_pool pool(network_threads);

        // 🟢 Настройка БД
        auto db = std::make_shared<Database>(
                fmt::format("host={} port={} user={} password={} dbname={}",
                            std::getenv("DB_URL") ?: "127.0.0.1",
                            std::getenv("DB_PORT") ?: "5432",
                            std::getenv("DB_USER") ?: "postgres",
                            std::getenv("DB_PASSWORD") ?: "postgres",
                            std::getenv("DB_NAME") ?: "postgres"),
                2   // Для каждого потока должна быть своя сессия к бд
        );

        auto server = std::make_shared<Server>(io_context, pool, db, port);
        server->start_accept();
        log->info("[Server] Running on port {}", port);

        // 🟢 Перехват SIGINT
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code &, int signal_number) {
            log->info("[Server] Signal {} received, shutting down...", signal_number);
            server->stop();
            io_context.stop();
            pool.stop();
        });

        // 🟢 Прокачиваем io_context внутри pool
        for (unsigned int i = 0; i < network_threads; ++i) {
            boost::asio::post(pool, [&io_context]() {
                io_context.run();
            });
        }

        pool.join();
        log->info("[Server] Gracefully shut down.");
    } catch (const std::exception &e) {
        log->error("[Server] Exception: {}", e.what());
    }

    spdlog::shutdown();
    return 0;
}
