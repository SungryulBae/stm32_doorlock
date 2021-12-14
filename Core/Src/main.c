/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "stm32_ds3231.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define regi_addr       0x001
#define lognum_addr     0x002
#define door_delay_addr 0x003
#define LIST_NUM           30
#define NOT_FOUND         255

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

void save_LOG(uint16_t eeprom_addr, uint8_t door_open_man);

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim9;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM7_Init(void);
static void MX_TIM9_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM6_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


// ------------------------------ variables

struct people {
	uint8_t reg_num;
	char	un[11];
	char	id[11];
	uint8_t	pw[2];
	uint8_t	unnum;
	uint8_t	idnum;
	uint8_t	exist;
	uint8_t	log[6];
};
struct people list[LIST_NUM];
uint8_t exist_temp[LIST_NUM];

uint8_t regi_num[1] = {0};
uint8_t log_num[1] = {0};
uint8_t door_delay_num[1] = {5};

uint8_t tx_unidpw[26];
uint8_t rx_unidpw[26];

uint8_t delete_find_regnum = NOT_FOUND;
uint16_t input_pw = 10000;	// because PW = 0~9999. used when delete, open door
uint8_t enterCNT = 1;	// 0 ~ 4
uint8_t door_open_man = NOT_FOUND;

// -------------------- trigger ----------------------------


uint8_t add_trig = 0;
uint8_t delete_trig = 0;
uint8_t update_trig = 0;
uint8_t getid_trig = 0;
uint8_t getlog_trig = 0;
uint8_t dellog_trig = 0;
uint8_t find_reginum_trig = 0;
uint8_t door_trig = 0;
uint8_t help_trig = 0;
uint8_t music_trig = 0;
uint8_t svm_trig = 0;


//-------------------------- Time Struct -----------------------

/*
typedef struct
{
  uint8_t Year;			// addr = 3
  uint8_t Month;		// addr = 4
  uint8_t Date;			// addr = 5
  uint8_t DaysOfWeek;	// addr = 6	.... not using
  uint8_t Hour;			// addr = 7
  uint8_t Min;			// addr = 8
  uint8_t Sec;			// addr = 9
} TimeTypeDef;


TimeTypeDef door_open_time[LIST_NUM];
*/

uint32_t timeout = 10000; // 10 sec



// ------------------------- AT24C32 (EEPROM) --------------------------

uint8_t data = 1;
uint8_t dataarr[50];


#define AT24C32_ADDR  (0x57 << 1) // 0xAE


I2C_HandleTypeDef *i2c;

void AT24C32_Init(I2C_HandleTypeDef *handle)
{
  i2c = handle;
}



bool EEPROM_ReadByte(uint16_t addr)	// T 2 R 1
{
	uint8_t buffer[2];

	buffer[0] = (addr >> 8) & 0x0F;
	buffer[1] =  addr & 0xFF;

	if(HAL_I2C_Master_Transmit(i2c, AT24C32_ADDR, buffer, 2, HAL_MAX_DELAY) != HAL_OK) return false;
	if(HAL_I2C_Master_Receive(i2c, AT24C32_ADDR, &data, 1, HAL_MAX_DELAY) != HAL_OK) return false;

	return true;

}

bool EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t datanum) // why data?
// T 2 R datanum
{
	uint8_t buffer[2+datanum] ;

	buffer[0] = (addr >> 8) & 0x0F;
	buffer[1] =  addr & 0xFF;


	if(HAL_I2C_Master_Transmit(i2c, AT24C32_ADDR, buffer, 2, HAL_MAX_DELAY) != HAL_OK) return false;
	if(HAL_I2C_Master_Receive(i2c, AT24C32_ADDR, data, datanum, HAL_MAX_DELAY) != HAL_OK) return false;
												//datarr?
	return true;
}

bool EEPROM_WriteByte(uint16_t addr, uint8_t data)
{
	uint8_t buffer[3];

	buffer[0] = (addr >> 8) & 0x0F;
	buffer[1] =  addr & 0xFF;
	buffer[2] = data;

	if(HAL_I2C_Master_Transmit(i2c, AT24C32_ADDR, buffer, 3, HAL_MAX_DELAY) != HAL_OK) return false;

	return true;
}

bool EEPROM_Write(uint16_t addr, uint8_t *data, uint16_t datanum)
{

	uint8_t buffer[2+datanum];
	buffer[0] = (addr >> 8) & 0x0F;
	buffer[1] =  addr & 0xFF;

	for (int i=0; i<datanum; i++)
		buffer[i+2] = data[i];

	if(HAL_I2C_Master_Transmit(i2c, AT24C32_ADDR, buffer, (2+datanum) , HAL_MAX_DELAY) != HAL_OK) return false;

	return true;
}


//---------------------------RTC---------------------

uint8_t OneSecFlag = 0;
uint8_t rx_complete =  0;
uint8_t buffer[50], adtemp[10];
uint8_t *str;
uint8_t rxd;
uint8_t disp = 0;

_RTC rtc;


//---------------------------- UART ----------------

uint8_t rx; 	// 1 byte
uint8_t buf[50], buf_index=0;

void PutString(char *str)
{
	HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), 1000);
}

void DispCommand(void)
{
	char buffer[50];
	strcpy(buffer, "\r\n> ");
	HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
}


//------------------------------BUZZER (TIM7(delay),TIM9(pwm))-----------------------------------

typedef struct{
	uint16_t freq;
	uint16_t delay;
}_BUZZER;


#define MEL_NUM 	8

#define C	262
#define D	294
#define E	330
#define F	349
#define G	392
#define A	440
#define B	494
#define C1	523
#define D1	587
#define E1	659

/*
_BUZZER buzzer[MEL_NUM] = {
		{G, 1}, {G, 1}, {A, 1}, {A, 1}, // 4
		{G, 1}, {G, 1}, {E, 2}, // 3
		{G, 1}, {G, 1}, {E, 1}, {E, 1}, {D, 2}, // 5
		{G, 1}, {G, 1}, {A, 1}, {A, 1}, // 4
		{G, 1}, {G, 1}, {E, 2}, // 3
		{G, 1}, {E, 1}, {D, 1}, {E, 1}, {C, 3} // 5
};
*/

_BUZZER buzzer_yes[8] = {
		{C, 1}, {D, 1}, {E, 1}, {F, 1},
		{G, 1}, {A, 1}, {B, 1}, {C1, 1}
};

_BUZZER buzzer_no[8] = {
		{E, 1}, {G, 1}, {E, 1}, {G, 1},
		{E, 1}, {G, 1}, {E, 1}, {G, 1}
};

uint8_t pause = 0;
uint8_t seq = 0;
uint8_t stop = 0;

uint8_t music_num = 0;

//------------------------------SERVO (TIM6(delay),TIM2(pwm))------------------

uint16_t svm_seq = 0;
uint8_t svm_angle = 50;

//------------------------------------------------------------

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_TIM7_Init();
  MX_TIM9_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */

  DS3231_Init(&hi2c1);

  HAL_UART_Receive_IT(&huart3, &rx, 1);

  //---Load registration number
  EEPROM_Read(regi_addr, regi_num, 1);
  HAL_Delay(10);
  EEPROM_Read(lognum_addr, log_num, 1);
  HAL_Delay(10);
  EEPROM_Read(door_delay_addr, door_delay_num, 1);
  HAL_Delay(10);

  update_trig = 1;	// load list to struct once

  HAL_Delay(100);

  //---Load help
  help_trig = 1;



  //find_reginum_trig = 1;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // ------------------------ add trig ------------------------------

	  if (add_trig == 1)			// command : Add
	  {

		  EEPROM_Read(regi_addr, regi_num, 1);

		  uint16_t start_addr = 0x010 + regi_num[0]*32;  // 1 = 0x10(16), 2 = 0x30(48), 3 = 0x50(80)..

		  //EEPROM_Write(start_addr, tx_unidpw, 26); --> didn't work

		  for (int i = 0 ; i<26; i++)
		  {
			  EEPROM_WriteByte(start_addr + i, tx_unidpw[i]);
		      HAL_Delay(10);
		  }


		  find_reginum_trig = 1;
		  update_trig = 1;
		  add_trig = 0;

	  }

	  // ------------------------ update trig ------------------------------

	  else if (update_trig == 1)	//update   ok   (update to struct)
	  {
		  uint16_t start_addr = 0x010;

		  for (uint8_t num=0;num<15;num++)
		  {
			  for (int i = 0 ; i<26; i++)
			  {
				  EEPROM_ReadByte(start_addr+i);
				  HAL_Delay(1);
				  rx_unidpw[i] = data;
			  }
			  start_addr += 32;

			  // initialize array
			  memset(list[num].un,0,sizeof(list[num].un));
			  memset(list[num].id,0,sizeof(list[num].id));
			  memset(list[num].log,0,sizeof(list[num].log));

			  // save
			  list[num].reg_num = rx_unidpw[0];
			  list[num].pw[0] = rx_unidpw[21];
			  list[num].pw[1] = rx_unidpw[22];
			  list[num].unnum = rx_unidpw[23];
			  list[num].idnum = rx_unidpw[24];
			  list[num].exist = rx_unidpw[25];


			  for (int i = 0 ; i< list[num].unnum; i++)
				  list[num].un[i] = rx_unidpw[i+1];
			  list[num].un[ list[num].unnum ] = 0; // \0

			  for (int i = 0 ; i< list[num].idnum; i++)
				  list[num].id[i] = rx_unidpw[i+11];
			  list[num].id[ list[num].idnum ] = 0; // \0


			  if (list[num].exist == 1)
				  exist_temp[num] = 1;
			  else
				  exist_temp[num] = 0;

		  }

		  update_trig = 0;
	  }

	  // ------------------------ delete trig ------------------------------

	  else if (delete_trig == 1) // command : Delete (if id found)
	  {
		  char buffer[50]={""};
		  sprintf(buffer, "\r\n id_num :%d, Input Password : ",delete_find_regnum+1);
		  PutString(buffer);

		  delete_trig = 2;
	  }

	  //(delete_trig == 2) is PW search

	  else if (delete_trig == 3) // Delete (if PW right)
	  {
		  uint16_t pw = (list[delete_find_regnum].pw[0])*256 + list[delete_find_regnum].pw[1];

		  if (pw == input_pw)
		  {
			  char buffer[50]={"\n\r Delete Complete!"};
			  PutString(buffer);

			  uint16_t start_addr = 0x010 + delete_find_regnum*32;
			  for (int i = 0 ; i<26; i++)
			  {
				  EEPROM_WriteByte(start_addr + i, 0xFF);
			      HAL_Delay(10);
			  }
			  find_reginum_trig = 1;
			  update_trig = 1;
		  }
		  else
		  {
			  char buffer[50]={"\n\r Incorrect Password"};
			  PutString(buffer);
		  }


		  DispCommand();
		  //initialize
		  input_pw = 10000;
		  delete_find_regnum = NOT_FOUND;
		  delete_trig = 0;
	  }

	  // ------------------------ getid trig ------------------------------

	  else if (getid_trig == 1) //GetID
	  {

		  for (uint8_t num=0;num<LIST_NUM;num++)
		  {
			  if(list[num].exist == 1)
			  {
				  uint16_t password = (list[num].pw[0])*256 + list[num].pw[1];

				  char buffer[50]={""},temp[50];
				  sprintf(temp, "\r\n %d. name:%s  id:%s   password:%d",(list[num].reg_num)+1,list[num].un,list[num].id,password);
				  strcat(buffer,temp);
				  PutString(buffer);
			  }
		  }

		  DispCommand();
		  getid_trig = 0;
	  }

	  // ------------------------ find_reginum trig ------------------------------

	  else if (find_reginum_trig == 1)	// command : findnum
	  {

		  for(uint8_t i=0;i<LIST_NUM;i++)
			  exist_temp[i] = list[i].exist;

		  for(uint8_t i=0;i<LIST_NUM;i++)
		  {
			  if (exist_temp[i] != 1)
			  {
			  	regi_num[0] = i;

			  	EEPROM_WriteByte(regi_addr, regi_num[0]);
			  	break;
			  }
		  }

		  /*
		  //show
		  for(uint8_t i=0;i<LIST_NUM;i++)
		  {
			  char buffer[50]={""};
			  sprintf(buffer, "\r\n %d beon is %d",i+1,exist_temp[i]);
			  PutString(buffer);
		  }
		  */

		  find_reginum_trig = 0;
	  }

	  // ------------------------ door trig ------------------------------

	  else if (door_trig == 2)	// find pw from struct
	  {
		  uint8_t findpw_trig = 0;

		  for(uint8_t num=0;num<LIST_NUM;num++)
		  {
			  uint16_t list_pw = (list[num].pw[0])*256 + list[num].pw[1];

			  if (input_pw == list_pw)
			  {
				  door_open_man = num;
				  findpw_trig = 1;
				  break;
			  }

		  }
		  if (findpw_trig == 1)
			  door_trig = 4;
		  else
			  door_trig = 3;
	  }

	  else if (door_trig == 3)	// if NOT FIND PW
	  {
		  char buffer[50]={""};
		  sprintf(buffer, "\r\n Incorrect Password");
		  PutString(buffer);

		  music_trig = 1;	// music on
		  music_num = 0;	// music no

		  //init
		  input_pw = 10000;
		  door_trig = 0;
	  }
	  else if (door_trig == 4)	// if FIND PW
	  {
		  DS3231_GetTime(&rtc);

		  char buffer[60]={""},temp1[30],temp2[30];
		  //sprintf(buffer, "\r\n Door Open. %d beon's pw is %d.",door_open_man+1,input_pw);
		  sprintf(buffer, "\r\n Door Open. ");
		  sprintf(temp1, "\r\n Hello %s !!!",list[door_open_man].id);
		  strcat(buffer,temp1);
		  sprintf(temp2, "\r\n20%0d-%02d-%02d %02d:%02d:%02d", rtc.Year, rtc.Month, rtc.Date, rtc.Hour, rtc.Min, rtc.Sec);
		  strcat(buffer,temp2);
		  PutString(buffer);

		  music_trig = 1;	// music on
		  music_num = 1;	// music yes
		  svm_trig = 1;		// door open

		  // save LOG
		  uint16_t eeprom_addr = (1000 + log_num[0] * 10); // 1000,1010,1020...
		  save_LOG(eeprom_addr, door_open_man);

		  // save log_num
		  log_num[0]++;
		  EEPROM_WriteByte(lognum_addr, log_num[0]);
		  HAL_Delay(5);

		  // init
		  door_open_man = NOT_FOUND;
		  input_pw = 10000;
		  door_trig = 0;
	  }

	  // ------------------------ music trig ------------------------------

	  else if (music_trig == 1)
	  {
		  if(pause == 0)
				seq = 0;

		  HAL_TIM_Base_Start_IT(&htim7);
		  //HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);

		  /*
		  //show
		  char buffer[50]={""};
		  sprintf(buffer,"music START");
		  HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
		  */

		  music_trig = 0;
	  }

	  // ------------------------ svm trig ------------------------------

	  else if (svm_trig == 1)	// command : startservo
	  {
		  HAL_TIM_Base_Start_IT(&htim6);

		  svm_trig = 0;
	  }


	  // ------------------------ getlog trig ------------------------------

	  else if (getlog_trig == 1)	// command : GetLog
	  {

		  char buffer[50]={""};
		  sprintf(buffer, "\r\n =================== LOG ===================");
		  PutString(buffer);

		  uint16_t eeprom_addr = (1000 + log_num[0] * 10);

		  for(int i = 0;i<log_num[0];i++)
		  {
			  eeprom_addr -= 10;

			  EEPROM_ReadByte(eeprom_addr+0); HAL_Delay(5);
			  uint8_t log_id         = data;
			  EEPROM_ReadByte(eeprom_addr+1); HAL_Delay(5);
			  uint8_t log_Year       = data;
			  EEPROM_ReadByte(eeprom_addr+2); HAL_Delay(5);
			  uint8_t log_Month      = data;
			  EEPROM_ReadByte(eeprom_addr+3); HAL_Delay(5);
			  uint8_t log_Date       = data;
			  //EEPROM_ReadByte(eeprom_addr+4); HAL_Delay(5);
			  //uint8_t log_DaysOfWeek = data;
			  EEPROM_ReadByte(eeprom_addr+5); HAL_Delay(5);
			  uint8_t log_Hour       = data;
			  EEPROM_ReadByte(eeprom_addr+6); HAL_Delay(5);
			  uint8_t log_Min        = data;
			  EEPROM_ReadByte(eeprom_addr+7); HAL_Delay(5);
			  uint8_t log_Sec        = data;

			  // print
			  char buffer[60]={""},temp1[30];
			  sprintf(buffer, "\r\n %02d. | LOG : 20%0d-%02d-%02d %02d:%02d:%02d",i+1, log_Year, log_Month, log_Date, log_Hour, log_Min, log_Sec);
			  sprintf(temp1, " |  id: %s (Number %d)",list[log_id].id,list[log_id].reg_num +1);
			  strcat(buffer,temp1);
			  PutString(buffer);

		  }

		  DispCommand();
		  getlog_trig = 0;
	  }

	  // ------------------------ dellog trig ------------------------------

	  else if (dellog_trig == 1) // command : DelLog
	  {
		  // save loG_num
		  log_num[0] = 0;
		  EEPROM_WriteByte(lognum_addr, log_num[0]);
		  HAL_Delay(5);

		  // show
		  char buffer[50]={""};
		  sprintf(buffer, "\r\n Delete Log Complete!");
		  PutString(buffer);

		  DispCommand();
		  dellog_trig = 0;
	  }

	  // ------------------------ help trig ------------------------------


	  else if (help_trig == 1)
	  {
			char buffer[200]={""},temp[50];
			sprintf(buffer, "\r\n+-----------------------------------+");
			sprintf(temp, "\r\n|            \\    /\\");
			strcat(buffer,temp);
			sprintf(temp, "\r\n|             )  ( ')");
			strcat(buffer,temp);
			sprintf(temp, "\r\n|            (  /  )");
			strcat(buffer,temp);
			sprintf(temp, "\r\n|             \\(__)|");
			strcat(buffer,temp);
			sprintf(temp, "\r\n|        MADE BY BAE SUNG RYUL      ");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| -------------- MENU --------------");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| Add [UserName] [ID] [PW]");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| Delete [ID]");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| GetID");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| GetLog");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| GetTime");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| SetTime [time]");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| SetDate [date]");
			strcat(buffer,temp);
			sprintf(temp, "\r\n| SetDelay [sec]");
			strcat(buffer,temp);
			sprintf(temp, "\r\n+-----------------------------------+");
			strcat(buffer,temp);

			PutString(buffer);

			DispCommand();
			help_trig = 0;
			/*
			+-----------------------------------+
			| Add [UserName] [ID] [PW]
			| Delete [ID]
			| GetID
			| GetLog
			| DelLog
			| GetTime
			| GetDate
			| SetTime [time]
			| SetDate [date]
			| SetDelay [sec]
			+------------------------------------+
			*/

	  }

	  /*
	  else if (timeout == 0)	// save time by 10sec
	  {
		  save_RTC();

		  DS3231_GetTime(&rtc);
		  HAL_Delay(10);


		  //show
		  char buffer[50]={""};
		  sprintf(buffer, "\r\n20%0d-%02d-%02d %02d:%02d:%02d", rtc.Year, rtc.Month, rtc.Date, rtc.Hour, rtc.Min, rtc.Sec);
		  PutString(buffer);


		  timeout = 10000;
	  }
  	  */

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* I2C1_ER_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
  /* I2C1_EV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
  /* TIM7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);
  /* TIM1_BRK_TIM9_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 16-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 20000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 1600-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 100-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 1600-1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 200-1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 16-1;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 200-1;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */
  HAL_TIM_MspPostInit(&htim9);

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */



//============================ UART ========================================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart -> Instance == USART3)
	{
		HAL_UART_Transmit(&huart3, &rx, 1, 1000);

		//-------------------------------BACK SPACE-----------------------

		if( rx==0x08 )
		{
			buf[buf_index] = NULL;
			buf[buf_index - 1] = NULL;
			buf_index--;

			HAL_UART_Transmit(&huart3, " ", 1, 1000);
			HAL_UART_Transmit(&huart3, "\b", 1, 1000);

			if(buf_index == 255)
			{
				buf_index = 0;
			}

			//buf[buf_index--]= rx;
		}
		else
			buf[buf_index++]= rx;	// save rx to buffer

		//--------------------------------- ENTER --------------------------------

		if( (rx==0x0a) || (rx==0x0d) ) //LF or CR
		{
			char *p;
			buf[buf_index-1]=0;  // write \0 at the end of a string


			//-------------------------------- I2C -------------------------------

			//EEPROM_ReadByte(uint16_t addr)
			//EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t datanum)
			//EEPROM_WriteByte(uint16_t addr, uint8_t data)
			//EEPROM_Write(uint16_t addr, uint8_t *data, uint16_t datanum)

			if((p=strstr((char*)buf,"read"))!= 0)	// readN N = 0~4095
			{
				char buffer[50]={""};
				uint16_t num = atoi(&p[4]);
				EEPROM_ReadByte(num);
				sprintf(buffer,"read1byte  addr %d -> %d", num,data);
				HAL_UART_Transmit(&huart3,(uint8_t*)buffer, strlen(buffer), 1000);

				enterCNT = 0;
			}

			else if((p=strstr((char*)buf,"write"))!= 0) //ex) writeMMMM N (M=address 0000~4095)(N=0~255)
			{
				uint8_t addr,num;
				addr = atoi(&p[5]);
				num = atoi(&p[10]);

				EEPROM_WriteByte(addr, num);

				enterCNT = 0;
			}
			else if((p=strstr((char*)buf,"rrrr"))!= 0)
			{
				//EEPROM_Read(uint16_t addr, uint8_t *data, uint16_t datanum)

				char buffer[50]={""};

				EEPROM_Read(0x30,dataarr,3);

				sprintf(buffer,"read = %d %d %d", dataarr[0], dataarr[1], dataarr[2]);
				HAL_UART_Transmit(&huart3,(uint8_t*)buffer, strlen(buffer), 1000);

				enterCNT = 0;
			}
			else if((p=strstr((char*)buf,"ww"))!= 0) //wwA B C (A,B,C=000~255)
			{
				//EEPROM_Write(uint16_t addr, uint8_t *data, uint16_t datanum)

				uint8_t arr[3];
				arr[0] = atoi(&p[2]);	//A
				arr[1] = atoi(&p[6]);	//B
				arr[2] = atoi(&p[10]);	//C

				EEPROM_Write(0x30, arr , 3);

				enterCNT = 0;
			}

			/*

			SA : Save Address

			SA = 0x010(16) , 0x030(48), 0x050(80) ...  : 15 people

			SA + 0   			: reg_num
			SA + 1  ~ SA + 10	: user name
			SA + 11 ~ SA + 20	: id
			SA + 21 ~ SA + 22	: pw
			SA + 23 			: user name number
			SA + 24 			: id number
			SA + 25				: exist (0x01) non-exist (other)
			*/
			//----------- a : Registration ------------------------------

			else if((p=strstr((char*)buf,"Add"))!= 0)	// Add un id pw
			{

				//------------ find space
				uint8_t space[3]={0,0,0};

				int j = 0;

				for (int i=0;i<(buf_index-1);i++)
				{
					if(p[i]==' ')
						space[j++] = i;
				}

				//------------ register [rg & un & id & pw]

				uint8_t rg_num = regi_num[0];			//  1 byte register number
				uint8_t exist  = 1;
				uint8_t un_num = space[1]-space[0]-1;	//  1 byte user name number
				uint8_t id_num = space[2]-space[1]-1;	//  1 byte id number
				uint16_t pw ;   // 0~65535


				uint8_t tx_un[11],tx_id[11];
				//un
				for (int j=0; j<un_num; j++)
				{
					tx_un[j] = p[space[0]+1+j];
				}
				tx_un[un_num]='\0'; // \0

				//id
				for (int j=0; j<id_num; j++)
				{
					tx_id[j] = p[space[1]+1+j];
				}
				tx_id[id_num]='\0'; // \0

				//pw
				pw = atoi(&p[ space[2]+1 ]);
				uint8_t tx_pw[2] = {pw/256,pw%256};


				//----------- Write EEPROM

				tx_unidpw[0] = rg_num;
				for (int j= 1; j<=10; j++)
					tx_unidpw[j] = tx_un[j- 1];
				for (int j=11; j<=20; j++)
					tx_unidpw[j] = tx_id[j-11];
				for (int j=21; j<=22; j++)
					tx_unidpw[j] = tx_pw[j-21];
				tx_unidpw[23] = un_num;
				tx_unidpw[24] = id_num;
				tx_unidpw[25] = exist;


				//show
				char buffer[50]={""};
				sprintf(buffer, "\r\n%s  %s  %d",tx_un,tx_id,pw);
				PutString(buffer);


				//trigger
				add_trig = 1;

				enterCNT = 0;
				/*
				//show

				sprintf(buffer, "\r\n %d %d",space[0],space[1]);
				sprintf(temp, "\r\n%s \r\n%s \r\n%d",tx_un,tx_id,pw_num);
				strcat(buffer,temp);
				sprintf(temp, "\r\n%d",regi_num[0]);
				strcat(buffer,temp);
				*/
			}


			//----------- b : Delete ID

			else if((p=strstr((char*)buf,"Delete"))!= 0)
			{

				//------------ find id
				if ( ( (p[7] >= 0x41) && (p[7] <= 0x5A) ) || ( (p[7] >= 0x61) && (p[7] <= 0x7A) ))	//alphabet ASCII
				{
					uint8_t id[11];

					for (int i=0;i<(buf_index-1);i++)
						id[i] = p[7+i];
					id[buf_index-1]='\0'; // \0

					for (int i=0;i<LIST_NUM;i++)
					{
						if( (strcmp((char*)id,list[i].id))== 0 )
						{
							delete_find_regnum = i;
							delete_trig = 1;
							break;
						}
					}
				}

				enterCNT = 0;
			}
			//----------- c
			else if((p=strstr((char*)buf,"GetID"))!= 0)
			{
				getid_trig = 1;

				enterCNT = 0;
			}
			//----------- d
			else if((p=strstr((char*)buf,"GetLog"))!= 0)
			{
				getlog_trig = 1;

				enterCNT = 0;
			}
			//----------- e
			else if((p=strstr((char*)buf,"DelLog"))!= 0)
			{
				dellog_trig = 1;

				enterCNT = 0;
			}

			//----------- f
			else if((p=strstr((char*)buf,"GetTime"))!= 0)
			{
				DS3231_GetTime(&rtc);

				// show
				char buffer[50]={""};
				sprintf(buffer, "\r\n%02d:%02d:%02d",rtc.Hour, rtc.Min, rtc.Sec);
				PutString(buffer);

				enterCNT = 0;
			}
			//----------- g
			else if((p=strstr((char*)buf,"GetDate"))!= 0)
			{
				DS3231_GetTime(&rtc);

				// show
				char buffer[50]={""};
				sprintf(buffer, "\r\n20%0d-%02d-%02d", rtc.Year, rtc.Month, rtc.Date);
				PutString(buffer);

				enterCNT = 0;
			}
			//----------- h
			else if((p=strstr((char*)buf,"SetTime"))!= 0) // SetTime 130000
			{

				rtc.Hour =  (p[8]-'0')*10 + (p[9]-'0');
				rtc.Min  = (p[10]-'0')*10 + (p[11]-'0');
				rtc.Sec  = (p[12]-'0')*10 + (p[13]-'0');

				DS3231_SetTime(&rtc);

				enterCNT = 0;
			}
			//----------- i
			else if((p=strstr((char*)buf,"SetDate"))!= 0)
			{
				rtc.Year  =  (p[8]-'0')*10 + (p[9]-'0');
				rtc.Month = (p[10]-'0')*10 + (p[11]-'0');
				rtc.Date  = (p[12]-'0')*10 + (p[13]-'0');

				DS3231_SetTime(&rtc);

				enterCNT = 0;
			}
			//----------- j
			else if((p=strstr((char*)buf,"SetDelay"))!= 0)
			{
				door_delay_num[0] = atoi(&p[9]);
				EEPROM_Write(door_delay_addr, door_delay_num , 1);

				// show
				char buffer[30]={""};
				sprintf(buffer, "\r\n door delay = %d", door_delay_num[0]);
				PutString(buffer);

				enterCNT = 0;
			}

			//----------- k
			else if((p=strstr((char*)buf,"Help"))!= 0)
			{
				help_trig = 1;

				enterCNT = 0;
			}

			//----------- l

			else if((p=strstr((char*)buf,"showall"))!= 0)
			{
				char buffer[150]={""},temp[50];

				DS3231_GetTime(&rtc);
				sprintf(buffer, "\r\n20%0d-%02d-%02d %02d:%02d:%02d", rtc.Year, rtc.Month, rtc.Date, rtc.Hour, rtc.Min, rtc.Sec);

				EEPROM_Read(door_delay_addr, door_delay_num, 1);
				sprintf(temp, "\r\n door_delay_num = %d", door_delay_num[0]);
				strcat(buffer,temp);
				PutString(buffer);

				enterCNT = 0;
			}
			//----------- m : reset registration_num
			else if((p=strstr((char*)buf,"resetRN"))!= 0)
			{
				EEPROM_Write(0x001, 0, 1);
				EEPROM_Read(0x001, regi_num, 1);

				enterCNT = 0;
			}

			else if((p=strstr((char*)buf,"update"))!= 0)
			{
				update_trig = 1;

				enterCNT = 0;
			}
			else if((p=strstr((char*)buf,"findnum"))!= 0)
			{
				find_reginum_trig = 1;

				enterCNT = 0;
			}


			else if((p=strstr((char*)buf,"ssss"))!= 0)	// not used...
			{
				// show rx_unidpw
				char buffer[50]={""},temp[3];

				for (int i = 0 ; i<26;i++)
				{
					sprintf(temp, " %d",rx_unidpw[i]);
				  	strcat(buffer,temp);
				}
				PutString(buffer);

				enterCNT = 0;
			}


			//---------------------------- BUZZER (TIM7,TIM9) ------------------------------

			else if((p=strstr((char*)buf,"PLAY"))!= 0)
			{
				music_trig = 1;

				enterCNT = 0;
			}

			/*
			else if((p=strstr((char*)buf,"STOP"))!= 0)
			{

				char buffer[50]={""};

				pause = 0;
				HAL_TIM_Base_Stop_IT(&htim7);
				HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);


				sprintf(buffer,"music stop");
				HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);

			}

			else if((p=strstr((char*)buf,"PAUSE"))!= 0)
			{
				char buffer[50]={""};

				pause = 1;
				HAL_TIM_Base_Stop_IT(&htim7);
				HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);

				sprintf(buffer,"music pause");
				HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);

			}
			*/


			else if((p=strstr((char*)buf,"svmstart"))!= 0)
			{
				svm_trig = 1;

				enterCNT = 0;
			}


			//----------------------------------------------------------------------

			else	// change input code to ASCII number
			{
				uint8_t num_temp[4];

				for (int i = 0 ; i<(buf_index-1);i++) //buf_index-1 gae
				{
					num_temp[i] = buf[i]-'0';
					//char buffer[10];
					//sprintf(buffer, "\n\r buf[%d] = %d (%d)",i,buf[i],num_temp[i]);
					//PutString(buffer);
				}

				if ( ((buf_index-1) == 4 ) && (delete_trig == 2))
				{
					input_pw = num_temp[0]*1000+num_temp[1]*100+num_temp[2]*10+num_temp[3];
					delete_trig = 3;

					//show
					char buffer[20];
					sprintf(buffer, "\n\r input_pw = %d",input_pw);
					PutString(buffer);

					enterCNT = 0;
				}

				else if ( ((buf_index-1) == 4 ) && (door_trig == 1))
				{
					input_pw = num_temp[0]*1000+num_temp[1]*100+num_temp[2]*10+num_temp[3];

					//show
					char buffer[20];
					sprintf(buffer, "\n\r input_pw = %d",input_pw);
					PutString(buffer);

					door_trig = 2;

					enterCNT = 0;
				}
				else
				{
					door_trig = 0;
				}
			}

			// -----------  enterCNT  ---------------

			enterCNT++;

			if (enterCNT > 4)
				enterCNT = 0;

			if ((enterCNT == 4) && (door_trig == 0))
			{
				char buffer[20];
				sprintf(buffer, "\n\r Input Password :");
				PutString(buffer);

				door_trig = 1;
				enterCNT = 0;
			}

			/*
			//show
			char buffer[50];
			sprintf(buffer, "    enterCNT = %d, doortrig = %d ",enterCNT,door_trig);
			PutString(buffer);
			*/

			//----------------------------------------

			buf_index=0;
			memset(buf,0,sizeof(buf));
			DispCommand();
		}
		HAL_UART_Receive_IT(&huart3, &rx, 1); // when receive 1byte, make callback
	}
}


//======================= SYSTICK_Callback ================================

// you must put "HAL_SYSTICK_IRQHandler()" into void Systick_Handler(void)

// not using...

void HAL_SYSTICK_Callback(void)
{
	if(timeout != 0)
		timeout--;
}

//----------------------------------------------------


void save_LOG(uint16_t eeprom_addr, uint8_t door_open_man)
{
	EEPROM_WriteByte(eeprom_addr, door_open_man);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+1, rtc.Year);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+2, rtc.Month);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+3, rtc.Date);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+4, rtc.Date);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+5, rtc.Hour);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+6, rtc.Min);
	HAL_Delay(10);
	EEPROM_WriteByte(eeprom_addr+7, rtc.Sec);
	HAL_Delay(10);
}

//----------------------------------------------------


//========================= TIMER CALLBACK (TIM 7) ===============================

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	// -------------------- BUZZER --------------------------------
	if(htim->Instance == TIM7)
	{
		uint16_t melody;

		if (music_num == 1)
			melody = (uint16_t)(1000000 / buzzer_yes[seq].freq);
		else
			melody = (uint16_t)(1000000 / buzzer_no[seq].freq);

		if(stop == 1)
		{
			TIM7->ARR = 2000;
			HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);
			stop = 0;
		}
		else
		{
			if(seq == MEL_NUM)
			{
				HAL_TIM_Base_Stop_IT(&htim7);
				HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_1);
			}
			else
			{
				TIM9->ARR = melody;
				TIM9->CCR1 = melody / 2;

				if (music_num == 1)
					TIM7->ARR = buzzer_yes[seq].delay * 2000;
				else
					TIM7->ARR = buzzer_no[seq].delay * 2000;

				HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1);
				stop = 1;

				seq++;
			}
		}
	}

	// -------------------- SERVO --------------------------------

	else if(htim->Instance == TIM6)
	{
		// svm_angle 50~150

		// door init
		if (svm_seq <= 100)
		{
			svm_angle = 50;

			TIM2->CCR1 = 500 + (uint16_t)((svm_angle*2000/180));
			HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);
		}

		// door open time
		else if (svm_seq > 100 && svm_seq <= 300)
		{
			if (svm_angle<150)
				svm_angle++;

			TIM2->CCR1 = 500 + (uint16_t)((svm_angle*2000/180));
			HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);
		}

		// door delay
		else if (svm_seq > 300 && svm_seq <= (door_delay_num[0]*100) + 300 )
		{
			HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);
		}

		// door close time
		else if (svm_seq > (door_delay_num[0]*100) + 300 && svm_seq <= (door_delay_num[0]*100) + 500)
		{
			if (svm_angle>50)
				svm_angle--;

			TIM2->CCR1 = 500 + (uint16_t)((svm_angle*2000/180));
			HAL_TIM_PWM_Start_IT(&htim2, TIM_CHANNEL_1);
		}

		// door pwm stop
		else if (svm_seq > (door_delay_num[0]*100) + 500 )
		{
			svm_seq = 0;
			HAL_TIM_PWM_Stop_IT(&htim2, TIM_CHANNEL_1);
			HAL_TIM_Base_Stop_IT(&htim6);
		}
		svm_seq++;

		/*
		//show
		char buffer[20];
		sprintf(buffer, "svm_seq= %d svm_angle = %d \n\r",svm_seq,svm_angle);
		PutString(buffer);
		*/

	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
