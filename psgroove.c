/*
    PSGroove Exploit

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include <LUFA/Version.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/CDC.h>

#include "descriptor.h"

// Teensy board only has the first LED, so it will turn off when 
// exploit succeeds.
#define RED		(LEDS_LED1)
#define GREEN	(LEDS_LED2)
#define BOTH	(RED|GREEN)
#define NONE	(LEDS_NO_LEDS)
#define LED(x)	LEDs_SetAllLEDs(x)

#define PORT_EMPTY 0x0100   /* powered only */
#define PORT_FULL 0x0103    /* connected, enabled, powered, full-speed */
#define C_PORT_CONN  0x0001 /* connection */
#define C_PORT_RESET 0x0010 /* reset */
#define C_PORT_NONE  0x0000 /* no change */
uint16_t port_status[6] = { PORT_EMPTY, PORT_EMPTY, PORT_EMPTY, PORT_EMPTY, PORT_EMPTY, PORT_EMPTY };
uint16_t port_change[6] = { C_PORT_NONE, C_PORT_NONE, C_PORT_NONE, C_PORT_NONE, C_PORT_NONE, C_PORT_NONE };
enum { 
	init,
	wait_hub_ready,
	hub_ready,
	p1_wait_reset,
	p1_wait_enumerate,
	p1_ready,
	p2_wait_reset,
	p2_wait_enumerate,
	p2_ready,
	p3_wait_reset,
	p3_wait_enumerate,
	p3_ready,
	p2_wait_disconnect,
	p4_wait_connect,
	p4_wait_reset,
	p4_wait_enumerate,
	p4_ready,
	p5_wait_reset,
	p5_wait_enumerate,
	p5_challenged,
	p5_responded,
	p3_wait_disconnect,
	p3_disconnected,
	p5_wait_disconnect,
	p5_disconnected,
	p4_wait_disconnect,
	p4_disconnected,
	p1_wait_disconnect,
	p1_disconnected,
	done,
} state = init;

uint8_t hub_int_response = 0x00;
uint8_t hub_int_force_data0 = 0;
int last_port_conn_clear = 0;
int last_port_reset_clear = 0;

int8_t port_addr[7] = { -1, -1, -1, -1, -1, -1, -1 };
int8_t port_cur = -1;

void USB_Device_SetDeviceAddress(uint8_t Address)
{
	port_addr[port_cur] = Address & 0x7f;
	UDADDR = Address & 0x7f;
	UDADDR |= (1 << ADDEN);
}

void switch_port(int8_t port)
{
	if (port_cur == port) return;
	port_cur = port;
	if (port_addr[port] < 0)
		port_addr[port] = 0;
	UDADDR = port_addr[port] & 0x7f;
	UDADDR |= (1 << ADDEN);
}

volatile uint8_t expire = 0; /* counts down every 10 milliseconds */
volatile uint8_t expire_led = 0; /* counts down every 10 milliseconds */
ISR(TIMER1_OVF_vect) 
{ 
	uint16_t rate = (uint16_t) -(F_CPU / 64 / 100);
	TCNT1H = rate >> 8;
	TCNT1L = rate & 0xff;
	if (expire > 0)
		expire--;
	if (expire_led > 0) {
          expire_led--;
          if (expire_led == 0 && state != done)
            LED (RED);
        }
}

void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Setup timer */
	TCCR1B = 0x03;  /* timer rate clk/64 */
	TIMSK1 = 0x01;

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();
	sei(); 
}

void panic(int led1, int led2)
{
	for(;;) {
		_delay_ms(100);
		LED(led1);
		_delay_ms(100);
		LED(led2);
	}		
}

void HUB_Task(void)
{
	Endpoint_SelectEndpoint(1);

	if (Endpoint_IsReadWriteAllowed())
	{
		if (hub_int_response) {
			if (hub_int_force_data0) {
				Endpoint_ResetDataToggle();
				hub_int_force_data0 = 0;
			}
			Endpoint_Write_Byte(hub_int_response);
			Endpoint_ClearIN();
			hub_int_response = 0x00;
		}
	}
}


void JIG_Task(void)
{
	static int bytes_out = 0, bytes_in = 0;

        Endpoint_SelectEndpoint(2);
        if (Endpoint_IsReadWriteAllowed())
        {
		Endpoint_Discard_Stream(8, NO_STREAM_CALLBACK);
                Endpoint_ClearOUT();
		bytes_out += 8;
		if (bytes_out >= 64) {
			state = p5_challenged;
			expire = 50; // was 90
		}
	}

        Endpoint_SelectEndpoint(1);
        if (Endpoint_IsReadWriteAllowed() && state == p5_challenged && expire == 0) 
	{
		if (bytes_in < 64) {
			Endpoint_Write_PStream_LE(&jig_response[bytes_in], 8, NO_STREAM_CALLBACK);
			Endpoint_ClearIN();
			bytes_in += 8;
			if (bytes_in >= 64) {
				state = p5_responded;
				expire = 15;
			}
		}
	}
}

void connect_port(int port)
{
	last_port_reset_clear = 0;
	hub_int_response = (1 << port);
	port_status[port - 1] = PORT_FULL;
	port_change[port - 1] = C_PORT_CONN;
}

void disconnect_port(int port)
{
	last_port_conn_clear = 0;
	hub_int_response = (1 << port);
	port_status[port - 1] = PORT_EMPTY;
	port_change[port - 1] = C_PORT_CONN;
}

int main(void)
{
	SetupHardware();

	LED(RED);

	state = init;
	switch_port(0);

	for (;;)
	{
		if (port_cur == 0)
			HUB_Task();

		if (port_cur == 5)
			JIG_Task();

		USB_USBTask();
		
		// connect 1
		if (state == hub_ready && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			connect_port(1);
			state = p1_wait_reset;
		}
		
		if (state == p1_wait_reset && last_port_reset_clear == 1)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(1);
			state = p1_wait_enumerate;
		}

		// connect 2
		if (state == p1_ready && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			connect_port(2);
			state = p2_wait_reset;
		}

		if (state == p2_wait_reset && last_port_reset_clear == 2)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(2);
			state = p2_wait_enumerate;
		}

		// connect 3
		if (state == p2_ready && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			connect_port(3);
			state = p3_wait_reset;
		}

		if (state == p3_wait_reset && last_port_reset_clear == 3)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(3);
			state = p3_wait_enumerate;
		}

		// disconnect 2
		if (state == p3_ready && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			disconnect_port(2);
			state = p2_wait_disconnect;
		}

		if (state == p2_wait_disconnect && last_port_conn_clear == 2)
		{
			LED(GREEN);
                        expire_led = 10;
			state = p4_wait_connect;
			expire = 15;
		}

		// connect 4
		if (state == p4_wait_connect && expire == 0) 
		{
			LED(GREEN);
                        expire_led = 10;
			connect_port(4);
			state = p4_wait_reset;
		}

		if (state == p4_wait_reset && last_port_reset_clear == 4)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(4);
			state = p4_wait_enumerate;
		}

		// connect 5
		if (state == p4_ready && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			/* When first connecting port 5, we need to
			   have the wrong data toggle for the PS3 to
			   respond */
			hub_int_force_data0 = 1;
			connect_port(5);
			state = p5_wait_reset;
		}

		if (state == p5_wait_reset && last_port_reset_clear == 5)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(5);
			state = p5_wait_enumerate;
		}

		// disconnect 3
		if (state == p5_responded && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			/* Need wrong data toggle again */
			hub_int_force_data0 = 1;
			disconnect_port(3);
			state = p3_wait_disconnect;
		}

		if (state == p3_wait_disconnect && last_port_conn_clear == 3)
		{
			LED(GREEN);
                        expire_led = 10;
			state = p3_disconnected;
			expire = 45;
		}

		// disconnect 5
		if (state == p3_disconnected && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			disconnect_port(5);
			state = p5_wait_disconnect;
		}

		if (state == p5_wait_disconnect && last_port_conn_clear == 5)
		{
			LED(GREEN);
                        expire_led = 10;
			state = p5_disconnected;
			expire = 20;
		}

		// disconnect 4
		if (state == p5_disconnected && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			disconnect_port(4);
			state = p4_wait_disconnect;
		}

		if (state == p4_wait_disconnect && last_port_conn_clear == 4)
		{
			LED(GREEN);
                        expire_led = 10;
			state = p4_disconnected;
			expire = 20;
		}

		// disconnect 1
		if (state == p4_disconnected && expire == 0)
		{
			LED(GREEN);
                        expire_led = 10;
			switch_port(0);
			disconnect_port(1);
			state = p1_wait_disconnect;
		}

		if (state == p1_wait_disconnect && last_port_conn_clear == 1)
		{
			state = done;
			LED(GREEN);
		}
	}
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);
	const uint16_t  wLength = USB_ControlRequest.wLength;

	void*          Address = NULL;
	uint16_t       Size    = NO_DESCRIPTOR;

	switch (DescriptorType)
	{
	case DTYPE_Device:
		switch (port_cur) {
		case 0:
			Address = (void *) HUB_Device_Descriptor;
			Size    = sizeof(HUB_Device_Descriptor);
			break;
		case 1:
			Address = (void *) port1_device_descriptor;
			Size    = sizeof(port1_device_descriptor);
			break;
		case 2:
			Address = (void *) port2_device_descriptor;
			Size    = sizeof(port2_device_descriptor);
			break;
		case 3:
			Address = (void *) port3_device_descriptor;
			Size    = sizeof(port3_device_descriptor);
			break;
		case 4:
			Address = (void *) port4_device_descriptor;
			Size    = sizeof(port4_device_descriptor);
			break;
		case 5:
			Address = (void *) port5_device_descriptor;
			Size    = sizeof(port5_device_descriptor);
			break;
		}
		break;
	case DTYPE_Configuration: 
		switch (port_cur) {
		case 0:
			Address = (void *) HUB_Config_Descriptor;
			Size    = sizeof(HUB_Config_Descriptor);
			break;
		case 1:
			// 4 configurations are the same.
			// For the initial 8-byte request, we give a different
			// length response than in the full request.
			if (DescriptorNumber < PORT1_NUM_CONFIGS) {
				if (wLength == 8) {
					Address = (void *) port1_short_config_descriptor;
					Size    = sizeof(port1_short_config_descriptor);
				} else {
					Address = (void *) port1_config_descriptor;
					Size    = PORT1_DESC_LEN;
				}
				if (DescriptorNumber == (PORT1_NUM_CONFIGS - 1) &&
                                    wLength > 8) {
					state = p1_ready;
					expire = 10;
				}
			}
			break;
		case 2:
			// only 1 config
			Address = (void *) port2_config_descriptor;
			Size    = sizeof(port2_config_descriptor);
			state = p2_ready;
			expire = 15;
			break;
		case 3:
			// 2 configurations are the same
			Address = (void *) port3_config_descriptor;
			Size    = sizeof(port3_config_descriptor);
			if (DescriptorNumber == 1 && wLength > 8) {
				state = p3_ready;
				expire = 10;
			}
			break;
		case 4:
			// 3 configurations
			if (DescriptorNumber == 0) {
				Address = (void *) port4_config_descriptor_1;
				Size    = sizeof(port4_config_descriptor_1);
			} else if (DescriptorNumber == 1) {
				if (wLength == 8) {
					Address = (void *) port4_short_config_descriptor_2;
					Size    = sizeof(port4_short_config_descriptor_2);
				} else {
					Address = (void *) port4_config_descriptor_2;
					Size    = sizeof(port4_config_descriptor_2);
				}
			} else if (DescriptorNumber == 2) {
				Address = (void *) port4_config_descriptor_3;
				Size    = sizeof(port4_config_descriptor_3);
				if (wLength > 8) {
					state = p4_ready;
					expire = 20;  // longer seems to help this one?
				}
			}
			break;
		case 5:
			// 1 config
			Address = (void *) port5_config_descriptor;
			Size    = sizeof(port5_config_descriptor);
			break;
		}
		break;
	case 0x29: // HUB descriptor (always to port 0 we'll assume)
		switch (port_cur) {
		case 0:
			Address = (void *) HUB_Hub_Descriptor;
			Size    = sizeof(HUB_Hub_Descriptor);
			break;
		}
		break;
	}
	
	*DescriptorAddress = Address;
	return Size;
}

void EVENT_USB_Device_Connect(void) { }
void EVENT_USB_Device_Disconnect(void) { }

void EVENT_USB_Device_UnhandledControlRequest(void)
{
	if (port_cur == 5 && USB_ControlRequest.bRequest == REQ_SetInterface)
	{
		/* can ignore this */
		Endpoint_ClearSETUP();
		Endpoint_ClearIN();
		Endpoint_ClearStatusStage();
		return;
	}

	if (port_cur == 0 &&
	    USB_ControlRequest.bmRequestType == 0xA0 &&
	    USB_ControlRequest.bRequest == 0x00 &&  // GET HUB STATUS
	    USB_ControlRequest.wValue == 0x00 &&
	    USB_ControlRequest.wIndex == 0x00 &&
	    USB_ControlRequest.wLength == 0x04) {
		Endpoint_ClearSETUP();
		Endpoint_Write_Word_LE(0x0000); // wHubStatus
		Endpoint_Write_Word_LE(0x0000); // wHubChange
		Endpoint_ClearIN();
		Endpoint_ClearStatusStage();
		return;
	}

	if (port_cur == 0 &&
	    USB_ControlRequest.bmRequestType == 0xA3 &&  
	    USB_ControlRequest.bRequest == 0x00 &&   //  GET PORT STATUS
	    USB_ControlRequest.wValue == 0x00 &&
	    USB_ControlRequest.wLength == 0x04) {
		uint8_t p = USB_ControlRequest.wIndex;
		if (p < 1 || p > 6) return;

		Endpoint_ClearSETUP();
		Endpoint_Write_Word_LE(port_status[p - 1]); // wHubStatus
		Endpoint_Write_Word_LE(port_change[p - 1]); // wHubChange
		Endpoint_ClearIN();
		Endpoint_ClearStatusStage();
		return;
	}

	if (port_cur == 0 &&
	    USB_ControlRequest.bmRequestType == 0x23 &&
	    USB_ControlRequest.bRequest == 0x03 && // SET_FEATURE
	    USB_ControlRequest.wLength == 0x00) {
		uint8_t p = USB_ControlRequest.wIndex;
		if (p < 1 || p > 6) return;
		
		Endpoint_ClearSETUP();
		Endpoint_ClearIN();
		Endpoint_ClearStatusStage();

		switch(USB_ControlRequest.wValue) {
		case 0x0008: // PORT_POWER
			if (p == 6 && state == init) {
				/* after the 6th port is powered, wait a bit and continue */
				state = hub_ready;
				expire = 15;
			}
			break;
		case 0x0004: // PORT_RESET
			hub_int_response = (1 << p);
			port_change[p - 1] |= C_PORT_RESET;
			break;
		}
		return;
	}

	if (port_cur == 0 &&
	    USB_ControlRequest.bmRequestType == 0x23 &&
	    USB_ControlRequest.bRequest == 0x01 && // CLEAR_FEATURE
	    USB_ControlRequest.wLength == 0x00) {
		uint8_t p = USB_ControlRequest.wIndex;
		if (p < 1 || p > 6) return;
		
		Endpoint_ClearSETUP();
		Endpoint_ClearIN();
		Endpoint_ClearStatusStage();

		switch(USB_ControlRequest.wValue) {
		case 0x0010: // C_PORT_CONNECTION
			port_change[p - 1] &= ~C_PORT_CONN;
			last_port_conn_clear = p;
			break;
		case 0x0014: // C_PORT_RESET
			port_change[p - 1] &= ~C_PORT_RESET;
			last_port_reset_clear = p;
			break;
		}
		return;
	}

	panic(RED, GREEN);
}

void EVENT_USB_Device_ConfigurationChanged(void)
{ 
	/* careful with endpoints: we don't reconfigure when "switching ports"
	   so we need the same configuration on all of them */
	if (!Endpoint_ConfigureEndpoint(1, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN, 8, ENDPOINT_BANK_SINGLE))
		panic(GREEN, BOTH);
	if (!Endpoint_ConfigureEndpoint(2, EP_TYPE_INTERRUPT, ENDPOINT_DIR_OUT, 8, ENDPOINT_BANK_SINGLE))
		panic(GREEN, BOTH);
}

void EVENT_USB_Device_Suspend(void) { }
void EVENT_USB_Device_WakeUp(void) { }
void EVENT_USB_Device_Reset(void) { }
void EVENT_USB_Device_StartOfFrame(void) { }
void EVENT_USB_InitFailure(const uint8_t ErrorCode) { }
void EVENT_USB_UIDChange(void) {}

