//V1.0.0 Kavie 20141021 RK3126 SP2519 k71
//V1.1.0 sp-yjp 20141022 RK3126 SP2519 k71
//V1.1.1 LC 20141120 RK3126 SP2519 k71

#include "generic_sensor.h"
/*
 *      Driver Version Note
 *v0.0.1: this driver is compatible with generic_sensor
 *v0.1.1:
 *        add sensor_focus_af_const_pause_usr_cb;
 */
static int version = KERNEL_VERSION(0,1,1);
module_param(version, int, S_IRUGO);



static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug > level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_SP2519
#define SENSOR_V4L2_IDENT V4L2_IDENT_SP2519
#define SENSOR_ID 0x25
#define SENSOR_BUS_PARAM                     (V4L2_MBUS_MASTER |\
															 V4L2_MBUS_PCLK_SAMPLE_RISING|V4L2_MBUS_HSYNC_ACTIVE_HIGH| V4L2_MBUS_VSYNC_ACTIVE_HIGH|\
															 V4L2_MBUS_DATA_ACTIVE_HIGH  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W                     800
#define SENSOR_PREVIEW_H                     600
#define SENSOR_PREVIEW_FPS                   15000     // 15fps 
#define SENSOR_FULLRES_L_FPS                 7500      // 7.5fps
#define SENSOR_FULLRES_H_FPS                 7500      // 7.5fps
#define SENSOR_720P_FPS                      0
#define SENSOR_1080P_FPS                     0

#define SENSOR_REGISTER_LEN                  1         // sensor register address bytes
#define SENSOR_VALUE_LEN                     1         // sensor register value bytes

static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;
};

/*
 * Local define
 */
//AE target
#define  SP2519_P1_0xeb  0x70
#define  SP2519_P1_0xec  0x70

#define SP2519_NORMAL_Y0ffset  0x08
#define SP2519_LOWLIGHT_Y0ffset  0x20

//HEQ
#define  SP2519_P1_0x10  0x80
#define  SP2519_P1_0x11  0x80 //ku indoor  //0x80
#define  SP2519_P1_0x12  0x88 //ku dummy //0x80
#define  SP2519_P1_0x13  0x88 //ku low  //0x80

#define  SP2519_P1_0x14  0xc8//0xba //kl outdoor //0xa0	//modify by sp_yjp,2014-10-22
#define  SP2519_P1_0x15  0xc8//0xb6 //kl indoor  //0xa0	//modify by sp_yjp,2014-10-22
#define  SP2519_P1_0x16  0xb8//0xac// kl dummy //0xa0	//modify by sp_yjp,2014-10-22
#define  SP2519_P1_0x17  0xa4 //kl lowlight //0xa0

#define  SP2519_P1_0xdb  0x00//0x00

/*
 *  The follow setting need been filled.
 *  
 *  Must Filled:
 *  sensor_init_data :               Sensor initial setting;
 *  sensor_fullres_lowfps_data :     Sensor full resolution setting with best auality, recommand for video;
 *  sensor_preview_data :            Sensor preview resolution setting, recommand it is vga or svga;
 *  sensor_softreset_data :          Sensor software reset register;
 *  sensor_check_id_data :           Sensir chip id register;
 *
 *  Optional filled:
 *  sensor_fullres_highfps_data:     Sensor full resolution setting with high framerate, recommand for video;
 *  sensor_720p:                     Sensor 720p setting, it is for video;
 *  sensor_1080p:                    Sensor 1080p setting, it is for video;
 *
 *  :::::WARNING:::::
 *  The SensorEnd which is the setting end flag must be filled int the last of each setting;
 */

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] = 
{	//modify by sp_yjp,2014-10-22
	//{0xfd,0x00},
	//{0x36,0x01},//20170524 
	{0xfd,0x01},
	{0x36,0x02},
	{0xfd,0x00},
	{0x30,0x08},
	{0x2f,0x11},//04 modified by LC 20141120
	{0x09,0x01},
	{0xfd,0x00},
	{0x0c,0x55},
	{0x27,0xa5},
	{0x1a,0x4b},
	{0x20,0x2f},
	{0x22,0x5a},
	{0x25,0xad},
	{0x21,0x0d}, //
	{0x28,0x08},
	{0x1d,0x01},
	{0x7a,0x5d},
	{0x70,0x41},
	{0x74,0x40},
	{0x75,0x40},
	{0x15,0x3e},
	{0x71,0x3f},
	{0x7c,0x3f},
	{0x76,0x3f},
	{0x7e,0x29},
	{0x72,0x29},
	{0x77,0x28},
	{0x1e,0x01},
	{0x1c,0x1f},
	{0x2e,0xc5},
	{0x1f,0xf0},
	{0x6c,0x00},
	
	{0xfd,0x01},
	{0x32,0x00},
	{0xfd,0x02},
	{0x85,0x00},
	
	{0xfd,0x02},
	{0x26,0xa0},
	{0x27,0x96},
	{0xfd,0x00},
	{0xe7,0x03},
	{0xe7,0x00},
	#if 1
	//ae setting 24M 50hz 2.5pll 8-11fps uxga
	{0xfd,0x00},
	{0x03,0x03},
	{0x04,0x30},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x00},
	{0x0a,0xb5},//9d modified by LC 20141120
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x88},
	{0xf8,0x71},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x88},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x0f},
	{0x3e,0x71},
	{0x3f,0x00},
	{0x88,0xc3},
	{0x89,0x87},
	{0x8a,0x43},
	{0xfd,0x02},
	{0xbe,0x60},
	{0xbf,0x06},
	{0xd0,0x60},
	{0xd1,0x06},
	{0xc9,0x60},
	{0xca,0x06},
	#else  //20141021
	//23M 50HZ 2.5PLL 8-10.85fps UXGA
	{0xfd,0x00},
	{0x03,0x03},
	{0x04,0x24},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x00},
	{0x0a,0x97},
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x86},
	{0xf8,0x70},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x86},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x0f},
	{0x3e,0x70},
	{0x3f,0x00},
	{0x88,0xd2},
	{0x89,0x92},
	{0x8a,0x43},
	{0xfd,0x02},
	{0xbe,0x48},
	{0xbf,0x06},
	{0xd0,0x48},
	{0xd1,0x06},
	{0xc9,0x48},
	{0xca,0x06},
	#endif
	{0xfd,0x01},
	{0x32,0x15},
	{0xfd,0x00},
	{0xe7,0x03},
	{0xe7,0x00},
	
	{0xfd,0x02},
	{0xb8,0x70},  //mean_nr_dummy                               
	{0xb9,0x80},
	{0xba,0x30},  //mean_dummy_low                              
	{0xbb,0x45},  //mean_low_dummy                              
	{0xbc,0x90},
	{0xbd,0x70},
	{0xfd,0x03},
	{0x77,0x48},
	//rpc                                                        
	{0xfd,0x01},
	{0xe0,0x48},
	{0xe1,0x38},
	{0xe2,0x30},
	{0xe3,0x2c},
	{0xe4,0x2c},
	{0xe5,0x2a},
	{0xe6,0x2a},
	{0xe7,0x28},
	{0xe8,0x28},
	{0xe9,0x28},
	{0xea,0x26},
	{0xf3,0x26},
	{0xf4,0x26},
	
	{0xfd,0x01},  //ae min gain        
	{0x04,0xa0},  //0xc0 //modify by sp_yjp,2014-10-22
	{0x05,0x26},//rpc_min_indr    
	{0x0a,0x48},
	{0x0b,0x26},  //rpc_min_outdr      
	
	{0xfd,0x01},  //ae target          
	{0xf2,0x09},
	{0xeb,SP2519_P1_0xeb},  //target_indr		     
	{0xec,SP2519_P1_0xec},  //target_outdr		   
	{0xed,0x06},
	{0xee,0x0a},
	{0xfd,0x02},
	{0x4f,0x46},
	
	//¨¨£¤?¦Ì????
	{0xfd,0x03},
	{0x52,0xff},//dpix_wht_ofst_outdoor                                                                    
	{0x53,0x60},//dpix_wht_ofst_normal1
	{0x94,0x00},
	{0x54,0x00},//dpix_wht_ofst_dummy                                                                      
	{0x55,0x00},//dpix_wht_ofst_low
	
	{0x56,0x80},//dpix_blk_ofst_outdoor
	{0x57,0x80},//dpix_blk_ofst_normal1
	{0x95,0x00},
	{0x58,0x00},//dpix_blk_ofst_dummy  
	{0x59,0x00},//dpix_blk_ofst_low    
	
	{0x5a,0xf6},//dpix_wht_ratio
	{0x5b,0x00},
	{0x5c,0x88},//dpix_blk_ratio
	{0x5d,0x00},
	{0x96,0x00},
	
	{0xfd,0x03},
	{0x8a,0x00},
	{0x8b,0x00},
	{0x8c,0xff},
	
	{0x22,0xff},//dem_gdif_thr_outdoor
	{0x23,0xff},//dem_gdif_thr_normal
	{0x24,0xff},//dem_gdif_thr_dummy
	{0x25,0xff},//dem_gdif_thr_low
	
	{0x5e,0xff},//dem_gwnd_wht_outdoor
	{0x5f,0xff},//dem_gwnd_wht_normal 
	{0x60,0xff},//dem_gwnd_wht_dummy  
	{0x61,0xff},//dem_gwnd_wht_low    
	{0x62,0x00},//dem_gwnd_blk_outdoor
	{0x63,0x00},//dem_gwnd_blk_normal 
	{0x64,0x00},//dem_gwnd_blk_dummy  
	{0x65,0x00},//dem_gwnd_blk_low 
	
	//lsc                                                                            
	{0xfd,0x01},
	{0x21,0x00},  //lsc_sig_ru lsc_sig_lu                                            
	{0x22,0x00},  //lsc_sig_rd lsc_sig_ld                                            
	{0x26,0x60},
	{0x27,0x14},  //lsc_exp_thrl                                                     
	{0x28,0x05},  //lsc_exp_thrh                                                     
	{0x29,0x00},
	{0x2a,0x01},  //lsc_rpc_en lens?£¤??¡Á?¨º¨º¨®|                                        
	
	{0xfd,0x01},
	{0xa1,0x23},//0x27 Kavie 20141104
	{0xa2,0x24},//0x27 Kavie 20141104
	{0xa3,0x24},//0x25 Kavie 20141104
	{0xa4,0x24},//0x28 Kavie 20141104
	{0xa5,0x1c},
	{0xa6,0x1a},
	{0xa7,0x17},
	{0xa8,0x1c},
	{0xa9,0x18},
	{0xaa,0x18},
	{0xab,0x18},
	{0xac,0x18},
	{0xad,0x07},//0x08  Kavie 20141104
	{0xae,0x08},//0x09  Kavie 20141104
	{0xaf,0x01},//0x03  Kavie 20141104
	{0xb0,0x03},//0x05  Kavie 20141104
	{0xb1,0x08},
	{0xb2,0x0a},
	{0xb3,0x03},
	{0xb4,0x06},
	{0xb5,0x0a},
	{0xb6,0x0a},
	{0xb7,0x05},
	{0xb8,0x08}, 
	
	//AWB                                                                                             
	{0xfd,0x02},
	{0x26,0xa0},
	{0x27,0x96},
	{0x28,0xcc}, //Y top value limit                                                                  
	{0x29,0x01}, //Y bot value limit                                                                  
	{0x2a,0x00},
	{0x2b,0x00},
	{0x2c,0x20}, //Awb image center row start                                                         
	{0x2d,0xdc}, //Awb image center row end                                                           
	{0x2e,0x20}, //Awb image center col start                                                         
	{0x2f,0x96}, //Awb image center col end                                                           
	{0x1b,0x80}, //b,g mult a constant for detect white pixel                                         
	{0x1a,0x80}, //r,g mult a constant for detect white pixel                                         
	{0x18,0x16}, //wb_fine_gain_step,wb_rough_gain_step                                               
	{0x19,0x26}, //wb_dif_fine_th, wb_dif_rough_th                                                    
	{0x1d,0x04},
	{0x1f,0x06},
	
	//d65 10                                                                                 
	{0x66,0x36},
	{0x67,0x5c},
	{0x68,0xbb},
	{0x69,0xdf},
	{0x6a,0xa5},
	
	//indoor                                                                                         
	{0x7c,0x26},
	{0x7d,0x4a},
	{0x7e,0xe0},
	{0x7f,0x05},
	{0x80,0xa6},
	
	//cwf   12                                                                               
	{0x70,0x21},
	{0x71,0x41},
	{0x72,0x05},
	{0x73,0x25},
	{0x74,0xaa},
	
	//tl84                                                                                   
	{0x6b,0x00},
	{0x6c,0x20},
	{0x6d,0x0e},
	{0x6e,0x2a},
	{0x6f,0xaa},
	
	//f                                                                                      
	{0x61,0xdb},
	{0x62,0xfe},
	{0x63,0x37},
	{0x64,0x56},
	{0x65,0x5a},
	
	{0x75,0x00},                                                                                     
	{0x76,0x09},                                                                                     
	{0x77,0x02},                                                                                     
	{0x0e,0x16}, 
	
	{0x3b,0x09},//awb                                                                                 
	{0xfd,0x02},//awb outdoor mode                                                                  
	{0x02,0x00},//outdoor exp 5msb                                                                    
	{0x03,0x10},//outdoor exp 8lsb                                                                    
	{0x04,0xf0},//outdoor rpc                                                                         
	{0xf5,0xb3},//outdoor rgain top                                                                   
	{0xf6,0x80},//outdoor rgain bot                                                                   
	{0xf7,0xe0},//outdoor bgain top                                                                   
	{0xf8,0x89},//outdoor bgain bot   
	
	//skin detect                                                                                    
	{0xfd,0x02},                                                                                     
	{0x08,0x00},                                                                                     
	{0x09,0x04},                                                                                     
	
	{0xfd,0x02},                                                                                     
	{0xdd,0x0f}, //raw smooth en                                                                      
	{0xde,0x0f}, //sharpen en                                                                         
	
	{0xfd,0x02},  // sharp                                                                            
	{0x57,0x30},
	{0x58,0x10},  //raw_sharp_y_min                                                                   
	{0x59,0xe0},  //raw_sharp_y_max                                                                   
	{0x5a,0x00},
	{0x5b,0x12},
	
	{0xcb,0x04},	//0x08  modify by sp_yjp,2014-10-22
	{0xcc,0x08},	//0x0b  modify by sp_yjp,2014-10-22
	{0xcd,0x0b},	//0x10  modify by sp_yjp,2014-10-22
	{0xce,0x1a},	//0x1a  modify by sp_yjp,2014-10-22
	
	{0xfd,0x03},
	{0x87,0x04},
	{0x88,0x08},
	{0x89,0x10},
	#if 0
	{0xfd,0x02},
	{0xe8,0x58},
	{0xec,0x60},
	{0xe9,0x50},
	{0xed,0x58},
	{0xea,0x48},
	{0xee,0x50},
	{0xeb,0x48},
	{0xef,0x40},
	#else
	{0xfd,0x02},
	{0xe8,0x58},
	{0xec,0x68},
	{0xe9,0x60},
	{0xed,0x68},
	{0xea,0x58},
	{0xee,0x60},
	{0xeb,0x48},
	{0xef,0x50},
	#endif
	{0xfd,0x02},
	{0xdc,0x04},
	{0x05,0x6f},
	
	//????¡Á?¨º¨º¨®|                                                                                       
	{0xfd,0x02},
	{0xf4,0x30},  //raw_ymin                                                                          
	{0xfd,0x03},
	{0x97,0x98},
	{0x98,0x88},
	{0x99,0x88},
	{0x9a,0x80},  //raw_ymax_low                                                                      
	{0xfd,0x02},                                                                                     
	{0xe4,0xff},//40 //raw_yk_fac_outdoor                                                              
	{0xe5,0xff},//40 //raw_yk_fac_normal                                                               
	{0xe6,0xff},//40 //raw_yk_fac_dummy                                                                
	{0xe7,0xff},//40 //raw_yk_fac_low                                                                  
	
	{0xfd,0x03},
	{0x72,0x18},
	{0x73,0x28},
	{0x74,0x28},
	{0x75,0x30},
	
	//????¨ª¡§¦Ì¨¤?¨²?D?¦Ì                                                                                   
	{0xfd,0x02}, //raw_dif_thr                                                                        
	{0x78,0x30},//20 //raw_dif_thr_outdoor    //0x40                                                            
	{0x79,0x2c},//18//0a     //0x40                                                                             
	{0x7a,0x24},//10//10                                                                                  
	{0x7b,0x08},//08//20                                                                                  
	
	//Gr?¡éGb¨ª¡§¦Ì¨¤?D?¦Ì                                                                                  
	{0x81,0x02},	//0x02 modify by sp_yjp,2014-10-22
	{0x82,0x20},	//0x20 modify by sp_yjp,2014-10-22
	{0x83,0x20},	//0x20 modify by sp_yjp,2014-10-22
	{0x84,0x08},	//0x08 modify by sp_yjp,2014-10-22
	
	
	{0xfd,0x03},
	{0x7e,0x02},	//0x06 modify by sp_yjp,2014-10-22
	{0x7f,0x04},	//0x0d modify by sp_yjp,2014-10-22
	{0x80,0x06},	//0x10 modify by sp_yjp,2014-10-22	
	{0x81,0x10},	//0x16 modify by sp_yjp,2014-10-22 
	
	{0x7c,0xff},
	
	{0x82,0x54},
	{0x83,0x43},
	{0x84,0x00},
	{0x85,0x20},
	{0x86,0x40},
	
	//¨¨£¤¡Á?¡À?1|?¨¹                                                                                       
	{0xfd,0x03},
	{0x66,0x18}, //pf_bg_thr_normal b-g>thr                                                          
	{0x67,0x28}, //pf_rg_thr_normal r-g<thr                                                          
	{0x68,0x20}, //pf_delta_thr_normal |val|>thr                                                     
	{0x69,0x88}, //pf_k_fac val/16                                                                   
	{0x9b,0x18}, //pf_bg_thr_outdoor                                                                 
	{0x9c,0x28}, //pf_rg_thr_outdoor                                                                 
	{0x9d,0x20}, //pf_delta_thr_outdoor                                                              
	
	//Gamma                                                                                            
	{0xfd,0x01},
	{0x8b,0x00},
	{0x8c,0x08},
	{0x8d,0x13},
	{0x8e,0x22},
	{0x8f,0x34},
	{0x90,0x4d},
	{0x91,0x60},
	{0x92,0x6f},
	{0x93,0x7b},
	{0x94,0x90},
	{0x95,0xa0},
	{0x96,0xaf},
	{0x97,0xbb},
	{0x98,0xc6},
	{0x99,0xcf},
	{0x9a,0xd4},
	{0x9b,0xda},
	{0x9c,0xdf},
	{0x9d,0xe4},
	{0x9e,0xe8},
	{0x9f,0xeb},
	{0xa0,0xed},
	
	//CCM                                                                                              
	{0xfd,0x02},
	{0x15,0xa9},
	{0x16,0x84},
	//!F                                                                                             
	{0xa0,0x97},//0x97	0x99 modify by sp_yjp,2014-10-22
	{0xa1,0xea},
	{0xa2,0xff},
	{0xa3,0x0e},
	{0xa4,0x77},
	{0xa5,0xfa},
	{0xa6,0x08},
	{0xa7,0xcb},
	{0xa8,0xad},
	{0xa9,0x3c},
	{0xaa,0x30},
	{0xab,0x0c},
	//F                                                                                          
	{0xac,0x7f},
	{0xad,0x08},
	{0xae,0xf8},
	{0xaf,0xff},
	{0xb0,0x6e},
	{0xb1,0x13},
	{0xb2,0xd2},
	{0xb3,0x6e},
	{0xb4,0x40},
	{0xb5,0x30},
	{0xb6,0x03},
	{0xb7,0x1f},
	
	{0xfd,0x01},		//auto_sat                                                                 
	{0xd2,0x2d},
	{0xd1,0x38},		//lum thr in green enhance                                                 
	{0xdd,0x3f},
	{0xde,0x37},
	
	
	//auto sat                                                                                         
	{0xfd,0x02},                                                                                     
	{0xc1,0x40},  //                                                                                   
	{0xc2,0x40},                                                                                     
	{0xc3,0x40},
	{0xc4,0x40},
	
	{0xc5,0x80},  //outdoor  ¨ª?¡À£¤o¨ª?¨¨??¨°?                                                                             
	{0xc6,0x60},
	{0xc7,0x00},   //0x80                                                                                  
	{0xc8,0x00},   //0x80                                                                               
	
	//sat u                                                                                            
	{0xfd,0x01},
	
	{0xd3,0xa0},	//0xb0 modify by sp_yjp,2014-10-22
	{0xd4,0xa0},	//0xac modify by sp_yjp,2014-10-22
	{0xd5,0x98},
	{0xd6,0x80},
	//sat v     7                                                                                      
	{0xd7,0xa0},	//0xb0 modify by sp_yjp,2014-10-22
	{0xd8,0xa0},	//0xac modify by sp_yjp,2014-10-22
	{0xd9,0x98},
	{0xda,0x80},
	
	{0xfd,0x03},
	{0x76,0x0a},
	{0x7a,0x40},
	{0x7b,0x40},
	//auto_sat                                                                                        
	{0xfd,0x01},
	{0xc2,0xaa},
	{0xc3,0xaa},
	{0xc4,0x66},
	{0xc5,0x66},
	
	
	//low_lum_offset                                                                                   
	{0xfd,0x01},
	{0xcd,0x08},
	{0xce,0x18},
	
	//gw                                                                                               
	{0xfd,0x02},                                                                                     
	{0x32,0x60},
	{0x35,0x60},
	{0x37,0x13},                                                                                     
	
	//heq                                                                                              
	{0xfd,0x01},
	{0xdb,SP2519_P1_0xdb},//buf_heq_offset                                                                      
	
	{0x10,SP2519_P1_0x10},//ku_outdoor                                                                          
	{0x11,SP2519_P1_0x11},//ku_nr
	{0x12,SP2519_P1_0x12},//ku_dummy
	{0x13,SP2519_P1_0x13},//ku_low 
	{0x14,SP2519_P1_0x14},//kl_outdoor                                                                              
	{0x15,SP2519_P1_0x15},//kl_nr                                                                               
	{0x16,SP2519_P1_0x16},//kl_dummy                                                                            
	{0x17,SP2519_P1_0x17},//kl_low                                                                                           
	
	{0xfd,0x03},
	{0x00,0x80},
	{0x03,0x68},
	{0x06,0xd8},
	{0x07,0x28},
	{0x0a,0xfd},
	{0x01,0x16},
	{0x02,0x16},
	{0x04,0x16},
	{0x05,0x16},
	{0x0b,0x40},
	{0x0c,0x40},
	{0x0d,0x40},
	{0x0e,0x40},
	{0x08,0x0c},
	{0x09,0x0c},
	
	
	{0xfd,0x02},
	{0x8e,0x0a},
	{0x90,0x40},
	{0x91,0x40},
	{0x92,0x60},
	{0x93,0x80},
	{0x9e,0x44},
	{0x9f,0x44},
	
	{0xfd,0x02},
	{0x85,0x00},
	{0xfd,0x01},
	{0x00,0x00},
	{0xfb,0x25},
	//{0x32,0x15},
	{0x33,0xef},
	{0x34,0xef},
	{0x35,0x40},
	{0xfd,0x00},
	{0x3f,0x03},
	
	{0xfd,0x01},
	{0x50,0x00},
	{0x66,0x00},
	
	{0xfd,0x02},
	{0xd6,0x0f},
	
	{0xfd,0x00},
	{0x1b,0x30},
	{0xfd,0x01},
	{0x36,0x00},
	
	{0xf2,0x09},
	
	//svga
	{0xfd,0x01},
	{0x4a,0x00},
	{0x4b,0x04},
	{0x4c,0xb0},
	{0x4d,0x00},
	{0x4e,0x06},
	{0x4f,0x40},
	
	{0xfd,0x00},
	{0x19,0x00},
	{0x30,0x08},
	{0x33,0x00},
	{0x31,0x00},
	
	{0xfd,0x02},
	{0x8f,0x02},
	{0x0f,0x01},
	{0x40,0x00},
	{0x41,0x40},
	{0x42,0x00},
	{0x43,0x40},
	{0x44,0x02},
	{0x45,0x58},//600
	{0x46,0x03},
	{0x47,0x20},//800
	{0xfd,0x00},
	SensorEnd
};

/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	//UXGA window
	{0xfd,0x01},	
	{0x4a,0x00},
	{0x4b,0x04},
	{0x4c,0xb0},	
	{0x4d,0x00},
	{0x4e,0x06},
	{0x4f,0x40},
	
	{0xfd,0x00},
	{0x19,0x00},
	{0x30,0x08},
	{0x33,0x00},
	{0x31,0x00},
	
	{0xfd,0x02},
	{0x8f,0x03},
	{0x0f,0x00},
	{0xfd,0x00},
#if 1  //  0//ndef DEBUG_SENSOR
	{0xfd,0x02},
	{0xe8,0x54},
	{0xec,0x58},
	{0xe9,0x4c},
	{0xed,0x54},
	{0xea,0x44},
	{0xee,0x48},
	{0xeb,0x40},
	{0xef,0x40},
	{0xfd,0x00},
#endif
	SensorEnd
};
/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	//UXGA window
	{0xfd,0x01},	
	{0x4a,0x00},
	{0x4b,0x04},
	{0x4c,0xb0},	
	{0x4d,0x00},
	{0x4e,0x06},
	{0x4f,0x40},
	
	{0xfd,0x00},
	{0x19,0x00},
	{0x30,0x08},
	{0x33,0x00},
	{0x31,0x00},
	
	{0xfd,0x02},
	{0x8f,0x03},
	{0x0f,0x00},
	{0xfd,0x00},
#if 1  //  0//ndef DEBUG_SENSOR
	{0xfd,0x02},
	{0xe8,0x54},
	{0xec,0x58},
	{0xe9,0x4c},
	{0xed,0x54},
	{0xea,0x44},
	{0xee,0x48},
	{0xeb,0x40},
	{0xef,0x40},
	{0xfd,0x00},
#endif
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	//svga
	{0xfd,0x01},
	{0x4a,0x00},
	{0x4b,0x04},
	{0x4c,0xb0},
	{0x4d,0x00},
	{0x4e,0x06},
	{0x4f,0x40},
	
	{0xfd,0x00},
	{0x19,0x00},
	{0x30,0x08},
	{0x33,0x00},
	{0x31,0x00},
	
	{0xfd,0x02},
	{0x8f,0x02},
	{0x0f,0x01},
	{0x40,0x00},
	{0x41,0x40},
	{0x42,0x00},
	{0x43,0x40},
	{0x44,0x02},
	{0x45,0x58},//600
	{0x46,0x03},
	{0x47,0x20},//800
	{0xfd,0x00},
#if 1  //  0//ndef DEBUG_SENSOR
 	{0xfd,0x02},
	{0xe8,0x64},
	{0xec,0x68},
	{0xe9,0x5c},
	{0xed,0x64},
	{0xea,0x54},
	{0xee,0x58},
	{0xeb,0x50},
	{0xef,0x50},
	{0xfd,0x00},
#endif
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorRegVal(0x02,0),
	SensorEnd
};
/*
 *  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
 */
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0xfd,0x02}, 
	{0x26,0xa0},
	{0x27,0x86},
	{0xfd,0x01},
	{0x32,0x15},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0xfd,0x01},
	{0x32,0x05},
	{0xfd,0x02},
	{0x26,0xe2},
	{0x27,0x82},	
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	{0xfd,0x01},
	{0x32,0x05},
	{0xfd,0x02},
	{0x26,0xc1},
	{0x27,0x88},	
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	{0xfd,0x01},
	{0x32,0x05},
	{0xfd,0x02},
	{0x26,0x7b},
	{0x27,0xd3},	
	SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	{0xfd,0x01},
	{0x32,0x05},
	{0xfd,0x02},
	{0x26,0xae},
	{0x27,0xcc},	
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//	Brightness 0
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//	Brightness +2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//	Brightness +3
	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0xfd,0x01},
	{0x66,0x00},
	{0x67,0x80},
	{0x68,0x80},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
	{0xfd,0x01},
	{0x66,0x20},
	{0x67,0x80},
	{0x68,0x80},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0x98},
	{0x68,0x58},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	{0xfd,0x01},
	{0x66,0x08},
	{0x67,0x80},
	{0x68,0x80},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0x80},
	{0x68,0xb0},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0x50},
	{0x68,0x50},
	{0xdb,SP2519_P1_0xdb},
	{0x34,0xc7},
	{0xfd,0x02},
	{0x14,0x00},
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	{0xfd, 0x01},
	{0xcd,SP2519_NORMAL_Y0ffset},
 #if 1
	//ae setting 24M 50hz 2.5pll 8-11.2fps uxga
	{0xfd,0x00},
	{0x03,0x03},
	{0x04,0x42},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x00},
	{0x0a,0x9e},//9d modified by LC 20141120
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x8b},
	{0xf8,0x74},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x8b},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x0f},
	{0x3e,0x74},
	{0x3f,0x00},
	{0x88,0xae},
	{0x89,0x69},
	{0x8a,0x43},
	{0xfd,0x02},
	{0xbe,0x84},
	{0xbf,0x06},
	{0xd0,0x84},
	{0xd1,0x06},
	{0xc9,0x84},
	{0xca,0x06},
	{0xfd,0x00},
 #else
	 //23M 50HZ 2.5PLL 8-10.85fps UXGA
	{0xfd,0x00},
	{0x03,0x03},
	{0x04,0x24},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x00},
	{0x0a,0x97},
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x86},
	{0xf8,0x70},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x86},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x0f},
	{0x3e,0x70},
	{0x3f,0x00},
	{0x88,0xd2},
	{0x89,0x92},
	{0x8a,0x43},
	{0xfd,0x02},
	{0xbe,0x48},
	{0xbf,0x06},
	{0xd0,0x48},
	{0xd1,0x06},
	{0xc9,0x48},
	{0xca,0x06},
	{0xfd,0x00},
 #endif
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	{0xfd, 0x01},
	{0xcd,SP2519_LOWLIGHT_Y0ffset},
#if 1
	//ae setting 24M 2.5pll 50hz 6-10fps uxga
	{0xfd,0x00},
	{0x03,0x02},
	{0x04,0xe8},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x01},
	{0x0a,0x21},//20 modified by LC 20141120
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x7c},
	{0xf8,0x67},
	{0x02,0x10},
	{0x03,0x01},
	{0x06,0x7c},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x14},
	{0x3e,0x67},
	{0x3f,0x00},
	{0x88,0x21},
	{0x89,0xf8},
	{0x8a,0x44},
	{0xfd,0x02},
	{0xbe,0xc0},
	{0xbf,0x07},
	{0xd0,0xc0},
	{0xd1,0x07},
	{0xc9,0xc0},
	{0xca,0x07},
	{0xfd,0x00},
#else
	//23M 50HZ 6-10fps 2.5PLL UXGA
	{0xfd,0x00},
	{0x03,0x02},
	{0x04,0xe8},
	{0x05,0x00},
	{0x06,0x00},
	{0x07,0x00},
	{0x08,0x00},
	{0x09,0x00},
	{0x0a,0xed},
	{0xfd,0x01},
	{0xf0,0x00},
	{0xf7,0x7c},
	{0xf8,0x67},
	{0x02,0x10},
	{0x03,0x01},
	{0x06,0x7c},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	{0xfd,0x02},
	{0x3d,0x14},
	{0x3e,0x67},
	{0x3f,0x00},
	{0x88,0x21},
	{0x89,0xf8},
	{0x8a,0x44},
	{0xfd,0x02},
	{0xbe,0xc0},
	{0xbf,0x07},
	{0xd0,0xc0},
	{0xd1,0x07},
	{0xc9,0xc0},
	{0xca,0x07},
	{0xfd,0x00},
#endif

	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
 * User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
 */
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
 * User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
 */
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_YUYV8_2X8, V4L2_COLORSPACE_JPEG} 
};
/*static struct soc_camera_ops sensor_ops;*/


/*
 **********************************************************
 * Following is local code:
 * 
 * Please codeing your program here 
 **********************************************************
 */

/*
 **********************************************************
 * Following is callback
 * If necessary, you could coding these callback
 **********************************************************
 */
/*
 * the function is called in open sensor  
 */
static int sensor_activate_cb(struct i2c_client *client)
{
	SENSOR_DG("%s",__FUNCTION__);	
	return 0;
}
/*
 * the function is called in close sensor
 */
static int sensor_deactivate_cb(struct i2c_client *client)
{
	//struct generic_sensor *sensor = to_generic_sensor(client);

	SENSOR_DG("%s",__FUNCTION__);
	return 0;
}
/*
 * the function is called before sensor register setting in VIDIOC_S_FMT  
 */
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
/*
 * the function is called after sensor register setting finished in VIDIOC_S_FMT  
 */
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{

	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");

	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{
	SENSOR_DG("Resume");
	return 0;
}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	int err = 0;
	SENSOR_DG("mirror: %d",mirror);
	return err;    
}
/*
 * the function is v4l2 control V4L2_CID_HFLIP callback  
 */
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
		struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);

	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	int err = 0;	

	SENSOR_DG("flip: %d",flip);

	return err;    
}
/*
 * the function is v4l2 control V4L2_CID_VFLIP callback  
 */
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
		struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);

	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);
	return 0;
}
/*
 * the functions are focus callbacks
 */
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
	return 0;
}

/*
   face defect call back
   */
static int 	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
 *   The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
 * initialization in the function. 
 */
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	return;
}

/*
 * :::::WARNING:::::
 * It is not allowed to modify the following code
 */

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();


