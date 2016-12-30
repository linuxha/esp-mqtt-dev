#ifndef FONT5X7_H
#define FONT5X7_H

#include <user_config.h>

// Standard ASCII 5x7 font

// different code set (ISO 8859-1), modified places!!!

//static const unsigned char font[] ICACHE_RODATA_ATTR = {
//static const unsigned char font[] ICACHE_RAM_ATTR = {
static const unsigned char font[]  = {
0x00, 0x00, 0x00, 0x00, 0x00,	//	,0
0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
0x18, 0x3C, 0x7E, 0x3C, 0x18,
0x1C, 0x57, 0x7D, 0x57, 0x1C,
0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
0x00, 0x18, 0x3C, 0x18, 0x00,
0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
0x00, 0x18, 0x24, 0x18, 0x00,
0xFF, 0xE7, 0xDB, 0xE7, 0xFF,	//	, 10
0x30, 0x48, 0x3A, 0x06, 0x0E,
0x26, 0x29, 0x79, 0x29, 0x26,
0x40, 0x7F, 0x05, 0x05, 0x07,
0x40, 0x7F, 0x05, 0x25, 0x3F,
0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
0x7F, 0x3E, 0x1C, 0x1C, 0x08,
0x08, 0x1C, 0x1C, 0x3E, 0x7F,
0x14, 0x22, 0x7F, 0x22, 0x14,
0x5F, 0x5F, 0x00, 0x5F, 0x5F,
0x06, 0x09, 0x7F, 0x01, 0x7F,
0x00, 0x66, 0x89, 0x95, 0x6A,
0x60, 0x60, 0x60, 0x60, 0x60,
0x94, 0xA2, 0xFF, 0xA2, 0x94,
0x08, 0x04, 0x7E, 0x04, 0x08,
0x10, 0x20, 0x7E, 0x20, 0x10,
0x08, 0x08, 0x2A, 0x1C, 0x08,
0x08, 0x1C, 0x2A, 0x08, 0x08,
0x1E, 0x10, 0x10, 0x10, 0x10,
0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
0x30, 0x38, 0x3E, 0x38, 0x30,
0x06, 0x0E, 0x3E, 0x0E, 0x06,
0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x5F, 0x00, 0x00,
0x00, 0x07, 0x00, 0x07, 0x00,
0x14, 0x7F, 0x14, 0x7F, 0x14,
0x24, 0x2A, 0x7F, 0x2A, 0x12,
0x23, 0x13, 0x08, 0x64, 0x62,
0x36, 0x49, 0x56, 0x20, 0x50,
0x00, 0x08, 0x07, 0x03, 0x00,
0x00, 0x1C, 0x22, 0x41, 0x00,
0x00, 0x41, 0x22, 0x1C, 0x00,
0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
0x08, 0x08, 0x3E, 0x08, 0x08,
0x00, 0x80, 0x70, 0x30, 0x00,
0x08, 0x08, 0x08, 0x08, 0x08,
0x00, 0x00, 0x60, 0x60, 0x00,
0x20, 0x10, 0x08, 0x04, 0x02,
0x3E, 0x51, 0x49, 0x45, 0x3E,
0x00, 0x42, 0x7F, 0x40, 0x00,
0x72, 0x49, 0x49, 0x49, 0x46,
0x21, 0x41, 0x49, 0x4D, 0x33,
0x18, 0x14, 0x12, 0x7F, 0x10,
0x27, 0x45, 0x45, 0x45, 0x39,
0x3C, 0x4A, 0x49, 0x49, 0x31,
0x41, 0x21, 0x11, 0x09, 0x07,
0x36, 0x49, 0x49, 0x49, 0x36,
0x46, 0x49, 0x49, 0x29, 0x1E,
0x00, 0x00, 0x14, 0x00, 0x00,
0x00, 0x40, 0x34, 0x00, 0x00,
0x00, 0x08, 0x14, 0x22, 0x41,
0x14, 0x14, 0x14, 0x14, 0x14,
0x00, 0x41, 0x22, 0x14, 0x08,
0x02, 0x01, 0x59, 0x09, 0x06,
0x3E, 0x41, 0x5D, 0x59, 0x4E,
0x7C, 0x12, 0x11, 0x12, 0x7C,
0x7F, 0x49, 0x49, 0x49, 0x36,
0x3E, 0x41, 0x41, 0x41, 0x22,
0x7F, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x49, 0x49, 0x49, 0x41,
0x7F, 0x09, 0x09, 0x09, 0x01,
0x3E, 0x41, 0x41, 0x51, 0x73,
0x7F, 0x08, 0x08, 0x08, 0x7F,
0x00, 0x41, 0x7F, 0x41, 0x00,
0x20, 0x40, 0x41, 0x3F, 0x01,
0x7F, 0x08, 0x14, 0x22, 0x41,
0x7F, 0x40, 0x40, 0x40, 0x40,
0x7F, 0x02, 0x1C, 0x02, 0x7F,
0x7F, 0x04, 0x08, 0x10, 0x7F,
0x3E, 0x41, 0x41, 0x41, 0x3E,
0x7F, 0x09, 0x09, 0x09, 0x06,
0x3E, 0x41, 0x51, 0x21, 0x5E,
0x7F, 0x09, 0x19, 0x29, 0x46,
0x26, 0x49, 0x49, 0x49, 0x32,
0x03, 0x01, 0x7F, 0x01, 0x03,
0x3F, 0x40, 0x40, 0x40, 0x3F,
0x1F, 0x20, 0x40, 0x20, 0x1F,
0x3F, 0x40, 0x38, 0x40, 0x3F,
0x63, 0x14, 0x08, 0x14, 0x63,
0x03, 0x04, 0x78, 0x04, 0x03,
0x61, 0x59, 0x49, 0x4D, 0x43,
0x00, 0x7F, 0x41, 0x41, 0x41,
0x02, 0x04, 0x08, 0x10, 0x20,
0x00, 0x41, 0x41, 0x41, 0x7F,
0x04, 0x02, 0x01, 0x02, 0x04,
0x40, 0x40, 0x40, 0x40, 0x40,
0x00, 0x03, 0x07, 0x08, 0x00,
0x20, 0x54, 0x54, 0x78, 0x40,
0x7F, 0x28, 0x44, 0x44, 0x38,
0x38, 0x44, 0x44, 0x44, 0x28,
0x38, 0x44, 0x44, 0x28, 0x7F,
0x38, 0x54, 0x54, 0x54, 0x18,
0x00, 0x08, 0x7E, 0x09, 0x02,
0x18, 0xA4, 0xA4, 0x9C, 0x78,
0x7F, 0x08, 0x04, 0x04, 0x78,
0x00, 0x44, 0x7D, 0x40, 0x00,
0x20, 0x40, 0x40, 0x3D, 0x00,
0x7F, 0x10, 0x28, 0x44, 0x00,
0x00, 0x41, 0x7F, 0x40, 0x00,
0x7C, 0x04, 0x78, 0x04, 0x78,
0x7C, 0x08, 0x04, 0x04, 0x78,
0x38, 0x44, 0x44, 0x44, 0x38,
0xFC, 0x18, 0x24, 0x24, 0x18,
0x18, 0x24, 0x24, 0x18, 0xFC,
0x7C, 0x08, 0x04, 0x04, 0x08,
0x48, 0x54, 0x54, 0x54, 0x24,
0x04, 0x04, 0x3F, 0x44, 0x24,
0x3C, 0x40, 0x40, 0x20, 0x7C,
0x1C, 0x20, 0x40, 0x20, 0x1C,
0x3C, 0x40, 0x30, 0x40, 0x3C,
0x44, 0x28, 0x10, 0x28, 0x44,
0x4C, 0x90, 0x90, 0x90, 0x7C,
0x44, 0x64, 0x54, 0x4C, 0x44,
0x00, 0x08, 0x36, 0x41, 0x00,
0x00, 0x00, 0x77, 0x00, 0x00,
0x00, 0x41, 0x36, 0x08, 0x00,
0x02, 0x01, 0x02, 0x04, 0x02,
0x3C, 0x26, 0x23, 0x26, 0x3C,
0x1E, 0xA1, 0xA1, 0x61, 0x12,	//	,128
0x3A, 0x40, 0x40, 0x20, 0x7A,	//	ü ,129
0x38, 0x54, 0x54, 0x55, 0x59,	// é, 130
0x21, 0x55, 0x55, 0x79, 0x41,	//	,131
0x22, 0x54, 0x54, 0x78, 0x42, 	// a-umlaut, 132
0x21, 0x55, 0x54, 0x78, 0x40,	// á,133
0x20, 0x54, 0x55, 0x79, 0x40,	//	,134
0x0C, 0x1E, 0x52, 0x72, 0x12,	//	,135
0x39, 0x55, 0x55, 0x55, 0x59,	//	,136
0x39, 0x54, 0x54, 0x54, 0x59,	//	,137
0x39, 0x55, 0x54, 0x54, 0x58,	//	,138
0x00, 0x00, 0x45, 0x7C, 0x41,	//	,139
0x00, 0x02, 0x45, 0x7D, 0x42,	//	,140
0x00, 0x01, 0x45, 0x7C, 0x40,	//	,141
0x7D, 0x12, 0x11, 0x12, 0x7D, 	// A-umlaut, 142
0xF0, 0x28, 0x25, 0x28, 0xF0,	//	,143
0x7C, 0x54, 0x55, 0x45, 0x00,	// É, 144
0x20, 0x54, 0x54, 0x7C, 0x54,	//	, 145
0x7C, 0x0A, 0x09, 0x7F, 0x49,	//	, 146	//0x32, 0x49, 0x49, 0x49, 0x32,	// ő, 147
0x32, 0x49, 0x48, 0x4A, 0x31,	// ő, 147  ne kalapos ő legyen, mod
0x3A, 0x44, 0x44, 0x44, 0x3A, 	// ö, 148
0x32, 0x4A, 0x48, 0x48, 0x30,	// ó, 149
0x3A, 0x41, 0x41, 0x21, 0x7A,	// ű, 150
0x3A, 0x42, 0x40, 0x20, 0x78,	// ú, 151
0x00, 0x9D, 0xA0, 0xA0, 0x7D,	// 	, 152
0x3D, 0x42, 0x42, 0x42, 0x3D, 	// Ö, 153
0x3D, 0x40, 0x40, 0x40, 0x3D,	// Ü, 154
0x3C, 0x24, 0xFF, 0x24, 0x24,	//	,155
0x48, 0x7E, 0x49, 0x43, 0x66,	//	,156
0x2B, 0x2F, 0xFC, 0x2F, 0x2B,	//	,157
0xFF, 0x09, 0x29, 0xF6, 0x20,	//	,158
0xC0, 0x88, 0x7E, 0x09, 0x03,	//	,159
0x20, 0x54, 0x54, 0x79, 0x41,	// á, 160
0x00, 0x00, 0x44, 0x7D, 0x41,	// í, 161
0x30, 0x48, 0x48, 0x4A, 0x32,	// ó, 162
0x38, 0x40, 0x40, 0x22, 0x7A,	// ú, 163
0x00, 0x7A, 0x0A, 0x0A, 0x72,	// , 164
0x7D, 0x0D, 0x19, 0x31, 0x7D,	// , 165
0x26, 0x29, 0x29, 0x2F, 0x28,	// , 166
0x26, 0x29, 0x29, 0x29, 0x26,	// , 167
0x30, 0x48, 0x4D, 0x40, 0x20,	// , 168
0x38, 0x08, 0x08, 0x08, 0x08,	// , 169
0x08, 0x08, 0x08, 0x08, 0x38,	// , 170
0x2F, 0x10, 0xC8, 0xAC, 0xBA,	// , 171
0x2F, 0x10, 0x28, 0x34, 0xFA,	// , 172
0x00, 0x00, 0x7B, 0x00, 0x00,	// , 173
0x08, 0x14, 0x2A, 0x14, 0x22,	// , 174
0x22, 0x14, 0x2A, 0x14, 0x08,	// , 175
0xAA, 0x00, 0x55, 0x00, 0xAA,	// , 176
0xAA, 0x55, 0xAA, 0x55, 0xAA,	// , 177
0x00, 0x00, 0x00, 0xFF, 0x00,	// , 178
0x10, 0x10, 0x10, 0xFF, 0x00,	// , 179
0x14, 0x14, 0x14, 0xFF, 0x00,	// , 180
0x10, 0x10, 0xFF, 0x00, 0xFF,	// , 181
0x10, 0x10, 0xF0, 0x10, 0xF0,	// , 182
0x14, 0x14, 0x14, 0xFC, 0x00,	// , 183
0x14, 0x14, 0xF7, 0x00, 0xFF,	// , 184
0x00, 0x00, 0xFF, 0x00, 0xFF,	// , 185
0x14, 0x14, 0xF4, 0x04, 0xFC,	// , 186
0x14, 0x14, 0x17, 0x10, 0x1F,	// , 187
0x10, 0x10, 0x1F, 0x10, 0x1F,	// , 188
0x14, 0x14, 0x14, 0x1F, 0x00,	// , 189
0x10, 0x10, 0x10, 0xF0, 0x00,	// , 190
0x00, 0x00, 0x00, 0x1F, 0x10,	// , 191
0x10, 0x10, 0x10, 0x1F, 0x10,	// , 192 0x10, 0x10, 0x10, 0xF0, 0x10,	// , 193
0x7D, 0x12, 0x11, 0x12, 0x7D, 	// A-umlaut, 142
0x00, 0x00, 0x00, 0xFF, 0x10,	// , 194
0x10, 0x10, 0x10, 0x10, 0x10,	// , 195
0x10, 0x10, 0x10, 0xFF, 0x10,	// , 196
0x00, 0x00, 0x00, 0xFF, 0x14,	// , 197
0x00, 0x00, 0xFF, 0x00, 0xFF,	// , 198
0x00, 0x00, 0x1F, 0x10, 0x17,	// , 199
0x00, 0x00, 0xFC, 0x04, 0xF4,	// , 200
0x14, 0x14, 0x17, 0x10, 0x17,	// , 201
0x14, 0x14, 0xF4, 0x04, 0xF4,	// , 202
0x00, 0x00, 0xFF, 0x00, 0xF7,	// , 203
0x14, 0x14, 0x14, 0x14, 0x14,	// , 204
0x14, 0x14, 0xF7, 0x00, 0xF7,	// , 205
0x14, 0x14, 0x14, 0x17, 0x14,	// , 206
0x10, 0x10, 0x1F, 0x10, 0x1F,	// , 207
0x14, 0x14, 0x14, 0xF4, 0x14,	// , 208
0x10, 0x10, 0xF0, 0x10, 0xF0,	// , 209
0x00, 0x00, 0x1F, 0x10, 0x1F,	// , 210
0x00, 0x00, 0x00, 0x1F, 0x14,	// , 211
0x00, 0x00, 0x00, 0xFC, 0x14,	// , 212
0x00, 0x00, 0xF0, 0x10, 0xF0,	// , 213
0x10, 0x10, 0xFF, 0x10, 0xFF,	// , 214
0x14, 0x14, 0x14, 0xFF, 0x14,	// , 215
0x10, 0x10, 0x10, 0x1F, 0x00,	// , 216
0x00, 0x00, 0x00, 0xF0, 0x10,	// , 217
0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// , 218
0xF0, 0xF0, 0xF0, 0xF0, 0xF0,	// , 219
0xFF, 0xFF, 0xFF, 0x00, 0x00,	// , 220
0x00, 0x00, 0x00, 0xFF, 0xFF,	// , 221
0x0F, 0x0F, 0x0F, 0x0F, 0x0F,	// , 222
0x38, 0x44, 0x44, 0x38, 0x44,	// , 223
0xFC, 0x4A, 0x4A, 0x4A, 0x34, 	// sharp-s or beta alpha???, 224 0x7E, 0x02, 0x02, 0x06, 0x06,	// , 225
0x21, 0x55, 0x54, 0x78, 0x40,	// á,133
0x02, 0x7E, 0x02, 0x7E, 0x02,	// , 226
0x63, 0x55, 0x49, 0x41, 0x63,	// , 227
0x38, 0x44, 0x44, 0x3C, 0x04,	// , 228
0x40, 0x7E, 0x20, 0x1E, 0x20,	// , 229
0x06, 0x02, 0x7E, 0x02, 0x02,	// , 230
0x99, 0xA5, 0xE7, 0xA5, 0x99,	// , 231
0x1C, 0x2A, 0x49, 0x2A, 0x1C,	// , 232 0x4C, 0x72, 0x01, 0x72, 0x4C,	// , 233
0x38, 0x54, 0x54, 0x55, 0x59,	// é, 130
0x30, 0x4A, 0x4D, 0x4D, 0x30,	// , 234
0x30, 0x48, 0x78, 0x48, 0x30,	// , 235
0xBC, 0x62, 0x5A, 0x46, 0x3D,	// , 236
0x3E, 0x49, 0x49, 0x49, 0x00,	// , 237
0x7E, 0x01, 0x01, 0x01, 0x7E,	// , 238
0x2A, 0x2A, 0x2A, 0x2A, 0x2A,	// , 239
0x44, 0x44, 0x5F, 0x44, 0x44,	// , 240
0x40, 0x51, 0x4A, 0x44, 0x40,	// , 241
0x40, 0x44, 0x4A, 0x51, 0x40,	// , 242 0x00, 0x00, 0xFF, 0x01, 0x03,	// , 243
0x30, 0x48, 0x48, 0x4A, 0x32,	// ó, 162
0xE0, 0x80, 0xFF, 0x00, 0x00,	// , 244 0x08, 0x08, 0x6B, 0x6B, 0x08,	// , 245
0x32, 0x49, 0x48, 0x4A, 0x31,	// ő, 147  ne kalapos ő legyen, mod
0x36, 0x12, 0x36, 0x24, 0x36,	// , 246
0x06, 0x0F, 0x09, 0x0F, 0x06,	// , 247
0x00, 0x00, 0x18, 0x18, 0x00,	// , 248
0x00, 0x00, 0x10, 0x10, 0x00,	// , 249
0x30, 0x40, 0xFF, 0x01, 0x01,	// , 250 0x00, 0x1F, 0x01, 0x01, 0x1E,	// , 251 0x00, 0x19, 0x1D, 0x17, 0x12,	// , 252
0x3A, 0x41, 0x41, 0x21, 0x7A,	// ű, 150
0x3A, 0x40, 0x40, 0x20, 0x7A,	//ü, 129
0x00, 0x3C, 0x3C, 0x3C, 0x3C,	// , 253
0x00, 0x00, 0x00, 0x00, 0x00	// , 254
};

#endif // FONT5X7_H

/* [] END OF FILE */
