#ifndef ZCOIN_BIP47SECRETPOINT_H
#define ZCOIN_BIP47SECRETPOINT_H
#include "key.h"
#include "pubkey.h"
#include "../secp256k1/include/Scalar.h"

namespace bip47 {

class CSecretPoint {
public:
    CSecretPoint() = delete;
    CSecretPoint(CKey const & privkey, CPubKey const & pubkey);

    std::vector<unsigned char> getEcdhSecret() const;

    bool isShared(CSecretPoint const & other) const;
private:
    secp_primitives::Scalar a;
    CPubKey pubkey;
    mutable std::vector<unsigned char> ecdhSecret;
};

}

#endif // ZCOIN_BIP47SECRETPOINT_H
