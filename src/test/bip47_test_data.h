#ifndef BIP47_TEST_DATA_H
#define BIP47_TEST_DATA_H

#include "bip47/bip47utils.h"

namespace {
namespace alice {
bip47::Bytes const bip32seed = ParseHex("64dca76abc9c6f0cf3d212d248c380c4622c8f93b2c425ec6a5567fd5db57e10d3e6f94a2f6af4ac2edb8998072aad92098db73558c323777abf5bd1082d970a");
std::string const paymentcode = "PM8TJTLJbPRGxSbc8EJi42Wrr6QbNSaSSVJ5Y3E4pbCYiTHUskHg13935Ubb7q8tx9GVbh2UuRnBc3WSyJHhUrw8KhprKnn9eDznYGieTzFcwQRya4GA";
}
namespace bob {
bip47::Bytes const bip32seed = ParseHex("87eaaac5a539ab028df44d9110defbef3797ddb805ca309f61a69ff96dbaa7ab5b24038cf029edec5235d933110f0aea8aeecf939ed14fc20730bba71e4b1110");
std::string const paymentcode = "PM8TJS2JxQ5ztXUpBBRnpTbcUXbUHy2T1abfrb3KkAAtMEGNbey4oumH7Hc578WgQJhPjBxteQ5GHHToTYHE3A1w6p7tU6KSoFmWBVbFGjKPisZDbP97";
}


namespace alice {
std::vector<bip47::Bytes> const ecdhparams = {
    ParseHex("8d6a8ecd8ee5e0042ad0cb56e3a971c760b5145c3917a8e7beaf0ed92d7a520c"),
    ParseHex("0353883a146a23f988e0f381a9507cbdb3e3130cd81b3ce26daf2af088724ce683")
    };
}
namespace bob {
std::vector<bip47::Bytes> const ecdhparams = {
    ParseHex("04448fd1be0c9c13a5ca0b530e464b619dc091b299b98c5cab9978b32b4a1b8b"),
    ParseHex("024ce8e3b04ea205ff49f529950616c3db615b1e37753858cc60c1ce64d17e2ad8"),
    ParseHex("6bfa917e4c44349bfdf46346d389bf73a18cec6bc544ce9f337e14721f06107b"),
    ParseHex("03e092e58581cf950ff9c8fc64395471733e13f97dedac0044ebd7d60ccc1eea4d"),
    ParseHex("46d32fbee043d8ee176fe85a18da92557ee00b189b533fce2340e4745c4b7b8c"),
    ParseHex("029b5f290ef2f98a0462ec691f5cc3ae939325f7577fcaf06cfc3b8fc249402156"),
    ParseHex("4d3037cfd9479a082d3d56605c71cbf8f38dc088ba9f7a353951317c35e6c343"),
    ParseHex("02094be7e0eef614056dd7c8958ffa7c6628c1dab6706f2f9f45b5cbd14811de44"),
    ParseHex("97b94a9d173044b23b32f5ab64d905264622ecd3eafbe74ef986b45ff273bbba"),
    ParseHex("031054b95b9bc5d2a62a79a58ecfe3af000595963ddc419c26dab75ee62e613842"),
    ParseHex("ce67e97abf4772d88385e66d9bf530ee66e07172d40219c62ee721ff1a0dca01"),
    ParseHex("03dac6d8f74cacc7630106a1cfd68026c095d3d572f3ea088d9a078958f8593572"),
    ParseHex("ef049794ed2eef833d5466b3be6fe7676512aa302afcde0f88d6fcfe8c32cc09"),
    ParseHex("02396351f38e5e46d9a270ad8ee221f250eb35a575e98805e94d11f45d763c4651"),
    ParseHex("d3ea8f780bed7ef2cd0e38c5d943639663236247c0a77c2c16d374e5a202455b"),
    ParseHex("039d46e873827767565141574aecde8fb3b0b4250db9668c73ac742f8b72bca0d0"),
    ParseHex("efb86ca2a3bad69558c2f7c2a1e2d7008bf7511acad5c2cbf909b851eb77e8f3"),
    ParseHex("038921acc0665fd4717eb87f81404b96f8cba66761c847ebea086703a6ae7b05bd"),
    ParseHex("18bcf19b0b4148e59e2bba63414d7a8ead135a7c2f500ae7811125fb6f7ce941"),
    ParseHex("03d51a06c6b48f067ff144d5acdfbe046efa2e83515012cf4990a89341c1440289")
    };
}

std::vector<bip47::Bytes> const sharedsecrets = {
    ParseHex("f5bb84706ee366052471e6139e6a9a969d586e5fe6471a9b96c3d8caefe86fef"),
    ParseHex("adfb9b18ee1c4460852806a8780802096d67a8c1766222598dc801076beb0b4d"),
    ParseHex("79e860c3eb885723bb5a1d54e5cecb7df5dc33b1d56802906762622fa3c18ee5"),
    ParseHex("d8339a01189872988ed4bd5954518485edebf52762bf698b75800ac38e32816d"),
    ParseHex("14c687bc1a01eb31e867e529fee73dd7540c51b9ff98f763adf1fc2f43f98e83"),
    ParseHex("725a8e3e4f74a50ee901af6444fb035cb8841e0f022da2201b65bc138c6066a2"),
    ParseHex("521bf140ed6fb5f1493a5164aafbd36d8a9e67696e7feb306611634f53aa9d1f"),
    ParseHex("5f5ecc738095a6fb1ea47acda4996f1206d3b30448f233ef6ed27baf77e81e46"),
    ParseHex("1e794128ac4c9837d7c3696bbc169a8ace40567dc262974206fcf581d56defb4"),
    ParseHex("fe36c27c62c99605d6cd7b63bf8d9fe85d753592b14744efca8be20a4d767c37")
    };

namespace alice {
std::string const notificationaddress = "1JDdmqFLhpzcUwPeinhJbUPw4Co3aWLyzW";
}
namespace bob {
std::string const notificationaddress = "1ChvUUvht2hUQufHBXF8NgLhW8SwE2ecGV";
}

namespace alice {
std::vector<std::string> sendingaddresses = {
    "141fi7TY3h936vRUKh1qfUZr8rSBuYbVBK",
    "12u3Uued2fuko2nY4SoSFGCoGLCBUGPkk6",
    "1FsBVhT5dQutGwaPePTYMe5qvYqqjxyftc",
    "1CZAmrbKL6fJ7wUxb99aETwXhcGeG3CpeA",
    "1KQvRShk6NqPfpr4Ehd53XUhpemBXtJPTL",
    "1KsLV2F47JAe6f8RtwzfqhjVa8mZEnTM7t",
    "1DdK9TknVwvBrJe7urqFmaxEtGF2TMWxzD",
    "16DpovNuhQJH7JUSZQFLBQgQYS4QB9Wy8e",
    "17qK2RPGZMDcci2BLQ6Ry2PDGJErrNojT5",
    "1GxfdfP286uE24qLZ9YRP3EWk2urqXgC4s"
    };
}

namespace bob {
std::vector<std::string> sendingaddresses = {
    "17SSoP6pwU1yq6fTATEQ7gLMDWiycm68VT",
    "1KNFAqYPoiy29rTQF44YT3v9tvRJYi15Xf",
    "1HQkbVeZoLoDpkZi1MB6AgaCs5ZbxTBdZA",
    "14GfiZb1avg3HSiacMLaoG5xdfPjc1Unvm",
    "15yHVDiYJn146EKHuJiN79L9S2EZAjGVaK"
    };
}

namespace alice {
std::string maskedpayload = "010002063e4eb95e62791b06c50e1a3a942e1ecaaa9afbbeb324d16ae6821e091611fa96c0cf048f607fe51a0327f5e2528979311c78cb2de0d682c61e1180fc3d543b00000000000000000000000000";
}

}

#endif /* BIP47_TEST_DATA_H */

