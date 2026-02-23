#pragma once

namespace Kyber 
{
    struct SocketSpawnInfo
    {
        SocketSpawnInfo(bool isProxied, const char* proxyAddress, const char* serverName)
            : isProxied(isProxied)
            , proxyAddress(proxyAddress)
            , serverName(serverName)
        {}

        bool isProxied;
        const char* proxyAddress;
        const char* serverName;
        const char* serverMode;
        const char* serverLevel;
    };
}