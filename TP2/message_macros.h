#ifndef MESSAGE_MACROS_H
#define MESSAGE_MACROS_H


/*  Supervisao e nao numeradas  */
/* [F,A,C,BCC1(A^C),F] */
 
#define FLAG 			0x7E

#define A_RECEIVER 		0x01
#define A_SENDER        0x03

#define C_SET        	0x03
#define C_DISC          0x0B
#define C_UA            0x07
#define C_RR_1          0x05
#define C_RR_2          0x85
#define C_REJ_1         0x01
#define C_REJ_2         0x81
 
#define C_INFO1         0x40
#define C_INFO2         0x00

#define ESC             0x7D
#define SEQ_1           0x5E
#define SEQ_2           0x5D
 
#endif