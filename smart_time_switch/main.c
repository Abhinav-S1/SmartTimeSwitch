//////////////////// SMART TIME SWITCH ///////////////////////
#include <lpc214x.h>
#include <stdio.h>
#define PLOCK 0x00000400
#define LED_OFF (IO0SET = 1U << 31)
#define LED_ON (IO0CLR = 1U << 31)
#define RS_ON (IO0SET = 1U << 20)
#define RS_OFF (IO0CLR = 1U << 20)
#define EN_ON (IO1SET = 1U << 25)
#define EN_OFF (IO1CLR = 1U << 25)
#define SW2 (IO0PIN & (1 << 14))
#define SW3 (IO0PIN & (1 << 15))
#define SW4 (IO1PIN & (1 << 18))
#define SW5 (IO1PIN & (1 << 19))
#define SW6 (IO1PIN & (1 << 20))
void SystemInit(void);
static void delay_ms(unsigned int j);	  // millisecond delay
static void delay_us(unsigned int count); // microsecond delay
static void LCD_SendCmdSignals(void);
static void LCD_SendDataSignals(void);
static void LCD_SendHigherNibble(unsigned char dataByte);
static void LCD_CmdWrite(unsigned char cmdByte);
static void LCD_DataWrite(unsigned char dataByte);
static void LCD_Reset(void);
static void LCD_Init(void);
void LCD_DisplayString(const char *ptr_stringPointer_u8);

void LCD_DisplayNumber(unsigned int n);
void LCD_print_current_time(unsigned char);

void LCD_Display_2_digit_Number(unsigned int n);
void LCD_shiftCursor_right(unsigned char n);
void RTC_Init(void);
void RTC_SetDateTime(void);

//////////////// GLOBAL VARIABLES //////////////////
struct timeSlot
{
	unsigned char hour, min, flag;
	unsigned char dur_hour, dur_min;
};
struct timeSlot slot[7][10]; // slot[day][slot number]
unsigned char relay1 = 0;
char *days_code[7] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};

/////////////////////  MAIN  /////////////////////

int main()
{
	// SystemInit();
	RTC_Init();
	// RTC_SetDateTime();
	IO0DIR |= 1U << 31 | 0x00FF0000; // to set P0.16 to P0.23 as o/ps
	IO1DIR |= 1U << 25;				 // to set P1.25 as o/p used for EN

	LED_ON;
	delay_ms(500);
	LED_OFF;
	delay_ms(500);

	LCD_Reset();
	delay_ms(50);
	LCD_Init();
	delay_ms(50);

	LCD_CmdWrite(0x80);
	LCD_DisplayString("SMART TIME SWITCH");
	LCD_CmdWrite(0xc0);
	LCD_DisplayString(" *-------*-------* ");
	LCD_CmdWrite(0x94);
	delay_ms(1000);

	LCD_CmdWrite(0x01u); // Clear the LCD
	delay_ms(100);
	unsigned char button2 = 0, pbutton2 = 10, button3 = 0, button4 = 0, button5 = 0, button6 = 0;
	unsigned char day = 0, slot_no = 0, hour = 0, min = 0, dur_hour = 0, dur_min = 0;
	unsigned char new_slot_flag = 0;
	while (1)
	{
		///////////////// INPUT LOGIC ////////////////////

		if (!SW2)
		{
			button2++;
			delay_ms(200);
			button2 %= 3;
		}
		switch (button2)
		{
		case 0: // CURRENT TIME
			if (pbutton2 != button2)
			{
				pbutton2 = button2;
				LCD_CmdWrite(0x01u); // Clear the LCD
				delay_ms(100);
				LCD_print_current_time(1);
			}
			else
				LCD_print_current_time(0);
			break;

		case 1: // SET NEW SLOT
			if (pbutton2 != button2)
			{
				pbutton2 = button2;
				LCD_CmdWrite(0x01u); // Clear the LCD
				delay_ms(100);
			}
			LCD_CmdWrite(0x80); // go to first line
			LCD_DisplayString("Set new slot");
			LCD_CmdWrite(0xc0); // go to second line
			LCD_DisplayString("Day:");
			if (!SW3) // DAY BUTTON
			{
				button3++;
				delay_ms(200);
				button3 %= 7;
				day = button3;
			}
			LCD_DisplayString(days_code[day]);
			new_slot_flag = 0;
			if (!SW5) // CONFIRM
			{
				new_slot_flag = 1;
				delay_ms(200);
				LCD_CmdWrite(0x01u); // Clear the LCD
				delay_ms(100);
				button4 = slot_no = 0;
				button5 = hour = slot[day][slot_no].hour;
				button6 = min = slot[day][slot_no].min;
			}
			while (new_slot_flag) // READING START TIME
			{
				LCD_CmdWrite(0x80); // go to first line
				LCD_DisplayString("D:");
				LCD_DisplayString(days_code[day]); // CANNOT CHANGE DAY HERE
				LCD_DisplayString(" Start time");

				LCD_CmdWrite(0xc0); // go to second line
				if (!SW4)			// SLOT NUMBER
				{
					button4++;
					delay_ms(200);
					button4 %= 10;
					slot_no = button4;
					button5 = hour = slot[day][slot_no].hour;
					button6 = min = slot[day][slot_no].min;
				}
				LCD_DisplayString("Slot:");
				LCD_Display_2_digit_Number(slot_no);
				if (!SW5) // HOUR BUTTON
				{
					button5++;
					delay_ms(200);
					button5 %= 24;
					hour = button5;
				}
				LCD_DisplayString(" H:");
				LCD_Display_2_digit_Number(hour);
				if (!SW6) // MIN BUTTON
				{
					button6++;
					delay_ms(200);
					while (!SW6)
					{
						button6++;
						delay_ms(75);
						button6 %= 60;
						min = button6;
						LCD_CmdWrite(0xc0); // go to second line
						LCD_shiftCursor_right(15);
						LCD_Display_2_digit_Number(min);
					}
					button6 %= 60;
					min = button6;
				}
				LCD_CmdWrite(0xc0); // go to second line
				LCD_shiftCursor_right(12);
				LCD_DisplayString(" m:");
				LCD_Display_2_digit_Number(min);
				if (!SW3) // READING DURATION
				{
					LCD_CmdWrite(0x01u); // Clear the LCD
					delay_ms(100);
					LCD_CmdWrite(0x80); // go to first line
					LCD_DisplayString("D:"); 
					LCD_DisplayString(days_code[day]);
					LCD_DisplayString(" Duration");
					LCD_CmdWrite(0xc0); // go to second line
					LCD_DisplayString("Slot:");
					LCD_Display_2_digit_Number(slot_no); // CANNOT CHANGE SLOT NO. HERE
					new_slot_flag = 0;
					pbutton2 = 10;
					button5 = dur_hour = slot[day][slot_no].dur_hour;
					button6 = dur_min = slot[day][slot_no].dur_min;
					while (1) // READING DURATION
					{
						LCD_CmdWrite(0xc0); // go to second line
						LCD_DisplayString("Slot:");
						LCD_Display_2_digit_Number(slot_no); // CANNOT CHANGE SLOT NO. HERE
						if (!SW5)							 // HOUR BUTTON
						{
							button5++;
							delay_ms(200);
							button5 %= 24;
							dur_hour = button5; // UPDATE DURATION HOURS
						}
						LCD_DisplayString(" H:");
						LCD_Display_2_digit_Number(dur_hour);
						if (!SW6) // MIN BUTTON
						{
							button6++;
							delay_ms(200);
							while (!SW6)
							{
								button6++;
								delay_ms(75);
								button6 %= 60;
								dur_min = button6;
								LCD_CmdWrite(0xc0); // go to second line
								LCD_shiftCursor_right(15);
								LCD_Display_2_digit_Number(dur_min);
							}
							button6 %= 60;
							dur_min = button6;
						}
						LCD_CmdWrite(0xc0); // go to second line
						LCD_shiftCursor_right(12);
						LCD_DisplayString(" m:");
						LCD_Display_2_digit_Number(dur_min);
						if (!SW2) // EXIT DURATION MENU
						{
							delay_ms(300);
							break;
						}
					}
					button2 = 2;
					break;
				}
			}
			break;

		case 2: // SAVE SLOT
			if (pbutton2 != button2)
			{
				pbutton2 = button2;
				LCD_CmdWrite(0x01u); // Clear the LCD
				delay_ms(100);
			}
			LCD_CmdWrite(0x80); // go to first line
			LCD_DisplayString("Save Timer ?");
			if (!SW5) // CONFIRM
			{
				slot[day][slot_no].hour = hour;
				slot[day][slot_no].min = min;
				slot[day][slot_no].dur_hour = dur_hour;
				slot[day][slot_no].dur_min = dur_min;
				slot[day][slot_no].flag = 1; 
				LCD_CmdWrite(0x01u);		 // Clear the LCD
				delay_ms(100);
				LCD_CmdWrite(0x80); // go to first line
				LCD_DisplayString("Timer Saved!");
				delay_ms(2000);
				button2 = 0; // GO BACK TO SHOWING CURRENT TIME
			}
			if (!SW6) // CANCEL
			{
				LCD_CmdWrite(0x01u); // Clear the LCD
				delay_ms(100);
				LCD_CmdWrite(0x80); // go to first line
				LCD_DisplayString("Timer Canceled!");
				delay_ms(2000);
				button2 = 0; // GO BACK TO SHOWING CURRENT TIME
			}
			break;
		}

		///////////////// TIMER LOGIC ////////////////////

		relay1 = 0;
		
			for (unsigned char j = 0; j < 10; j++) // ITERATING THROUGH ALL SLOTS FOR THAT SPECIF DAY
			{
				if (slot[DOW][j].flag)
				{
					if (((slot[DOW][j].hour == HOUR && slot[DOW][j].min <= MIN) || slot[DOW][j].hour < HOUR) &&
						((slot[DOW][j].hour + slot[DOW][j].dur_hour == HOUR && slot[DOW][j].min + slot[DOW][j].dur_min >= MIN) || slot[DOW][j].hour + slot[DOW][j].dur_hour > HOUR))
						relay1 = 1;
				}
			}
		

		if (relay1)
		{
			LED_ON;
			delay_ms(25);
			//relay can be used here
		}
		else
		{
			LED_OFF;
			delay_ms(25);
			//relay can be used here
		}
	}
}

/////////////////// USER FUNCTIONS /////////////////////

void SystemInit(void)
{
	PLL0CON = 0x01;
	PLL0CFG = 0x24;
	PLL0FEED = 0xAA;
	PLL0FEED = 0x55;
	while (!(PLL0STAT & PLOCK))
		;
	PLL0CON = 0x03;
	PLL0FEED = 0xAA; // lock the PLL registers after setting the required PLL
	PLL0FEED = 0x55;
	VPBDIV = 0x01; // PCLK is same as CCLK i.e 60Mhz
}

static void LCD_CmdWrite(unsigned char cmdByte)
{
	LCD_SendHigherNibble(cmdByte);
	LCD_SendCmdSignals();
	cmdByte = cmdByte << 4;
	LCD_SendHigherNibble(cmdByte);
	LCD_SendCmdSignals();
}

static void LCD_DataWrite(unsigned char dataByte)
{
	LCD_SendHigherNibble(dataByte);
	LCD_SendDataSignals();
	dataByte = dataByte << 4;
	LCD_SendHigherNibble(dataByte);
	LCD_SendDataSignals();
}

static void LCD_Reset(void)
{
	/* LCD reset sequence for 4-bit mode*/
	LCD_SendHigherNibble(0x30);
	LCD_SendCmdSignals();
	delay_ms(100);
	LCD_SendHigherNibble(0x30);
	LCD_SendCmdSignals();
	delay_us(200);
	LCD_SendHigherNibble(0x30);
	LCD_SendCmdSignals();
	delay_us(200);
	LCD_SendHigherNibble(0x20);
	LCD_SendCmdSignals();
	delay_us(200);
}

static void LCD_SendHigherNibble(unsigned char dataByte)
{
	// send the D7,6,5,D4(uppernibble) to P0.16 to P0.19
	IO0CLR = 0X000F0000;
	IO0SET = ((dataByte >> 4) & 0x0f) << 16;
}

static void LCD_SendCmdSignals(void)
{
	RS_OFF; // RS - 1
	EN_ON;
	delay_us(100);
	EN_OFF; // EN - 1 then 0
}

static void LCD_SendDataSignals(void)
{
	RS_ON; // RS - 1
	EN_ON;
	delay_us(100);
	EN_OFF; // EN - 1 then 0
}

static void LCD_Init(void)
{
	delay_ms(100);
	LCD_Reset();
	LCD_CmdWrite(0x28u); // Initialize the LCD for 4-bit 5x7 matrix type
	LCD_CmdWrite(0x0Eu); // Display ON cursor ON
	LCD_CmdWrite(0x01u); // Clear the LCD
	LCD_CmdWrite(0x80u); // go to First line First Position
}

void LCD_DisplayString(const char *ptr_string)
{
	// Loop through the string and display char by char
	while ((*ptr_string) != 0)
		LCD_DataWrite(*ptr_string++);
}

void delay_ms(unsigned int j)
{
	unsigned int x, i;
	for (i = 0; i < j; i++)
		for (x = 0; x < 10000; x++)
			;
}

void delay_us(unsigned int count)
{
	unsigned int j = 0, i = 0;
	for (j = 0; j < count; j++)
		for (i = 0; i < 10; i++)
			;
}
/////////////////////////////////////////////////////////////////////////////////

void LCD_DisplayNumber(unsigned int n)
{
	// extracting the digits and then printing them
	unsigned int arr[10], i = 0;
	while (n != 0)
	{
		arr[i++] = n % 10;
		n = n / 10;
	}
	while (i > 0)
		LCD_DataWrite(arr[--i] + '0');
}

void LCD_print_current_time(unsigned char flag)
{
	static int pdate = -1, psec = -1, phr = -1, pmin = -1;
	if (flag)
	{
		pdate = -1, psec = -1, phr = -1, pmin = -1;
	}
	if (pdate != DOM)
	{
		pdate = DOM;
		LCD_CmdWrite(0x80);				 // go to first line
		LCD_Display_2_digit_Number(DOM); // date
		LCD_DataWrite('-');
		LCD_Display_2_digit_Number(MONTH); // month
		LCD_DataWrite('-');
		LCD_DisplayNumber(YEAR);  // year
		LCD_CmdWrite(0xc0);		  // go to second line
		LCD_shiftCursor_right(2); // shift cursor right
		LCD_DataWrite(':');
		LCD_shiftCursor_right(2); // shift cursor right
		LCD_DataWrite(':');
	}

	if (phr != HOUR)
	{
		phr = HOUR;
		psec = pmin = -1;
		LCD_CmdWrite(0xc0);				  // go to second line
		LCD_Display_2_digit_Number(HOUR); // hour
	}
	if (pmin != MIN)
	{
		pmin = MIN;
		psec = -1;
		LCD_CmdWrite(0xc0);				 // go to second line
		LCD_shiftCursor_right(3);		 // shift cursor right
		LCD_Display_2_digit_Number(MIN); // minute
	}
	if (psec != SEC)
	{
		psec = SEC;
		LCD_CmdWrite(0xc0);				 // go to second line
		LCD_shiftCursor_right(6);		 // shift cursor right
		LCD_Display_2_digit_Number(SEC); // second
	}
}

void RTC_Init(void)
{
	CCR = 1 << 0 | 1 << 4; // Clock Control Register
}

void LCD_Display_2_digit_Number(unsigned int n)
{
	// extracting the digits and then printing them
	char arr[2];
	sprintf(arr, "%02u", n);
	LCD_DataWrite(arr[0]);
	LCD_DataWrite(arr[1]);
}

void LCD_shiftCursor_right(unsigned char n)
{
	while (n--)
		LCD_CmdWrite(0x14); // shift cursor right
}

void RTC_SetDateTime()
{

	SEC = 5;	 // Update sec value
	MIN = 7;	 // Update min value
	HOUR = 22;	 // Update hour value
	DOW = 6;	 // Update day value
	DOM = 27;	 // Update date value
	MONTH = 8;	 // Update month value
	YEAR = 2022; // Update year value
}