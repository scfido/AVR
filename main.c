#include <iom128v.h>  ///////////////////////////////////////////////
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <macros.h>

#include "M128_UART.h"
#include "Main.h"

#define	Framesize			128
#define	GFsize				128

#define	Pulse_HOut_Time	1900

////////////////////////////////////////////////
unsigned char PID_Parameter[7];
/*
#define	P_Gain				45
#define	I_Gain				5
#define	D_Gain				10
#define	Decel_Section		10
#define	Decel_Slope			25
#define	Set_Velo			35		// Scan Speed  20mS S 동안 펄스수 
 */
int Err_Old = 0;
int Err_Sum = 0;
 
unsigned int Encoder_Count = 0;

unsigned int Duty_Old = 0;
unsigned int Velo_Count = 0;
unsigned int Velo_Data = 0;
unsigned int Turn_Count = 7258; ///7258.333333

unsigned char Scan_Flag = 0;
unsigned char Set_Flag = 0;

unsigned char Frame[Framesize];
unsigned char FrameCount = 0;
unsigned char FR_Count = 0;

unsigned char GFrame[GFsize];
unsigned char GCount = 0;

unsigned char X_data[5];
unsigned char Y_data[5];

unsigned int X_Axis = 0;
unsigned int Y_Axis = 0;

unsigned char T10mS_Count = 0;
unsigned char T20mS_Count = 0;

unsigned char Moving_Sequence = Wait;
unsigned char Moving_Sequence_Old = Wait;

unsigned char Sequence_State = Wait_State;
unsigned char Sequence_Num = 1;
unsigned char Sequence_Num_Old = 1;

/*
typedef union{
    unsigned int   intBuff;
    unsigned char ByteBuff[2];
}Int2Byte;
*/

 void Port_Init(void)
{
	DDRA = 0xFF;

	DDRB = (1 << CW_CCW) | (1 << PWM) | (1 << ENABLE) | (1 << SCK);

	DDRC = (1 << Sol_Valve8) | (1 << Sol_Valve7) | (1 << Sol_Valve6) | (1 << Sol_Valve5) | (1 << Sol_Valve4) | (1 << Sol_Valve3) | (1 << Sol_Valve2) | (1 << Sol_Valve1);

	DDRD = (1 << TXD_1) | (0 << RXD_1);
	PORTD = (1 << RXD_1) | (1 << Encoder_B) | (1 << Encoder_A);

	DDRE = (1 << STATE6) | (1 << STATE5) | (1 << STATE4) |(1 << TXD) | (0 << RXD); 
	PORTE = (0 << STATE6) | (0 << STATE5) | (0 << STATE4) |(1 << TXD) | (0 << RXD);
}

void EXT_INT_Init(void)
{
	EICRA = (1 << ISC11) | (1 << ISC01);
	EIMSK = (1 << INT1) | (1 << INT0);
}

void Timer0_init(void)
{
	 TCCR0 = 0x00;				 //stop
	 ASSR  = 0x00; 				//set async mode
	 TCNT0 = 0x71; 				//set count
	 OCR0  = 0x8F;				// 143  > Overflow > 0
	 TCCR0 = (1 << WGM01) | (0 << WGM00) | ( 1 << CS02) | ( 1 << CS01) | ( 1 << CS00);   // CTC Mode       Resoltion : 1024
}								// 14.7456 /1024 /144 = 100 Hz , 10mS
 
void PWM_Init(void)
{
	TCCR1B = 0x00; 			//stop	

	ICR1 = 500;				//30KHz 주파수로 설정
	OCR1A = 150;                     
	OCR1B = 250;			//250  // Duty 비 설정  

	TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << COM1B1) | (0 << COM1B0) |				//|모드15 Fast PWM(16-bit) TOP = 500
	 		 (1 << WGM11) | (0 << WGM10);												//| OC1A: 일반적인 I/O, OC1B: PWM출력
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12)  | (0 << CS11) | (0 << CS10);		// 분주비 1
}

void PWM_On(void)
{
	TCCR1B |= (1 << CS10);		// 분주비 1  Timer1 Count Start
}

void PWM_Off(void)
{
	TCCR1B &= 0xf8;			// PWM용 Timer1 Count Stop
	PORTB &= ~0x40;			//PORTB6 Low
}

void PWM_Duty(unsigned int du){
	OCR1B = du;				//Duty 비 설정		Default Value : 250  
}	

void delay_us(unsigned int _t)
{
	register unsigned int i;
	unsigned int time;
	time = _t/6 * Cycles_Us;
	for (i=0;i<time;i++);
}

void delay_ms(unsigned int _t)
{
	register unsigned int i;
	for(i=0;i<_t;i++) delay_us(1000);
}

void EEPROM_Put(char adr,char val)
{
    	while(EECR & 0x02);
    	EEAR = adr;
    	EEDR = val;
    	EECR |= 0x04;
    	EECR |= 0x02;
}

unsigned char EEPROM_Get(char adr)
{
	while(EECR & 0x02);
	EEAR = adr;
	EECR |= 0x01;
	return (EEDR); 
}

void PID_Data_SAVE(void)
{	
	unsigned char i = 0;
	unsigned char P_buff = 0;
	
	for(i = 0; i < 6 ; i++){
		EEPROM_Put(i, PID_Parameter[i]);
	}
	EEPROM_Put(P_err, 1);
}

void PID_Data_Read(void)
{	
	unsigned char i = 0;

	
	for(i = 0; i < 7 ; i++){
		PID_Parameter[i]= EEPROM_Get(i);
	}

	if(PID_Parameter[P_err] != 1){
		PID_Parameter[P_Gain] = 45;
		PID_Parameter[I_Gain] = 5;
		PID_Parameter[D_Gain] = 10;
		PID_Parameter[Decel_Section] = 10;
		PID_Parameter[Decel_Slope] = 25;
		PID_Parameter[Set_Velo] = 35;
	}	
}

void Moving_Delay(unsigned int Delay_Time)
{	
	T10mS_Count = 0;

	Timer0_On();
	while(T10mS_Count < Delay_Time);
	Timer0_Off();	
}	

void BintoASCII(unsigned int bin)   //0xFC00 == -1024, 0x00 == 0, 0x0400 == 1024
{	
	unsigned int bi_tmp;

	if((bin / 10000) != 0){
		bi_tmp = (bin /10000) | 0x30;
		Com2_CharTX(bi_tmp);
	}
	
	bi_tmp = ((bin % 10000) / 1000) | 0x30;
	Com2_CharTX(bi_tmp);
	
	bi_tmp = ((bin % 1000) / 100) | 0x30;
	Com2_CharTX(bi_tmp);

	bi_tmp = ((bin % 100 ) / 10) | 0x30;
	Com2_CharTX(bi_tmp);

	bi_tmp = (bin % 10) | 0x30;
	Com2_CharTX(bi_tmp);
	
	Com2_CharTX(LF);
	Com2_CharTX(CR);
}

void Stop_Mode(void)
{	
	Arm_OFF(Sensor);
	
	switch(PORTC){
		case 0x00:
			Arm_ON(BSide);
			Moving_Delay(100);

			Arm_ON(Center);
			Moving_Delay(100);

			Arm_OFF(BSide);
			break;

		case 0x02:
			Com2_StringTX("Already Stop");
			break;

		case 0x05:
			Arm_OFF(FSide);
			Moving_Delay(100);

			Arm_ON(Center);
			Moving_Delay(100);

			Arm_OFF(BSide);
			break;

		case 0x07:
			Arm_OFF(ASide);

		default :
			All_Arm_OFF();
			Moving_Delay(100);
			
			Arm_ON(BSide);
			Moving_Delay(100);

			Arm_ON(Center);
			Moving_Delay(100);

			Arm_OFF(BSide);
			break;

	}	
}	

void Mode_Prepare(Void)
{	
	Arm_ON(BSide);
	Moving_Delay(100);
	
	Arm_ON(Center);
	Moving_Delay(100);

	Arm_ON(FSide);
}

 void Moving_Foward(void)
{	
	Arm_OFF(Sensor);
	Arm_OFF(BSide);
	Moving_Delay(100);

	Arm_ON(Center);
	Moving_Delay(100);

	Arm_ON(BSide);
	Moving_Delay(100);

	Arm_OFF(FSide);
	Moving_Delay(100);

	Arm_OFF(Center);
	Moving_Delay(100);

	Arm_ON(FSide);
}

void Moving_Reverse(void)
{
	Arm_OFF(Sensor);
	Arm_OFF(FSide);
	Moving_Delay(100);

	Arm_ON(Center);
	Moving_Delay(100);

	Arm_ON(FSide);
	Moving_Delay(100);

	Arm_OFF(BSide);
	Moving_Delay(100);

	Arm_OFF(Center);
	Moving_Delay(100);

	Arm_ON(BSide);
}	
	
void Gyro_cmd_Info(void)
{
	unsigned char cmd = 0;

	Com1_CharTX('q');
		
	cmd = Com1_rd_char();

	while(cmd != 0){
		
		switch(cmd){
			case STX :
				GCount = 0;
				break;

			case ETX : 
				X_data[0] = GFrame[0]; 
				X_data[1] = GFrame[1]; 
				X_data[2] = GFrame[2]; 
				X_data[3] = GFrame[3];
				X_data[4] = 0;
					
				X_Axis = atoi(X_data);
				
				Y_data[0] = GFrame[4]; 
				Y_data[1] = GFrame[5]; 
				Y_data[2] = GFrame[6]; 
				Y_data[3] = GFrame[7]; 
				Y_data[4] = 0;

				Y_Axis = atoi(Y_data);
				
				break;
				
			default :
				GFrame[GCount++] = cmd;
				if(GCount == GFsize)GCount = 0;
				break;
		}
		
		cmd = Com1_rd_char();
	}
}

void Gyro_Data_send(void)
{	
	if((X_Axis !=0) && (Y_Axis !=0)){   
		Com2_CharTX('X');
		Com2_CharTX(':');
		Com2_CharTX(X_data[0]);
		Com2_CharTX(X_data[1]);
		Com2_CharTX(X_data[2]);
		Com2_CharTX(X_data[3]);

		Com2_CharTX(' ');

		Com2_CharTX('Y');
		Com2_CharTX(':');
		Com2_CharTX(Y_data[0]);
		Com2_CharTX(Y_data[1]);
		Com2_CharTX(Y_data[2]);
		Com2_CharTX(Y_data[3]);

		Com2_CharTX(LF);
		Com2_CharTX(CR);
	}	
}

void Command_Info(void)
{
	unsigned char comm = 0;
	unsigned char com_buff = 0;
	unsigned char st[3];
	unsigned char i = 0;
		
	comm = Com2_rd_char();
	while(comm != 0){
		switch(comm){
			case STX :
				FrameCount = 0;
				break;

			case ETX : 				
				for(i = 0; i < 3; i++) st[i]= 0;

				st[0] = Frame[3];
				st[1] = Frame[4];
				com_buff = atoi(st);
				 				
				if(memcmp(Frame, "MS", 2) == 0)Moving_Sequence = Moving_Scan;		// 연속 스캔	                               
				if(memcmp(Frame, "IS", 2) == 0)Moving_Sequence = Init_Scan;             // 초기 스캔
				if(memcmp(Frame, "PR", 2) == 0)Moving_Sequence = Prepare;			// 준비 모드 ( 스캔 / 영점 셋팅 )
				if(memcmp(Frame, "SC", 2) == 0)Moving_Sequence = Scan_Mode;		// Scan Only 
				if(memcmp(Frame, "SE", 2) == 0)Moving_Sequence = Axis_Set;		// 영점 셋팅 
				if(memcmp(Frame, "MF", 2) == 0)Moving_Sequence = Foward_Moving;	// 전진   
				if(memcmp(Frame, "MR", 2) == 0)Moving_Sequence = Reverse_Moving; 	// 후진
				if(memcmp(Frame, "PA", 2) == 0)Moving_Sequence = Pause_Mode;		// 일시 정지 
				if(memcmp(Frame, "RT", 2) == 0)Moving_Sequence = Restart_Mode;	// 일시 정지 후 다시 시작	
				if(memcmp(Frame, "GS", 2) == 0)Moving_Sequence = Gyro_Read;		// 자이로 센서 데이터 리드 
				if(memcmp(Frame, "SP", 2) == 0)Moving_Sequence = Mode_Stop;		// 일시 정지 후 정지	
/*		if(memcmp(Frame, "GC", 2) == 0){
					switch(Frame[2]){
						case "P" : PID_Parameter[P_Gain] = com_buff;
							break;
						case "I" : PID_Parameter[I_Gain] = com_buff;
							break;
						case "D" : PID_Parameter[D_Gain] = com_buff;
							break;
						case "E" : PID_Parameter[Decel_Section] = com_buff;
							break;
						case "O" : PID_Parameter[Decel_Slope] = com_buff;
							break;
						case "S" : PID_Parameter[Set_Velo] = com_buff;
							break;
					}
				}
				if(memcmp(Frame, "SA", 2) == 0)PID_Data_SAVE();
*/				break;
				
			default :
				Frame[FrameCount++] = comm;
				if(FrameCount == Framesize)FrameCount = 0;
				break;
		}
		comm = Com2_rd_char();
	}
}
	
void Zero_Set(Void)
{	
	unsigned long buff_Cnt = 0;
	unsigned char Axis_state = 0;

	PWM_Duty(150);

	Arm_ON(Sensor);

	Moving_Delay(100);

	Gyro_cmd_Info();
	Gyro_cmd_Info();
 
	while(!((X_Axis >= 2038) && (X_Axis <= 2058) && (Y_Axis > 2048))){
				
		if((X_Axis <= 2048) && ( Y_Axis > 2048))Axis_state = 1;
	 	if((X_Axis <= 2048) && ( Y_Axis < 2048))Axis_state = 2;
	 	if((X_Axis > 2048) && ( Y_Axis < 2048))Axis_state = 3;
	 	if((X_Axis > 2048) && ( Y_Axis > 2048))Axis_state = 4;
		
		if(!((X_Axis >= 1748) && (X_Axis <= 2348) && (Y_Axis > 2048))){
			switch(Axis_state){
				case  1:
					Com2_CharTX(Axis_state + 0x34);
					Motor_CCW();
					Motor_Enable();
					while(X_Axis < 1748)Gyro_cmd_Info();
					Motor_Disable();
					 break;

				case  2:
					Com2_CharTX(Axis_state + 0x34);
					Motor_CCW();
					Motor_Enable();
					while(X_Axis > 1500)Gyro_cmd_Info();
					while(X_Axis <1748)Gyro_cmd_Info();
					Motor_Disable();
					 break;

				case  3:
					Com2_CharTX(Axis_state + 0x34);
					Motor_CW();
					Motor_Enable();
					while(X_Axis < 2000)Gyro_cmd_Info();
					while(X_Axis > 2348)Gyro_cmd_Info();
					break;

				case  4:
					Com2_CharTX(Axis_state + 0x34);
					Motor_CW();
					Motor_Enable();
					while(X_Axis > 2348)Gyro_cmd_Info();
					Motor_Disable();
					break;
			}	
		
			Moving_Delay(100);
			Gyro_cmd_Info();
			Gyro_cmd_Info();
		}
		
		Encoder_Count = 0;
		PWM_Duty(100);
		Gyro_cmd_Info();
		Gyro_cmd_Info();
		 
		if((X_Axis < 2038) && (X_Axis >= 1536)){
			X_Axis = 2048 - X_Axis;
			buff_Cnt = (Turn_Count / 4096) * X_Axis;

			Motor_CCW();
			Motor_Enable();
			while(Encoder_Count < buff_Cnt);
			Motor_Disable();

 			Encoder_Count = 0;
			Moving_Delay(100);
			Gyro_cmd_Info();
			Gyro_cmd_Info();
		}


		if((X_Axis > 2058) && (X_Axis <= 2560)){
			X_Axis = X_Axis - 2048;
			buff_Cnt = (Turn_Count / 4096) * X_Axis;

			Motor_CW();
			Motor_Enable();
			while(Encoder_Count < buff_Cnt);
			Motor_Disable();

 			Encoder_Count = 0;
			Moving_Delay(100);
		}
 
		PWM_Duty(250);
		Motor_CW();
		
		Gyro_cmd_Info();
		Gyro_cmd_Info();
		Com2_StringTX("Zero_Set_G2_Data >>  ");
		Gyro_Data_send();
	}
}	

void Sensor_Scan(void)
{	
	Scan_Start();
	T10mS_Count = 0;
	Encoder_Count = 0;
	Velo_Count = 0;
	Scan_Flag = 1;									//Scan Flag == 1 일 경우, Timer0 속도 제어로 동작 

			
	Motor_CW();
	PWM_Duty(250);
	
	if(Sensor != (PORTC & Sensor)){
		Arm_ON(Sensor);
		Moving_Delay(100);
	}	
	 
	Motor_Enable();
 	Timer0_On();
	while(Scan_Flag != 0){													                  		
		Com2_CharTX('D');
		Com2_CharTX(':');
		BintoASCII(OCR1B);
		Com2_CharTX('V');
		Com2_CharTX(':');
		BintoASCII(Velo_Data);    
	}	
	Scan_Flag = 0;

	Com2_CharTX('P');
	Com2_CharTX(':');
	BintoASCII(Encoder_Count);

	Moving_Delay(100);
	Com2_CharTX('S');
	Com2_CharTX(':');
	BintoASCII(Encoder_Count);
	Arm_OFF(Sensor);
	Scan_Stop();
	}	

void Sequence_Select(void)						// Moving Sequence 선택 
{	
	Moving_Sequence_Old = Moving_Sequence;
	switch(Moving_Sequence){
		case Prepare :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Prepare_state; }
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1; 	}
			break;

		case Axis_Set:
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Set_Zero; }
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1;}
			break;	

		case Foward_Moving :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Foward; }	
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1;}
			break;
			
		case Reverse_Moving :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Foward; }	
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1;}
			break;
			
		case Scan_Mode :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Scan; }
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1; 	}
			break;

		case Gyro_Read:
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = GData_Read; }
			if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1; 	}
			break;	
			
		case Init_Scan :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Prepare_state; }	
			if(Sequence_Num == 2){ Moving_Sequence = Continue; Sequence_State = Set_Zero; }	
			if(Sequence_Num == 3){ Moving_Sequence = Continue; Sequence_State = Scan; }	
			if(Sequence_Num > 3){ Moving_Sequence = Wait; Sequence_State = Wait_State; Sequence_Num = 1;}
			break;

		case Moving_Scan :
			if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Foward; }
			if(Sequence_Num == 2){ Moving_Sequence = Continue; Sequence_State = Set_Zero; } 
			if(Sequence_Num == 3){ Moving_Sequence = Continue; Sequence_State = Scan; }	
			if(Sequence_Num > 3){ Moving_Sequence = Moving_Scan; Sequence_State = Wait_State; Sequence_Num = 1; }
			break;

		case Pause_Mode :
			Moving_Sequence_Old = Moving_Sequence;	
			Sequence_Num_Old = Sequence_Num;
			Sequence_Num = 1;
			Moving_Sequence = Wait;
			Sequence_State = Pause;
			Com2_CharTX(STX);
			Com2_StringTX("PA00");
			Com2_CharTX(ETX);
			break;		

		case Restart_Mode :
			if(Sequence_State == Pause){
				Moving_Sequence = Moving_Sequence_Old;
				Sequence_Num = Sequence_Num_Old;
				Com2_CharTX(STX);
				Com2_StringTX("RT00");
				Com2_CharTX(ETX);
			}	
			break;	
			
		case Mode_Stop :
			if((Sequence_State == Wait_State) || (Sequence_State == Pause)){
				if(Sequence_Num == 1){ Moving_Sequence = Continue; Sequence_State = Stop; }
				if(Sequence_Num > 1){ Moving_Sequence = Wait; Sequence_State = Stop_State; Sequence_Num = 1;}
			}
			else{
				Com2_CharTX(STX);
				Com2_StringTX("SP00");
				Com2_CharTX(ETX);
			}
			break;

		
	}
}

void	State_Select(void)								// Movinge Sequence에서의 부분 동작 선택
{	
	PORTE= (Sequence_State << 4) | 0x02;
		
	switch(Sequence_State){
		case Stop :
			Stop_Mode();
			break;

		case Prepare_state : 
			Mode_Prepare(); 
			break;

		case Set_Zero :  
			Zero_Set();
			break;

		case Scan : 
			Sensor_Scan();
			break;
			
		case Foward : 
			Moving_Foward();
			break;
			
		case Reverse : 
			Moving_Reverse();
			break;

		case GData_Read : 
			Gyro_cmd_Info();
			Gyro_Data_send();
			break;
	}

	Com2_CharTX(STX);
	if(Moving_Sequence == Moving_Scan) Com2_StringTX("MS");
	if(Moving_Sequence == Init_Scan) Com2_StringTX("IS");
	if(Moving_Sequence == Axis_Set) Com2_StringTX("SE");
	if(Moving_Sequence == Scan_Mode) Com2_StringTX("SC");
	if(Moving_Sequence == Mode_Stop) Com2_StringTX("SP");
	if(Moving_Sequence == Foward_Moving) Com2_StringTX("MF");
	if(Moving_Sequence == Reverse_Moving) Com2_StringTX("MR");
	if(Moving_Sequence == Prepare) Com2_StringTX("PR");
	if(Moving_Sequence == Gyro_Read) Com2_StringTX("GS");
	Com2_CharTX(Sequence_Num + 0x30);
	Com2_CharTX(ETX);
}
/*
#pragma interrupt_handler INT0_interrupt:iv_INT0
void INT0_interrupt(void)
{
 	Encoder_Count++;
}
*/

#pragma interrupt_handler INT1_interrupt:iv_INT1
void INT1_interrupt(void)
{
	Encoder_Count++;	
	Velo_Count++;
	if(Scan_Flag == 1){
		if(Encoder_Count > Turn_Count){						// Encoder Count 
			Motor_Disable();
			Timer0_Off();
			Scan_Flag = 0;
		}	
	}
}


#pragma interrupt_handler timer0_ovf_isr:iv_TIMER0_OVF
void timer0_ovf_isr(void)
{	
	unsigned int Target_Distance = 0;
	int Err_Data = 0, Err_Old =0, Err_Defer = 0;
	unsigned int Out_Duty = 0;
	unsigned int Cont_Velo = PID_Parameter[Set_Velo];
	
	T10mS_Count++;
	T20mS_Count++;												   // 20ms 간격으로 속도 데이터 수집	
	if(Scan_Flag == 1){
		if(T20mS_Count > 1){
			Velo_Data = Velo_Count;                              			   // 현재 속도값 
			Velo_Count = 0;											
			T20mS_Count = 0;

			Target_Distance = Turn_Count - Encoder_Count;                   // 남은 거리
			if(Target_Distance < Velo_Data)Motor_Disable();			   //현재 속도 값이 남은 거리 보다 작을 경우 제동  	
		
			if(Target_Distance < (PID_Parameter[Decel_Section]* 100)){						  // 감속 구간에서의 속도값 감쇠 연산	
				Cont_Velo = (Cont_Velo * Target_Distance * PID_Parameter[Decel_Slope]) /PID_Parameter[Decel_Section]
//				if(Cont_Velo < 20)Cont_Velo = 0;
			}	
 			Duty_Old = OCR1B;									 	

			Err_Data = Cont_Velo -Velo_Data;						  // 속도의 편차			
 			Err_Defer = Err_Old - Err_Data;							  // 손도 편차의 미분값(속도 편차 값의 편차) 	

			Out_Duty = Duty_Old + (Err_Data * PID_Parameter[P_Gain] /100 + (Velo_Data * PID_Parameter[I_Gain] 	/100 + (Err_Defer * PID_Parameter[D_Gain] /100;
		 			//출력 Duty값 = 기존 Duty + 속도편차 * P_Gain + 현재속도 * I_Gain + 속도편차의 미분값 * D_Gain

			if(Out_Duty > 400)Out_Duty = 400;
  			OCR1B = Out_Duty;
			Err_Old = Err_Data;			     
		}
	}
}
 
void main(void)
{
	Port_Init();	
	EXT_INT_Init();
	Com1_Init(B115200);
	Com2_Init(B19200);
	Timer0_init();
	PWM_Init();
	
	PWM_On();

	SEI();

	PID_Data_Read();
	
	while(1){
		Command_Info();
		Sequence_Select();
		
		if(Moving_Sequence == Continue){
			Moving_Sequence = Moving_Sequence_Old;
			
			State_Select();
			Moving_Delay(100);
			Sequence_Num++;
			
		}
	}
}

