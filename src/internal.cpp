#include <iostream>

#include "psutils.hpp"
#include "signature.hpp"
#include "internal.hpp"

bool GameInterface::HasMarket(int playerId) {
    return *reinterpret_cast<int*>(marketIdAddress + 0x39f4 * playerId) != 0;
}

// In a single player game, the player ID is always 1, but in a multiplayer game, everyone is assigned a different id.
// Currently, it has not been possible to find a signature that will help calculate the player's id.
int GameInterface::GetPlayerId() {
    return 1; 
}

void GameInterface::initializeAddresses() {
    const std::string tradeFuncAddressSignature = "8B ?? ?? ?? 85 ?? ?? ?? ?? 75 ?? 8B ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ??";
    const std::string marketIdAddressPointerSignature = "?? ?? ?? ?? E9 ?? ?? ?? ?? 0F BF ?? ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? F7 ?? B9 ?? ?? ?? ?? 89 ?? ??";
    const std::string productBaseAddressPointerSignature = "?? ?? ?? ?? 83 ?? ?? 83 ?? ?? 7C ?? B8 ?? ?? ?? ?? 39 ?? ?? 89 ?? ?? ?? 0F 8E ?? ?? ?? ?? 8D ??";

    const std::string processName = psutils::getProcessName();
    const uintptr_t baseAddress = psutils::getBaseAddress();

    const uintptr_t tradeFuncAddress = baseAddress + signature::getAddressBySignatureInFile(processName, tradeFuncAddressSignature);
    const uintptr_t marketIdAddressPointer = baseAddress + signature::getAddressBySignatureInFile(processName, marketIdAddressPointerSignature);
    const uintptr_t productBaseAddressPointer = baseAddress + signature::getAddressBySignatureInFile(processName, productBaseAddressPointerSignature);

    if (!psutils::isValidAddress(tradeFuncAddress) ||
        !psutils::isValidAddress(marketIdAddressPointer) ||
        !psutils::isValidAddress(productBaseAddressPointer)) {
        MessageBoxW(NULL, L"Error: One or more signatures not found or invalid addresses", L"Error", MB_OK | MB_ICONERROR);
        return;
    }   

    tradeFunction = reinterpret_cast<void (*)(int, bool, int)>(tradeFuncAddress);
    marketIdAddress = *reinterpret_cast<int*>(marketIdAddressPointer);
    productBaseAddress = *reinterpret_cast<int*>(productBaseAddressPointer);
}

void GameInterface::sellProduct(const std::string& productName) {
    int playerId = GetPlayerId();

    if (HasMarket(playerId)) {
        tradeFunction(playerId, true, productIds.at(productName));
    }
}

void GameInterface::buyProduct(const std::string& productName) {
    int playerId = GetPlayerId();

    if (HasMarket(playerId)) {
        tradeFunction(playerId, false, productIds.at(productName));
    }
}

int GameInterface::getNumberProducts(const std::string& productName) {
    return *reinterpret_cast<int*>(productBaseAddress + (0xe7d + productIds.at(productName)) * 0x4);
}

std::vector<std::string> GameInterface::getAvailableProducts() const {
    std::vector<std::string> availableProducts;
    for (const auto& pair : productIds) {
        availableProducts.push_back(pair.first);
    }
    return availableProducts;
}

GameInterface::GameInterface() {
    initializeAddresses();
}
