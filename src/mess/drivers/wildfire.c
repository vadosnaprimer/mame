// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

  Parker Brothers Wildfire, by Bob and Holly Doyle (prototype), and Garry Kitchen
  * AMI S2150, labeled C10641

  This is an electronic handheld pinball game. It has dozens of small leds
  to create the illusion of a moving ball, and even the flippers are leds.
  A drawing of a pinball table is added as overlay.

  NOTE!: MESS external artwork is required to be able to play


  TODO:
  - no sound
  - flipper buttons aren't working correctly
  - some 7segs digits are wrong (mcu on-die decoder is customizable?)
  - MCU clock is unknown

***************************************************************************/

#include "emu.h"
#include "cpu/amis2000/amis2000.h"
#include "sound/speaker.h"

#include "wildfire.lh" // this is a test layout, external artwork is necessary

// master clock is a single stage RC oscillator: R=?K, C=?pf,
// S2150 default frequency is 850kHz
#define MASTER_CLOCK (850000)


class wildfire_state : public driver_device
{
public:
	wildfire_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_speaker(*this, "speaker")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<speaker_sound_device> m_speaker;

	UINT8 m_d;
	UINT16 m_a;

	UINT16 m_display_state[0x10];
	UINT16 m_display_cache[0x10];
	UINT8 m_display_decay[0x100];

	DECLARE_READ8_MEMBER(read_k);
	DECLARE_WRITE8_MEMBER(write_d);
	DECLARE_WRITE16_MEMBER(write_a);

	TIMER_DEVICE_CALLBACK_MEMBER(display_decay_tick);
	void display_update();
	bool index_is_7segled(int index);

	virtual void machine_start();
};



/***************************************************************************

  LED Display

***************************************************************************/

// The device strobes the outputs very fast, it is unnoticeable to the user.
// To prevent flickering here, we need to simulate a decay.

// decay time, in steps of 1ms
#define DISPLAY_DECAY_TIME 40

inline bool wildfire_state::index_is_7segled(int index)
{
	// first 3 A are 7segleds
	return (index < 3);
}

// lamp translation table: Lzz from patent US4334679 FIG.4 = MESS lampxxy,
// where xx is led column and y is led row, eg. lamp103 is output A10 D3
/*
    L0  = -         L10 = lamp60    L20 = lamp41    L30 = lamp53    L40 = lamp57    L50 = lamp110  
    L1  = lamp107   L11 = lamp50    L21 = lamp42    L31 = lamp43    L41 = lamp66    L51 = lamp111  
    L2  = lamp106   L12 = lamp61    L22 = lamp52    L32 = lamp54    L42 = lamp76    L52 = lamp112  
    L3  = lamp105   L13 = lamp71    L23 = lamp63    L33 = lamp55    L43 = lamp86    L53 = lamp113  
    L4  = lamp104   L14 = lamp81    L24 = lamp73    L34 = lamp117   L44 = lamp96    L60 = lamp30   
    L5  = lamp103   L15 = lamp92    L25 = lamp115   L35 = lamp75    L45 = lamp67    L61 = lamp30(!)
    L6  = lamp102   L16 = lamp82    L26 = lamp93    L36 = lamp95    L46 = lamp77    L62 = lamp31   
    L7  = lamp101   L17 = lamp72    L27 = lamp94    L37 = lamp56    L47 = lamp87    L63 = lamp31(!)
    L8  = lamp80    L18 = lamp114   L28 = lamp84    L38 = lamp65    L48 = lamp97    L70 = lamp33   
    L9  = lamp70    L19 = lamp51    L29 = lamp116   L39 = lamp85    L49 = -     
*/

void wildfire_state::display_update()
{
	UINT16 active_state[0x10];

	for (int i = 0; i < 0x10; i++)
	{
		// update current state
		m_display_state[i] = (~m_a >> i & 1) ? m_d : 0;

		active_state[i] = 0;

		for (int j = 0; j < 0x10; j++)
		{
			int di = j << 4 | i;

			// turn on powered segments
			if (m_display_state[i] >> j & 1)
				m_display_decay[di] = DISPLAY_DECAY_TIME;

			// determine active state
			int ds = (m_display_decay[di] != 0) ? 1 : 0;
			active_state[i] |= (ds << j);
		}
	}

	// on difference, send to output
	for (int i = 0; i < 0x10; i++)
		if (m_display_cache[i] != active_state[i])
		{
			if (index_is_7segled(i))
				output_set_digit_value(i, BITSWAP8(active_state[i],7,0,1,2,3,4,5,6) & 0x7f);

			for (int j = 0; j < 8; j++)
				output_set_lamp_value(i*10 + j, active_state[i] >> j & 1);
		}

	memcpy(m_display_cache, active_state, sizeof(m_display_cache));
}

TIMER_DEVICE_CALLBACK_MEMBER(wildfire_state::display_decay_tick)
{
	// slowly turn off unpowered segments
	for (int i = 0; i < 0x100; i++)
		if (!(m_display_state[i & 0xf] >> (i>>4) & 1) && m_display_decay[i])
			m_display_decay[i]--;

	display_update();
}



/***************************************************************************

  I/O

***************************************************************************/

READ8_MEMBER(wildfire_state::read_k)
{
	// ?
	return 0xf;
}

WRITE8_MEMBER(wildfire_state::write_d)
{
	m_d = data;
	display_update();
}

WRITE16_MEMBER(wildfire_state::write_a)
{
	m_a = data;
	display_update();
}



/***************************************************************************

  Inputs

***************************************************************************/

static INPUT_PORTS_START( wildfire )
	PORT_START("IN1") // I
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Shooter Button")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Left Flipper")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Right Flipper")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/***************************************************************************

  Machine Config

***************************************************************************/

void wildfire_state::machine_start()
{
	// zerofill
	memset(m_display_state, 0, sizeof(m_display_state));
	memset(m_display_cache, 0, sizeof(m_display_cache));
	memset(m_display_decay, 0, sizeof(m_display_decay));

	m_d = 0;
	m_a = 0;

	// register for savestates
	save_item(NAME(m_display_state));
	save_item(NAME(m_display_cache));
	save_item(NAME(m_display_decay));

	save_item(NAME(m_d));
	save_item(NAME(m_a));
}


static MACHINE_CONFIG_START( wildfire, wildfire_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", AMI_S2150, MASTER_CLOCK)
	MCFG_AMI_S2000_READ_I_CB(IOPORT("IN1"))
	MCFG_AMI_S2000_READ_K_CB(READ8(wildfire_state, read_k))
	MCFG_AMI_S2000_WRITE_D_CB(WRITE8(wildfire_state, write_d))
	MCFG_AMI_S2000_WRITE_A_CB(WRITE16(wildfire_state, write_a))

	MCFG_TIMER_DRIVER_ADD_PERIODIC("display_decay", wildfire_state, display_decay_tick, attotime::from_msec(1))

	MCFG_DEFAULT_LAYOUT(layout_wildfire)

	/* no video! */

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_CONFIG_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( wildfire )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "us4341385", 0x0000, 0x0400, CRC(84ac0f1f) SHA1(1e00ddd402acfc2cc267c34eed4b89d863e2144f) ) // from patent US4334679, data should be correct (it included checksums)
	ROM_CONTINUE(          0x0600, 0x0200 )
ROM_END


CONS( 1979, wildfire, 0, 0, wildfire, wildfire, driver_device, 0, "Parker Brothers", "Wildfire (prototype)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_SUPPORTS_SAVE )
