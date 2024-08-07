#ifndef INTERNAL_HPP
#define INTERNAL_HPP

#include <vector>
#include <string>
#include <unordered_map>

class GameInterface {
public:
    GameInterface();
    std::vector<std::string> getAvailableProducts() const;
    int getNumberProducts(const std::string& productName);
    void buyProduct(const std::string& productName);
    void sellProduct(const std::string& productName);

private:
    uintptr_t marketIdAddress = 0;
    uintptr_t productBaseAddress = 0;
    void (*tradeFunction)(int, bool, int) = nullptr;

    const std::unordered_map<std::string, int> productIds = {
        {"meat", 12}, {"cheese", 11}, {"fruit", 13}, {"bread", 10}, {"wheat", 9},
        {"flour", 16}, {"hops", 3}, {"ale", 14}, {"wood", 2}, {"stone", 4},
        {"iron", 6}, {"pitch", 7}, {"spears", 19}, {"bows", 17}, {"maces", 21},
        {"crossbows", 18}, {"pikes", 20}, {"swords", 22}, {"leather armor", 23}, {"metal armor", 24}
    };

    bool HasMarket(int playerId);
    int GetPlayerId();
    void initializeAddresses();
};

#endif // INTERNAL_HPP