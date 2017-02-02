/*   Copyright February 2017 - havencking@gmail.com
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This is for a Wii Classic Controller adapted to connect via I2C as a 
 * Raspberry Pi (3) virtual keyboard.  See http://havencking.blogspot.com 
 * for more info.
 *
 * For more info about the Wii Classic Controller, see:
 * http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Classic_Controller
 * 
 * For more info about the wiringPi library, see:  http://wiringpi.com/
 * 
 * For more info about libsuinput, see:  http://tjjr.fi/sw/libsuinput/
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <string.h>
#include <suinput.h>

// delay in microseconds after I2C transactions
#define I2CWRITEDELAY	500
#define I2CREADDELAY	500
#define I2CQUERYDELAY	5000

int main(void) {
	
	// U, D, L, R, A, B, X, Y, Start, Select, Home, LT, RT, ZL, ZR
	int keys[] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_X, KEY_Z, KEY_S, KEY_A, KEY_J, KEY_G, KEY_H, KEY_Q, KEY_P, KEY_W, KEY_O};
	int buttons[21]; 
	int prev_buttons[21]; // track button state
	int p;
	for (p=0; p<21; p++){
		prev_buttons[p]=0;
		buttons[p]=0;
	}

	// initialize wiringPi
	wiringPiSetup();
	int I2C_fd = wiringPiI2CSetup(0x52); 
	if (I2C_fd < 0) {
		printf("Error setting up I2C: %d\n", errno);
		exit(0);
	}
	// initialize Wii I2C using "the new way"
	// see: http://wiibrew.org/wiki/Wiimote/Extension_Controllers
	wiringPiI2CWriteReg8(I2C_fd, 0xF0, 0x55);
	delayMicroseconds(I2CWRITEDELAY);
	wiringPiI2CWriteReg8(I2C_fd, 0xFB, 0x00);
	delayMicroseconds(I2CWRITEDELAY);

	// initialize uinput
	struct uinput_user_dev user_dev;
	memset(&user_dev, 0, sizeof(struct uinput_user_dev));
	strcpy(user_dev.name, "wiiclassic-virtual-keyboard");
	int uinput_fd = suinput_open();
	int i;
	for (i = 0; i < 15; ++i) {
		suinput_enable_event(uinput_fd, EV_KEY, keys[i]);
	}
	suinput_create(uinput_fd, &user_dev);
	
	while(1){
		// read raw controller data starting at the beginning
		wiringPiI2CWrite(I2C_fd, 0x00);
		delayMicroseconds(I2CWRITEDELAY);
		
		int I2C_data[6];
		int j;
		for (j=0; j<6; j++) {
			I2C_data[j] = wiringPiI2CRead(I2C_fd);
			delayMicroseconds(I2CREADDELAY);
		}
	
//uncomment to debug	
//printf("%x %x %x %x %x %x\n",I2C_data[0],I2C_data[1],I2C_data[2],I2C_data[3],I2C_data[4],I2C_data[5]);

		// parse the controller data
		// digital controls: U, D, L, R, A, B, X, Y, Start, Select, Home, LT, RT, ZL, ZR
		buttons[0] = ~(I2C_data[5]) & 0b00000001;
		buttons[1] = ~((I2C_data[4] >> 6)) & 0b00000001;
		buttons[2] = ~((I2C_data[5] >> 1)) & 0b00000001;
		buttons[3] = ~((I2C_data[4] >> 7)) & 0b00000001;
		buttons[4] = ~((I2C_data[5] >> 4)) & 0b00000001;
		buttons[5] = ~((I2C_data[5] >> 6)) & 0b00000001;
		buttons[6] = ~((I2C_data[5] >> 3)) & 0b00000001;
		buttons[7] = ~((I2C_data[5] >> 5)) & 0b00000001;
		buttons[8] = ~((I2C_data[4] >> 2)) & 0b00000001;
		buttons[9] = ~((I2C_data[4] >> 4)) & 0b00000001;
		buttons[10] = ~((I2C_data[4] >> 3)) & 0b00000001;
		buttons[11] = ~((I2C_data[4] >> 5)) & 0b00000001;
		buttons[12] = ~((I2C_data[4] >> 1)) & 0b00000001;
		buttons[13] = ~((I2C_data[5] >> 7)) & 0b00000001;
		buttons[14] = ~((I2C_data[5] >> 2)) & 0b00000001;
		// for future use
		// analog controls: LX, LY, LT, RX, RY, RT
		//buttons[15] = I2C_data[0] & 0b00111111;
		//buttons[16] = I2C_data[1] & 0b00111111;
		//buttons[17] = ((I2C_data[2] >> 2) & 0b00011000) | ((I2C_data[3] >> 5) & 0b00000111);
		//buttons[18] = ((I2C_data[0] >> 3) & 0b00011000) | ((I2C_data[1] >> 5) & 0b00000110) | ((I2C_data[2] >> 7) & 0b00000001);
		//buttons[19] = I2C_data[2] & 0b00011111;
		//buttons[20] = I2C_data[3] & 0b00011111;

//uncomment to debug
//printf("U=%x D=%x L=%x R=%x A=%x B=%x X=%x Y=%x start=%x sel=%x home=%x L=%x R=%x ZL=%x ZR=%x\n",buttons[0],buttons[1],buttons[2],buttons[3],buttons[4],buttons[5],buttons[6],buttons[7],buttons[8],buttons[9],buttons[10],buttons[11],buttons[12],buttons[13],buttons[14]);
		
		// send uinput keys
		int k;
		for (k=0; k<15; k++){
			if (prev_buttons[k] != buttons[k]){
				suinput_emit(uinput_fd, EV_KEY, keys[k], buttons[k]);
				prev_buttons[k] = buttons[k];
			}
		}
		suinput_syn(uinput_fd);
		
		// delay between reading button state
		delayMicroseconds(I2CQUERYDELAY);
	}
	
	suinput_destroy(uinput_fd);

	return 0;
}


