#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include "client/Client.hpp"
#include "Logger.hpp"

int main() {
    Logger::init_thread_pool();  // Инициализировать thread pool до первого лога!
    boost::asio::io_context io;

    auto client = std::make_shared<Client>(io, "127.0.0.1", 12345);
    client->connect();

    // Отправка сообщений из cin на сервер в виде опкода MESSAGE
    // если не нужно, просто закомментить
    std::thread([client]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            client->send_message(line);
        }
    }).detach();

//    // Первый таймер — отправка первого пакета через 1000мс
//    boost::asio::steady_timer timer1(io);
//    timer1.expires_after(std::chrono::milliseconds(1000));
//    timer1.async_wait([&client](const boost::system::error_code &ec) {
//        if (!ec) {
//            Logger::get()->info("sending request on lookup account");
//            client->send_select_acc_by_username("test_User");
//        }
//    });

    // Настраиваем сигнал Ctrl+C
    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) {
        Logger::get()->info("[Main] Caught signal. Disconnecting client...");
        client->disconnect();

        // Останавливаем io_context после graceful shutdown
        io.stop();
    });

    io.run();

    Logger::get()->info("[Main] Exiting.");
    return 0;
}
