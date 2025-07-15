#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <pqxx/pqxx>
#include <boost/asio/thread_pool.hpp>
#include <mutex>
#include <queue>
#include <memory>
#include <optional>
#include "QueryResults.hpp"
#include "PreparedStatement.hpp"
#include "Logger.hpp"

class Database {
public:
    Database(boost::asio::thread_pool& pool, const std::string& conninfo, size_t pool_size = 4)
            : pool_(pool), conninfo_(conninfo)
    {
        for (size_t i = 0; i < pool_size; ++i) {
            auto conn = std::make_unique<pqxx::connection>(conninfo_);
            prepare_all(*conn);
            Logger::get()->info("[Database] Connection {} established", i + 1);
            connections_.push(std::move(conn));
        }
    }

    ~Database() {
        shutdown();
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!connections_.empty()) {
            auto& c = connections_.front();
            if (c && c->is_open()) {
                c->disconnect();
                Logger::get()->info("[Database] Connection closed.");
            }
            connections_.pop();
        }
    }

    /// Чётко называется как sync, чтобы никто не звал co_await
    template <typename Struct>
    std::optional<Struct> execute_sync(const PreparedStatement& stmt) {
        auto conn = acquire_connection();
        if (!conn) throw std::runtime_error("No DB connection available");

        try {
            pqxx::work txn(*conn);
            auto invoc = txn.prepared(stmt.name());
            for (const auto& param : stmt.params()) {
                if (param.has_value()) {
                    invoc(param.value());
                } else {
                    invoc(static_cast<const char*>(nullptr));
                }
            }

            auto result = invoc.exec();
            txn.commit();

            release_connection(std::move(conn));

            if constexpr (std::is_same_v<Struct, NothingRow>) {
                return Struct{};
            }

            if (result.empty()) {
                return std::nullopt;
            }

            return PgRowMapper<Struct>::map(result[0]);
        }
        catch (const pqxx::broken_connection&) {
            Logger::get()->error("[Database] Connection broken. Attempting to reconnect.");
            conn = reconnect_connection();
            if (!conn) throw std::runtime_error("Reconnection failed");

            release_connection(std::move(conn));
            throw;
        }
    }

    boost::asio::thread_pool& thread_pool() { return pool_; }

private:
    std::unique_ptr<pqxx::connection> acquire_connection() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connections_.empty()) return nullptr;

        auto conn = std::move(connections_.front());
        connections_.pop();

        if (!conn->is_open()) {
            Logger::get()->warn("[Database] Connection was closed. Reconnecting.");
            conn = reconnect_connection();
        }

        return conn;
    }

    void release_connection(std::unique_ptr<pqxx::connection> conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.push(std::move(conn));
    }

    std::unique_ptr<pqxx::connection> reconnect_connection() {
        auto conn = std::make_unique<pqxx::connection>(conninfo_);
        prepare_all(*conn);
        Logger::get()->info("[Database] Connection re-established.");
        return conn;
    }

    void prepare_all(pqxx::connection& conn) {
        pqxx::work txn(conn);
        conn.prepare("SELECT_ACCOUNT_BY_USERNAME",
                     "SELECT id, username, salt, verifier, email, created_at FROM accounts WHERE username = $1");
        conn.prepare("INSERT_ACCOUNT_BY_USERNAME",
                     "INSERT INTO accounts (username, salt, verifier) VALUES ($1, $2, $3) RETURNING id");
        txn.commit();
    }

    boost::asio::thread_pool& pool_;
    std::string conninfo_;
    std::queue<std::unique_ptr<pqxx::connection>> connections_;
    std::mutex mutex_;
};

#pragma GCC diagnostic pop