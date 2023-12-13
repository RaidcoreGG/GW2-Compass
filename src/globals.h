#ifndef GLOBALS_H
#define GLOBALS_H

#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

extern Mumble::Data* MumbleLink;
extern NexusLinkData* NexusLink;
extern Mumble::Identity* identity;

extern IDXGISwapChain* swapChain;
extern ID3D11Device* device;
extern ID3D11DeviceContext* deviceContext;
extern DXGI_SWAP_CHAIN_DESC swapChainDesc;

#endif // GLOBALS_H