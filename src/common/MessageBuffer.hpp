#pragma once

#include <vector>
#include <cstring>
#include <cstdint>
#include <stdexcept>

class MessageBuffer {
public:
    using size_type = std::vector<uint8_t>::size_type;

    explicit MessageBuffer(std::size_t initialSize = 4096)
            : _wpos(0), _rpos(0), _storage(initialSize) {}

    void reset() {
        _wpos = 0;
        _rpos = 0;
    }

    void clear() {
        reset();
        _storage.clear();
        // Можно зарезервировать обратно начальный размер:
        _storage.shrink_to_fit();
    }

    uint8_t* base() { return _storage.data(); }
    uint8_t* read_ptr() { return base() + _rpos; }
    uint8_t* write_ptr() { return base() + _wpos; }

    void write_completed(size_type bytes) {
        if (bytes > get_remaining_space())
            throw std::runtime_error("MessageBuffer: write overflow");
        _wpos += bytes;
    }

    void read_completed(size_type bytes) {
        if (bytes > get_active_size())
            throw std::runtime_error("MessageBuffer: read overflow");
        _rpos += bytes;
    }

    size_type get_active_size() const { return _wpos - _rpos; }
    size_type get_remaining_space() const { return _storage.size() - _wpos; }

    void normalize() {
        if (_rpos) {
            if (_rpos != _wpos)
                memmove(base(), read_ptr(), get_active_size());
            _wpos -= _rpos;
            _rpos = 0;
        }
    }

    void ensure_free_space(size_type min_free = 512) {
        if (get_remaining_space() < min_free) {
            normalize();
            if (get_remaining_space() < min_free) {
                _storage.resize(_storage.size() + min_free);
            }
        }
    }

private:
    size_type _wpos;
    size_type _rpos;
    std::vector<uint8_t> _storage;
};
