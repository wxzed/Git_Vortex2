import sys
import argparse
import pywinusb.hid as hid
import serial
import time


writeboot0=0
writeboot1=1
writeapp  =2
datacs    =0
updata_done = 1
report_id  = [0x00]
device_num = [0x00]
hh_data    = [0xff]*22
flash_data = [0x00]*54#去除数据头和report id 的有效数据
contorl_commands = 0
down_over = 0
now_status = 0
times = 0
'''CTR0-->read/write  CTR1-->boot0/boot1/app/softdevice/'''
'''                                      LEN  CS  CTR0 CTR1 ADD0 ADD1 ADD2 ADD3          '''
write_boot0           =      [0x55,0xAA,0x40,0x00,0x01,0x00,0x00,0x02,0xB0,0x00]
write_boot1           =      [0x55,0xAA,0x40,0x00,0x01,0x01,0x00,0x03,0xA0,0x00]
write_app             =      [0x55,0xAA,0x06,0x97,0x01,0x02,0x00,0x04,0x90,0x00]
data_head             =      [0x55,0xAA,0x06,0x03,0x01,0x02,0x00,0x00,0x00,0x00]
data_head_update_done =      [0x55,0xAA,0x06,0xB4,0x10,0x10,0x00,0x04,0x90,0x00]
app_addr    =0x00049000
offset_addr    =0x00000000

def updata_datahead(data):
    global updata_done
    global data_head
    global data_head_update_done
    if data is updata_done:
        data_head = data_head_update_done
class hidHelper(object):
    def __init__(self, vid=0x1915,pid=0x521a):
        self.alive = False
        self.device = None
        self.report = None
        self.vid = vid
        self.pid = pid
        
    def start(self):
        _filter = hid.HidDeviceFilter(vendor_id = self.vid, product_id = self.pid)
        hid_device = _filter.get_devices()
        if len(hid_device) > 0:
            self.device = hid_device[0]
            self.device.open()
            self.report = self.device.find_output_reports()
            self.alive = True
            
    def stop(self):
        self.alive = False
        if self.device:
            self.device.close()
            
    def setcallback(self):
        if self.device:
            self.device.set_raw_data_handler(self.read)
            
    def read(self,data):
        global offset_addr
        global contorl_commands
        global data_head
        global flash_data
        global down_over
        global updata_done
        #print([hex(item).upper() for item in data[1:]])
        if data[6] is 0x01:               #返回的是等待写入的命令
            datacs = 0
            c = file.read(54)
            if len(c) is 0:               #bin文件读取完成,发送APP更新完成的命令
                #print("len = 0")
                file.close()
                updata_datahead(updata_done)
                contorl_commands = 1
            else:
                for i in range(0,len(c)):
                    flash_data[i] = c[i]
                    datacs = (datacs + c[i])
                data_head[2] = len(c)
                data_head[6] = (offset_addr >> 24) & 0xFF
                data_head[7] = (offset_addr >> 16) & 0xFF
                data_head[8] = (offset_addr >> 8 ) & 0xFF
                data_head[9] = (offset_addr)       & 0xFF
                datacs = (datacs + data_head[4] + data_head[5] + data_head[6] + data_head[7] + data_head[8] + data_head[9])
                data_head[3] = (datacs & 0xFF)
                #print("len")
                #print(len(c))
                offset_addr = offset_addr + len(c)
                contorl_commands = 1
        elif data[6] is 0x10:            #接收到接收完成的命令
            down_over = 1
            contorl_commands = 0
        else:
            contorl_commands = 0
    def write(self,send_list):
        if self.device:
            if self.report:
                self.report[0].set_raw_data(send_list)
                bytes_num = self.report[0].send()
                return bytes_num
if __name__ == '__main__':
    #global data_head
    #global report_id
    #global down_over
    #global contorl_commands
    #global now_status
    parser = argparse.ArgumentParser(description='manual to this script')
    parser.add_argument('--url', type=str, default = None)
    parser.add_argument('--port', type=str, default = None)
    args = parser.parse_args()
    try:
        com = serial.Serial(args.port,115200,timeout=20)
        com.setDTR(True)
        com.setRTS(True)
        begin = bytes([0x55,0xAA])
        com.write(begin)
    except:
        print("Open Serial error")
    #print("begin search hid")
    file = open(args.url.replace('\\','/'),'rb')
    myhid = hidHelper()
    myhid.start()
    while myhid.alive is False:
        #print("no hid")
        myhid.start()
        time.sleep(0.1)
        times = times + 1
        if times > 20:
            print("no found Vortex2 HID")
            exit()
    times = 0
    if myhid.alive:
        myhid.setcallback()
        send_list = report_id + write_app + flash_data
        myhid.write(send_list)                           #发送写app 命令
    while True:
        if contorl_commands is 1:                        #写入flash数据
            #print("send data")
            send_list = report_id + data_head +flash_data
            myhid.write(send_list)
            contorl_commands = 0
            times = 0
        if down_over is 1:
            print("download done")
            break
        time.sleep(0.001)
        times = times + 1
        if times > 20:
            #print("try again")
            #print(offset_addr)
            contorl_commands = 1
    myhid.stop()
