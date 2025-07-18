#pragma once

#include <cstdint>
#include "SRP.hpp"

class BNCSAccount{
public:
    BNCSAccount() {
        srp_ = std::make_unique<SRP>();
    }
    ~BNCSAccount() {
        archtag_ = 0;
        clienttag_ = 0;
        versionid_ = 0;
        servertoken_ = 0;
        clienttoken_ = 0;
    }

    // PvPGN handshake данные
    void setArchTag(const uint32_t &val) { archtag_ = val; }

    void setClientTag(const uint32_t &val) { clienttag_ = val; }

    void setVersionId(const uint32_t &val) { versionid_ = val; }

    void setServerToken(const uint32_t &val) { servertoken_ = val; }

    void setClientToken_(const uint32_t &val) { clienttoken_ = val; }

    SRP* srp() { return srp_.get(); }

private:
    uint32_t archtag_ = 0;
    uint32_t clienttag_ = 0;
    uint32_t versionid_ = 0;
    uint32_t servertoken_ = 0;
    uint32_t clienttoken_ = 0;

    std::unique_ptr<SRP> srp_;
};