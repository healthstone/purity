#pragma once

#include <pqxx/pqxx>
#include <string>
#include <chrono>
#include <optional>

#include "TimeUtils.hpp"

// === Структуры ===

struct AccountsRow {
    int64_t id;
    std::optional<std::string> name;
    std::optional<std::string> salt;
    std::optional<std::string> verifier;
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
        row.id = r["id"].as<int64_t>();

        if (!r["username"].is_null())
            row.name = r["username"].as<std::string>();

        if (!r["salt"].is_null())
            row.salt = r["salt"].as<std::string>();

        if (!r["verifier"].is_null())
            row.verifier = r["verifier"].as<std::string>();

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
struct PgRowMapper<int64_t> {
    static int64_t map(const pqxx::row &r) {
        return r[0].as<int64_t>();
    }
};
