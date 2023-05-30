/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Lukasz Majewski <l.majewski@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <errno.h>

#define I2C_WRITE 0
#define I2C_READ 1

#define I2C_OK		0
#define I2C_NOK		1
#define I2C_NACK	2
#define I2C_NOK_LA	3		/* Lost arbitration */
#define I2C_NOK_TOUT	4		/* time out */

#define I2CSTAT_BSY	0x20	/* Busy bit */
#define I2CSTAT_NACK	0x01		/* Nack bit */
#define I2CCON_IRPND 0x10  /* interrupt pending bit */
#define I2C_MODE_MT	0xC0		/* Master Transmit Mode */
#define I2C_MODE_MR	0x80		/* Master Receive Mode */
#define I2C_START_STOP	0x20		/* START / STOP */
#define I2C_TXRX_ENA	0x10		/* I2C Tx/Rx enable */
#define I2C_TIMEOUT 1 // 1 second

#define S5M8767_I2C_ADDR	(0xCC)
//I2C 1 add by jinhc
#define rIICCON		*((volatile unsigned long *) 0x13870000)
#define rIICSTAT	*((volatile unsigned long *) 0x13870004)
#define rIICADD		*((volatile unsigned long *) 0x13870008)
#define rIICDS		*((volatile unsigned long *) 0x1387000c)
#define rIICIC		*((volatile unsigned long *) 0x13870010)

#define BUCK9_CTRL_ADDR		0x5A
#define LDO18_CTRL_ADDR		0x70


#define CONFIG_PM_VDD_ARM		1.2
#define CONFIG_PM_VDD_INT		1.0
#define CONFIG_PM_VDD_G3D		1.1
#define CONFIG_PM_VDD_MIF		1.1
#define CONFIG_PM_VDD_LDO14		1.8

#define CALC_S5M8767_VOLT1(x)		((x < 600) ? 0 : ((x - 600) / 6.25))
#define CALC_S5M8767_VOLT2(x)		((x < 650) ? 0 : ((x - 650) / 6.25))

typedef enum {
	PMIC_BUCK1 = 0,
	PMIC_BUCK2,
	PMIC_BUCK3,
	PMIC_BUCK4,
	PMIC_LDO14,
	PMIC_LDO10,
	PMIC_BUCK8,
}PMIC_RegNum;
static
int WaitForXfer (void)
{
	int i;
	unsigned long status;

	i = I2C_TIMEOUT * 10000;
	status = rIICCON;
	while ((i > 0) && !(status & I2CCON_IRPND)) {
		udelay (100);
		status = rIICCON;
		i--;
	}

	return (status & I2CCON_IRPND) ? I2C_OK : I2C_NOK_TOUT;
}

static
int IsACK (void)
{
	return (!(rIICSTAT & I2CSTAT_NACK));
}

static
void ReadWriteByte (void)
{
	rIICCON &= ~I2CCON_IRPND;
}

/*
 * cmd_type is 0 for write, 1 for read.
 *
 * addr_len can take any value from 0-255, it is only limited
 * by the char, we could make it larger if needed. If it is
 * 0 we skip the address write cycle.
 */
static
int i2c_transfer (unsigned char cmd_type,
		  unsigned char chip,
		  unsigned char addr[],
		  unsigned char addr_len,
		  unsigned char data[], unsigned short data_len)
{
	int i, status, result;

	if (data == 0 || data_len == 0) {

		return I2C_NOK;
	}

	i = I2C_TIMEOUT * 1000;
	status = rIICSTAT;
	while ((i > 0) && (status & I2CSTAT_BSY)) {
		udelay (1000);
		status = rIICSTAT;
		i--;
	}

	if (status & I2CSTAT_BSY)
		return I2C_NOK_TOUT;

	rIICCON |= 0x80;
	result = I2C_OK;

	switch (cmd_type) {
	case I2C_WRITE:
		if (addr && addr_len) {
			rIICDS = chip;

			rIICSTAT = I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP;
			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				result = WaitForXfer ();
				rIICDS = addr[i];
				ReadWriteByte ();
				i++;
			}
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				result = WaitForXfer ();
				rIICDS = data[i];
				ReadWriteByte ();
				i++;
			}
		} else {
			rIICDS = chip;
			rIICSTAT = I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP;
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				result = WaitForXfer ();
				rIICDS = data[i];
				ReadWriteByte ();
				i++;
			}
		}

		if (result == I2C_OK)
			result = WaitForXfer ();

		rIICSTAT = I2C_MODE_MR | I2C_TXRX_ENA;
		ReadWriteByte ();
		break;
	case I2C_READ:
		rIICSTAT = I2C_MODE_MT | I2C_TXRX_ENA ;
		rIICDS = chip;
		rIICSTAT |= I2C_START_STOP;
		i = 0;
		result = WaitForXfer ();
		if (IsACK ()) {			

				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
					rIICDS = addr[i];
					ReadWriteByte ();
					result = WaitForXfer ();
					i++;
				}

				rIICDS = chip;
				rIICSTAT =  I2C_MODE_MR | I2C_TXRX_ENA |
						I2C_START_STOP;
				
				ReadWriteByte ();

				result = WaitForXfer ();
				i = 0;

				while ((i < data_len) && (result == I2C_OK)) {
					if (i == data_len - 1)
						rIICCON &= ~0x80;
					ReadWriteByte ();
					result = WaitForXfer ();
					data[i] = rIICDS;
					i++;
				}
				
			} else {
				result = I2C_NACK;
		}
			
			rIICSTAT = I2C_MODE_MR | I2C_TXRX_ENA;
			ReadWriteByte ();
		break;
	default:
		result = I2C_NOK;
		break;
	}

	return (result);
}
static int i2c_read(unsigned char chip, unsigned int addr,
		int alen, unsigned char *buffer, int len)
{
	unsigned char xaddr[4];
	int ret;
	if (alen > 4)
		return 1;

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}

	if ((ret = i2c_transfer(I2C_READ, chip, &xaddr[4 - alen], alen,
					buffer, len)) != 0) {
		return 1;
	}
	return 0;
}
static int i2c_write(unsigned char chip, unsigned int addr,
		int alen, unsigned char *buffer, int len)
{
	unsigned char xaddr[4];
	int ret;

	if (alen > 4)
		return 1;
	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
#if 0
		printf("xaddr[3] = %02x\n", xaddr[3]);
#endif
	}

	return(i2c_transfer(I2C_WRITE, chip, &xaddr[4 - alen], alen,
					buffer, len) != 0); 

}
static void i2c1_init(void) { 
	int i, status; 
	volatile unsigned int* gpd1_con_reg = (unsigned int*)0x114000C0; //GPD1CON
	unsigned int value;

	value = *gpd1_con_reg;
	value &= ~(0xFF<<8); //clear bits gpio_reg1 [15:7]
	value |= 0x2200; //set GPD1CON2 & GPD1CON3 as I2C_1_SDA I2C_1_SCL
	*gpd1_con_reg = value;


	/*wait for some time to give previous transfer a chance to finish */
	i = I2C_TIMEOUT * 1000;
	status = rIICCON;
	while ((i > 0) && !(status & I2CSTAT_BSY)) {
		udelay(1000);
		status = rIICSTAT;
		i--;
	}

	rIICCON = (1 << 6) | (1 << 5) | (7&0xF);
	rIICSTAT = 0;
	rIICADD	= S5M8767_I2C_ADDR >> 1;  //7-bit slave address Slave address[7:1]
	
	rIICSTAT = I2C_MODE_MT | I2C_TXRX_ENA;
}

void lowlevel_init_s5m8767(unsigned char Address, unsigned char *Val, int flag)
{
	unsigned char addr;
	unsigned char data[4] = {0, 0, 0, 0};
	unsigned char write_data = 0;
	int ret;

	addr = Address;
	data[0] = *Val;
	write_data = *Val;

	//flag == 0 is read reg, flag==1 is write reg;
	if (flag == 0){
		ret = i2c_read(S5M8767_I2C_ADDR, addr, 1, data, 1);
		*Val = data[0];
		//printf("s5m8767: addr 0x%x, value 0x%x\n", addr, data[0]);
	}else {
		ret = i2c_write(S5M8767_I2C_ADDR, addr, 1, data, 1);
		data[0] = 0;
#if 0
		ret = i2c_read(S5M8767_I2C_ADDR, addr, 1, data, 1);
		*Val = data[0];
		printf("s5m8767: addr 0x%x, write value 0x%x, read value0x%x\n", addr, write_data, data[0]);
#endif
	}
}

void I2C_S5M8767_VolSetting(PMIC_RegNum eRegNum, unsigned char ucVolLevel, unsigned char unEnable)
{
	unsigned char reg_addr, vol_level;

	switch(eRegNum) {
		case PMIC_BUCK1: reg_addr = 0x33; break;
		case PMIC_BUCK2: reg_addr = 0x35; break;
		case PMIC_BUCK3: reg_addr = 0x3E; break;
		case PMIC_BUCK4: reg_addr = 0x47; break;
		case PMIC_BUCK8: reg_addr = 0x59; break;
		default:
				while(1);
	}
	vol_level = ucVolLevel&0xFF;

	lowlevel_init_s5m8767(reg_addr, &vol_level, 1);
}

void s5m8767_voltage_set(void)
{
	float vdd_arm, vdd_int, vdd_g3d;
	float vdd_mif;
	unsigned char read_data;

	vdd_arm = CONFIG_PM_VDD_ARM;
	vdd_int = CONFIG_PM_VDD_INT;
	vdd_g3d = CONFIG_PM_VDD_G3D;
	vdd_mif = CONFIG_PM_VDD_MIF;

	I2C_S5M8767_VolSetting(PMIC_BUCK1, CALC_S5M8767_VOLT2(vdd_mif * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK2, CALC_S5M8767_VOLT1(vdd_arm * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK3, CALC_S5M8767_VOLT1(vdd_int * 1000), 1);
	I2C_S5M8767_VolSetting(PMIC_BUCK4, CALC_S5M8767_VOLT1(vdd_g3d * 1000), 1);
	//set BUCK8 to 1.55v, because LDO2's out decide by BUCK8
	I2C_S5M8767_VolSetting(PMIC_BUCK8, CALC_S5M8767_VOLT1(1.55 * 1000), 1);
	
}
int  Is_TC4_Dvt = 0;
void pmic_s5m8767_init(void)
{
	unsigned char id;
	unsigned int val;

	/* init i2c_1 */
	i2c1_init();	
	/* read s5m8767's chip id */
	lowlevel_init_s5m8767(0, &id, 0);
#if 0
	printf("s5m8767 chip id = %02x\n", id);
#endif
	if (id == 0x15) {
		printf("PMIC: S5M8767(VER5.0)\n");
		s5m8767_voltage_set();
		Is_TC4_Dvt = 2;
	}
	if (Is_TC4_Dvt == 2) {
		val = 0x58;
		lowlevel_init_s5m8767(BUCK9_CTRL_ADDR, &val, 1);
	}
	val = 0x32;
	lowlevel_init_s5m8767(LDO18_CTRL_ADDR, &val, 1);
}
