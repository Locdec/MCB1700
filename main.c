
#include "main.h"

const int frameShift = 8;
const int lineXCoordinate	=	1;
const int lineYCoordinate	= 135;

volatile GameData gameData = {
    GLCD_PIXELS_LENGTH,
    initialPlayerYCoordinate,
    GLCD_PIXELS_LENGTH,
    initialObstacleYCoordinate,
    initialObstacleWidth,
    initialObstacleHeight,
    true,
    false,
    false,
    false,
    240,
    0
};

int main(void) {
	uint32_t buttonBuffer;
	uint32_t joystickBuffer;
	uint32_t joystickMoved = 0;
	uint32_t buttonPushed = 0;

	uint32_t difficultyMs = 50;
	int level = 1;
	int score = 0;

	bool mode = false;
	bool gameProcessing = true;

	SystemInit();
	Buttons_Initialize();				/* Initialize buttons                 */
  GLCD_Initialize();          /* Initialize Graphical LCD           */
  Joystick_Initialize();      /* Initialize joystick                */
  LED_Initialize();           /* Initialize LED                     */

	startScreen(mode);

	while(buttonPushed==0){

		buttonBuffer = Buttons_GetState();
    if(buttonPushed == 0 && buttonBuffer == 1){
			buttonPushed = 1;
			gameScreen();
		}
	}

	configTimerInt(difficultyMs);

	while(gameProcessing){

			buttonBuffer = Buttons_GetState();
			joystickBuffer = Joystick_GetState();
			if(joystickMoved != joystickBuffer || buttonPushed != buttonBuffer ){
				joystickMoved = joystickBuffer;
				buttonPushed = buttonBuffer;
			movePlayer(&gameProcessing, mode);
			}

			refreshPlayerPosition(&gameProcessing, mode, level, score);
			refreshScore(&level, &score, &difficultyMs);
			moveObstacle(mode, gameData.obstacleTypeInt);
			refreshScreen(level, score);
			setDifficulty(difficultyMs);

	}

	restartScreen(level,score);

}

// TIMER INTERRUPTIONS
void TIMER0_IRQHandler(void) {
	unsigned int isrMask;

	isrMask = LPC_TIM0->IR;
	LPC_TIM0->IR = isrMask;         // Clear the Interrupt Bit

	if (gameData.randomPopping <= GLCD_PIXELS_LENGTH) {
		gameData.nextObstacleXCoordinate = gameData.previousObstacleXCoordinate - frameShift;
		if (gameData.nextObstacleXCoordinate == frameShift*4 ){
			gameData.obstacleClear = true;
		}
		if (gameData.nextObstacleXCoordinate <= 0 - gameData.obstacleWidth*2 ){
			gameData.nextObstacleXCoordinate = GLCD_PIXELS_LENGTH;
			gameData.previousObstacleXCoordinate = GLCD_PIXELS_LENGTH;
			gameData.randomPopping = (rand()%(GLCD_PIXELS_LENGTH*2)-GLCD_PIXELS_LENGTH)+GLCD_PIXELS_LENGTH;
			gameData.obstacleTypeInt = rand()%10;
		}
	}
	gameData.randomPopping = gameData.randomPopping - frameShift;
	gameData.frame = !gameData.frame;
}

void TIMER1_IRQHandler(void) {
	unsigned int isrMask;

	isrMask = LPC_TIM1->IR;
	LPC_TIM1->IR = isrMask;         // Clear the Interrupt Bit

		if (gameData.playerYCoordinate == playerJumpYCoordinate) {
			gameData.playerYCoordinate = initialPlayerYCoordinate;
			gameData.playerJumped = true;
		}
		else if (gameData.playerYCoordinate == playerSquatYCoordinate) {
			gameData.playerYCoordinate = initialPlayerYCoordinate;
			gameData.playerSquated = false;
		}
	NVIC_DisableIRQ(TIMER1_IRQn);
	LPC_TIM1->TCR = (1 << SBIT_RESET); // Reset counter
}

// HARDWARE SETTINGS FUNCTIONS
void delay_ms(uint32_t ms) {

	volatile uint32_t i;
	uint32_t count = ms * (SystemCoreClock / 1000 / 4);  // Adjust the division with the CPU Frequency

	for (i = 0; i < count; i++);
}

unsigned int getPrescalarForUs(uint8_t timerPclkBit){

	unsigned int pclk,prescalarForUs;
	pclk = (LPC_SC->PCLKSEL0 >> timerPclkBit) & 0x03;  /* get the pclk info for required timer */

	switch ( pclk ){                                   /* Decode the bits to determine the pclk*/
		case 0x00:
			pclk = SystemCoreClock/4;
			break;
		case 0x01:
			pclk = SystemCoreClock;
			break;
		case 0x02:
			pclk = SystemCoreClock/2;
			break;
		case 0x03:
			pclk = SystemCoreClock/8;
			break;
		default:
			pclk = SystemCoreClock/4;
			break;
	}
	prescalarForUs = pclk/1000000 - 1;                    /* Prescalar for 1us (1000000Counts/sec) */
	return prescalarForUs;
}

void configTimerInt(uint32_t ms){

	LPC_SC->PCONP |= (1<<SBIT_TIMER0) | (1<<SBIT_TIMER1); /* Power ON Timer0,1 */

	LPC_TIM0->MCR  = (1<<SBIT_MR0I) | (1<<SBIT_MR0R);     /* Clear TC on MR0 match and Generate Interrupt*/
	LPC_TIM0->PR   = getPrescalarForUs(PCLK_TIMER0);      /* Prescalar for 1us */
	LPC_TIM0->MR0  = (ms*1000);             								/* Load timer value to generate x ms delay*/
	LPC_TIM0->TCR  = (1 <<SBIT_CNTEN);                    /* Start timer by setting the Counter Enable*/
	NVIC_EnableIRQ(TIMER0_IRQn);                          /* Enable Timer0 Interrupt */

	LPC_TIM1->MCR  = (1<<SBIT_MR0I) | (1<<SBIT_MR0R);     /* Clear TC on MR0 match and Generate Interrupt*/
	LPC_TIM1->PR   = getPrescalarForUs(PCLK_TIMER1);      /* Prescalar for 1us */
	LPC_TIM1->MR0  = ((ms*1000)*10);              			  /* Load timer value to generate x ms delay*/
	//LPC_TIM1->TCR  = (1 <<SBIT_CNTEN);                  /* Start timer by setting the Counter Enable*/
	NVIC_DisableIRQ(TIMER1_IRQn);													/* Disable Timer1 Interrupt */
	//NVIC_EnableIRQ(TIMER1_IRQn);                        /* Enable Timer1 Interrupt */
}

void configureLEDPins(void){
	LPC_GPIO2->FIODIR = (1<<LED1) | (1<<LED2) | (1<<LED3);            /* Configure the LED pins(P2_0,P2_1) as outputs */
}

// DISPLAY FUNCTIONS
void startScreen(bool mode){
	if (mode == true) { //Game With Frames
	GLCD_SetBackgroundColor(GLCD_COLOR_WHITE);
	GLCD_ClearScreen();
  GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
  GLCD_SetFont(&GLCD_Font_16x24);
  GLCD_DrawString (0, 0*24,"  NEW T-REX GAME !  ");
  GLCD_DrawString(0, 1*24, "~~~~~~~~~~~~~~~~~~~~");
  GLCD_DrawString(0, 7*24, "PRESS INT0 TO BEGIN!");
	GLCD_DrawBitmap(250,50,30,30,sun_frame);
	GLCD_DrawBitmap(50,95,50,50,running_first_frame);
	GLCD_DrawHLine(1,135,319);
	}
	else { // Game Without Frames
	GLCD_SetBackgroundColor(GLCD_COLOR_WHITE);
	GLCD_ClearScreen();
  GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
  GLCD_SetFont(&GLCD_Font_16x24);
  GLCD_DrawString (0, 0*24,"  NEW T-REX GAME !  ");
  GLCD_DrawString(0, 1*24, "~~~~~~~~~~~~~~~~~~~~");
  GLCD_DrawString(0, 7*24, "PRESS INT0 TO BEGIN!");
	GLCD_DrawRectangle(250,50,25,25);
	GLCD_DrawRectangle(50,95,30,50);
	GLCD_DrawHLine(1,135,319);
	}
}

void gameScreen(void){
	GLCD_SetBackgroundColor(GLCD_COLOR_WHITE);
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_DrawString(0, 1*24, "~~~~~~~~~~~~~~~~~~~~");
  GLCD_DrawString(0, 7*24, "PRESS INT0 TO BEGIN!");
	GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
	GLCD_DrawString(0, 0*24, "Level:    Score:    ");
	GLCD_DrawBitmap(50,95,50,50,blank_frame);
	GLCD_DrawHLine(1,135,319);
}

void restartScreen(int level, int score){
	char str_score[3];
	char str_level[3];

	sprintf(str_score, "%d", score);
	sprintf(str_level, "%d", level);

	GLCD_SetBackgroundColor(GLCD_COLOR_BLACK);
	GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
	GLCD_SetFont(&GLCD_Font_16x24);
	GLCD_ClearScreen();
  GLCD_DrawString(0, 0*24,"  NEW T-REX GAME !  ");
  GLCD_DrawString(0, 1*24, "~~~~~~~~~~~~~~~~~~~~");
	GLCD_DrawString(0, 4*24, "Level:    Score:    ");
	GLCD_DrawString(17*16, 4*24, str_score);
	GLCD_DrawString(7*16, 4*24, str_level);
  GLCD_DrawString(0, 7*24, "   PRESS RESET TO   ");
	GLCD_DrawString(0, 8*24, "      RESTART       ");
}

void refreshScreen(int level, int score){
		char str_score[3];
		char str_level[3];

		sprintf(str_score, "%d", score);
		GLCD_DrawString(17*16, 0*24, str_score);
		sprintf(str_level, "%d", level);
		GLCD_DrawString(7*16, 0*24, str_level);

	switch (level) {
			case 2:
					LED_On(LED1);
					break;
			case 3:
					LED_On(LED2);
					break;
			case 4:
					LED_On(LED3);
					break;
			default:
					break;
	}
}

void restart(bool *gameProcessing, int *level, int *score, bool mode){
		*gameProcessing = true;
		*score = 0;
		*level = 0;
		startScreen(mode);
}

// GAME SETTINGS FUNCTIONS
bool collision(bool mode){
		if (mode == true) { // Game With Frames
			if((playerXCoordinate + playerWidth - gameData.nextObstacleXCoordinate) <= 60 && (playerXCoordinate + playerWidth - gameData.nextObstacleXCoordinate) > 10 && gameData.playerYCoordinate != playerJumpYCoordinate ){
				return true;
			}
			else {
				return false;
			}
		}
		else { // Game Without Frames
			if (gameData.obstacleYCoordinate == flyingObstacleYCoordinate) {
				if ((playerXCoordinate + squareWidth - gameData.nextObstacleXCoordinate) <= flyingObstacleWidth && (playerXCoordinate + squareWidth - gameData.nextObstacleXCoordinate) > 0  && gameData.playerYCoordinate != playerSquatYCoordinate ){
				return true;
				}
				else{
					return false;
				}
			}
			if (gameData.obstacleYCoordinate == groundObstacleYCoordinate) {
				if((playerXCoordinate + squareWidth - gameData.nextObstacleXCoordinate) <= groundObstacleWidth && (playerXCoordinate + squareWidth - gameData.nextObstacleXCoordinate) > 0 && gameData.playerYCoordinate != playerJumpYCoordinate ){
					return true;
				}
				else{
					return false;
				}
		}
	}
	return false;
}

void setDifficulty(uint32_t ms) {
	LPC_TIM0->MR0  = ms*1000;
}

void movePlayer(bool *gameProcessing, bool mode){

	if (mode == true) { // Game With Frames
		if(*gameProcessing) {
			if((Joystick_GetState() == JOYSTICK_UP && gameData.playerYCoordinate == initialPlayerYCoordinate ) || ( Buttons_GetState() == 1 && gameData.playerYCoordinate == initialPlayerYCoordinate  )){
				GLCD_DrawBitmap(playerXCoordinate, gameData.playerYCoordinate, playerWidth, playerHeight, blank_frame);
				gameData.playerYCoordinate = playerJumpYCoordinate;
				LPC_TIM1->TCR = (1 << SBIT_CNTEN); // Start
				NVIC_EnableIRQ(TIMER1_IRQn);

			}
			else if(Joystick_GetState() == JOYSTICK_DOWN && gameData.playerYCoordinate == initialPlayerYCoordinate){
				GLCD_DrawBitmap(playerXCoordinate, gameData.playerYCoordinate, playerWidth, playerHeight, blank_frame);
			}
		}
		else
		{
		}
	}
	else { //Game Without Frames
		if(*gameProcessing) {
			if((Joystick_GetState() == JOYSTICK_UP && gameData.playerYCoordinate == initialPlayerYCoordinate ) || ( Buttons_GetState() == 1 && gameData.playerYCoordinate == initialPlayerYCoordinate  )){
				GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
				GLCD_DrawRectangle(playerXCoordinate, gameData.playerYCoordinate, squareWidth, squareHeight);
				gameData.playerYCoordinate = playerJumpYCoordinate;
				LPC_TIM1->TCR = (1 << SBIT_CNTEN); // Start
				NVIC_EnableIRQ(TIMER1_IRQn);

			}
			else if(Joystick_GetState() == JOYSTICK_DOWN && gameData.playerYCoordinate == initialPlayerYCoordinate){
				GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
				GLCD_DrawRectangle(playerXCoordinate, gameData.playerYCoordinate, squareWidth, squareHeight);
				gameData.playerYCoordinate = playerSquatYCoordinate;
				gameData.playerSquated = true;
				LPC_TIM1->TCR = (1 << SBIT_CNTEN); // Start
				NVIC_EnableIRQ(TIMER1_IRQn);
			}
		}
		else
		{
		}
	}
}

void moveObstacle(bool mode, int obstacleTypeInt){

	if (mode == true) { // Game With Frames
		GLCD_DrawBitmap(gameData.previousObstacleXCoordinate + gameData.obstacleWidth - frameShift, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight, cactus_blank_frame);
		GLCD_DrawHLine(gameData.previousObstacleXCoordinate, lineYCoordinate, gameData.obstacleWidth*2);
		gameData.previousObstacleXCoordinate = gameData.nextObstacleXCoordinate;
		GLCD_DrawBitmap(gameData.nextObstacleXCoordinate, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight, cactus_frame);
	}
	else { // Game Without Frames
		if (obstacleTypeInt < 5){ // Ground Obstacle
			GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
			GLCD_DrawRectangle(gameData.previousObstacleXCoordinate, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight);
			gameData.obstacleYCoordinate = groundObstacleYCoordinate;
			gameData.previousObstacleXCoordinate = gameData.nextObstacleXCoordinate;
			gameData.obstacleWidth = initialObstacleWidth;
			gameData.obstacleHeight = initialObstacleHeight;
			GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
			GLCD_DrawRectangle(gameData.nextObstacleXCoordinate, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight);
		}
		if (obstacleTypeInt >= 5){ // Flying Obstacle
			GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
			GLCD_DrawRectangle(gameData.previousObstacleXCoordinate, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight);
			gameData.obstacleYCoordinate = flyingObstacleYCoordinate;
			gameData.previousObstacleXCoordinate = gameData.nextObstacleXCoordinate;
			gameData.obstacleWidth = flyingObstacleWidth;
			gameData.obstacleHeight = flyingObstacleHeight;
			GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
			GLCD_DrawRectangle(gameData.nextObstacleXCoordinate, gameData.obstacleYCoordinate, gameData.obstacleWidth, gameData.obstacleHeight);
		}

	}
}


void refreshPlayerPosition(bool *gameProcessing, bool mode, int level, int score){
	bool collisionState = false;

	if (mode == true) { // Game With Frames
		if (gameData.playerJumped == true) {
				GLCD_DrawBitmap(playerXCoordinate, playerJumpYCoordinate, playerWidth, playerHeight, blank_frame);
				gameData.playerJumped = false;
		}
		else if (gameData.playerJumped == false) {
			if (gameData.frame == true){
					GLCD_DrawBitmap(playerXCoordinate, gameData.playerYCoordinate, playerWidth, playerHeight, running_first_frame);
					GLCD_DrawHLine(lineXCoordinate, lineYCoordinate, GLCD_PIXELS_LENGTH-1);
			}
			else if (gameData.frame == false) {
					//GLCD_DrawBitmap(playerXCoordinate, playerYCoordinate, playerWidth, playerHeight, running_second_frame);
					GLCD_DrawHLine(lineXCoordinate, lineYCoordinate, GLCD_PIXELS_LENGTH-1);
			}
		}
	}
	else { // Game Without Frames
		if (gameData.playerJumped == true) {
				GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
				GLCD_DrawRectangle(playerXCoordinate, playerJumpYCoordinate, squareWidth, playerHeight);
				gameData.playerJumped = false;
		}
		else if (gameData.playerSquated == true) {
				GLCD_DrawRectangle(playerXCoordinate, playerSquatYCoordinate, squareWidth, squatSquareHeight);
		}
		else if (gameData.playerJumped == false && gameData.playerSquated == false) {
			GLCD_SetForegroundColor(GLCD_COLOR_WHITE);
			GLCD_DrawRectangle(playerXCoordinate, playerSquatYCoordinate, squareWidth, squatSquareHeight);
			if (gameData.frame == true){
					GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
					GLCD_DrawRectangle(playerXCoordinate, gameData.playerYCoordinate, squareWidth, playerHeight);
					GLCD_DrawHLine(lineXCoordinate, lineYCoordinate, GLCD_PIXELS_LENGTH-1);
			}
			else if (gameData.frame == false) {
					GLCD_SetForegroundColor(GLCD_COLOR_BLUE);
					GLCD_DrawRectangle(playerXCoordinate, gameData.playerYCoordinate, squareWidth, playerHeight);
					GLCD_SetForegroundColor(GLCD_COLOR_BLACK);
					GLCD_DrawHLine(lineXCoordinate, lineYCoordinate, GLCD_PIXELS_LENGTH-1);
			}
		}
	}

	collisionState = collision(mode);

	if(collisionState == true){
		*gameProcessing = false;
		NVIC_DisableIRQ(TIMER0_IRQn);
		NVIC_DisableIRQ(TIMER1_IRQn);
	}
}

void refreshScore(int *level, int *score, uint32_t *difficultyMs){


		if (gameData.obstacleClear == true ){
			*score = *score + 1;
			gameData.obstacleClear = false;
			if (*score % 10 == 0 && *difficultyMs >= 20) {
				*difficultyMs = *difficultyMs - 5;
				*level = *level + 1;
			}
		}

}
