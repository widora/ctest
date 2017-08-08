//--vertical 16x8 ASCII for oled
const unsigned char ascii_8x16[1536] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x38,0x00,0xfc,0x0d,0xfc,0x0d,0x38,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x0e,0x00,0x1e,0x00,0x00,0x00,0x00,0x00,0x1e,0x00,0x0e,0x00,0x00,0x00,
0x20,0x02,0xf8,0x0f,0xf8,0x0f,0x20,0x02,0xf8,0x0f,0xf8,0x0f,0x20,0x02,0x00,0x00,
0x38,0x03,0x7c,0x06,0x44,0x04,0x47,0x1c,0x47,0x1c,0xcc,0x07,0x98,0x03,0x00,0x00,
0x30,0x0c,0x30,0x06,0x00,0x03,0x80,0x01,0xc0,0x00,0x60,0x0c,0x30,0x0c,0x00,0x00,
0x80,0x07,0xd8,0x0f,0x7c,0x08,0xe4,0x08,0xbc,0x07,0xd8,0x0f,0x40,0x08,0x00,0x00,
0x00,0x00,0x10,0x00,0x1e,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xf0,0x03,0xf8,0x07,0x0c,0x0c,0x04,0x08,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x04,0x08,0x0c,0x0c,0xf8,0x07,0xf0,0x03,0x00,0x00,0x00,0x00,
0x80,0x00,0xa0,0x02,0xe0,0x03,0xc0,0x01,0xc0,0x01,0xe0,0x03,0xa0,0x02,0x80,0x00,
0x00,0x00,0x80,0x00,0x80,0x00,0xe0,0x03,0xe0,0x03,0x80,0x00,0x80,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x1e,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,
0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x0c,0x00,0x06,0x00,0x03,0x80,0x01,0xc0,0x00,0x60,0x00,0x30,0x00,0x00,0x00,
0xf8,0x07,0xfc,0x0f,0x04,0x09,0xc4,0x08,0x24,0x08,0xfc,0x0f,0xf8,0x07,0x00,0x00,
0x00,0x00,0x10,0x08,0x18,0x08,0xfc,0x0f,0xfc,0x0f,0x00,0x08,0x00,0x08,0x00,0x00,
0x08,0x0e,0x0c,0x0f,0x84,0x09,0xc4,0x08,0x64,0x08,0x3c,0x0c,0x18,0x0c,0x00,0x00,
0x08,0x04,0x0c,0x0c,0x44,0x08,0x44,0x08,0x44,0x08,0xfc,0x0f,0xb8,0x07,0x00,0x00,
0xc0,0x00,0xe0,0x00,0xb0,0x00,0x98,0x08,0xfc,0x0f,0xfc,0x0f,0x80,0x08,0x00,0x00,
0x7c,0x04,0x7c,0x0c,0x44,0x08,0x44,0x08,0xc4,0x08,0xc4,0x0f,0x84,0x07,0x00,0x00,
0xf0,0x07,0xf8,0x0f,0x4c,0x08,0x44,0x08,0x44,0x08,0xc0,0x0f,0x80,0x07,0x00,0x00,
0x0c,0x00,0x0c,0x00,0x04,0x0f,0x84,0x0f,0xc4,0x00,0x7c,0x00,0x3c,0x00,0x00,0x00,
0xb8,0x07,0xfc,0x0f,0x44,0x08,0x44,0x08,0x44,0x08,0xfc,0x0f,0xb8,0x07,0x00,0x00,
0x38,0x00,0x7c,0x08,0x44,0x08,0x44,0x08,0x44,0x0c,0xfc,0x07,0xf8,0x03,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x06,0x30,0x06,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x08,0x30,0x0e,0x30,0x06,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x80,0x00,0xc0,0x01,0x60,0x03,0x30,0x06,0x18,0x0c,0x08,0x08,0x00,0x00,
0x40,0x02,0x40,0x02,0x40,0x02,0x40,0x02,0x40,0x02,0x40,0x02,0x40,0x02,0x00,0x00,
0x00,0x00,0x08,0x08,0x18,0x0c,0x30,0x06,0x60,0x03,0xc0,0x01,0x80,0x00,0x00,0x00,
0x18,0x00,0x1c,0x00,0x04,0x00,0xc4,0x0d,0xe4,0x0d,0x3c,0x00,0x18,0x00,0x00,0x00,
0xf0,0x07,0xf8,0x0f,0x08,0x08,0xc8,0x0b,0xc8,0x0b,0xf8,0x0b,0xf0,0x01,0x00,0x00,
0xe0,0x0f,0xf0,0x0f,0x98,0x00,0x8c,0x00,0x98,0x00,0xf0,0x0f,0xe0,0x0f,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x44,0x08,0x44,0x08,0xfc,0x0f,0xb8,0x07,0x00,0x00,
0xf0,0x03,0xf8,0x07,0x0c,0x0c,0x04,0x08,0x04,0x08,0x0c,0x0c,0x18,0x06,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x04,0x08,0x0c,0x0c,0xf8,0x07,0xf0,0x03,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x44,0x08,0xe4,0x08,0x0c,0x0c,0x1c,0x0e,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x44,0x08,0xe4,0x00,0x0c,0x00,0x1c,0x00,0x00,0x00,
0xf0,0x03,0xf8,0x07,0x0c,0x0c,0x84,0x08,0x84,0x08,0x8c,0x07,0x98,0x0f,0x00,0x00,
0xfc,0x0f,0xfc,0x0f,0x40,0x00,0x40,0x00,0x40,0x00,0xfc,0x0f,0xfc,0x0f,0x00,0x00,
0x00,0x00,0x00,0x00,0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x04,0x08,0x00,0x00,0x00,0x00,
0x00,0x07,0x00,0x0f,0x00,0x08,0x04,0x08,0xfc,0x0f,0xfc,0x07,0x04,0x00,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0xc0,0x00,0xf0,0x01,0x3c,0x0f,0x0c,0x0e,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x04,0x08,0x00,0x08,0x00,0x0c,0x00,0x0e,0x00,0x00,
0xfc,0x0f,0xfc,0x0f,0x38,0x00,0x70,0x00,0x38,0x00,0xfc,0x0f,0xfc,0x0f,0x00,0x00,
0xfc,0x0f,0xfc,0x0f,0x38,0x00,0x70,0x00,0xe0,0x00,0xfc,0x0f,0xfc,0x0f,0x00,0x00,
0xf0,0x03,0xf8,0x07,0x0c,0x0c,0x04,0x08,0x0c,0x0c,0xf8,0x07,0xf0,0x03,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x44,0x08,0x44,0x00,0x7c,0x00,0x38,0x00,0x00,0x00,
0xf8,0x07,0xfc,0x0f,0x04,0x08,0x04,0x0e,0x04,0x3c,0xfc,0x3f,0xf8,0x27,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x44,0x00,0xc4,0x00,0xfc,0x0f,0x38,0x0f,0x00,0x00,
0x18,0x06,0x3c,0x0e,0x64,0x08,0x44,0x08,0xc4,0x08,0x9c,0x0f,0x18,0x07,0x00,0x00,
0x00,0x00,0x1c,0x00,0x0c,0x08,0xfc,0x0f,0xfc,0x0f,0x0c,0x08,0x1c,0x00,0x00,0x00,
0xfc,0x07,0xfc,0x0f,0x00,0x08,0x00,0x08,0x00,0x08,0xfc,0x0f,0xfc,0x07,0x00,0x00,
0xfc,0x01,0xfc,0x03,0x00,0x06,0x00,0x0c,0x00,0x06,0xfc,0x03,0xfc,0x01,0x00,0x00,
0xfc,0x03,0xfc,0x0f,0x00,0x0e,0x80,0x03,0x00,0x0e,0xfc,0x0f,0xfc,0x03,0x00,0x00,
0x0c,0x0c,0x3c,0x0f,0xf0,0x03,0xc0,0x00,0xf0,0x03,0x3c,0x0f,0x0c,0x0c,0x00,0x00,
0x00,0x00,0x3c,0x00,0x7c,0x08,0xc0,0x0f,0xc0,0x0f,0x7c,0x08,0x3c,0x00,0x00,0x00,
0x1c,0x0e,0x0c,0x0f,0x84,0x09,0xc4,0x08,0x64,0x08,0x3c,0x0c,0x1c,0x0e,0x00,0x00,
0x00,0x00,0x00,0x00,0xfc,0x0f,0xfc,0x0f,0x04,0x08,0x04,0x08,0x00,0x00,0x00,0x00,
0x38,0x00,0x70,0x00,0xe0,0x00,0xc0,0x01,0x80,0x03,0x00,0x07,0x00,0x0e,0x00,0x00,
0x00,0x00,0x00,0x00,0x04,0x08,0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x00,0x00,0x00,0x00,
0x08,0x00,0x0c,0x00,0x06,0x00,0x03,0x00,0x06,0x00,0x0c,0x00,0x08,0x00,0x00,0x00,
0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,0x00,0x20,
0x00,0x00,0x00,0x00,0x03,0x00,0x07,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x07,0xa0,0x0f,0xa0,0x08,0xa0,0x08,0xe0,0x07,0xc0,0x0f,0x00,0x08,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x07,0x20,0x08,0x60,0x08,0xc0,0x0f,0x80,0x07,0x00,0x00,
0xc0,0x07,0xe0,0x0f,0x20,0x08,0x20,0x08,0x20,0x08,0x60,0x0c,0x40,0x04,0x00,0x00,
0x80,0x07,0xc0,0x0f,0x60,0x08,0x24,0x08,0xfc,0x07,0xfc,0x0f,0x00,0x08,0x00,0x00,
0xc0,0x07,0xe0,0x0f,0xa0,0x08,0xa0,0x08,0xa0,0x08,0xe0,0x0c,0xc0,0x04,0x00,0x00,
0x40,0x08,0xf8,0x0f,0xfc,0x0f,0x44,0x08,0x0c,0x00,0x18,0x00,0x00,0x00,0x00,0x00,
0xc0,0x27,0xe0,0x6f,0x20,0x48,0x20,0x48,0xc0,0x7f,0xe0,0x3f,0x20,0x00,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x40,0x00,0x20,0x00,0xe0,0x0f,0xc0,0x0f,0x00,0x00,
0x00,0x00,0x00,0x00,0x20,0x08,0xec,0x0f,0xec,0x0f,0x00,0x08,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x30,0x00,0x70,0x00,0x40,0x20,0x40,0xec,0x7f,0xec,0x3f,0x00,0x00,
0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x80,0x01,0xc0,0x03,0x60,0x0e,0x20,0x0c,0x00,0x00,
0x00,0x00,0x00,0x00,0x04,0x08,0xfc,0x0f,0xfc,0x0f,0x00,0x08,0x00,0x00,0x00,0x00,
0xe0,0x0f,0xe0,0x0f,0x60,0x00,0xc0,0x0f,0x60,0x00,0xe0,0x0f,0xc0,0x0f,0x00,0x00,
0x20,0x00,0xe0,0x0f,0xc0,0x0f,0x20,0x00,0x20,0x00,0xe0,0x0f,0xc0,0x0f,0x00,0x00,
0xc0,0x07,0xe0,0x0f,0x20,0x08,0x20,0x08,0x20,0x08,0xe0,0x0f,0xc0,0x07,0x00,0x00,
0x20,0x40,0xe0,0x7f,0xc0,0x7f,0x20,0x48,0x20,0x08,0xe0,0x0f,0xc0,0x07,0x00,0x00,
0xc0,0x07,0xe0,0x0f,0x20,0x08,0x20,0x48,0xc0,0x7f,0xe0,0x7f,0x20,0x40,0x00,0x00,
0x20,0x08,0xe0,0x0f,0xc0,0x0f,0x60,0x08,0x20,0x00,0x60,0x00,0xc0,0x00,0x00,0x00,
0x40,0x04,0xe0,0x0c,0xa0,0x09,0x20,0x09,0x20,0x0b,0x60,0x0e,0x40,0x04,0x00,0x00,
0x20,0x00,0x20,0x00,0xf8,0x07,0xfc,0x0f,0x20,0x08,0x20,0x0c,0x00,0x04,0x00,0x00,
0xe0,0x07,0xe0,0x0f,0x00,0x08,0x00,0x08,0xe0,0x07,0xe0,0x0f,0x00,0x08,0x00,0x00,
0x00,0x00,0xe0,0x03,0xe0,0x07,0x00,0x0c,0x00,0x0c,0xe0,0x07,0xe0,0x03,0x00,0x00,
0xe0,0x07,0xe0,0x0f,0x00,0x0c,0x00,0x07,0x00,0x0c,0xe0,0x0f,0xe0,0x07,0x00,0x00,
0x20,0x08,0x60,0x0c,0xc0,0x07,0x80,0x03,0xc0,0x07,0x60,0x0c,0x20,0x08,0x00,0x00,
0xe0,0x47,0xe0,0x4f,0x00,0x48,0x00,0x48,0x00,0x68,0xe0,0x3f,0xe0,0x1f,0x00,0x00,
0x60,0x0c,0x60,0x0e,0x20,0x0b,0xa0,0x09,0xe0,0x08,0x60,0x0c,0x20,0x0c,0x00,0x00,
0x00,0x00,0x40,0x00,0x40,0x00,0xf8,0x07,0xbc,0x0f,0x04,0x08,0x04,0x08,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0xbc,0x0f,0xbc,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x04,0x08,0x04,0x08,0xbc,0x0f,0xf8,0x07,0x40,0x00,0x40,0x00,0x00,0x00,
0x08,0x00,0x0c,0x00,0x04,0x00,0x0c,0x00,0x08,0x00,0x0c,0x00,0x04,0x00,0x00,0x00,
0x80,0x07,0xc0,0x07,0x60,0x04,0x30,0x04,0x60,0x04,0xc0,0x07,0x80,0x07,0x00,0x00,
};
