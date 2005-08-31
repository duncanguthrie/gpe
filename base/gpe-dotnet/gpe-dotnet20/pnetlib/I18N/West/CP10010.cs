/*
 * CP10010.cs - Romania (Mac) code page.
 *
 * Copyright (c) 2002  Southern Storm Software, Pty Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Generated from "mac-10010.ucm".

namespace I18N.West
{

using System;
using I18N.Common;

public class CP10010 : ByteEncoding
{
	public CP10010()
		: base(10010, ToChars, "Romania (Mac)",
		       "windows-10010", "windows-10010", "windows-10010",
		       false, false, false, false, 1250)
	{}

	private static readonly char[] ToChars = {
		'\u0000', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005', 
		'\u0006', '\u0007', '\u0008', '\u0009', '\u000A', '\u000B', 
		'\u000C', '\u000D', '\u000E', '\u000F', '\u0010', '\u0011', 
		'\u0012', '\u0013', '\u0014', '\u0015', '\u0016', '\u0017', 
		'\u0018', '\u0019', '\u001A', '\u001B', '\u001C', '\u001D', 
		'\u001E', '\u001F', '\u0020', '\u0021', '\u0022', '\u0023', 
		'\u0024', '\u0025', '\u0026', '\u0027', '\u0028', '\u0029', 
		'\u002A', '\u002B', '\u002C', '\u002D', '\u002E', '\u002F', 
		'\u0030', '\u0031', '\u0032', '\u0033', '\u0034', '\u0035', 
		'\u0036', '\u0037', '\u0038', '\u0039', '\u003A', '\u003B', 
		'\u003C', '\u003D', '\u003E', '\u003F', '\u0040', '\u0041', 
		'\u0042', '\u0043', '\u0044', '\u0045', '\u0046', '\u0047', 
		'\u0048', '\u0049', '\u004A', '\u004B', '\u004C', '\u004D', 
		'\u004E', '\u004F', '\u0050', '\u0051', '\u0052', '\u0053', 
		'\u0054', '\u0055', '\u0056', '\u0057', '\u0058', '\u0059', 
		'\u005A', '\u005B', '\u005C', '\u005D', '\u005E', '\u005F', 
		'\u0060', '\u0061', '\u0062', '\u0063', '\u0064', '\u0065', 
		'\u0066', '\u0067', '\u0068', '\u0069', '\u006A', '\u006B', 
		'\u006C', '\u006D', '\u006E', '\u006F', '\u0070', '\u0071', 
		'\u0072', '\u0073', '\u0074', '\u0075', '\u0076', '\u0077', 
		'\u0078', '\u0079', '\u007A', '\u007B', '\u007C', '\u007D', 
		'\u007E', '\u007F', '\u00C4', '\u00C5', '\u00C7', '\u00C9', 
		'\u00D1', '\u00D6', '\u00DC', '\u00E1', '\u00E0', '\u00E2', 
		'\u00E4', '\u00E3', '\u00E5', '\u00E7', '\u00E9', '\u00E8', 
		'\u00EA', '\u00EB', '\u00ED', '\u00EC', '\u00EE', '\u00EF', 
		'\u00F1', '\u00F3', '\u00F2', '\u00F4', '\u00F6', '\u00F5', 
		'\u00FA', '\u00F9', '\u00FB', '\u00FC', '\u2020', '\u00B0', 
		'\u00A2', '\u00A3', '\u00A7', '\u2022', '\u00B6', '\u00DF', 
		'\u00AE', '\u00A9', '\u2122', '\u00B4', '\u00A8', '\u2260', 
		'\u0102', '\u0218', '\u221E', '\u00B1', '\u2264', '\u2265', 
		'\u00A5', '\u00B5', '\u2202', '\u2211', '\u220F', '\u03C0', 
		'\u222B', '\u00AA', '\u00BA', '\u03A9', '\u0103', '\u0219', 
		'\u00BF', '\u00A1', '\u00AC', '\u221A', '\u0192', '\u2248', 
		'\u2206', '\u00AB', '\u00BB', '\u2026', '\u00A0', '\u00C0', 
		'\u00C3', '\u00D5', '\u0152', '\u0153', '\u2013', '\u2014', 
		'\u201C', '\u201D', '\u2018', '\u2019', '\u00F7', '\u25CA', 
		'\u00FF', '\u0178', '\u2044', '\u20AC', '\u2039', '\u203A', 
		'\u021A', '\u021B', '\u2021', '\u00B7', '\u201A', '\u201E', 
		'\u2030', '\u00C2', '\u00CA', '\u00C1', '\u00CB', '\u00C8', 
		'\u00CD', '\u00CE', '\u00CF', '\u00CC', '\u00D3', '\u00D4', 
		'\uF8FF', '\u00D2', '\u00DA', '\u00DB', '\u00D9', '\u0131', 
		'\u02C6', '\u02DC', '\u00AF', '\u02D8', '\u02D9', '\u02DA', 
		'\u00B8', '\u02DD', '\u02DB', '\u02C7', 
	};

	protected override void ToBytes(char[] chars, int charIndex, int charCount,
	                                byte[] bytes, int byteIndex)
	{
		int ch;
		while(charCount > 0)
		{
			ch = (int)(chars[charIndex++]);
			if(ch >= 0) switch(ch)
			{
				case 0x0020:
				case 0x0021:
				case 0x0022:
				case 0x0023:
				case 0x0024:
				case 0x0025:
				case 0x0026:
				case 0x0027:
				case 0x0028:
				case 0x0029:
				case 0x002A:
				case 0x002B:
				case 0x002C:
				case 0x002D:
				case 0x002E:
				case 0x002F:
				case 0x0030:
				case 0x0031:
				case 0x0032:
				case 0x0033:
				case 0x0034:
				case 0x0035:
				case 0x0036:
				case 0x0037:
				case 0x0038:
				case 0x0039:
				case 0x003A:
				case 0x003B:
				case 0x003C:
				case 0x003D:
				case 0x003E:
				case 0x003F:
				case 0x0040:
				case 0x0041:
				case 0x0042:
				case 0x0043:
				case 0x0044:
				case 0x0045:
				case 0x0046:
				case 0x0047:
				case 0x0048:
				case 0x0049:
				case 0x004A:
				case 0x004B:
				case 0x004C:
				case 0x004D:
				case 0x004E:
				case 0x004F:
				case 0x0050:
				case 0x0051:
				case 0x0052:
				case 0x0053:
				case 0x0054:
				case 0x0055:
				case 0x0056:
				case 0x0057:
				case 0x0058:
				case 0x0059:
				case 0x005A:
				case 0x005B:
				case 0x005C:
				case 0x005D:
				case 0x005E:
				case 0x005F:
				case 0x0060:
				case 0x0061:
				case 0x0062:
				case 0x0063:
				case 0x0064:
				case 0x0065:
				case 0x0066:
				case 0x0067:
				case 0x0068:
				case 0x0069:
				case 0x006A:
				case 0x006B:
				case 0x006C:
				case 0x006D:
				case 0x006E:
				case 0x006F:
				case 0x0070:
				case 0x0071:
				case 0x0072:
				case 0x0073:
				case 0x0074:
				case 0x0075:
				case 0x0076:
				case 0x0077:
				case 0x0078:
				case 0x0079:
				case 0x007A:
				case 0x007B:
				case 0x007C:
				case 0x007D:
				case 0x007E:
				case 0x00A2:
				case 0x00A3:
				case 0x00A9:
				case 0x00B1:
				case 0x00B5:
					break;
				case 0x00A0: ch = 0xCA; break;
				case 0x00A1: ch = 0xC1; break;
				case 0x00A5: ch = 0xB4; break;
				case 0x00A7: ch = 0xA4; break;
				case 0x00A8: ch = 0xAC; break;
				case 0x00AA: ch = 0xBB; break;
				case 0x00AB: ch = 0xC7; break;
				case 0x00AC: ch = 0xC2; break;
				case 0x00AE: ch = 0xA8; break;
				case 0x00AF: ch = 0xF8; break;
				case 0x00B0: ch = 0xA1; break;
				case 0x00B4: ch = 0xAB; break;
				case 0x00B6: ch = 0xA6; break;
				case 0x00B7: ch = 0xE1; break;
				case 0x00B8: ch = 0xFC; break;
				case 0x00BA: ch = 0xBC; break;
				case 0x00BB: ch = 0xC8; break;
				case 0x00BF: ch = 0xC0; break;
				case 0x00C0: ch = 0xCB; break;
				case 0x00C1: ch = 0xE7; break;
				case 0x00C2: ch = 0xE5; break;
				case 0x00C3: ch = 0xCC; break;
				case 0x00C4: ch = 0x80; break;
				case 0x00C5: ch = 0x81; break;
				case 0x00C7: ch = 0x82; break;
				case 0x00C8: ch = 0xE9; break;
				case 0x00C9: ch = 0x83; break;
				case 0x00CA: ch = 0xE6; break;
				case 0x00CB: ch = 0xE8; break;
				case 0x00CC: ch = 0xED; break;
				case 0x00CD: ch = 0xEA; break;
				case 0x00CE: ch = 0xEB; break;
				case 0x00CF: ch = 0xEC; break;
				case 0x00D1: ch = 0x84; break;
				case 0x00D2: ch = 0xF1; break;
				case 0x00D3: ch = 0xEE; break;
				case 0x00D4: ch = 0xEF; break;
				case 0x00D5: ch = 0xCD; break;
				case 0x00D6: ch = 0x85; break;
				case 0x00D9: ch = 0xF4; break;
				case 0x00DA: ch = 0xF2; break;
				case 0x00DB: ch = 0xF3; break;
				case 0x00DC: ch = 0x86; break;
				case 0x00DF: ch = 0xA7; break;
				case 0x00E0: ch = 0x88; break;
				case 0x00E1: ch = 0x87; break;
				case 0x00E2: ch = 0x89; break;
				case 0x00E3: ch = 0x8B; break;
				case 0x00E4: ch = 0x8A; break;
				case 0x00E5: ch = 0x8C; break;
				case 0x00E7: ch = 0x8D; break;
				case 0x00E8: ch = 0x8F; break;
				case 0x00E9: ch = 0x8E; break;
				case 0x00EA: ch = 0x90; break;
				case 0x00EB: ch = 0x91; break;
				case 0x00EC: ch = 0x93; break;
				case 0x00ED: ch = 0x92; break;
				case 0x00EE: ch = 0x94; break;
				case 0x00EF: ch = 0x95; break;
				case 0x00F1: ch = 0x96; break;
				case 0x00F2: ch = 0x98; break;
				case 0x00F3: ch = 0x97; break;
				case 0x00F4: ch = 0x99; break;
				case 0x00F5: ch = 0x9B; break;
				case 0x00F6: ch = 0x9A; break;
				case 0x00F7: ch = 0xD6; break;
				case 0x00F9: ch = 0x9D; break;
				case 0x00FA: ch = 0x9C; break;
				case 0x00FB: ch = 0x9E; break;
				case 0x00FC: ch = 0x9F; break;
				case 0x00FF: ch = 0xD8; break;
				case 0x0102: ch = 0xAE; break;
				case 0x0103: ch = 0xBE; break;
				case 0x0131: ch = 0xF5; break;
				case 0x0152: ch = 0xCE; break;
				case 0x0153: ch = 0xCF; break;
				case 0x0178: ch = 0xD9; break;
				case 0x0192: ch = 0xC4; break;
				case 0x0218: ch = 0xAF; break;
				case 0x0219: ch = 0xBF; break;
				case 0x021A: ch = 0xDE; break;
				case 0x021B: ch = 0xDF; break;
				case 0x02C6: ch = 0xF6; break;
				case 0x02C7: ch = 0xFF; break;
				case 0x02D8: ch = 0xF9; break;
				case 0x02D9: ch = 0xFA; break;
				case 0x02DA: ch = 0xFB; break;
				case 0x02DB: ch = 0xFE; break;
				case 0x02DC: ch = 0xF7; break;
				case 0x02DD: ch = 0xFD; break;
				case 0x03A9: ch = 0xBD; break;
				case 0x03C0: ch = 0xB9; break;
				case 0x2013: ch = 0xD0; break;
				case 0x2014: ch = 0xD1; break;
				case 0x2018: ch = 0xD4; break;
				case 0x2019: ch = 0xD5; break;
				case 0x201A: ch = 0xE2; break;
				case 0x201C: ch = 0xD2; break;
				case 0x201D: ch = 0xD3; break;
				case 0x201E: ch = 0xE3; break;
				case 0x2020: ch = 0xA0; break;
				case 0x2021: ch = 0xE0; break;
				case 0x2022: ch = 0xA5; break;
				case 0x2026: ch = 0xC9; break;
				case 0x2030: ch = 0xE4; break;
				case 0x2039: ch = 0xDC; break;
				case 0x203A: ch = 0xDD; break;
				case 0x2044: ch = 0xDA; break;
				case 0x20AC: ch = 0xDB; break;
				case 0x2122: ch = 0xAA; break;
				case 0x2202: ch = 0xB6; break;
				case 0x2206: ch = 0xC6; break;
				case 0x220F: ch = 0xB8; break;
				case 0x2211: ch = 0xB7; break;
				case 0x221A: ch = 0xC3; break;
				case 0x221E: ch = 0xB0; break;
				case 0x222B: ch = 0xBA; break;
				case 0x2248: ch = 0xC5; break;
				case 0x2260: ch = 0xAD; break;
				case 0x2264: ch = 0xB2; break;
				case 0x2265: ch = 0xB3; break;
				case 0x25CA: ch = 0xD7; break;
				case 0xF8FF: ch = 0xF0; break;
				default: ch = 0x3F; break;
			}
			bytes[byteIndex++] = (byte)ch;
			--charCount;
		}
	}

	protected override void ToBytes(String s, int charIndex, int charCount,
	                                byte[] bytes, int byteIndex)
	{
		int ch;
		while(charCount > 0)
		{
			ch = (int)(s[charIndex++]);
			if(ch >= 0) switch(ch)
			{
				case 0x0020:
				case 0x0021:
				case 0x0022:
				case 0x0023:
				case 0x0024:
				case 0x0025:
				case 0x0026:
				case 0x0027:
				case 0x0028:
				case 0x0029:
				case 0x002A:
				case 0x002B:
				case 0x002C:
				case 0x002D:
				case 0x002E:
				case 0x002F:
				case 0x0030:
				case 0x0031:
				case 0x0032:
				case 0x0033:
				case 0x0034:
				case 0x0035:
				case 0x0036:
				case 0x0037:
				case 0x0038:
				case 0x0039:
				case 0x003A:
				case 0x003B:
				case 0x003C:
				case 0x003D:
				case 0x003E:
				case 0x003F:
				case 0x0040:
				case 0x0041:
				case 0x0042:
				case 0x0043:
				case 0x0044:
				case 0x0045:
				case 0x0046:
				case 0x0047:
				case 0x0048:
				case 0x0049:
				case 0x004A:
				case 0x004B:
				case 0x004C:
				case 0x004D:
				case 0x004E:
				case 0x004F:
				case 0x0050:
				case 0x0051:
				case 0x0052:
				case 0x0053:
				case 0x0054:
				case 0x0055:
				case 0x0056:
				case 0x0057:
				case 0x0058:
				case 0x0059:
				case 0x005A:
				case 0x005B:
				case 0x005C:
				case 0x005D:
				case 0x005E:
				case 0x005F:
				case 0x0060:
				case 0x0061:
				case 0x0062:
				case 0x0063:
				case 0x0064:
				case 0x0065:
				case 0x0066:
				case 0x0067:
				case 0x0068:
				case 0x0069:
				case 0x006A:
				case 0x006B:
				case 0x006C:
				case 0x006D:
				case 0x006E:
				case 0x006F:
				case 0x0070:
				case 0x0071:
				case 0x0072:
				case 0x0073:
				case 0x0074:
				case 0x0075:
				case 0x0076:
				case 0x0077:
				case 0x0078:
				case 0x0079:
				case 0x007A:
				case 0x007B:
				case 0x007C:
				case 0x007D:
				case 0x007E:
				case 0x00A2:
				case 0x00A3:
				case 0x00A9:
				case 0x00B1:
				case 0x00B5:
					break;
				case 0x00A0: ch = 0xCA; break;
				case 0x00A1: ch = 0xC1; break;
				case 0x00A5: ch = 0xB4; break;
				case 0x00A7: ch = 0xA4; break;
				case 0x00A8: ch = 0xAC; break;
				case 0x00AA: ch = 0xBB; break;
				case 0x00AB: ch = 0xC7; break;
				case 0x00AC: ch = 0xC2; break;
				case 0x00AE: ch = 0xA8; break;
				case 0x00AF: ch = 0xF8; break;
				case 0x00B0: ch = 0xA1; break;
				case 0x00B4: ch = 0xAB; break;
				case 0x00B6: ch = 0xA6; break;
				case 0x00B7: ch = 0xE1; break;
				case 0x00B8: ch = 0xFC; break;
				case 0x00BA: ch = 0xBC; break;
				case 0x00BB: ch = 0xC8; break;
				case 0x00BF: ch = 0xC0; break;
				case 0x00C0: ch = 0xCB; break;
				case 0x00C1: ch = 0xE7; break;
				case 0x00C2: ch = 0xE5; break;
				case 0x00C3: ch = 0xCC; break;
				case 0x00C4: ch = 0x80; break;
				case 0x00C5: ch = 0x81; break;
				case 0x00C7: ch = 0x82; break;
				case 0x00C8: ch = 0xE9; break;
				case 0x00C9: ch = 0x83; break;
				case 0x00CA: ch = 0xE6; break;
				case 0x00CB: ch = 0xE8; break;
				case 0x00CC: ch = 0xED; break;
				case 0x00CD: ch = 0xEA; break;
				case 0x00CE: ch = 0xEB; break;
				case 0x00CF: ch = 0xEC; break;
				case 0x00D1: ch = 0x84; break;
				case 0x00D2: ch = 0xF1; break;
				case 0x00D3: ch = 0xEE; break;
				case 0x00D4: ch = 0xEF; break;
				case 0x00D5: ch = 0xCD; break;
				case 0x00D6: ch = 0x85; break;
				case 0x00D9: ch = 0xF4; break;
				case 0x00DA: ch = 0xF2; break;
				case 0x00DB: ch = 0xF3; break;
				case 0x00DC: ch = 0x86; break;
				case 0x00DF: ch = 0xA7; break;
				case 0x00E0: ch = 0x88; break;
				case 0x00E1: ch = 0x87; break;
				case 0x00E2: ch = 0x89; break;
				case 0x00E3: ch = 0x8B; break;
				case 0x00E4: ch = 0x8A; break;
				case 0x00E5: ch = 0x8C; break;
				case 0x00E7: ch = 0x8D; break;
				case 0x00E8: ch = 0x8F; break;
				case 0x00E9: ch = 0x8E; break;
				case 0x00EA: ch = 0x90; break;
				case 0x00EB: ch = 0x91; break;
				case 0x00EC: ch = 0x93; break;
				case 0x00ED: ch = 0x92; break;
				case 0x00EE: ch = 0x94; break;
				case 0x00EF: ch = 0x95; break;
				case 0x00F1: ch = 0x96; break;
				case 0x00F2: ch = 0x98; break;
				case 0x00F3: ch = 0x97; break;
				case 0x00F4: ch = 0x99; break;
				case 0x00F5: ch = 0x9B; break;
				case 0x00F6: ch = 0x9A; break;
				case 0x00F7: ch = 0xD6; break;
				case 0x00F9: ch = 0x9D; break;
				case 0x00FA: ch = 0x9C; break;
				case 0x00FB: ch = 0x9E; break;
				case 0x00FC: ch = 0x9F; break;
				case 0x00FF: ch = 0xD8; break;
				case 0x0102: ch = 0xAE; break;
				case 0x0103: ch = 0xBE; break;
				case 0x0131: ch = 0xF5; break;
				case 0x0152: ch = 0xCE; break;
				case 0x0153: ch = 0xCF; break;
				case 0x0178: ch = 0xD9; break;
				case 0x0192: ch = 0xC4; break;
				case 0x0218: ch = 0xAF; break;
				case 0x0219: ch = 0xBF; break;
				case 0x021A: ch = 0xDE; break;
				case 0x021B: ch = 0xDF; break;
				case 0x02C6: ch = 0xF6; break;
				case 0x02C7: ch = 0xFF; break;
				case 0x02D8: ch = 0xF9; break;
				case 0x02D9: ch = 0xFA; break;
				case 0x02DA: ch = 0xFB; break;
				case 0x02DB: ch = 0xFE; break;
				case 0x02DC: ch = 0xF7; break;
				case 0x02DD: ch = 0xFD; break;
				case 0x03A9: ch = 0xBD; break;
				case 0x03C0: ch = 0xB9; break;
				case 0x2013: ch = 0xD0; break;
				case 0x2014: ch = 0xD1; break;
				case 0x2018: ch = 0xD4; break;
				case 0x2019: ch = 0xD5; break;
				case 0x201A: ch = 0xE2; break;
				case 0x201C: ch = 0xD2; break;
				case 0x201D: ch = 0xD3; break;
				case 0x201E: ch = 0xE3; break;
				case 0x2020: ch = 0xA0; break;
				case 0x2021: ch = 0xE0; break;
				case 0x2022: ch = 0xA5; break;
				case 0x2026: ch = 0xC9; break;
				case 0x2030: ch = 0xE4; break;
				case 0x2039: ch = 0xDC; break;
				case 0x203A: ch = 0xDD; break;
				case 0x2044: ch = 0xDA; break;
				case 0x20AC: ch = 0xDB; break;
				case 0x2122: ch = 0xAA; break;
				case 0x2202: ch = 0xB6; break;
				case 0x2206: ch = 0xC6; break;
				case 0x220F: ch = 0xB8; break;
				case 0x2211: ch = 0xB7; break;
				case 0x221A: ch = 0xC3; break;
				case 0x221E: ch = 0xB0; break;
				case 0x222B: ch = 0xBA; break;
				case 0x2248: ch = 0xC5; break;
				case 0x2260: ch = 0xAD; break;
				case 0x2264: ch = 0xB2; break;
				case 0x2265: ch = 0xB3; break;
				case 0x25CA: ch = 0xD7; break;
				case 0xF8FF: ch = 0xF0; break;
				default: ch = 0x3F; break;
			}
			bytes[byteIndex++] = (byte)ch;
			--charCount;
		}
	}

}; // class CP10010

public class ENCwindows_10010 : CP10010
{
	public ENCwindows_10010() : base() {}

}; // class ENCwindows_10010

}; // namespace I18N.West