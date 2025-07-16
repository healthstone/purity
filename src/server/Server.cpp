#include "Server.hpp"
#include "ClientSession/ClientSession.hpp"
#include "Logger.hpp"

using boost::asio::ip::tcp;

Server::Server(boost::asio::io_context &io_context,
               boost::asio::thread_pool &pool,
               std::shared_ptr<Database> db,
               int port)
        : io_context_(io_context),
          pool_(pool),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          db_(std::move(db)) {}

void Server::start_accept() {
    if (!acceptor_.is_open()) return;

    acceptor_.async_accept(
            [self = shared_from_this()](boost::system::error_code ec, tcp::socket socket) {
                auto log = Logger::get();
                if (ec) {
                    if (ec == boost::asio::error::operation_aborted) {
                        log->info("[Server] Accept aborted");
                    }
                    else
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
                self->start_accept();
            }
    );
}

void Server::stop() {
    auto log = Logger::get();
    boost::system::error_code ec;
    acceptor_.cancel(ec);
    if (ec && ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
        log->error("[Server] Failed to cancel acceptor: {}", ec.message());
    }

    acceptor_.close(ec);
    if (ec && ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
        log->error("[Server] Failed to close acceptor: {}", ec.message());
    }

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto &s : sessions_) {
            if (s->isOpened()) s->close();
        }
        sessions_.clear();
    }

    // ✅ Корректно закрываем все DB connections:
    if (db_) db_->shutdown();

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
