#pragma once

#include <cstdint>

enum AuthResult : uint8_t
{
    AUTH_OK                   = 0x00, // Успешно
    AUTH_FAILED               = 0x04, // Общая ошибка
    AUTH_REJECT               = 0x05, // Отклонено
    AUTH_BAD_SERVER_PROOF     = 0x06, // Неверный SRP proof
    AUTH_UNAVAILABLE          = 0x08, // Сервер недоступен
    AUTH_SYSTEM_ERROR         = 0x09, // Системная ошибка
    AUTH_BILLING_ERROR        = 0x0A, // Проблема с подпиской
    AUTH_BILLING_EXPIRED      = 0x0B, // Подписка истекла
    AUTH_VERSION_MISMATCH     = 0x0C, // Несовпадение версии клиента
    AUTH_UNKNOWN_ACCOUNT      = 0x0D, // Аккаунт не найден
    AUTH_INCORRECT_PASSWORD   = 0x0E, // Неверный пароль
    AUTH_SESSION_EXPIRED      = 0x0F, // Сессия устарела
    AUTH_SERVER_SHUTTING_DOWN = 0x10, // Сервер выключается
    AUTH_ALREADY_LOGGING_IN   = 0x11, // Уже логинится
    AUTH_LOGIN_SERVER_NOT_FOUND = 0x12, // Логин-сервер не найден
    AUTH_WAIT_QUEUE           = 0x1F, // В очереди
    AUTH_BANNED               = 0x03, // Аккаунт забанен
    AUTH_UNKNOWN_ERROR        = 0xFF, // Неизвестная ошибка
};