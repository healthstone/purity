#pragma once

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <chrono>
#include <optional>

#include "utils/TimeUtils.hpp"

// === Структуры ===

struct AccountsRow {
    uint64_t id;
    std::optional<std::string> name;
    std::optional<std::vector<uint8_t>> salt;
    std::optional<std::vector<uint8_t>> verifier;
    std::optional<std::string> email;
    std::optional<std::chrono::system_clock::time_point> created_at;
};

struct NothingRow {};

// === Шаблон PgRowMapper ===

template<typename T>
struct PgRowMapper;

template<>
struct PgRowMapper<AccountsRow> {
    static AccountsRow map(const pqxx::row &r) {
        AccountsRow row;

        // PostgreSQL BIGINT -> int64_t -> uint64_t ( 0 … 9223372036854775807 )
        row.id = static_cast<uint64_t>(r["id"].as<int64_t>());

        if (!r["username"].is_null())
            row.name = r["username"].as<std::string>();

        if (!r["salt"].is_null()) {
            pqxx::binarystring salt_bin(r["salt"]);
            row.salt = std::vector<uint8_t>(salt_bin.begin(), salt_bin.end());
        }

        if (!r["verifier"].is_null()) {
            pqxx::binarystring verifier_bin(r["verifier"]);
            row.verifier = std::vector<uint8_t>(verifier_bin.begin(), verifier_bin.end());
        }

        if (!r["email"].is_null())
            row.email = r["email"].as<std::string>();

        if (!r["created_at"].is_null())
            row.created_at = TimeUtils::parse_pg_timestamp_optional(r["created_at"].as<std::string>());

        return row;
    }
};

template<>
struct PgRowMapper<NothingRow> {
    static NothingRow map(const pqxx::row &) {
        return NothingRow{};
    }
};

template<>
struct PgRowMapper<uint64_t> {
    static uint64_t map(const pqxx::row &r) {
        return static_cast<uint64_t>(r[0].as<int64_t>());
    }
};
