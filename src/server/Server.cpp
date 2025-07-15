#include "Server.hpp"
#include "ClientSession/ClientSession.hpp"
#include <utility>

using boost::asio::ip::tcp;

Server::Server(boost::asio::thread_pool &pool, boost::asio::any_io_executor executor, std::shared_ptr<Database> db,
               int port)
        : pool_(pool),
          executor_(std::move(executor)),
          db_(std::move(db)),
          acceptor_(executor_, tcp::endpoint(tcp::v4(), port)) {}

void Server::start_accept() {
    acceptor_.async_accept(
            [self = shared_from_this()](boost::system::error_code ec, tcp::socket socket) {
                auto log = Logger::get();
                if (ec) {
                    if (ec == boost::asio::error::operation_aborted ||
                        ec == boost::asio::error::eof) {
                        // Сервер остановлен — молча выходим
                        return;
                    }
                    log->error("[Server] Accept failed: {}", ec.message());
                    return;
                }

                auto session = std::make_shared<ClientSession>(std::move(socket), self);

                {
                    std::lock_guard<std::mutex> lock(self->sessions_mutex_);
                    self->sessions_.insert(session);
                    log->info("[Server] New client connected.");
                    self->log_session_count();
                }

                session->start();
                self->start_accept();  // Принимаем следующий
            });
}

void Server::stop() {
    auto log = Logger::get();
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (ec) {
        if (ec == boost::asio::error::operation_aborted ||
            ec == boost::asio::error::eof) {
            log->info("[Server] Acceptor closed.");

        } else {
            log->error("[Server] Failed to close acceptor: {}", ec.message());
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto &session: sessions_) {
            if (session->isOpened())
                session->close();
        }
        sessions_.clear();
    }

    // ✅ Корректно закрываем все DB connections:
    if (db_) {
        db_->shutdown();
    }

    log_session_count();
}

void Server::remove_session(std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(session);
    log_session_count();
}

void Server::log_session_count() {
    Logger::get()->info("[Server] Active sessions: {}", sessions_.size());
}
