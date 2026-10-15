#pragma once
#include <Arduino.h>

namespace TextUtils {
    inline void utf8ToCleanAscii(const char* in, char* out, size_t max_out) {
        size_t i = 0, j = 0;
        while (in[i] != '\0' && j < max_out - 3) {
            uint8_t c = (uint8_t)in[i];
            if (c < 0x80) {
                out[j++] = (char)c;
                i++;
            } else if ((c == 0xD0 || c == 0xD1) && in[i + 1] != '\0') {
                uint8_t c2 = (uint8_t)in[i + 1];
                uint16_t code = (c << 8) | c2;
                i += 2;
                const char* rep = "";
                switch (code) {
                    case 0xD090: rep="A"; break;  case 0xD0B0: rep="a"; break;
                    case 0xD091: rep="B"; break;  case 0xD0B1: rep="b"; break;
                    case 0xD092: rep="V"; break;  case 0xD0B2: rep="v"; break;
                    case 0xD093: rep="H"; break;  case 0xD0B3: rep="h"; break;
                    case 0xD290: rep="G"; break;  case 0xD291: rep="g"; break;
                    case 0xD094: rep="D"; break;  case 0xD0B4: rep="d"; break;
                    case 0xD095: rep="E"; break;  case 0xD0B5: rep="e"; break;
                    case 0xD084: rep="Ye"; break; case 0xD194: rep="ie"; break;
                    case 0xD096: rep="Zh"; break; case 0xD0B6: rep="zh"; break;
                    case 0xD097: rep="Z"; break;  case 0xD0B7: rep="z"; break;
                    case 0xD098: rep="Y"; break;  case 0xD0B8: rep="y"; break;
                    case 0xD086: rep="I"; break;  case 0xD196: rep="i"; break;
                    case 0xD087: rep="Yi"; break; case 0xD197: rep="i"; break;
                    case 0xD099: rep="Y"; break;  case 0xD0B9: rep="i"; break;
                    case 0xD09A: rep="K"; break;  case 0xD0BA: rep="k"; break;
                    case 0xD09B: rep="L"; break;  case 0xD0BB: rep="l"; break;
                    case 0xD09C: rep="M"; break;  case 0xD0BC: rep="m"; break;
                    case 0xD09D: rep="N"; break;  case 0xD0BD: rep="n"; break;
                    case 0xD09E: rep="O"; break;  case 0xD0BE: rep="o"; break;
                    case 0xD09F: rep="P"; break;  case 0xD0BF: rep="p"; break;
                    case 0xD0A0: rep="R"; break;  case 0xD180: rep="r"; break;
                    case 0xD0A1: rep="S"; break;  case 0xD181: rep="s"; break;
                    case 0xD0A2: rep="T"; break;  case 0xD182: rep="t"; break;
                    case 0xD0A3: rep="U"; break;  case 0xD183: rep="u"; break;
                    case 0xD0A4: rep="F"; break;  case 0xD184: rep="f"; break;
                    case 0xD0A5: rep="Kh"; break; case 0xD185: rep="kh"; break;
                    case 0xD0A6: rep="Ts"; break; case 0xD186: rep="ts"; break;
                    case 0xD0A7: rep="Ch"; break; case 0xD187: rep="ch"; break;
                    case 0xD0A8: rep="Sh"; break; case 0xD188: rep="sh"; break;
                    case 0xD0A9: rep="Shch";break;case 0xD189: rep="shch";break;
                    case 0xD0AE: rep="Yu"; break; case 0xD18E: rep="iu"; break;
                    case 0xD0AF: rep="Ya"; break; case 0xD18F: rep="ia"; break;
                    default: rep=""; break;
                }
                while (*rep && j < max_out - 1) out[j++] = *rep++;
            } else {
                i++;
            }
        }
        out[j] = '\0';
    }
}