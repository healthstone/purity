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
        int port = 3724;

        // 🟢 Используем только io_context
        boost::asio::io_context io_context;

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

        auto server = std::make_shared<Server>(io_context, db, port);
        server->start_accept();
        log->info("[Server] Running on port {}", port);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code &, int signal_number) {
            log->info("[Server] Signal {} received, shutting down...", signal_number);
            server->stop();
        });

        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < network_threads; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        for (auto &t : threads) t.join();

        log->info("[Server] Gracefully shut down.");
    } catch (const std::exception &e) {
        log->error("[Server] Exception: {}", e.what());
    }

    spdlog::shutdown();
    return 0;
}
