namespace U3D
{
    public class BitStreamRead : IBitStreamRead
    {
        public BitStreamRead()
        {
            this.contextManager = new ContextManager();
            this.high = 0x0000FFFF;
        }

        private void ReadSymbol(Uint32 context, out UInt32 rSymbol)
        {
            UInt32 uValue = 0;

            UInt32 position = 0;
            GetBitCount(out position);
            ReadBit(out this.code);
            this.dataBitOffset += (Int32)this.underflow;

            while(this.dataBitOffset >= 32)
            {
                this.dataBitOffset -= 32;
                IncrementPosition();
            }

            UInt32 temp = 0;
            Read15Bits(out temp);
            this.code = (this.code << 15) | temp;
            SeekToBit(position);

            UInt32 totalCumFreq = this.contextManager.GetTotalSymbolFrequency(context);

            UInt32 range = this.high + 1 - this.low;

            UInt32 codeCumFreq = (totalCumFreq * (1 + this.code - this.low) - 1) / range;

            uValue = this.contextManager.GetSymbolFromFrequency(context, codeCumFreq);

            UInt32 valueCumFreq = this.contextManager.GetCumulativeSymbolFrequency(context, uValue);
            UInt32 valueFreq = this.contextManager.GetSymbolFrequency(context, uValue);

            UInt32 low = this.low;
            UInt32 high = this.high;

            high = low - 1 + range * (valueCumFreq + valueFreq) / totalCumFreq;
            low = low + range * valueCumFreq / totalCumFreq;
            this.contextManager.AddSymbol(context, uValue);

            Int32 bitCount = (Int32)ReadCount[((low >> 12) ^ (high >> 12)) & 0xF];

            low = (low & FastNotMask[bitCount]) << bitCount;
            high = ((high & FastNotMask[bitCount]) << bitCount) | (UInt32)((1 << bitCount) - 1);

            while((low & 0x8000) == (high & 0x8000))
            {
                low = (0x7FFF & low) << 1;
                high = ((0x7FFF & high) << 1) | 1;
                bitCount++;
            }

            UInt32 savedBitsLow = low & 0x8000;
            UInt32 savedBitsHigh = high & 0x8000;

            if(bitCount > 0)
            {
                bitCount += (Int32)this.underflow;
                this.underflow = 0;
            }

            while((low & 0x4000) && !(high & 0x4000))
            {
                low = (low & 0x3FFF) << 1;
                high = ((high & 0x3FFF) << 1) | 1;
                this.underflow++;
            }

            this.low = low | savedBitsLow;
            this.high = high | savedBitsHigh;
            this.dataBitOffset += bitCount;

            while(this.dataBitOffset >= 32)
            {
                this.dataBitOffset -= 32;
                IncrementPosition();
            }
            rSymbol = uValue;
        }
    }
}
