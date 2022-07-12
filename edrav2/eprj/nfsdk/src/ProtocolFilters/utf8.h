#ifndef _UTF_8
#define _UTF_8

inline void encodeUTF8Char(std::string& str, wchar_t character)
{
    unsigned int c = (unsigned int) character;

    // check for invalid chars
    if ((c >= 0xD800 && c <= 0xDFFF) || c == 0xFFFF || c == 0xFFFE || c >= 0x80000000)
    {
        // check for invalid sequence
        // NOTE 3 - Values of x in the range 0000 D800 .. 0000 DFFF
        // are reserved for the UTF-16 form and do not occur in UCS-4.
        // The values 0000 FFFE and 0000 FFFF also do not occur (see clause 8).
        // The mappings of these code positions in UTF-8 are undefined.
        c = 0xFFFD;
    }

    if (c < 0x80)
    {
        // x = 0000 0000 .. 0000 007F;	x;
        str += (char) c;
    }
    else if (c < 0x800)
    {
        // x = 0000 0080 .. 0000 07FF;	C0 + x/2**6;
        //                              80 + x%2**6;
        str += (char) (0xC0 + (c >> 6));
        str += (char) (0x80 + (c & 0x3F));
    }
    // catch 16bit wchar_t here
    // anything after this is 32bit wchar_t
    else if (sizeof(wchar_t) == 2 || c < 0x10000)
    {
        // x = 0000 0800 .. 0000 FFFF;	E0 + x/2**12;
        //                              80 + x/2**6%2**6;
        //                              80 + x%2**6;
        str += (char) (0xE0 + (c >> 12));
        str += (char) (0x80 + ((c >> 6) & 0x3F));
        str += (char) (0x80 + (c & 0x3F));
    }
    else if (c < 0x200000)
    {
        // x = 0001 0000 .. 001F FFFF;	F0 + x/2**18;
        //                              80 + x/2**12%2**6;
        //                              80 + x/2**6%2**6;
        //                              80 + x%2**6;
        str += (char) (0xF0 + (c >> 18));
        str += (char) (0x80 + ((c >> 12) & 0x3F));
        str += (char) (0x80 + ((c >> 6) & 0x3F));
        str += (char) (0x80 + (c & 0x3F));
    }
    else if (c < 0x4000000)
    {
        // x = 0020 0000 .. 03FF FFFF;	F8 + x/2**24;
        //                              80 + x/2**18%2**6;
        //                              80 + x/2**12%2**6;
        //                              80 + x/2**6%2**6;
        //                              80 + x%2**6;
        str += (char) (0xF8 + (c >> 24));
        str += (char) (0x80 + ((c >> 18) & 0x3F));
        str += (char) (0x80 + ((c >> 12) & 0x3F));
        str += (char) (0x80 + ((c >> 6) & 0x3F));
        str += (char) (0x80 + (c & 0x3F));
    }
    else // if (c<0x80000000)
    {
        // x = 0400 0000 .. 7FFF FFFF;	FC + x/2**30;
        //                              80 + x/2**24%2**6;
        //                              80 + x/2**18%2**6;
        //                              80 + x/2**12%2**6;
        //                              80 + x/2**6%2**6;
        //                              80 + x%2**6;
        str += (char) (0xFC + (c >> 30));
        str += (char) (0x80 + ((c >> 24) & 0x3F));
        str += (char) (0x80 + ((c >> 18) & 0x3F));
        str += (char) (0x80 + ((c >> 12) & 0x3F));
        str += (char) (0x80 + ((c >> 6) & 0x3F));
        str += (char) (0x80 + (c & 0x3F));
    }
}

inline std::string encodeUTF8(const std::wstring& source)
{
    std::string result;
    result.reserve(source.size() * 4 / 3); // optimization

    for (std::wstring::const_iterator it = source.begin(); it != source.end(); ++it)
    {
        encodeUTF8Char(result, (unsigned int) *it);
    }

    return result;
}

#define replacementChar 0xFFFD

inline std::wstring decodeUTF8(const std::string& source)
{
    unsigned int start = 0;

    std::wstring result;
    result.reserve(source.size());

    for (unsigned int i = start; i < source.length(); i++)
    {
        unsigned char c = source[i];

        wchar_t n;

        if (c != 0xFF && c != 0xFE)
        {
            int remainingBytes = (int)source.length() - i;
            int extraBytesForChar = -1;

            // check that character isn't a continuation byte
            if ((c >> 6) != 2)
            {
                // work out how many bytes the sequence is.
                // Bytes in sequence is equal to the number
                // 1 bits before the first 0.
                // e.g. 11100101 -> [1110] -> 3
                //      11110101 -> [11110] -> 4
                unsigned char d = c;

                // LHS contains sequence length.
                // RHS contains data.
                // dataMask is used to extract data.
                int dataMask = 0x7F;
                    
                // Find sequence length
                while ((d & 0x80) != 0)
                {
                    dataMask >>= 1;
                    extraBytesForChar++;
                    d <<= 1;
                }

                n = c & dataMask;

                // if there aren't enough bytes in the encoded sequence.
                if (extraBytesForChar > remainingBytes) 
                {
                    n = replacementChar;
                }
                else
                // decode sequence.
                for (int j = 0; j < extraBytesForChar; j++)
                {
                    // update index & get continuation byte.
                    c = source[++i];
                        
                    if (c == 0xFF || c == 0xFE)
                    {
                        n = replacementChar;
                        break;
                    }

                    // Make sure the continuation byte matchs 10xxxxxx.
                    if ((c >> 6) == 2)
                    {
                        n = (n << 6) + (c&0x3F);
                    }
                    else
                    {
                        // revert index as this bytes wasn't a continuation,
                        // so it might have been a start character.
                        i--;
                        n = replacementChar;
                        break;
                    }
                }
            }
            else
            {
                n = replacementChar;
            }

            // If the number of extra bytes in sequence is > 2 then it is a utf32 char.
            if (sizeof(wchar_t) == 2 && extraBytesForChar>2)
            {
                n = replacementChar;
            }
            else if (sizeof(wchar_t) == 4 && extraBytesForChar>5)
            {
                n = replacementChar;
            }
        }
        else
        {
            n = replacementChar;
        }

        // append the decoded character.
        result.append (1, n) ;
    }

    return result;
}

#endif