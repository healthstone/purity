#include "server/Server.hpp"
#include "Database.hpp"
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
        int port = 12345;

        // Единый thread pool на всё приложение
        boost::asio::thread_pool pool(network_threads);

        // 🟢 Инициализируем базу данных (передаём тот же пул!)
        std::string db_host = std::getenv("DB_URL") ? std::getenv("DB_URL") : "127.0.0.1";
        log->debug("DB_URL={}", db_host);

        std::string db_port = std::getenv("DB_PORT") ? std::getenv("DB_PORT") : "5432";
        log->debug("DB_PORT={}", db_port);

        std::string db_user = std::getenv("DB_USER") ? std::getenv("DB_USER") : "postgres";
        log->debug("DB_USER={}", db_user);

        std::string db_password = std::getenv("DB_PASSWORD") ? std::getenv("DB_PASSWORD") : "postgres";
        log->debug("DB_PASSWORD={}", db_password);

        std::string db_name = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "postgres";
        log->debug("DB_NAME={}", db_name);

        auto db = std::make_shared<Database>(fmt::format("host={} port={} user={} password={} dbname={}",
                                                         db_host, db_port, db_user, db_password, db_name),
                                             network_threads); // Для каждого потока должна быть своя сессия к бд

        // Сервер получает ссылку на pool и готовую БД
        auto server = std::make_shared<Server>(pool, pool.get_executor(), db, port);

        server->start_accept();

        log->info("[Server] Running on port {}", port);

        // Перехват SIGINT/SIGTERM
        boost::asio::signal_set signals(pool.get_executor(), SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code &, int signal_number) {
            Logger::get()->info("[Server] Signal {} received, shutting down...", signal_number);
            server->stop();
            pool.stop();
        });

        // Запускаем все worker-потоки
        pool.join();

        log->info("[Server] Gracefully shut down.");

    } catch (const std::exception &e) {
        Logger::get()->error("[Server] Exception: {}", e.what());
    }

    return 0;
}
