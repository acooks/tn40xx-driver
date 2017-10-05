#include "tn40.h"
#include "TLK10232_phy.h"

void 			sff_reset(struct bdx_priv *priv);
int  			sff_read_str(struct bdx_priv *priv, unsigned char  sfp_adr, unsigned char  addr,int len, char *buf);
int  			sff_write_str(struct bdx_priv *priv, unsigned char  sfp_adr, unsigned char  addr,int len, char *buf);
int 			i2c_write_byte(struct bdx_priv *priv,int send_start, int send_stop, unsigned char byte);
unsigned char	i2c_read_byte(struct bdx_priv *priv,int nack, int send_stop);
int 			i2c_read_bit(struct bdx_priv *priv);
int 			i2c_write_bit(struct bdx_priv *priv,int bit);
int 			i2c_start_cond(struct bdx_priv *priv, int I2C_started);
int 			i2c_stop_cond(struct bdx_priv *priv);
int 			i2c_SCL_stretch(struct bdx_priv *priv);
int 			read_sfp_id(struct bdx_priv *priv);
void 			set_GPIO(struct bdx_priv *priv);
void 			clear_GPIO_N(struct bdx_priv *priv, u32 n);
u32 			read_GPIO_N(struct bdx_priv *priv, u32 n);
int 			TLK10232_phy_config(struct bdx_priv *priv);
int 			TLK10232_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy);
u32 			TLK10232_get_link_speed(struct bdx_priv *priv);
u32 			TLK10232_link_changed(struct bdx_priv *priv);
void 			TLK10232_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op);

#define LINK_LOOP_MAX   (80)

void TLK10232_register_settings(struct bdx_priv *priv);

enum _SFP_MOD_TYPE{
SFP_ABS=0,
SFP_NA,
SFP_1G,
SFP_10G,
SFP_10G_DA
};

#define TEST_SFP_OM

#ifdef TEST_SFP_OM

#define USE_LOG_I2C 0

#define SCL_GPIO 4
#define SDA_GPIO 5
#define TX_DISABLE_GPIO 1
#define MOD_ABS_GPIO 2

#define I2C_delay(priv)  udelay(8)  
#define RWDELAY(priv)  udelay(1)

#define WRITE_GPOI_REG(priv,reg,val) do{\
 WRITE_REG(priv, 0x51E0,0x30010000|((reg&0xf)));\
READ_REG(priv, 0x5030); WRITE_REG(priv, 0x51F0, ( val) );\
} while(0)

#define READ_GPOI_REG(priv,reg,val) do{\
 WRITE_REG(priv, 0x51E0,0x30000000|((reg&0xf)));\
 READ_REG(priv, 0x5030); val=READ_REG(priv, 0x51F0);\
} while(0)

#if USE_LOG_I2C
#define LOG_I2C(fmt, args...)  printk(KERN_ERR fmt, ## args)
#else
#define LOG_I2C(fmt, args...)   do {  } while (0)
#endif

//-------------------------------------------------------------------------------------------------

u32 read_GPIO_N(struct bdx_priv *priv, u32 n){
   u32 ret,rw,i,msk=(1<<(n&0x7));	
	   // get current GPIO state
	READ_GPOI_REG(priv,6,rw);
	// set to input GPIO N
	WRITE_GPOI_REG(priv,6,rw|msk);
        RWDELAY(priv);
	   // read
	READ_GPOI_REG(priv,4,i);
        ret= (i&msk)?1:0;
LOG_I2C("read_GPIO_N(%x) 4= %x 6=(%x | %x) ret=%x) \n",n,i,rw,msk,ret);   

	return	ret; 

} // read_GPIO_N()

//-------------------------------------------------------------------------------------------------

void clear_GPIO_N(struct bdx_priv *priv, u32 n){
   u32 rw,msk= ~(1<<(n&0x7));	
	   // get current GPIO state
	READ_GPOI_REG(priv,6,rw);
	   // set to output GPIO N
	WRITE_GPOI_REG(priv,6,(rw & msk));
        RWDELAY(priv);
	LOG_I2C("clear_GPIO_N(%x)  6=(%x & %x)\n",n,rw , msk);   

} // clear_GPIO_N()

//-------------------------------------------------------------------------------------------------

// Set SCL as input and return current level of line, 0 or 1
#define read_SCL(p) read_GPIO_N((p),SCL_GPIO)
// Set SDA as input and return current level of line, 0 or 1
#define read_SDA(p)  read_GPIO_N((p),SDA_GPIO)
// Actively drive SCL signal low
#define  clear_SCL(p) clear_GPIO_N((p),SCL_GPIO)
// Actively drive SDA signal low
#define  clear_SDA(p) clear_GPIO_N((p),SDA_GPIO)

//return 0-ok 1-CLK==0  

//-------------------------------------------------------------------------------------------------

int i2c_SCL_stretch(struct bdx_priv *priv){
int i;
  for(i=50;i;i--){
     if(read_SCL(priv)) break;
     udelay(1);
  } 

  return (i)?0:1;

} // i2c_SCL_stretch()

//-------------------------------------------------------------------------------------------------

int i2c_start_cond(struct bdx_priv *priv, int I2C_started) {
int ret=0;
  LOG_I2C("TLK10232 i2c_start_cond start=====================\n");
  I2C_delay(priv);
  I2C_delay(priv);
  if(I2C_started){
    read_SDA(priv);
    I2C_delay(priv);
    ret=i2c_SCL_stretch(priv);
    if(ret) { 
	LOG_I2C("TLK10232 i2c_start_cond failed SCL==0\n");
    }
    I2C_delay(priv);
  }
  
  if (read_SDA(priv) == 0) {
   LOG_I2C("TLK10232 i2c_start_cond failed SDA==0\n");
   ret=1;
  }
 
 // SCL is high, set SDA from 1 to 0.
  clear_SDA(priv);
  I2C_delay(priv);
  clear_SCL(priv);
  LOG_I2C("TLK10232 i2c_start_cond %s======================\n",ret?"Failed ":"OK");

  return ret;

} // i2c_start_cond()

//-------------------------------------------------------------------------------------------------

int i2c_stop_cond(struct bdx_priv *priv){
  int ret=0;
  LOG_I2C("TLK10232 i2c_stop_cond start===================\n");
  clear_SCL(priv);
  // set SDA to 0
  clear_SDA(priv);
  I2C_delay(priv);
  // Clock stretching
  ret=i2c_SCL_stretch(priv);
  if(ret){
	LOG_I2C("TLK10232 i2c_stop_cond failed SCL==0\n");
	ret=1;
  }

  // Stop bit setup time, minimum 4us
  I2C_delay(priv);
  // SCL is high, set SDA from 0 to 1
  if (read_SDA(priv) == 0) {
    LOG_I2C("TLK10232 i2c_stop_cond failed SDA==0\n");
     ret=1;
  }
  I2C_delay(priv);
  LOG_I2C("TLK10232 i2c_stop_cond end===================\n");

  return ret;

} // i2c_stop_cond()

//-------------------------------------------------------------------------------------------------
 
// Write a bit to I2C bus
int i2c_write_bit(struct bdx_priv *priv,int bit) {
int ret=0;
  LOG_I2C("TLK10232 i2c_write_bit ==%x start \n", bit);
  if (bit) {
     read_SDA(priv); 
  } else {
     clear_SDA(priv);
  }
  i2c_SCL_stretch(priv);
  I2C_delay(priv);
  // SCL is high, now data is valid
  // If SDA is high, check that nobody else is driving SDA
  if (bit) if( read_SDA(priv) == 0) {
    //arbitration_lost();
    ret=1;
  }
  I2C_delay(priv);
  clear_SCL(priv);
  LOG_I2C("TLK10232 i2c_write_bit  %s\n",ret?"failed":"OK");

  return ret;

} // i2c_write_bit()

//-------------------------------------------------------------------------------------------------
 
// Read a bit from I2C bus
int i2c_read_bit(struct bdx_priv *priv) {
  int bit;
  LOG_I2C("TLK10232 i2c_read_bit start \n");
  // Let the slave drive data
  read_SDA(priv);
  i2c_SCL_stretch(priv);
  I2C_delay(priv);
  // SCL is high, now data is valid
  bit = read_SDA(priv);
  I2C_delay(priv);
  clear_SCL(priv);
  LOG_I2C("TLK10232 i2c_read_bit %x \n",bit);

  return bit?1:0;

} // i2c_read_bit()

//-------------------------------------------------------------------------------------------------
 
// Write a byte to I2C bus. Return 0 if ack by the slave.
int i2c_write_byte(struct bdx_priv *priv,int send_start,
                    int send_stop,
                    unsigned char byte) {
  unsigned bit;
  int nack;
LOG_I2C("i2c_write_byte 0x%x start\n",byte);
  if (send_start) {
    i2c_start_cond(priv,(send_start==2)?1:0);
  }
  for (bit = 0; bit < 8; bit++) {
    i2c_write_bit(priv,(byte & 0x80) != 0);
    byte <<= 1;
  }
  nack = i2c_read_bit(priv);
  if (send_stop) {
    i2c_stop_cond(priv);
  }
LOG_I2C("i2c_write_byte end nack=%x\n",nack);

  return nack;

} // i2c_write_byte()

//-------------------------------------------------------------------------------------------------
 
// Read a byte from I2C bus
unsigned char i2c_read_byte(struct bdx_priv *priv,int nack, int send_stop) {
  unsigned char byte = 0;
  unsigned bit;
LOG_I2C("i2c_read_byte start nack=%x\n",nack);
  for (bit = 0; bit < 8; bit++) {
    byte = (byte << 1);
    byte |= i2c_read_bit(priv);
  }
  i2c_write_bit(priv, nack);
  if (send_stop) {
    i2c_stop_cond(priv);
  }
LOG_I2C("i2c_read_byte stop byte=%x\n",byte);

  return byte;

} // i2c_read_byte()

//-------------------------------------------------------------------------------------------------

void sff_reset(struct bdx_priv *priv){
	int i;

	for (i=0;i<9;i++){
	   i2c_read_bit(priv);
	}
	i2c_start_cond(priv,0);
	I2C_delay(priv);
	read_SDA(priv);

} // sff_reset()

//-------------------------------------------------------------------------------------------------

/*
Return len or 0 on error
*/
int  sff_read_str(struct bdx_priv *priv, unsigned char  sfp_adr, unsigned char  addr,int len, char *buf){
    unsigned char  data;
    int i=0,err=0;
    
    if(buf==0) return 0;
    if(len==0) return 0;
    if(len&0xff00) return 0;

DBG("TLK10232 sff_read_str sfp_adr=%x addr=%x start \n",sfp_adr,addr);

    do{        
	err=i2c_write_byte(priv,/*send_start*/1,/*send_stop*/ 0,sfp_adr&0xfe);
        if(err){ err=1; break;}
	
	err=i2c_write_byte(priv,/*send_start*/0,/*send_stop*/ 1,addr&0xff);
        if(err){ err=2; break;}
	
	err=i2c_write_byte(priv,/*send_start*/1,/*send_stop*/ 0,sfp_adr|1);
        if(err){ err=3; break;}

        len--; 
        for(i=0;i<len;i++){ 	
            data=i2c_read_byte(priv,/*nack*/ 0, /*send_stop*/ 0);
            buf[i]=data; 
        }	
        data=i2c_read_byte(priv,/*nack*/ 1, /*send_stop*/ 1);
        buf[i]=data; 
        i++;
        DBG("TLK10232 sff_read_str sfp_adr=%x addr=%x end \n",sfp_adr,addr);
    } while(0); 
    if(err){
	DBG("TLK10232 sff_read step %u failed\n",err);
	i=0; 
        i2c_stop_cond(priv);
    }

    return i;

} // sff_read_str()

//-------------------------------------------------------------------------------------------------

int  sff_write_str(struct bdx_priv *priv, unsigned char  sfp_adr, unsigned char  addr,int len, char *buf){
    int i=0,err=0;
    
    if(buf==0) return 0;
    if(len==0) return 0;
    if(len>8) return 0; // not more then 7 bytes by SFF spec
    if(len&0xff00) return 0;

DBG("TLK10232 sff_write_str sfp_adr=%x addr=%x start \n",sfp_adr,addr);

    do{        
	err=i2c_write_byte(priv,/*send_start*/1,/*send_stop*/ 0,sfp_adr&0xfe);
        if(err){ err=1; break;}
	
	err=i2c_write_byte(priv,/*send_start*/0,/*send_stop*/ 0,addr&0xff);
        if(err){ err=2; break;}
	
        len--; 
        for(i=0;i<len;i++){ 	
            err=i2c_write_byte(priv,/*send_start*/ 0, /*send_stop*/ 0,buf[i]);
            if(err){ err=4; break;}
        }	
        if(err){ err=4; break;}
        err=i2c_write_byte(priv,/*send_start*/0, /*send_stop*/ 1,buf[i]);
        i++;
        DBG("TLK10232 sff_write_str sfp_adr=%x addr=%x end \n",sfp_adr,addr);
    } while(0); 
    if(err){
	ERR("TLK10232 sff_write step %u failed\n",err);
	i=0; 
        i2c_stop_cond(priv);
    }

    return i;

} // sff_write_str()

//-------------------------------------------------------------------------------------------------

void set_GPIO(struct bdx_priv *priv){
  u32 input_msk=(1<<11)|(1<<SCL_GPIO)|(1<<SDA_GPIO)|(1<<MOD_ABS_GPIO);
	WRITE_GPOI_REG(priv,4,0);
	WRITE_GPOI_REG(priv,6,input_msk);

} // set_GPIO()

//-------------------------------------------------------------------------------------------------


int read_sfp_id(struct bdx_priv *priv){
char buff[10];
int i,ret;
int sff_addr=0xa0;
int addr=0x3;
int len=6;

	set_GPIO(priv);
        i=read_GPIO_N(priv,MOD_ABS_GPIO);
       	DBG("MOD_ABS_GPIO=%u  \n",i);
        if(i) return SFP_ABS;
	ret=sff_read_str(priv, sff_addr, addr,len, buff);
	DBG("sff_read_str(0x%x,0x%x,0x%x)=%x  \n",sff_addr, addr,len,ret);
#if 0
	for(i=0;i<ret;i++){
        	ERR("sff_read_str buf[%u]=0x%x \n",i,buff[i]&0xff);
	}
#endif
	if(buff[5]&0xF) return SFP_10G_DA;
	if(buff[0]) return SFP_10G;
	if(buff[3]&0xF) return SFP_1G;

	return SFP_10G;

} // read_sfp_id()

//-------------------------------------------------------------------------------------------------


#else
#define read_sfp_id(p) (SFP_10G)
#endif

//-------------------------------------------------------------------------------------------------

 int TLK10232_phy_config(struct bdx_priv *priv)
{
	int 			regVal, j=0;
        int port=priv->phy_mdio_port;

	u32 sfp_mod_type=priv->sfp_mod_type;
        if(sfp_mod_type==SFP_ABS) return 0;  


        switch(sfp_mod_type){
	   default:		
           case SFP_10G:
                BDX_MDIO_WRITE(priv,0x1e, CHANNEL_CONTROL_1,	0x0B00);
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_2, 0xA848); //reg 1e.3
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_3, 0x9518); //reg 1e.4
		BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_4, 0x3300); //reg 1e.5
                break; 

           case SFP_10G_DA:
                BDX_MDIO_WRITE(priv,0x1e, CHANNEL_CONTROL_1,	0x0B00);
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_2, 0xF848); //reg 1e.3
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_3, 0x1500); //reg 1e.4
		BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_4, 0x2000); //reg 1e.5
                break; 

           case SFP_1G:
                BDX_MDIO_WRITE(priv,0x1e, CHANNEL_CONTROL_1,	0x0300);
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_2, 0xA848); //reg 1e.3
                BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_3, 0x9518); //reg 1e.4
		BDX_MDIO_WRITE(priv,0x1e, HS_SERDES_CONTROL_4, 0x3300); //reg 1e.5

                break;
        }


	// Datapath reset
	do
	{
		udelay(25);
		BDX_MDIO_WRITE(priv,0x1e, RESET_CONTROL,     	    0x8);
		BDX_MDIO_WRITE(priv,0x1e, RESET_CONTROL,     	    0x0);
		udelay(25);
		regVal = bdx_mdio_read(priv, 0x1e, port, CHANNEL_STATUS_1);
		regVal = bdx_mdio_read(priv, 0x1e, port, CHANNEL_STATUS_1);
		DBG("TLK10232_phy_config() CHANNEL_STATUS_1 = 0x%x Fail:%x loops:%d (Fail mask 0x0100)\n", regVal, (regVal & 0x0100), j);
	} while ((regVal & 0x0100) && (j++ < 100));

	return 0;
}

//-------------------------------------------------------------------------------------------------

 int TLK10232_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{
	int 			regVal;
	priv->sfp_mod_type=0;  //read_sfp_id(priv);

	// Device reset
	BDX_MDIO_WRITE(priv,0x1e, GLOBAL_CONTROL_1,  0x8610); 
	regVal = bdx_mdio_read(priv, 0x1e, port, GLOBAL_CONTROL_1);
	DBG("TLK10232_mdio_reset() GLOBAL_CONTROL_1 = 0x%x\n", regVal);
	msleep(100);

	BDX_MDIO_WRITE(priv,0x7, AN_CONTROL, 	0x2000);
	BDX_MDIO_WRITE(priv,0x1, LT_TRAIN_CONTROL,   0x0); 

    priv->link_speed = TLK10232_get_link_speed(priv);
    bdx_speed_set(priv, priv->link_speed);

    return 0;

} // TLK10232_mdio_reset
      
//-------------------------------------------------------------------------------------------------

u32 TLK10232_get_link_speed(struct bdx_priv *priv)
{
	u32 sfp_mod_type,speed;

        sfp_mod_type=read_sfp_id(priv);
 //       ERR("TLK10232_get_link_speed  sfp_mod_type=%u\n",sfp_mod_type);
        if(priv->sfp_mod_type!=sfp_mod_type){
            priv->sfp_mod_type=sfp_mod_type;
            TLK10232_phy_config(priv);
        }
        switch (sfp_mod_type){
	    case SFP_10G:
	    case SFP_10G_DA:
		 speed=SPEED_10000;break;
	    case SFP_1G:
	      //speed=SPEED_1000; break;
	    default:
	      speed=0; break;
        } 

	return speed;

} // TLK10232_get_link_speed()

//-------------------------------------------------------------------------------------------------

u32 TLK10232_link_changed(struct bdx_priv *priv)
{
     u32 	link	= 0;

     priv->link_speed = TLK10232_get_link_speed(priv);
     link = READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT;
     if (link)
     {
    	 link = priv->link_speed;
    	 DBG("TLK10232 link speed is %s\n", (priv->link_speed == SPEED_10000) ? "10G" : "1G");
    	 WRITE_REG(priv, 0x5150,0); //  stop timer
     }
     else
     {
    	 if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
    	 {
			 DBG("QT2025 trying to recover link after %d tries\n", LINK_LOOP_MAX);
			 // MAC reset
    		 bdx_speed_set(priv,0);
    		 bdx_speed_set(priv,priv->link_speed);
    		 priv->link_loop_cnt = 0;
    		 priv->sfp_mod_type=SFP_ABS;
    	 }
    	 //DBG("TLK10232 no link, setting 1/5 sec timer\n");
    	 WRITE_REG(priv, 0x5150,1000000); // 1/5 sec timeout
     }

     return link;

} // TLK10232_link_changed()

//-------------------------------------------------------------------------------------------------

void TLK10232_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
	switch (op)
	{
		case PHY_LEDS_SAVE:
			break;

		case PHY_LEDS_RESTORE:
			break;

		case PHY_LEDS_ON:
			WRITE_REG(priv, regBLNK_LED, 4);
			break;

		case PHY_LEDS_OFF:
			WRITE_REG(priv, regBLNK_LED, 0);
			break;

		default:
			DBG("TLK10232_leds() unknown op 0x%x\n", op);
			break;

	}

} // TLK10232_leds()

//-------------------------------------------------------------------------------------------------

__init enum PHY_TYPE  TLK10232_register(struct bdx_priv *priv)
{
	priv->isr_mask= IR_RX_FREE_0 |  IR_LNKCHG0 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = TLK10232_mdio_reset;
    priv->phy_ops.link_changed = TLK10232_link_changed;
    priv->phy_ops.ledset       = TLK10232_leds;
    priv->phy_ops.mdio_speed   = MDIO_SPEED_1MHZ;
    TLK10232_register_settings(priv);

    return PHY_TYPE_TLK10232;

} // TLK10232_init()

//-------------------------------------------------------------------------------------------------

