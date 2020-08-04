/* vim: set ai et ts=4 sw=4: */
/*Things done to make this work without HAL dependency
 *obviously remove hal functions and make your own
 *delay
 *spi send
 *also added the toggle of CS pin to the writeChar function and removed static atribute
 *to make it accesible to rest of the program
 *
 *first couple of functions need hal removal and also ST7735_FillRectangle makes a call to hal*/
#include "st7735.h"
#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"

#define DELAY 0x80

#define TFT_BL_H()	ST7735_BL_GPIO_Port->BSRR = ST7735_BL_Pin
#define TFT_BL_L()	ST7735_BL_GPIO_Port->BRR 	= ST7735_BL_Pin
#define TFT_CS_H()	ST7735_CS_GPIO_Port->BSRR = ST7735_CS_Pin
#define TFT_CS_L()	ST7735_CS_GPIO_Port->BRR 	= ST7735_CS_Pin
#define TFT_DC_D()	ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin
#define TFT_DC_C()	ST7735_DC_GPIO_Port->BRR 	= ST7735_DC_Pin
#define TFT_RES_H()	ST7735_RES_GPIO_Port->BSRR = ST7735_RES_Pin
#define TFT_RES_L()	ST7735_RES_GPIO_Port->BRR	 = ST7735_RES_Pin

#define ST7735_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))
#define SWAP_INT16_T(a, b) { int16_t t = a; a = b; b = t; }
#define DELAY 0x80

#if defined(ST7735_1_8_DEFAULT_ORIENTATION) || defined(ST7735S_1_8_DEFAULT_ORIENTATION)
static uint8_t _data_rotation[4] = { ST7735_MADCTL_MX, ST7735_MADCTL_MY, ST7735_MADCTL_MV, ST7735_MADCTL_RGB };
#endif

#if defined(ST7735_1_44_DEFAULT_ORIENTATION) || defined(ST7735_MINI_DEFAULT_ORIENTATION)
static uint8_t _data_rotation[4] = { ST7735_MADCTL_MX, ST7735_MADCTL_MY, ST7735_MADCTL_MV, ST7735_MADCTL_BGR };
#endif

static uint8_t _value_rotation = 0;
static int16_t _height = ST7735_HEIGHT, _width = ST7735_WIDTH;
static uint8_t _xstart = ST7735_XSTART, _ystart = ST7735_YSTART;


extern void spiSend(uint8_t *data, uint32_t len);
extern void spiSend16(uint16_t *data, uint32_t len);
extern void delayMS(uint32_t ms);
// based on Adafruit ST7735 library for Arduino
static const uint8_t
  init_cmds1[] = {
	// Init for 7735R, part 1 (red or green tab)
15,
	// 15 commands in list:
ST7735_SWRESET,
	DELAY,
	//  1: Software reset, 0 args, w/delay
   150,
	//     150 ms delay
ST7735_SLPOUT,
	DELAY,
	//  2: Out of sleep mode, 0 args, w/delay
   255, 
	//     500 ms delay
ST7735_FRMCTR1,
	3,
	//  3: Frame rate ctrl - normal mode, 3 args:
   0x01,
	0x2C,
	0x2D, 
	//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
ST7735_FRMCTR2,
	3,
	//  4: Frame rate control - idle mode, 3 args:
   0x01,
	0x2C,
	0x2D,
	//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
ST7735_FRMCTR3,
	6,
	//  5: Frame rate ctrl - partial mode, 6 args:
   0x01,
	0x2C,
	0x2D, 
	//     Dot inversion mode
0x01,
	0x2C,
	0x2D, 
	//     Line inversion mode
ST7735_INVCTR,
	1, 
	//  6: Display inversion ctrl, 1 arg, no delay:
	0x07, 
	//     No inversion
ST7735_PWCTR1,
	3,
	//  7: Power control, 3 args, no delay:
   0xA2,
	0x02,
	//     -4.6V
0x84,
	//     AUTO mode
ST7735_PWCTR2,
	1,
	//  8: Power control, 1 arg, no delay:
 0xC5, 
	//     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
	ST7735_PWCTR3,
	2, 
	//  9: Power control, 2 args, no delay:
0x0A,
	//     Opamp current small
0x00,
	//     Boost frequency
	ST7735_PWCTR4,
	2,
	// 10: Power control, 2 args, no delay:
0x8A,
	//     BCLK/2, Opamp current small & Medium low
0x2A,  
	ST7735_PWCTR5,
	2, 
	// 11: Power control, 2 args, no delay:
	0x8A,
	0xEE,
	ST7735_VMCTR1,
	1,
	// 12: Power control, 1 arg, no delay:
   0x0E,
	ST7735_INVOFF,
	0,
	// 13: Don't invert display, no args, no delay
 ST7735_MADCTL,
	1,
	// 14: Memory access control (directions), 1 arg:
   ST7735_ROTATION, 
	//     row addr/col addr, bottom to top refresh
ST7735_COLMOD,
	1, 
	// 15: set color mode, 1 arg, no delay:
	0x05
},                    //     16-bit color

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
	    init_cmds2[] = {
	// Init for 7735R, part 2 (1.44" display)
2,
	//  2 commands in list:
ST7735_CASET,
	4,
	//  1: Column addr set, 4 args, no delay:
	     0x00,
	0x00,
	//     XSTART = 0
0x00,
	0x7F, 
	//     XEND = 127
ST7735_RASET,
	4,
	//  2: Row addr set, 4 args, no delay:
	   0x00,
	0x00, 
	//     XSTART = 0
0x00,
	0x7F
},              //     XEND = 127
#endif // ST7735_IS_128X128

#ifdef ST7735_IS_160X80
	        init_cmds2[] = {
	// Init for 7735S, part 2 (160x80 display)
 3,
	//  3 commands in list:
ST7735_CASET,
	4,
	//  1: Column addr set, 4 args, no delay:
	         0x00,
	0x00, 
	//     XSTART = 0
   0x00,
	0x4F,
	//     XEND = 79
ST7735_RASET,
	4,
	//  2: Row addr set, 4 args, no delay:
	       0x00,
	0x00, 
	//     XSTART = 0
 0x00,
	0x9F,
	//     XEND = 159
ST7735_INVON,
	0
},           //  3: Invert colors
#endif

	        init_cmds3[] = { 
	// Init for 7735R, part 3 (red or green tab)
  4,
	//  4 commands in list:
ST7735_GMCTRP1,
	16,
	//  1: Magical unicorn dust, 16 args, no delay:
	0x02,
	0x1c,
	0x07,
	0x12,
	0x37,
	0x32,
	0x29,
	0x2d,
	0x29,
	0x25,
	0x2B,
	0x39,
	0x00,
	0x01,
	0x03,
	0x10,
	ST7735_GMCTRN1,
	16,
	//  2: Sparkles and rainbows, 16 args, no delay:
	0x03,
	0x1d,
	0x07,
	0x06,
	0x2E,
	0x2C,
	0x29,
	0x2D,
	0x2E,
	0x2E,
	0x37,
	0x3F,
	0x00,
	0x00,
	0x02,
	0x10,
	ST7735_NORON,
	DELAY,
	//  3: Normal display on, no args, w/delay
	10,
	//     10 ms delay
ST7735_DISPON,
	DELAY,
	//  4: Main screen turn on, no args w/delay
	100
};                     //     100 ms delay



static void ST7735_Select() {
	// HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET);
	 ST7735_CS_GPIO_Port->BRR = ST7735_CS_Pin;
}

void ST7735_Unselect() {
	//HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET);
	ST7735_CS_GPIO_Port->BSRR = ST7735_CS_Pin;
}

static void ST7735_Reset() {
	// HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
	 ST7735_RES_GPIO_Port->BRR = ST7735_RES_Pin;
	delayMS(5);
	ST7735_RES_GPIO_Port->BSRR = ST7735_RES_Pin;
	//HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
}

static void ST7735_WriteCommand(uint8_t cmd) {
	//HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
	ST7735_DC_GPIO_Port->BRR = ST7735_DC_Pin;
	//  HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
	   spiSend(&cmd, 1);    //doesnt work when called from here

}

static void ST7735_WriteData(uint8_t* buff, size_t buff_size) {
	//  HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
	  ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
	//HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
	spiSend(buff, buff_size);    // works fine when called from here

}
static void ST7735_WriteData16(uint16_t* buff, uint32_t buff_size) {
	//  HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
	  ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
	//HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
	spiSend16(buff, buff_size);     // works fine when called from here

}
void ST7735_Backlight_On(void)
{
			
	GPIOA->BSRR = LL_GPIO_PIN_4;
}
void ST7735_Backlight_Off(void)
{
			
	ST7735_BL_GPIO_Port->BRR = ST7735_BL_Pin;
}
void ST7735_printMsg(uint8_t x , uint8_t y ,char *msg, ...)
{
	
	char buff[80];

	
	va_list args;
	va_start(args, msg);
	vsprintf(buff, msg, args);
		
	ST7735_WriteString(x, y, buff, Font_7x10, ST7735_WHITE, 0x00A4);
	

	
}
//******************************************************************************/
//******************************************************************************/
//******************************************************************************/
static void ST7735_ExecuteCommandList(const uint8_t *addr) //all commands are 8bit so leave as is
{
	uint8_t numCommands, numArgs;
	uint16_t ms;

	numCommands = *addr++;
	while (numCommands--) {
		uint8_t cmd = *addr++;
		ST7735_WriteCommand(cmd);

		numArgs = *addr++;
		// If high bit set, delay follows args
		ms = numArgs & DELAY;
		numArgs &= ~DELAY;
		if (numArgs) {
			ST7735_WriteData((uint8_t*)addr, numArgs);
			addr += numArgs;
		}

		if (ms) {
			ms = *addr++;
			if (ms == 255) ms = 500;
			delayMS(ms);
		}
	}
}
/*
static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	// column address set
	ST7735_WriteCommand(ST7735_CASET);
	uint8_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
	uint16_t data2[] = {  x0 + ST7735_XSTART,  x1 + ST7735_XSTART };
	ST7735_WriteData(data, sizeof(data));
	ST7735_WriteData16(data2, sizeof(data2));

	// row address set
	ST7735_WriteCommand(ST7735_RASET);
	data[1] = y0 + ST7735_YSTART;
	data[3] = y1 + ST7735_YSTART;
	ST7735_WriteData(data, sizeof(data));

	// write to RAM
	ST7735_WriteCommand(ST7735_RAMWR);
} */
static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	// column address set
	ST7735_WriteCommand(ST7735_CASET);
	uint16_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
	//uint16_t data2[] = { x0 + ST7735_XSTART, x1 + ST7735_XSTART };
	
	ST7735_WriteData16(data, sizeof(data)/2);

	// row address set
	ST7735_WriteCommand(ST7735_RASET);
	data[1] = y0 + ST7735_YSTART;
	data[3] = y1 + ST7735_YSTART;
	ST7735_WriteData16(data, sizeof(data)/2);

	// write to RAM
	ST7735_WriteCommand(ST7735_RAMWR);
}

void ST7735_Init() {
	ST7735_Select();
	ST7735_Reset();
	ST7735_ExecuteCommandList(init_cmds1);
	ST7735_ExecuteCommandList(init_cmds2);
	ST7735_ExecuteCommandList(init_cmds3);
	ST7735_Unselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
	if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
		return;

	ST7735_Select();

	ST7735_SetAddressWindow(x, y, x + 1, y + 1);
	uint8_t data[] = { color >> 8, color & 0xFF };
	
	ST7735_WriteData(data, 2);
	//ST7735_WriteData16(&color, 1);
	//LL_SPI_TransmitData16(SPI,color);
	ST7735_Unselect();
}

void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
	uint32_t i, b, j;
	ST7735_Select();
	ST7735_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);
	//delayMS(1);
 //	LL_SPI_Disable(SPI1);
//	LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_16BIT);
	//LL_SPI_Enable(SPI1);
	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				uint8_t data[] = { color >> 8, color & 0xFF };
				ST7735_WriteData(data, sizeof(data));
				 //ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
			//	LL_SPI_TransmitData16(SPI1, color);
			}
			else {
				uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
				//ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
				ST7735_WriteData(data, sizeof(data));
				//LL_SPI_TransmitData16(SPI1, bgcolor);
			}
		}
	}	ST7735_Unselect();
	//delayMS(1);
	//LL_SPI_Disable(SPI1);
//	LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT);
	//LL_SPI_Enable(SPI1);
}

/*
Simpler (and probably slower) implementation:

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color) {
    uint32_t i, b, j;

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                ST7735_DrawPixel(x + j, y + i, color);
            } 
        }
    }
}
*/

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
	ST7735_Select();

	while (*str) {
		if (x + font.width >= ST7735_WIDTH) {
			x = 0;
			y += font.height;
			if (y + font.height >= ST7735_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}

		ST7735_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}

	ST7735_Unselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
	// clipping
	if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
	if ((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
	if ((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

	ST7735_Select();
	ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

	uint8_t data[] = { color >> 8, color & 0xFF };
	//HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
	ST7735_DC_GPIO_Port->BSRR = ST7735_DC_Pin;
	for (y = h; y > 0; y--) {
		for (x = w; x > 0; x--) {
			// HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
			   spiSend(data, sizeof(data));
		}
	}

	ST7735_Unselect();
}

void ST7735_FillScreen(uint16_t color) {
	ST7735_FillRectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
	if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
	if ((x + w - 1) >= ST7735_WIDTH) return;
	if ((y + h - 1) >= ST7735_HEIGHT) return;

	ST7735_Select();
	ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	uint32_t temp =  sizeof(uint16_t)*w*h;
	ST7735_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
	ST7735_Unselect();
}

void ST7735_InvertColors(bool invert) {
	ST7735_Select();
	ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
	ST7735_Unselect();
}
/***************************************************************************************
** Function name:           drawCircle
** Description:             Draw a circle outline
***************************************************************************************/
void ST7735_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -r - r;
	int16_t x = 0;

	ST7735_DrawPixel(x0 + r, y0, color);
	ST7735_DrawPixel(x0 - r, y0, color);
	ST7735_DrawPixel(x0, y0 - r, color);
	ST7735_DrawPixel(x0, y0 + r, color);

	while (x < r)
	{
		if (f >= 0)
		{
			r--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7735_DrawPixel(x0 + x, y0 + r, color);
		ST7735_DrawPixel(x0 - x, y0 + r, color);
		ST7735_DrawPixel(x0 - x, y0 - r, color);
		ST7735_DrawPixel(x0 + x, y0 - r, color);

		ST7735_DrawPixel(x0 + r, y0 + x, color);
		ST7735_DrawPixel(x0 - r, y0 + x, color);
		ST7735_DrawPixel(x0 - r, y0 - x, color);
		ST7735_DrawPixel(x0 + r, y0 - x, color);
	}
}

/***************************************************************************************
** Function name:           drawCircleHelper
** Description:             Support function for circle drawing
***************************************************************************************/
void ST7735_DrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x     = 0;

	while (x < r)
	{
		if (f >= 0)
		{
			r--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;
		if (cornername & 0x8)
		{
			ST7735_DrawPixel(x0 - r, y0 + x, color);
			ST7735_DrawPixel(x0 - x, y0 + r, color);
		}
		if (cornername & 0x4)
		{
			ST7735_DrawPixel(x0 + x, y0 + r, color);
			ST7735_DrawPixel(x0 + r, y0 + x, color);
		}
		if (cornername & 0x2)
		{
			ST7735_DrawPixel(x0 + r, y0 - x, color);
			ST7735_DrawPixel(x0 + x, y0 - r, color);
		}
		if (cornername & 0x1)
		{
			ST7735_DrawPixel(x0 - x, y0 - r, color);
			ST7735_DrawPixel(x0 - r, y0 - x, color);
		}

	}
}

/***************************************************************************************
** Function name:           fillCircle
** Description:             draw a filled circle
***************************************************************************************/
void ST7735_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	ST7735_DrawFastVLine(x0, y0 - r, r + r + 1, color);
	ST7735_FillCircleHelper(x0, y0, r, 3, 0, color);
}

/***************************************************************************************
** Function name:           fillCircleHelper
** Description:             Support function for filled circle drawing
***************************************************************************************/
// Used to do circles and roundrects
void ST7735_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color)
{
	int16_t f     = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -r - r;
	int16_t x     = 0;

	delta++;
	while (x < r)
	{
		if (f >= 0)
		{
			r--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1)
		{
			ST7735_DrawFastVLine(x0 + x, y0 - r, r + r + delta, color);
			ST7735_DrawFastVLine(x0 + r, y0 - x, x + x + delta, color);
		}
		if (cornername & 0x2)
		{
			ST7735_DrawFastVLine(x0 - x, y0 - r, r + r + delta, color);
			ST7735_DrawFastVLine(x0 - r, y0 - x, x + x + delta, color);
		}
	}
}

/***************************************************************************************
** Function name:           drawEllipse
** Description:             Draw a ellipse outline
***************************************************************************************/
void ST7735_DrawEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color)
{
	if (rx < 2) return;
	if (ry < 2) return;
	int16_t x, y;
	int32_t rx2 = rx * rx;
	int32_t ry2 = ry * ry;
	int32_t fx2 = 4 * rx2;
	int32_t fy2 = 4 * ry2;
	int32_t s;

	for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++)
	{
		ST7735_DrawPixel(x0 + x, y0 + y, color);
		ST7735_DrawPixel(x0 - x, y0 + y, color);
		ST7735_DrawPixel(x0 - x, y0 - y, color);
		ST7735_DrawPixel(x0 + x, y0 - y, color);
		if (s >= 0)
		{
			s += fx2 * (1 - y);
			y--;
		}
		s += ry2 * ((4 * x) + 6);
	}

	for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++)
	{
		ST7735_DrawPixel(x0 + x, y0 + y, color);
		ST7735_DrawPixel(x0 - x, y0 + y, color);
		ST7735_DrawPixel(x0 - x, y0 - y, color);
		ST7735_DrawPixel(x0 + x, y0 - y, color);
		if (s >= 0)
		{
			s += fy2 * (1 - x);
			x--;
		}
		s += rx2 * ((4 * y) + 6);
	}
}

/***************************************************************************************
** Function name:           fillEllipse
** Description:             draw a filled ellipse
***************************************************************************************/
void ST7735_FillEllipse(int16_t x0, int16_t y0, int16_t rx, int16_t ry, uint16_t color)
{
	if (rx < 2) return;
	if (ry < 2) return;
	int16_t x, y;
	int32_t rx2 = rx * rx;
	int32_t ry2 = ry * ry;
	int32_t fx2 = 4 * rx2;
	int32_t fy2 = 4 * ry2;
	int32_t s;

	for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++)
	{
		ST7735_DrawFastHLine(x0 - x, y0 - y, x + x + 1, color);
		ST7735_DrawFastHLine(x0 - x, y0 + y, x + x + 1, color);

		if (s >= 0)
		{
			s += fx2 * (1 - y);
			y--;
		}
		s += ry2 * ((4 * x) + 6);
	}

	for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++)
	{
		ST7735_DrawFastHLine(x0 - x, y0 - y, x + x + 1, color);
		ST7735_DrawFastHLine(x0 - x, y0 + y, x + x + 1, color);

		if (s >= 0)
		{
			s += fy2 * (1 - x);
			x--;
		}
		s += rx2 * ((4 * y) + 6);
	}

}

/***************************************************************************************
** Function name:           drawRect
** Description:             Draw a rectangle outline
***************************************************************************************/
// Draw a rectangle
void ST7735_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	ST7735_DrawFastHLine(x, y, w, color);
	ST7735_DrawFastHLine(x, y + h - 1, w, color);
	ST7735_DrawFastVLine(x, y, h, color);
	ST7735_DrawFastVLine(x + w - 1, y, h, color);
}

/***************************************************************************************
** Function name:           drawRoundRect
** Description:             Draw a rounded corner rectangle outline
***************************************************************************************/
// Draw a rounded rectangle
void ST7735_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
	// smarter version
	ST7735_DrawFastHLine(x + r, y, w - r - r, color);   // Top
	ST7735_DrawFastHLine(x + r, y + h - 1, w - r - r, color);   // Bottom
	ST7735_DrawFastVLine(x, y + r, h - r - r, color);   // Left
	ST7735_DrawFastVLine(x + w - 1, y + r, h - r - r, color);   // Right
	// draw four corners
	ST7735_DrawCircleHelper(x + r, y + r, r, 1, color);
	ST7735_DrawCircleHelper(x + r, y + h - r - 1, r, 8, color);
	ST7735_DrawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	ST7735_DrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
}

/***************************************************************************************
** Function name:           fillRoundRect
** Description:             Draw a rounded corner filled rectangle
***************************************************************************************/
// Fill a rounded rectangle
void ST7735_FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
	// smarter version
	ST7735_FillRectangle(x + r, y, w - r - r, h, color);

	// draw four corners
	ST7735_FillCircleHelper(x + w - r - 1, y + r, r, 1, h - r - r - 1, color);
	ST7735_FillCircleHelper(x + r, y + r, r, 2, h - r - r - 1, color);
}

/***************************************************************************************
** Function name:           drawTriangle
** Description:             Draw a triangle outline using 3 arbitrary points
***************************************************************************************/
// Draw a triangle
void ST7735_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	ST7735_DrawLine(x0, y0, x1, y1, color);
	ST7735_DrawLine(x1, y1, x2, y2, color);
	ST7735_DrawLine(x2, y2, x0, y0, color);
}

/***************************************************************************************
** Function name:           fillTriangle
** Description:             Draw a filled triangle using 3 arbitrary points
***************************************************************************************/
// Fill a triangle - original Adafruit function works well and code footprint is small
void ST7735_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
	int16_t a, b, y, last;

	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if(y0 > y1)
	{
		SWAP_INT16_T(y0, y1); SWAP_INT16_T(x0, x1);
	}

	if (y1 > y2)
	{
		SWAP_INT16_T(y2, y1); SWAP_INT16_T(x2, x1);
	}

	if (y0 > y1)
	{
		SWAP_INT16_T(y0, y1); SWAP_INT16_T(x0, x1);
	}

	if (y0 == y2)
	{
		// Handle awkward all-on-same-line case as its own thing
	 a = b = x0;
		if (x1 < a)      a = x1;
		else if (x1 > b) b = x1;
		if (x2 < a)      a = x2;
		else if (x2 > b) b = x2;
		ST7735_DrawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int16_t
	dx01 = x1 - x0,
	dy01 = y1 - y0,
	dx02 = x2 - x0,
	dy02 = y2 - y0,
	dx12 = x2 - x1,
	dy12 = y2 - y1,
	sa   = 0,
	sb   = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if(y1 == y2) last = y1;    // Include y1 scanline
	else         last = y1 - 1;   // Skip it

	for(y = y0 ; y <= last ; y++)
	{
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;

		if (a > b) SWAP_INT16_T(a, b);
		ST7735_DrawFastHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++)
	{
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;

		if (a > b) SWAP_INT16_T(a, b);
		ST7735_DrawFastHLine(a, y, b - a + 1, color);
	}
}

/***************************************************************************************
** Function name:           drawLine
** Description:             draw a line between 2 arbitrary points
***************************************************************************************/

// Slower but more compact line drawing function
void ST7735_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		SWAP_INT16_T(x0, y0);
		SWAP_INT16_T(x1, y1);
	}

	if (x0 > x1)
	{
		SWAP_INT16_T(x0, x1);
		SWAP_INT16_T(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0 <= x1; x0++)
	{
		if (steep)
		{
			ST7735_DrawPixel(y0, x0, color);
		}
		else
		{
			ST7735_DrawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

/***************************************************************************************
** Function name:           drawFastVLine
** Description:             draw a vertical line
***************************************************************************************/
void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (y >= _height)) return;
	if ((y + h - 1) >= _height) h = _height - y;

	ST7735_DrawLine(x, y, x, y + h - 1, color);
}

/***************************************************************************************
** Function name:           drawFastHLine
** Description:             draw a horizontal line
***************************************************************************************/
void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if((x >= _width) || (y >= _height)) return;
	if ((x + w - 1) >= _width)  w = _width - x;

	ST7735_DrawLine(x, y, x + w - 1, y, color);
}

/***************************************************************************************
** Function name:           setRotation
** Description:             rotate the screen orientation m = 0-3
***************************************************************************************/
void ST7735_SetRotation(uint8_t m)
{
	_value_rotation = m % 4;

	TFT_CS_L();

	ST7735_WriteCommand(ST7735_MADCTL);

	switch (_value_rotation)
	{
	case 0:
		{
			uint8_t d_r = (_data_rotation[0] | _data_rotation[1] | _data_rotation[3]);
			ST7735_WriteData(&d_r, sizeof(d_r));
			_width  = ST7735_WIDTH;
			_height = ST7735_HEIGHT;
			_xstart = ST7735_XSTART;
			_ystart = ST7735_YSTART;
		}
		break;
	case 1:
		{
			uint8_t d_r = (_data_rotation[1] | _data_rotation[2] | _data_rotation[3]);
			ST7735_WriteData(&d_r, sizeof(d_r));
			_width  = ST7735_HEIGHT;
			_height = ST7735_WIDTH;
			_xstart = ST7735_YSTART;
			_ystart = ST7735_XSTART;
		}
		break;
	case 2:
		{
			uint8_t d_r = _data_rotation[3];
			ST7735_WriteData(&d_r, sizeof(d_r));
			_width  = ST7735_WIDTH;
			_height = ST7735_HEIGHT;
			_xstart = ST7735_XSTART;
			_ystart = ST7735_YSTART;
		}
		break;
	case 3:
		{
			uint8_t d_r = (_data_rotation[0] | _data_rotation[2] | _data_rotation[3]);
			ST7735_WriteData(&d_r, sizeof(d_r));
			_width  = ST7735_HEIGHT;
			_height = ST7735_WIDTH;
			_xstart = ST7735_YSTART;
			_ystart = ST7735_XSTART;
		}
		break;
	}
	TFT_CS_H();
}

uint8_t ST7735_GetRotation(void)
{
	return _value_rotation;
}

int16_t ST7735_GetHeight(void)
{
	return _height;
}

int16_t ST7735_GetWidth(void)
{
	return _width;
}

uint16_t ST7735_Color565(uint8_t r, uint8_t g, uint8_t b) {
	return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3);
}
/*       Scrolling support       
 *       
 **/
void setScrollDefinition(uint8_t top_fix_height, uint8_t bottom_fix_height, uint8_t _scroll_direction) {
	uint8_t scroll_height;
	scroll_height = _height - top_fix_height - bottom_fix_height;
	ST7735_WriteCommand(ST7735_VSCRDEF);
	/*ST7735_WriteData(0x00);
	ST7735_WriteData(top_fix_height);
	ST7735_WriteData(0x00);
	ST7735_WriteData(scroll_height);
	ST7735_WriteData(0x00);
	ST7735_WriteData(bottom_fix_height);*/
	uint8_t data[] = { 0x00, top_fix_height, 0x00, scroll_height, 0x00, bottom_fix_height };
	ST7735_WriteData(data, sizeof(data));
	ST7735_WriteCommand(ST7735_MADCTL);
	uint8_t _tft_type = 0;
	uint8_t temp = 0x00;
	if (_scroll_direction) {
		if (_tft_type == 0) {
			temp = 0xD8;
			ST7735_WriteData(&temp,1);
		}
		if (_tft_type == 1) {
			temp = 0xD0;
			ST7735_WriteData(&temp, 1);
		}
		if (_tft_type == 2) {
			temp = 0x18;
			ST7735_WriteData(&temp, 1);
		}
	}
	else {
		if (_tft_type == 0) {
			temp = 0xC8;
			ST7735_WriteData(&temp, 1);
		}
		if (_tft_type == 1) {
			temp = 0xC0;
			ST7735_WriteData(&temp, 1);
		}
		if (_tft_type == 2) {
			temp = 0x08;
			ST7735_WriteData(&temp, 1);
		}
	}
}
void VerticalScroll(uint8_t _vsp) {
	ST7735_WriteCommand(ST7735_VSCRSADD);
	uint8_t temp = 0x00;
	ST7735_WriteData(&temp , 1);
	ST7735_WriteData(&_vsp,1);
}