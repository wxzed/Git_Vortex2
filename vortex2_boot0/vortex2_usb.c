
#include "app_uart.h"
#include "app_usbd.h"
#include "app_usbd_types.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"
#include "app_usbd_hid_generic.h"
#include "app_usbd_hid_mouse.h"
#include "app_usbd_hid_kbd.h"

#include "nrf_log.h"
#include "nrf_gpio.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_power.h"

#include "vortex2_usb.h"
#include "vortex2_global.h"
#include "vortex2_flash.h"
#include "vortex2_hid_protocol.h"

#include "sdk_errors.h"


#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

#define HID_GENERIC_INTERFACE  2
#define HID_GENERIC_EPIN       NRF_DRV_USBD_EPIN3

#define HID_DATA_EPOUT      NRF_DRV_USBD_EPOUT5

#define REPORT_IN_QUEUE_SIZE    1

#define REPORT_OUT_MAXSIZE 64

#define HID_GENERIC_EP_COUNT  2

#define ENDPOINT_LIST()                                      \
(                                                            \
        HID_GENERIC_EPIN                                     \
        ,NRF_DRV_USBD_EPOUT5                                  \
)

#define APP_USBD_HID_VORTEX2_REPORT_DSC(dst){                            \
    0x05, 0x8C,       /* Usage Page (Generic Desktop),       */     \
    0x09, 0x06,       /* Usage (),                      */     \
    0xA1, 0x01,       /*  Collection (Application),          */     \
    0x09, 0x06,       /*   Usage (Pointer),                  */     \
    0x15, 0x00,                                                     \
    0x26, 0x00, 0xff,                                               \
    0x75, 0x08,       /* REPORT_SIZE (8) */                         \
    0x95, 0x40,       /* REPORT_COUNT (64) */                       \
    0x91, 0x82,                                                     \
    0x09, 0x06,                                                     \
    0x15, 0x00,                                                     \
    0x26, 0x00, 0xff,                                               \
    0x75, 0x08,       /* REPORT_SIZE (8) */                         \
    0x95, 0x40,       /* REPORT_COUNT (64) */                       \
    0x81, 0x82, /* INPUT(Data,Var,Abs,Vol) */                       \
    0xc0 /* END_COLLECTION */                                       \
}
APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(vortex_desc, APP_USBD_HID_VORTEX2_REPORT_DSC(0));
static const app_usbd_hid_subclass_desc_t * reps[] = {&vortex_desc};

static bool m_usb_connected = false;
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

static uint8_t senddata[64]={0x00};


static char m_cdc_data_array[CDC_MAX_DATA_LEN];
static char CDC_DATA_BUF[CDC_MAX_DATA_LEN];

static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event);
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic,
                                HID_GENERIC_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(), 
                                reps,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_BOOT,
                                APP_USBD_HID_PROTO_GENERIC);

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;

        case APP_USBD_EVT_DRV_RESUME:
            break;

        case APP_USBD_EVT_STARTED:
            break;

        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;

        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;

        case APP_USBD_EVT_POWER_REMOVED:
        {
            NRF_LOG_INFO("USB power removed");
            m_usb_connected = false;
            app_usbd_stop();
        }
            break;

        case APP_USBD_EVT_POWER_READY:
        {
            NRF_LOG_INFO("USB ready");
            m_usb_connected = true;
            app_usbd_start();
        }
            break;

        default:
            break;
    }
}

size_t datasize;
uint64_t len = 0;
uint8_t now_status = idle_status;
uint8_t report_success_data[]       ={0x55,0xAA,0x06,0x11,0x10,0x01,0x00,0x00,0x00,0x00};
uint8_t report_fail_data[]          ={0x55,0xAA,0x06,0x10,0x10,0x00,0x00,0x00,0x00,0x00};
uint8_t report_recevice_done_data[] ={0x55,0xAA,0x06,0x20,0x10,0x10,0x00,0x00,0x00,0x00};
uint8_t report_data[REPORT_OUT_MAXSIZE];
static void report_hid_data(uint8_t *data)
{
    memset(report_data,0xFF,REPORT_OUT_MAXSIZE);
    memcpy(report_data,data,10);
    app_usbd_hid_generic_in_report_set(&m_app_hid_generic,report_data,REPORT_OUT_MAXSIZE);
}
static void analyze_data(uint8_t *data)
{
    if(data[0] == 0x55 && data[1] == 0xAA){
        uint8_t len = data[2];
        if(data[4] == writeflash){
            now_status = write_status;
            NRF_LOG_INFO("writeflash");
            report_hid_data(report_success_data);
        }
    }else{/*Invalid data*/

    }
}
struct protocol_data mydata;
bool get_package_valid(uint8_t *data)             /*Determine if it is a valid package*/
{
    if(data[0] == 0x55 & data[1] == 0xAA){
        mydata.head0 = data[0];
        mydata.head1 = data[1];
        mydata.len   = data[2];
        mydata.cs    = data[3];
        mydata.ctr0  = data[4];
        mydata.ctr1  = data[5];
        mydata.addr  = ((data[6] << 24) | (data[7] << 16) | (data[8] << 8) | (data[9]));
        memcpy(mydata.flash_data,data+10,mydata.len);
        if((mydata.ctr0 == 0x10) && (mydata.ctr1 ==0x10)){/*recevice down*/
          return false;
        }else{
          return true;
        }
        
    }else{
        return false;
    }
}
static bool check_address(uint32_t incom_addr,uint32_t partition_addr)                                      /*Check if the incoming address is legal */
{
    if(partition_addr == APP_ADDR){
        if((incom_addr >= APP_ADDR) && (incom_addr <= 0x7B000)){/*200K*/
          return true;
        }
    }else if(partition_addr == BOOT0_ADDR){
       if((incom_addr >= BOOT0_ADDR) && (incom_addr < BOOT1_ADDR)){
          return true;
       }
    }else if(partition_addr == BOOT1_ADDR){
       if((incom_addr >= BOOT1_ADDR) && (incom_addr < APP_ADDR)){
          return true;
       }
    }else{/*softdevice_add*/
      return false;
    }
    return false;
}
static void verify_and_write_flash(uint8_t *data,size_t datasize)                  /*verify and write flash*/
{
    if(get_package_valid(data)){                                            /*Data valid*/
        if((mydata.ctr0 ==writeflash) && (mydata.ctr1 == app_flash)){
            if(check_address(mydata.addr,APP_ADDR)){                                    /*write flash*/
                if(mydata.addr % 0x1000 == 0){
                    vortex2_flash_page_erase(mydata.addr);
                    NRF_LOG_INFO("erase_page=0x%X",mydata.addr);
                }
                vortex2_flash_write_bytes(mydata.addr,mydata.flash_data,mydata.len);
                report_hid_data(report_success_data);
                memset(mydata.flash_data,0xFF,32);
            }else{
                NRF_LOG_INFO("check_address_fail");
                report_hid_data(report_fail_data);
            }
        }else if((mydata.ctr0 ==writeflash) && (mydata.ctr1 == boot0_flash)){            /*write boot0*/
            if(check_address(mydata.addr,BOOT0_ADDR)){
            }
            NRF_LOG_INFO("writeflash---boot0_flash");
        }else if((mydata.ctr0 ==writeflash) && (mydata.ctr1 == boot1_flash)){             /*write boot1*/
            if(check_address(mydata.addr,BOOT1_ADDR)){
            }
            NRF_LOG_INFO("writeflash---boot1_flash");
        }else{                                                                     /*Reserved data interface*/
            NRF_LOG_INFO("else");
        }
    }else{                                                                         /*Data err or recevice down*/
        if(mydata.ctr0 == 0x10 && mydata.ctr1 == 0x10){                             
            now_status = idle_status;
            NRF_LOG_INFO("report recevie done data");
            report_hid_data(report_recevice_done_data);                            /*report recevice done data*/
            vortex2_updata_vram_messge("APP_OK");
        }
    }
}
static void resolution_hid_protocol(app_usbd_hid_generic_t const * p_generic){
    app_usbd_hid_inst_t const * p_hinst = &p_generic->specific.inst.hid_inst;
    if(now_status == idle_status){
        uint8_t revevice_data[REPORT_OUT_MAXSIZE];
        memset(revevice_data,0,REPORT_OUT_MAXSIZE);
        memcpy(revevice_data,p_hinst->p_rep_buffer_out->p_buff+1,datasize);
        analyze_data(revevice_data);
    }else if(now_status ==write_status ){/*begin transmission *.bin*/
        //NRF_LOG_INFO("verify_and_write_flash");
        verify_and_write_flash(p_hinst->p_rep_buffer_out->p_buff+1,datasize);
    }else{
    }
}

static void sendData(app_usbd_hid_generic_t const * p_generic){
    app_usbd_hid_inst_t const * p_hinst = &p_generic->specific.inst.hid_inst;
    /*
    app_usbd_hid_generic_in_report_set(
            &m_app_hid_generic,
            p_hinst->p_rep_buffer_out->p_buff+1,
            datasize);*/
            /*
    if(len % 0x1000 == 0){
      vortex2_flash_page_erase(APP_ADDR+len);
    }
    vortex2_flash_write_bytes(APP_ADDR+len,p_hinst->p_rep_buffer_out->p_buff+1,datasize-1);
    len += datasize-1;*/
}

static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event)
{
    switch (event)
    {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
        {
            app_usbd_hid_generic_out_report_get(&m_app_hid_generic,&datasize);
            //sendData(&m_app_hid_generic);
            resolution_hid_protocol(&m_app_hid_generic);
            break;
        }
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
        {
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        {
            NRF_LOG_INFO("SET_BOOT_PROTO");
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
        {
            NRF_LOG_INFO("SET_REPORT_PROTO");
            break;
        }
        default:
            NRF_LOG_INFO("default");
            break;
    }
}


static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            /*Setup first transfer*/
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_cdc_data_array,
                                                   1);
            UNUSED_VARIABLE(ret);
            NRF_LOG_INFO("CDC ACM port opened");
            break;
        }

        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            NRF_LOG_INFO("CDC ACM port closed");
            if (m_usb_connected)
            {
            }
            break;

        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            break;

        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            ret_code_t ret;
            static uint8_t index = 0;
            if(index > 0){
              uint8_t temp = m_cdc_data_array[0];
              memset(m_cdc_data_array,0,index);
              m_cdc_data_array[0] = temp;
              index = 0;
            }
            index++;

            do
            {
                size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                app_uart_put(m_cdc_data_array[index-1]);
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                            &m_cdc_data_array[index],
                                            1);
                if (ret == NRF_SUCCESS)
                {
                    index++;
                }
            }
            while (ret == NRF_SUCCESS);
            break;
        }
        default:
            NRF_LOG_INFO("default");
            break;
    }
}
static const app_usbd_config_t usbd_config = {
    .ev_state_proc = usbd_user_ev_handler
};

static void init_usb_cdc(){
    ret_code_t ret;
    app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);
}

static void init_usb_hid(){
    ret_code_t ret;
    app_usbd_class_inst_t const * class_inst_generic;
    class_inst_generic = app_usbd_hid_generic_class_inst_get(&m_app_hid_generic);
    ret = app_usbd_class_append(class_inst_generic);
    APP_ERROR_CHECK(ret);
}

void init_vortex2_usb(void){
    ret_code_t ret;
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    
    //app_usbd_serial_num_generate();
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    //init_usb_cdc();
    init_usb_hid();
    ret = app_usbd_power_events_enable();
    APP_ERROR_CHECK(ret);

}
void uninit_vortex2_usb(){
    ret_code_t ret;
    ret = app_usbd_uninit();
    APP_ERROR_CHECK(ret);
    app_usbd_stop();
    app_usbd_disable();
    nrf_drv_power_usbevt_uninit();
}