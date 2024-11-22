#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
 
#include "LPC17xx.h"                    // Device header
#include "Board_LED.h"                  // ::Board Support:LED
#include "Board_Buttons.h"              // ::Board Support:Buttons
#include "Board_Joystick.h"             // Board Support:Joystick
#include "Board_GLCD.h"                 // Board Support:Graphic LCD
#include "GLCD_Config.h"                // Board Support:Graphic LCD
#include "GLCD_Fonts.h"                 // Board Support:Graphic LCD
#include "Driver_SPI.h"                 // CMSIS Driver:SPI:Custom
#include "RTE_Device.h"                 // Device:Startup

#include "frames.h"

//Timer Parameters
#define SBIT_TIMER0  1									//Defining Timer Bits
#define SBIT_TIMER1  2									//Defining Timer Bits
#define SBIT_MR0I    0									//Defining Timer Bits
#define SBIT_MR0R    1									//Defining Timer Bits
#define SBIT_CNTEN   0									//Defining Timer Bits
#define SBIT_RESET 	 1									//Defining Timer Bits
#define PCLK_TIMER0  2									//Defining Timer Bits
#define PCLK_TIMER1  4									//Defining Timer Bits

//LEDs parameters
#define LED1         0 									// 
#define LED2         1 									// 
#define LED3         2 									// P2_0

//Display parameters
#define GLCD_PIXELS_WIDTH						240						//GLCD width in pixels
#define GLCD_PIXELS_LENGTH					320						//GLCD length in pixels
#define GLCD_PIXELS_SCORE_BANNER 		48 						//Score banner length in pixels

//Obstacle Parameters
#define initialObstacleYCoordinate	115
#define initialObstacleWidth 				15
#define initialObstacleHeight 			30
#define groundObstacleYCoordinate 	115
#define groundObstacleWidth					15	
#define groundObstacleHeight				30
#define flyingObstacleYCoordinate 	90
#define flyingObstacleWidth					20
#define	flyingObstacleHeight				15

//Player Parameters
#define initialPlayerYCoordinate 		((GLCD_PIXELS_WIDTH - GLCD_PIXELS_SCORE_BANNER) / 2) - 1
#define playerXCoordinate 					50
#define playerJumpYCoordinate				60
#define playerSquatYCoordinate 			115
#define playerWidth 								50
#define squareWidth									30
#define squatSquareHeight						30
#define playerHeight				  			50
#define squareHeight								50

//Game global variables into GameData structure
typedef struct {
    int nextObstacleXCoordinate;
    int playerYCoordinate;
    int previousObstacleXCoordinate;
    int obstacleYCoordinate;
    int obstacleWidth;
    int obstacleHeight;
    bool frame;
    bool obstacleClear;
    bool playerJumped;
    bool playerSquated;
    int randomPopping;
    int obstacleTypeInt;
} GameData;

//Game global variables definitions
extern volatile GameData gameData;
extern const int frameShift;	
extern const int lineXCoordinate;
extern const int lineYCoordinate;

//Hardware Settings functions prototypes
unsigned int getPrescalarForUs(uint8_t timerPclkBit);
void configTimerInt(uint32_t ms);
void delay_ms(uint32_t ms);
void configureLEDPins(void);

//Display functions prototypes
void startScreen(bool mode);
void gameScreen(void);
void refreshScreen(int level, int score);
void restartScreen(int level, int score);
void restart(bool *gameProcessing, int *level, int *score, bool mode);

// Game Settings functions prototypes
bool collision(bool mode);
void setDifficulty(uint32_t ms);
void movePlayer(bool *gameProcessing, bool mode);
void moveObstacle(bool mode, int obstacleTypeInt);
void refreshScore(int *level, int *score, uint32_t *difficultyMs);
void refreshPlayerPosition(bool *gameProcessing, bool mode, int level, int score);

#endif /* __MAIN_H */
