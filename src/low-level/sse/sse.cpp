#include "sse.hpp"
#include "../registers/Registers.hpp"

bool sseEnabled = false;

extern "C" bool SSEEnabled()
{
    return sseEnabled;
}

extern "C" void EnableSSE()
{
    Registers::WriteCR0((Registers::ReadCR0() & ~(1 << 2)) | (1 << 1));
    Registers::WriteCR4(Registers::ReadCR4() | (1 << 9) | (1 << 10));

    sseEnabled = true;
}