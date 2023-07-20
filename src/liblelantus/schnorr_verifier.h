#ifndef KIIRO_LIBLELANTUS_SCHNORR_VERIFIER_H
#define KIIRO_LIBLELANTUS_SCHNORR_VERIFIER_H

#include "lelantus_primitives.h"

namespace lelantus {
    
class SchnorrVerifier {
public:
    //g and h are being kept by reference, be sure it will not be modified from outside
    SchnorrVerifier(const GroupElement& g, const GroupElement& h, bool withFixes_);

    // values a, b and y are included into transcript if(withFixes_), also better to use CHash256 in that case
    bool verify(const GroupElement& y, const GroupElement& a, const GroupElement& b,const SchnorrProof& proof, std::unique_ptr<ChallengeGenerator>& challengeGenerator);
    bool verify(const GroupElement& y, const std::vector<GroupElement>& groupElements,const SchnorrProof& proof);

private:
    const GroupElement& g_;
    const GroupElement& h_;
    bool withFixes;
};

}//namespace lelantus

#endif //KIIRO_LIBLELANTUS_SCHNORR_VERIFIER_H
