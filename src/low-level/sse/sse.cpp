#include "sse.hpp"
#include "../registers/Registers.hpp"

void EnableSSE()
{
    Registers::WriteCR0((Registers::ReadCR0() & ~(1 << 2)) | (1 << 1));
    Registers::WriteCR4(Registers::ReadCR4() | (1 << 9) | (1 << 10));
}