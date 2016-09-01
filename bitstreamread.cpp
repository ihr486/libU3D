#include <stdint.h>

namespace U3D
{
    class BitStreamRead
    {
        uint32_t m_high, m_low;
        unsigned int m_underflow;
    public:
        BitStreamRead() : m_high(0x0000FFFF), m_low(0), m_underflow(0) {
        }
        ~BitStreamRead() {
        }
        uint32_t read_symbol(uint32_t context) {
            uint32_t position = GetBitCount();
            uint32_t code = ReadBit();
            SkipBits(m_underflow);
            code = (code << 15) | Read15Bits();
            SeekToBit(position);

            uint32_t totalCumFreq = GetTotalSymbolFrequency(context);
            uint32_t range = m_high + 1 - m_low;

            uint32_t codeCumFreq = (totalCumFreq * (1 + code - m_low) - 1) / range;

            uint32_t symbol = GetSymbolFromFrequency(context, codeCumFreq);

            uint32_t valueCumFreq = GetCumulativeSymbolFrequency(context, symbol);
            uint32_t valueFreq = GetSymbolFrequency(context, symbol);

            m_high = m_low - 1 + range * (valueCumFreq + valueFreq) / totalCumFreq;
            m_low = m_low + range * valueCumFreq / totalCumFreq;
            AddSymbol(context, symbol);

            unsigned int bitCount = 0;

            while(!((m_low & 0x8000) ^ (m_high & 0x8000))) {
                m_low = (0x7FFF & m_low) << 1;
                m_high = ((0x7FFF & m_high) << 1) | 1;
                bitCount++;
            }

            if(bitCount > 0) {
                bitCount += m_underflow;
                m_underflow = 0;
            }

            while((m_low & 0x4000) && !(m_high & 0x4000)) {
                m_low = (m_low & 0x3FFF) << 1 | (m_low & 0x8000);
                m_high = ((m_high & 0x3FFF) << 1) | 1 | (m_high & 0x8000);
                m_underflow++;
            }

            SkipBits(bitCount);
            return symbol;
        }
    };
}
