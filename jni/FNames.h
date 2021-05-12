#ifndef FNAMES_H
#define FNAMES_H

using namespace std;

uint32 GNameLimit = 170000;

string getUEString(kaddr address) {
    unsigned int MAX_SIZE = 100;

    string uestring(ReadStr(address, MAX_SIZE));
    uestring.shrink_to_fit();

    return uestring;
}

string GetFNameFromID(uint32 index) {
    if (isUE423) {
        uint32 Block = index >> 16;
        uint16 Offset = index & 65535;

        kaddr FNamePool = getRealOffset(Offsets::GNames) + Offsets::GNamesToFNamePool;

        kaddr NamePoolChunk = getPtr(
                FNamePool + Offsets::FNamePoolToBlocks + (Block * Offsets::PointerSize));
        kaddr FNameEntry = NamePoolChunk + (Offsets::FNameStride * Offset);

        int16 FNameEntryHeader = Read<int16>(FNameEntry);
        int StrLength = FNameEntryHeader >> Offsets::FNameEntryToLenBit;

        ///Unicode Dumping Not Supported
        if (StrLength > 0 && StrLength < 250) {
            bool wide = FNameEntryHeader & 1;
            ///Wide Char Dumping Not Supported Yet
            if (!wide) {
                return ReadStr2(FNameEntry + Offsets::FNameEntryToString, StrLength);
            } else {
                return "None";
            }
        } else {
            return "None";
        }
    } else {
        if (deRefGNames) {
            kaddr TNameEntryArray = getPtr(getRealOffset(Offsets::GNames));

            kaddr FNameEntryArr = getPtr(
                    TNameEntryArray + ((index / 0x4000) * Offsets::PointerSize));
            kaddr FNameEntry = getPtr(FNameEntryArr + ((index % 0x4000) * Offsets::PointerSize));

            return getUEString(FNameEntry + Offsets::FNameEntryToNameString);
        } else {
            kaddr TNameEntryArray = getRealOffset(Offsets::GNames);

            kaddr FNameEntryArr = getPtr(
                    TNameEntryArray + ((index / 0x4000) * Offsets::PointerSize));
            kaddr FNameEntry = getPtr(FNameEntryArr + ((index % 0x4000) * Offsets::PointerSize));

            return getUEString(FNameEntry + Offsets::FNameEntryToNameString);
        }
    }
}

void
DumpBlocks423(ofstream &gname, uint32 &count, kaddr FNamePool, uint32 blockId, uint32 blockSize) {
    kaddr It = getPtr(FNamePool + Offsets::FNamePoolToBlocks + (blockId * Offsets::PointerSize));
    kaddr End = It + blockSize - Offsets::FNameEntryToString;
    uint32 Block = blockId;
    uint16 Offset = 0;
    while (It < End) {
        kaddr FNameEntry = It;
        int16 FNameEntryHeader = Read<int16>(FNameEntry);
        int StrLength = FNameEntryHeader >> Offsets::FNameEntryToLenBit;
        if (StrLength) {
            bool wide = FNameEntryHeader & 1;

            ///Unicode Dumping Not Supported
            if (StrLength > 0) {
                //String Length Limit
                if (StrLength < 250) {
                    ///Wide Char Dumping Not Supported Yet
                    if (!wide) {
                        //Dump
                        uint32 key = (Block << 16 | Offset);
                        string str = ReadStr2(FNameEntry + Offsets::FNameEntryToString, StrLength);
                        if (isVerbose) {
                            cout << setbase(10) << "{" << StrLength << "} [" << key << "]: " << str
                                 << endl;
                        }
                        gname << "[" << key << "]: " << str << endl;
                        count++;
                    }
                }
            } else {
                StrLength = -StrLength;
            }

            //Next
            Offset += StrLength / Offsets::FNameStride;
            uint16 bytes = Offsets::FNameEntryToString +
                           StrLength * (wide ? sizeof(wchar_t) : sizeof(char));
            It += (bytes + Offsets::FNameStride - 1u) & ~(Offsets::FNameStride - 1u);
        } else {// Null-terminator entry found
            break;
        }
    }
}

void DumpStrings(string out) {
    uint32 count = 0;
    ofstream gname(out + "/Strings.txt", ofstream::out);
    if (gname.is_open()) {
        cout << "Dumping Strings" << endl;
        clock_t begin = clock();
        if (isUE423) {
            //cout << "String Dump for UE4.23+ Not Supported Yet" << endl;//No Longer Needed
            kaddr FNamePool = getRealOffset(Offsets::GNames) + Offsets::GNamesToFNamePool;

            uint32 BlockSize = Offsets::FNameStride * 65536;
            uint32 CurrentBlock = Read<uint32>(FNamePool + Offsets::FNamePoolToCurrentBlock);
            uint32 CurrentByteCursor = Read<uint32>(
                    FNamePool + Offsets::FNamePoolToCurrentByteCursor);

            //All Blocks Except Current
            for (uint32 BlockIdx = 0; BlockIdx < CurrentBlock; ++BlockIdx) {
                DumpBlocks423(gname, count, FNamePool, BlockIdx, BlockSize);
            }

            //Last Block
            DumpBlocks423(gname, count, FNamePool, CurrentBlock, CurrentByteCursor);
        } else {
            for (uint32 i = 0; i < GNameLimit; i++) {
                string s = GetFNameFromID(i);
                if (!s.empty()) {
                    gname << "[" << i << "]: " << s << endl;
                    count++;
                }
            }
        }
        gname.close();
        clock_t end = clock();
        double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        cout << count << " Strings Dumped in " << elapsed_secs << "S" << endl;
    }
}

#endif