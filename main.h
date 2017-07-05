#define	SOH 				0x01
#define	STX 				0x02
#define	ETX 				0x03
#define	EOT 				0x04
#define	ENO 				0x05
#define	ACK 				0x06
#define	BS 					0x08	// Back space
#define	LF 					0x0a	// Line feed
#define	VT 					0x0b  	// Home
#define	FF 					0x0c	// form feed
#define	CR 					0x0d	//carriage return
#define	NAK 				0x15
#define	EM 					0x19

#define	Main_XTAL			14745600L
#define   Cycles_Us     		(Main_XTAL/1340000)

#define	Timer0_On()			TIMSK |= (1 << OCIE0)
#define	Timer0_Off()			TIMSK &= ~(1 << OCIE0)

////////PORTB////////////////
#define	CW_CCW			7
#define	PWM				6
#define	ENABLE				4
#define	SCK					1

#define	Motor_Enable()		PORTB |= 0x10
#define	Motor_Disable()		PORTB &= ~0x10

#define	Motor_CW()			PORTB |= 0x80
#define 	Motor_CCW()			PORTB &= ~0x80

////////PORTC////////////////
#define	Arm_ON(Sol)			PORTC |= Sol
#define	Arm_OFF(Sol)		PORTC &= ~Sol
#define	All_Arm_OFF()		PORTC = 0x00

#define	Sol_Valve8			7
#define	Sol_Valve7			6
#define	Sol_Valve6			5
#define	Sol_Valve5			4
#define	Sol_Valve4			3		// Sensor	Arm
#define	Sol_Valve3			2		// Front	Side Arms
#define	Sol_Valve2			1		// Center	Arm
#define	Sol_Valve1			0		// Back Side  Arms

#define	Sensor			0x08
#define	ASide			0x05
#define	FSide			0x04
#define	Center			0x02
#define	BSide			0x01

////////PORTD////////////////
#define	Encoder_A			0
#define	Encoder_B			1
#define	RXD_1				2
#define	TXD_1				3

////////PORTE////////////////
#define	RXD					0
#define	TXD					1
#define	STATE1				2
#define	STATE2				3
#define	STATE3				4
#define	STATE4				5
#define	STATE5				6
#define	STATE6				7

#define	Scan_Start()			PORTE |= (1 << STATE1);
#define	Scan_Stop()			PORTE &= ~( 1 << STATE1);

////////PORTF////////////////
#define	SENSE_A			0


/////// Sequence_Mode		///////////////////

#define	Wait				1
#define	Continue			2
#define	Prepare				3
#define	Axis_Set			4
#define	Init_Scan			5
#define	Moving_Scan		6
#define	Scan_Mode			7
#define	Mode_Stop			8
#define	Foward_Moving		9
#define	Reverse_Moving		10
#define	Pause_Mode			11
#define	Restart_Mode		12
#define	Gyro_Read			13


////////////////////////////////////////////////

/////// Sequence_state		///////////////////

#define	Wait_State			0x00
#define	Stop				0x01
#define	Prepare_state		0x02
#define	Set_Zero			0x03
#define	Scan				0x04
#define	Foward				0x05
#define	Reverse				0x06
#define	GData_Read			0x07
#define	Pause				0x08
#define	Stop_State			0x09	
//#define	Restart				0x10

////////////////////////////////////////////////

/////// PID_Controll			////////////////////

#define	P_Gain				0
#define	I_Gain				1
#define	D_Gain				2
#define	Decel_Section		3
#define	Decel_Slope			4
#define	Set_Velo			5
#define	P_err				6

/////////////////////////////////////////////////

extern void delay_us(unsigned int _t);
extern void delay_ms(unsigned int _t);
