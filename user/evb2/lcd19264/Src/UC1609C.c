/*

  ******************************************************************************
  * @file 		( file ):   UC1609C.c
  * @brief 		( description ):  	
  ******************************************************************************
  * @attention 	( Attention ):	author: 
  ******************************************************************************
  
*/

/* Includes ----------------------------------------------------------*/
#include "UC1609C.h"

//---------------------------------------------------------------

uint8_t buffer[ ( UC1609C_WIDTH * UC1609C_HEIGHT / 8 ) + 1 ];  						// frame buffer, create a full screen buffer (192 * 64/8) + 1
uint8_t bufferWidth = UC1609C_WIDTH;
uint8_t bufferHeight = UC1609C_HEIGHT;
	

//===================================================================================================================

void init_spi(uint8_t clk_divider){
	PIN_INFO PIN_INFO;
	PIN_INFO.pin_func = GIO_FUNC1;
	PIN_INFO.pin_stat = GIO_PU;
	PIN_INFO.pin_ds		= GIO_DS_2_4;
	PIN_INFO.pin_od   = OD_OFF;
	PIN_INFO.pin_sonof = SONOF_ON;
	APB_GPIO->GPIO_OE.SET = CS_Pin << GPIO_PortB;		// set as output pin
	APB_GPIO->GPIO_DO.SET = CS_Pin << GPIO_PortB;		// output 1 to pin	
	PinCtrl_GIOSet(PIN_CTL_GPIOB, GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 , &PIN_INFO);  
	ssp_config(clk_divider);	
	
}

void gpio_init(void){
	PIN_INFO pin_info;
	pin_info.pin_func = GIO_FUNC0;
	pin_info.pin_stat = GIO_PU;				// if not this line will become open drain
	pin_info.pin_ds = GIO_DS_2_4;			// output driver select
	pin_info.pin_od = OD_OFF;				// open drain select
	PinCtrl_GIOSet(PIN_CTL_GPIOB, GPIO_Pin_4 | GPIO_Pin_5 , &pin_info);
	APB_GPIO->GPIO_OE.SET = DC_Pin  << GPIO_PortB;
	APB_GPIO->GPIO_OE.SET = RST_Pin << GPIO_PortB;
	APB_GPIO->GPIO_DO.SET = DC_Pin  << GPIO_PortB;
	APB_GPIO->GPIO_DO.SET = RST_Pin << GPIO_PortB;
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
	
}

void HAL_GPIO_WritePin(uint8_t PORT, uint32_t Pin, uint8_t state){
	if(state == GPIO_PIN_RESET){
		APB_GPIO->GPIO_DO.CLR = Pin << PORT;
	}
	else if (state == GPIO_PIN_SET) {
		APB_GPIO->GPIO_DO.SET = Pin << PORT;
	}
}

void HAL_SPI_Transmit(uint8_t data){
	APB_SPI->DataRegister = data;
  apSSP_DeviceEnable(APB_SPI);
  while(apSSP_DeviceBusyGet(APB_SPI));
  apSSP_DeviceDisable(APB_SPI);
  apSSP_ReadFIFO(APB_SPI);
}

//===================================================================================================================
/*
	******************************************************************************
	* @brief	( description ):  sending commands and data to the display
	* @param	( options ):	1- command, 2 - data
	* @return  	( returns ):	
	******************************************************************************
*/
static void UC1609C_sendCommand( uint8_t command, uint8_t value )
{	
	uint8_t data = command | value;
	
	HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_RESET );
	
	HAL_SPI_Transmit( data );
	
	HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  reset display (needed during initialization)
	* @param	( options ):	
	* @return  	( returns ):	
	******************************************************************************
*/
static void UC1609C_reset( void ){
	HAL_GPIO_WritePin( RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET );
	HAL_GPIO_WritePin( RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET );
	HAL_Delay( 200 );															// mS delay . datasheet says 3uS
	HAL_GPIO_WritePin( RST_GPIO_Port, RST_Pin, GPIO_PIN_SET );
	HAL_Delay( 150 );															// mS delay . DataSheet says 5mS
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  display initialization
	* @param	( options ):	
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_init( void )
{	
	init_spi(12);				// init mcu008 hardware spi
	gpio_init();				// init gpio for UC1609C LCD control
	
	HAL_Delay( 50 );															//3mS delay, datasheet
	HAL_GPIO_WritePin( DC_GPIO_Port, DC_Pin, GPIO_PIN_SET );
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
	
	UC1609C_reset();
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	HAL_Delay( 10 );

	UC1609C_sendCommand( UC1609C_TEMP_COMP_REG, UC1609C_TEMP_COMP_SET ); 		// Temperature Compensation Register, TC[1:0] = 00b= -0.00%/ C
	
	//--------------------------------------------------------------------------------------------------------------------------
	//UC1609C_sendCommand( UC1609C_ADDRESS_CONTROL, UC1609_ADDRESS_SET ); 		// set RAM address control for UC1609
	UC1609C_sendCommand( UC1609C_ADDRESS_CONTROL, UC1609C_ADDRESS_SET ); 		// set RAM address control for UC1609C
	//--------------------------------------------------------------------------------------------------------------------------
	
	UC1609C_sendCommand( UC1609C_FRAMERATE_REG, UC1609C_FRAMERATE_SET );
	UC1609C_sendCommand( UC1609C_BIAS_RATIO, UC1609C_BIAS_RATIO_SET );  		// set bias ratio to default
	UC1609C_sendCommand( UC1609C_POWER_CONTROL,  UC1609C_PC_SET ); 
  
	HAL_Delay( 100 );															//  mS delay
  
	UC1609C_sendCommand( UC1609C_GN_PM, 0 );									// set gain and potentiometer - double byte command
	
	//--------------------------------------------------------------------------------------------------------------------------
	UC1609C_sendCommand( UC1609C_GN_PM, 0x1E ); 								// Default contrast, default = 0x49 , range 0x00 to 0xFE set gain and potentiometer
	//--------------------------------------------------------------------------------------------------------------------------
	
	UC1609C_sendCommand( UC1609C_DISPLAY_ON, 0x01 ); 							// turn on display
	
	//--------------------------------------------------------------------------------------------------------------------------
	UC1609C_sendCommand( UC1609C_LCD_CONTROL, UC1609C_ROTATION_NORMAL ); 		// Default rotation, rotate to normal 
	//--------------------------------------------------------------------------------------------------------------------------
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  setting display contrast
	* @param	( options ):	value from 0 .... 255 (default 30)
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_contrast (uint8_t bits) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	UC1609C_sendCommand( UC1609C_GN_PM, 0 );
	UC1609C_sendCommand( UC1609C_GN_PM, bits ); 								// Default contrast, default = 0x49 , range 0x00 to 0xFE set gain and potentiometer
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  enabling and disabling information on the display (with saving information)
	* @param	( options ):	1 - ON    0 - OFF
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_enable (uint8_t bits) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	UC1609C_sendCommand( UC1609C_DISPLAY_ON, bits );
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  	scrolling the display vertically
	* @param	( options ):		value from 0 to 64
									how many lines do we shift
										from 0 to 64 move up
										from 64 to 0 move down
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_scroll (uint8_t bits) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	UC1609C_sendCommand( UC1609C_SCROLL, bits );
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	 ( description ):  display rotation
	* @param	( options ):	Param1: 4 possible values 000 010 100 110 (defined)
								specify the rotation parameter:
									UC1609C_ROTATION_FLIP_TWO
									UC1609C_ROTATION_NORMAL
									UC1609C_ROTATION_FLIP_ONE
									UC1609C_ROTATION_FLIP_THREE
	* @return  ( returns ):	
	******************************************************************************
*/
void UC1609C_rotate(uint8_t rotatevalue) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	switch ( rotatevalue )
	{
      case 0: 
	  {
		  rotatevalue = 0; 
		  break;
	  }
      case 0x02: 
	  {
		  rotatevalue = UC1609C_ROTATION_FLIP_ONE; 	
		  break;
	  }
      case 0x04:
	  {
		  rotatevalue = UC1609C_ROTATION_NORMAL; 
		  break;
	  }
      case 0x06: 
	  {
		  rotatevalue = UC1609C_ROTATION_FLIP_TWO; 	
		  break;
	  }
      default: 
	  {
		  rotatevalue = UC1609C_ROTATION_NORMAL; 	
		  break;
	  }
	}
	UC1609C_sendCommand( UC1609C_LCD_CONTROL, rotatevalue );
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  	display inversion
	* @param	( options ):		Param1: bits, 1 invert , 0 normal
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_invertDisplay (uint8_t bits) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	UC1609C_sendCommand( UC1609C_INVERSE_DISPLAY, bits );
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):	output to the display of all points at once (paint over the entire display)
	* @param	( options ): 		parameter: 1 - all points are on, 0 - all points are off
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_allPixelsOn(uint8_t bits) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	UC1609C_sendCommand( UC1609C_ALL_PIXEL_ON, bits );

	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  fill the entire display with bits (only do not touch the display buffer)
								( for example, if 0x00 then everything is empty, 0xFF then everything is painted over, 0x55 (0v01010101) then everything is through a line)
	* @param	( options ):	1- command, 2 - data
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_fillScreen(uint8_t dataPattern) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );
	
	uint16_t numofbytes = UC1609C_WIDTH * (	UC1609C_HEIGHT / 8 ); 							// width * height
	
	for ( uint16_t i = 0; i < numofbytes; i++ ) 
	{
		HAL_SPI_Transmit( dataPattern );
	}
	
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  buffer output to display
	* @param	( options ):	options:
								//Param1: x offset 0-192
								//Param2: y offset 0-64
								//Param3: width 0-192
								//Param4: height 0-64
								//Param5: the buffer itself
	* @return  	( returns ):	
	******************************************************************************
*/
static void UC1609C_buffer( int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t* data ) 
{
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET );

	uint8_t tx, ty; 
	uint16_t offset = 0; 
	uint8_t column = (x < 0) ? 0 : x;
	uint8_t page = (y < 0) ? 0 : y/8;

	  for ( ty = 0; ty < h; ty = ty + 8 ) 
	  {
		if ( y + ty < 0 || y + ty >= UC1609C_HEIGHT )
		{
			continue;
		}
		
		UC1609C_sendCommand( UC1609C_SET_COLADD_LSB, ( column & 0x0F )); 
		UC1609C_sendCommand( UC1609C_SET_COLADD_MSB, ( column & 0XF0 ) >> 4 ); 
		UC1609C_sendCommand( UC1609C_SET_PAGEADD, page++ ); 
	 
		for (tx = 0; tx < w; tx++) 
		{
			  if (x + tx < 0 || x + tx >= UC1609C_WIDTH)
			  {
				continue;
			  }
			  
			  offset = ( w * ( ty / 8 )) + tx; 
			  HAL_SPI_Transmit( data[offset++] );
		}
	  }
  
	HAL_GPIO_WritePin( CS_GPIO_Port, CS_Pin, GPIO_PIN_SET );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  output of information from the buffer to the display
								( called every time you need to display data)
								( for example, we brought out the text and then call the UC1609C_update (void) function)
	* @param	( options ):	
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_update( void ) 
{
	uint8_t x = 0; 
	uint8_t y = 0; 
	uint8_t w = bufferWidth; 
	uint8_t h = bufferHeight;
	
	UC1609C_buffer( x,  y,  w,  h, (uint8_t*)buffer );
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draws a pixel at the specified coordinates (do not forget to call UC1609C_update ();)
	* @param	( options ):	Param1: x offset 0-192
								Param2: y offset 0-64
								Param3: pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/

void UC1609C_drawPixel( int16_t x, int16_t y, uint8_t colour ) 
{
	if (( x < 0 ) || ( x >= bufferWidth ) || ( y < 0 ) || ( y >= bufferHeight )) {
		return;
	}
      uint16_t tc = ( bufferWidth * ( y / 8 )) + x; 
	
      switch ( colour )
      {
        case FOREGROUND:
		{
			buffer[tc] |= ( 1 << ( y & 7 )); 
			break;
		}
        case BACKGROUND:  
		{
			buffer[tc] &= ~( 1 << ( y & 7 ));
			break;
		}
        case INVERSE:
		{
			buffer[tc] ^= ( 1 << ( y & 7 ));
			break;
		}
      }
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  clear the framebuffer (but does not clear the display itself)
	* @param	( options ):
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_clearBuffer( void )
{
	memset( buffer, 0x00, ( bufferWidth * ( bufferHeight / 8 ))  ); 
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  displaying a picture (from an array) to the screen (do not forget to call UC1609C_update ();)
	* @param	( options ):	options:
									//Param1: x offset 0-192
									//Param2: y offset 0-64
									//Param3: width 0-192
									//Param4: height 0-64
									//Param5: array with picture
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_bitmap(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t* data, uint8_t colour) 
{
	int16_t byteWidth = (w + 7) / 8; 									// Bitmap scanline pad = whole byte
	uint8_t byte = 0;

	for(int16_t j=0; j<h; j++, y++)
	{
		for(int16_t i=0; i<w; i++)
		{
			if(i & 7)
			{
			   byte <<= 1;
			}
			else
			{
			   byte = (*(const unsigned char *)(&data[j * byteWidth + i / 8]));
			}
			if(byte & 0x80) UC1609C_drawPixel(x+i, y, colour);
		}
	}
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  a list of the display itself (the frame buffer is not cleared)
	* @param	( options ):	
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_clearDisplay( void )
{

	UC1609C_fillScreen(0x00);
}			
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  drawing one character
	* @param	( options ):	1- coordinate x
								2- coordinate y
								3- the symbol itself
								4- specify the font of the symbol
								5- symbol size multiplier
								6- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
static void UC1609C_DrawChar(int16_t x, int16_t y, unsigned char ch, FontDef_t* Font, uint8_t multiplier, uint8_t color)
{
	
	uint16_t i, j;
	
	uint16_t b;
	
	int16_t X = x, Y = y;
	
	int16_t xx, yy;
	
	if( multiplier < 1 ){
		multiplier = 1;
	}
	
	/* Check available space in LCD */
	if ( UC1609C_WIDTH >= ( x + Font->FontWidth) || UC1609C_HEIGHT >= ( y + Font->FontHeight)){
	
		/* Go through font */
		for (i = 0; i < Font->FontHeight; i++) {
			
			if( ch < 127 ){			
				b = Font->data[(ch - 32) * Font->FontHeight + i];
			}
			
			else if( (uint8_t) ch > 191 ){
				// +96 this is because Latin characters and characters in fonts occupy 96 positions
				// and if in a font that first contains the Latin alphabet and special characters and then 
				// only Cyrillic then you need to add 95 if the font
				// contains only Cyrillic then +96 is not needed
				b = Font->data[((ch - 192) + 96) * Font->FontHeight + i];
			}
			
			else if( (uint8_t) ch == 168 ){	// 168 ???????????? ???? ASCII - ??
				// 160 element (symbol Y)
				b = Font->data[( 160 ) * Font->FontHeight + i];
			}
			
			else if( (uint8_t) ch == 184 ){	// 184 ???????????? ???? ASCII - ??
				// 161 elements (symbol ??)
				b = Font->data[( 161 ) * Font->FontHeight + i];
			}
			//-------------------------------------------------------------------------------
			
			
			for (j = 0; j < Font->FontWidth; j++) {
				
				if ((b << j) & 0x8000) {
					
					for (yy = 0; yy < multiplier; yy++){
						for (xx = 0; xx < multiplier; xx++){
								UC1609C_drawPixel(X+xx, Y+yy, color);
						}
					}
					
				}
				// if the background is cleared, then we leave if so that the background remains old, then we comment on this part --------------------------------------------
				//-----------------------------------------------------------------------------------------------------------------------------------
				else{
					
					for (yy = 0; yy < multiplier; yy++){
						for (xx = 0; xx < multiplier; xx++){
								UC1609C_drawPixel(X+xx, Y+yy, !color);
						}
					}
				}
				//-----------------------------------------------------------------------------------------------------------------------------------
				//-----------------------------------------------------------------------------------------------------------------------------------
				
				X = X + multiplier;
			}
			
			X = x;
			Y = Y + multiplier;
		}
		
	}
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  displaying a string (Latin and Cyrillic) (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x
								2- coordinate y
								3- the string itself
								4- specify the font of the symbol
								5- symbol size multiplier
								6- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_Print(int16_t x, int16_t y, char* str, FontDef_t* Font, uint8_t multiplier, uint8_t color) {
	
	if( multiplier < 1 ){
		multiplier = 1;
	}
	
	unsigned char buff_char;
	
	uint16_t len = strlen(str);
	
	while (len--) {
		
		//---------------------------------------------------------------------
		// check for Cyrillic UTF-8, if Latin then skip if
		// Extended ASCII characters Win-1251 Cyrillic (character code 128-255)
		// check the first byte of two (since UTF-8 is two bytes)
		// if it is greater than or equal to 0xC0 (the first byte in Cyrillic will be equal to 0xD0 or 0xD1 exactly in the alphabet) 

		if ( (uint8_t)*str >= 0xC0 ){	// code 0xC0 corresponds to the Cyrillic character 'A' according to ASCII Win-1251
			
			// check which byte is the first 0xD0 or 0xD1
			switch ((uint8_t)*str) {
				case 0xD0: {
					// increase the array as we need the second byte
					str++;
					// check the second byte there the character itself
					if ((uint8_t)*str == 0x81) { buff_char = 0xA8; break; }		// byte of the ?? character (if more characters are needed, add here and in the DrawChar () function)
					if ((uint8_t)*str >= 0x90 && (uint8_t)*str <= 0xBF){ buff_char = (*str) + 0x30; }	// bytes of characters A ... I a ... n do a zdvig on +48
					break;
				}
				case 0xD1: {
					// increase the array as we need the second byte
					str++;
					// check the second byte there the character itself
					if ((uint8_t)*str == 0x91) { buff_char = 0xB8; break; }		// byte of character ?? (if more characters are needed, add here and in the DrawChar () function)
					if ((uint8_t)*str >= 0x80 && (uint8_t)*str <= 0x8F){ buff_char = (*str) + 0x70; }	// bytes of characters n ... I want the shift to +112
					break;
				}
			}
			// we also reduce the variable since we used up 2 bytes for the Cyrillic alphabet
			len--;
			
			UC1609C_DrawChar( x, y, buff_char, Font, multiplier, color);
		}
		//---------------------------------------------------------------------
		else{			
			UC1609C_DrawChar( x, y, *str, Font, multiplier, color);
		}
		
		x = x + (Font->FontWidth * multiplier);
		/* Increase string pointer */
		str++;
	}
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draw a line (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- coordinate x2
								4- coordinate y2
								5- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c) {
	
	int16_t dx, dy, sx, sy, err, e2, i, tmp; 
	
	/* Check for overflow */
	if (x0 >= UC1609C_WIDTH) {
		x0 = UC1609C_WIDTH - 1;
	}
	if (x1 >= UC1609C_WIDTH) {
		x1 = UC1609C_WIDTH - 1;
	}
	if (y0 >= UC1609C_HEIGHT) {
		y0 = UC1609C_HEIGHT - 1;
	}
	if (y1 >= UC1609C_HEIGHT) {
		y1 = UC1609C_HEIGHT - 1;
	}
	
	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1); 
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1); 
	sx = (x0 < x1) ? 1 : -1; 
	sy = (y0 < y1) ? 1 : -1; 
	err = ((dx > dy) ? dx : -dy) / 2; 

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			UC1609C_drawPixel(x0, i, c);
		}
		
		/* Return from function */
		return;
	}
	
	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			UC1609C_drawPixel(i, y0, c);
		}
		
		/* Return from function */
		return;
	}
	
	while (1) {
		UC1609C_drawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err; 
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		} 
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		} 
	}
}
//----------------------------------------------------------------------------------



/*
	******************************************************************************
	* @brief	( description ):  draw a hollow rectangle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- width
								4- height
								5- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
	/* Check input parameters */
	if (
		x >= UC1609C_WIDTH ||
		y >= UC1609C_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= UC1609C_WIDTH) {
		w = UC1609C_WIDTH - x;
	}
	if ((y + h) >= UC1609C_HEIGHT) {
		h = UC1609C_HEIGHT - y;
	}
	
	/* Draw 4 lines */
	UC1609C_DrawLine(x, y, x + w, y, c);         /* Top line */
	UC1609C_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	UC1609C_DrawLine(x, y, x, y + h, c);         /* Left line */
	UC1609C_DrawLine(x + w, y, x + w, y + h, c); /* Right line */
}
//----------------------------------------------------------------------------------
/*
	******************************************************************************
	* @brief	( description ):  draw a filled rectangle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- width
								4- height
								5- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawFilledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
	
	uint8_t i;
	
	/* Check input parameters */
	if (
		x >= UC1609C_WIDTH ||
		y >= UC1609C_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= UC1609C_WIDTH) {
		w = UC1609C_WIDTH - x;
	}
	if ((y + h) >= UC1609C_HEIGHT) {
		h = UC1609C_HEIGHT - y;
	}
	
	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		UC1609C_DrawLine(x, y + i, x + w, y + i, c);
	}
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draw a hollow triangle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- coordinate x2
								4- coordinate y2
								5- coordinate x3
								6- coordinate y3
								7- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t color) {
	/* Draw lines */
	UC1609C_DrawLine(x1, y1, x2, y2, color);
	UC1609C_DrawLine(x2, y2, x3, y3, color);
	UC1609C_DrawLine(x3, y3, x1, y1, color);
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draw a filled triangle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- coordinate x2
								4- coordinate y2
								5- coordinate x3
								6- coordinate y3
								7- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawFilledTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t color) {
	
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, 
	yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, 
	curpixel = 0;
	
	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		UC1609C_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draw a hollow circle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- radius
								4- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    UC1609C_drawPixel(x0, y0 + r, c);
    UC1609C_drawPixel(x0, y0 - r, c);
    UC1609C_drawPixel(x0 + r, y0, c);
    UC1609C_drawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        UC1609C_drawPixel(x0 + x, y0 + y, c);
        UC1609C_drawPixel(x0 - x, y0 + y, c);
        UC1609C_drawPixel(x0 + x, y0 - y, c);
        UC1609C_drawPixel(x0 - x, y0 - y, c);

        UC1609C_drawPixel(x0 + y, y0 + x, c);
        UC1609C_drawPixel(x0 - y, y0 + x, c);
        UC1609C_drawPixel(x0 + y, y0 - x, c);
        UC1609C_drawPixel(x0 - y, y0 - x, c);
    }
}
//----------------------------------------------------------------------------------


/*
	******************************************************************************
	* @brief	( description ):  draw a filled circle (do not forget to call UC1609C_update ();)
	* @param	( options ):	1- coordinate x1
								2- coordinate y1
								3- radius
								4- pixel view ( FOREGROUND or BACKGROUND  or INVERSE )
	* @return  	( returns ):	
	******************************************************************************
*/
void UC1609C_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint8_t c) {
	
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    UC1609C_drawPixel(x0, y0 + r, c);
    UC1609C_drawPixel(x0, y0 - r, c);
    UC1609C_drawPixel(x0 + r, y0, c);
    UC1609C_drawPixel(x0 - r, y0, c);
    UC1609C_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        UC1609C_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        UC1609C_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        UC1609C_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        UC1609C_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }
}
//----------------------------------------------------------------------------------



/************************ (C) COPYRIGHT GKP *****END OF FILE****/
