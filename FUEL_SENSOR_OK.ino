
//CFLS = Capacitve Fuel Level Sensor
//LECTURA DE SENSOR CLS-500
//JS67107
#define RS485_DIR 6
#define RS485_TX 1
#define RS485_RX 0

#define CFLS_CMD_LIQUID_LEVEL_READING_PERCENT    "DO"
#define CFLS_CMD_AD_VALUE_CURRENT_LIQUID_LEVEL   "RY"
#define CFLS_CMD_SETTING_ID_SENSOR               "ID"
//
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z0     "Z0"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z1     "Z1"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z2     "Z2"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z3     "Z3"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z4     "Z4"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z5     "Z5"
#define CFLS_CMD_SETTING_FILTERING_FACTOR_Z6     "Z6"
//
//#define CFLS_CMD_SETTING_UPLOADING_TIME          "ST"

#define CFSL_NUM_BYTES_PACKET_WRITE 10
union _CFSL_packetWrite
{
    char packet[CFSL_NUM_BYTES_PACKET_WRITE];
    
    struct _structW
    {
        char header[2];
        char command[2];
        char id[2];
        char checksum[2];
        char ofdata[2];
    }structW;
};
union _CFSL_packetWrite CFSL_packetWrite= {{0x24, 0x21, 0x00, 0x00, 0x30, 0x31, 0x00, 0x00, 0x0D, 0x0A}};

#define CFSL_NUM_BYTES_PACKET_READ 16
union _CFSL_packetRead
{
    char packet[CFSL_NUM_BYTES_PACKET_READ];
    
    struct _structR
    {
        char header;//0x2A = '*'
        char command[3];
        char id[2];
        char data[6];
        char checksum[2];
        char end[2];
    }structR;
};

union _CFSL_packetRead CFSL_packetRead;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t CFSL_get_checksum(char *data, uint8_t num_byte)
{
    uint8_t i;
    int16_t checksum=0x0000;
    
    for (i=0; i<num_byte; i++)
        {checksum += data[i];}
    
    return (uint8_t)checksum;
}
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
static void CFSL_set_packet(const char* command, const char* id)
{
    uint8_t checksum;
    
    CFSL_packetWrite.structW.command[0]= command[0];
    CFSL_packetWrite.structW.command[1]= command[1];
    
    CFSL_packetWrite.structW.id[0]= id[0];
    CFSL_packetWrite.structW.id[1]= id[1];
    
    checksum = CFSL_get_checksum(CFSL_packetWrite.packet, 6);
    
    CFSL_packetWrite.structW.checksum[0] = converttoascii(checksum>>4);
    CFSL_packetWrite.structW.checksum[1] = converttoascii(checksum & 0x0F);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
#define CFSL_TIME_BETWEEN_WRITE_BYTE 900    //after tunning
#define CFSL_TIME_SWITCH_RX_DIR 1280//1200

static void CFSL_write_packet(char *data)
{
    uint8_t i;
    digitalWrite(RS485_DIR, RS485_TX);
    i=0;
    do
    {
        Serial2.write(data[i]);
        delayMicroseconds(CFSL_TIME_BETWEEN_WRITE_BYTE);
    }while (++i <CFSL_NUM_BYTES_PACKET_WRITE);
    
    //delayMicroseconds(CFSL_TIME_SWITCH_RX_DIR);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t CFSL_read_packet(void)
{
    uint8_t checksum;
    char checksum_ascii[2];
  
    char packet[16],i;
    
    digitalWrite(RS485_DIR, RS485_RX);
    
//Serial2.readBytes(packet, CFSL_NUM_BYTES_PACKET_READ);
Serial2.readBytes(CFSL_packetRead.packet, CFSL_NUM_BYTES_PACKET_READ);
    
    digitalWrite(RS485_DIR, RS485_TX);
/*
    i=0;
    while (packet[i] != 'R') {i++;}
   
    for (uint8_t c=1; c<13; c++)      
      CFSL_packetRead.packet[c] = packet[i++];
*/
  
    //----------------------------------------------------
    for (uint8_t i=0; i<CFSL_NUM_BYTES_PACKET_READ; i++)
    {
      Serial.write(CFSL_packetRead.packet[i]);
      //Serial.write(packet[i]);
    }
    //----------------------------------------------------

//return 1;

    if (CFSL_packetRead.structR.header == '*')//'*'
    {
        checksum = CFSL_get_checksum(CFSL_packetRead.packet, 12);
        checksum_ascii[0] = converttoascii(checksum>>4);
        checksum_ascii[1] = converttoascii(checksum & 0x0F);
        
        if (checksum_ascii[0] == CFSL_packetRead.structR.checksum[0])
            if (checksum_ascii[1] == CFSL_packetRead.structR.checksum[1])
                return 1;
    }
    
    return 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
float CFSL_get_fuelpercent(const char *id)
{
    float fuelpercent;
    char percent_buff[6+1];
    
    CFSL_set_packet(CFLS_CMD_LIQUID_LEVEL_READING_PERCENT, id);
 //   flushReceive();
    CFSL_write_packet(CFSL_packetWrite.packet);
    
    delayMicroseconds(CFSL_TIME_SWITCH_RX_DIR);
    if (CFSL_read_packet())
    {
        if (CFSL_packetRead.structR.command[0] == 'R')
          if (CFSL_packetRead.structR.command[1] == 'F')
            if (CFSL_packetRead.structR.command[2] == 'V')
                  if (CFSL_packetRead.structR.id[0] == id[0])
                    if (CFSL_packetRead.structR.id[1] == id[1])
                    {
                        for (uint8_t c=0; c<6; c++)
                            percent_buff[c] =  CFSL_packetRead.structR.data[c];
                        percent_buff[6]='\0';
                        
                        fuelpercent = strtod(percent_buff, (char**)0);
                        
                        if (1)//strtod non-free
                        {
                            return fuelpercent;
                        }
                    }
    }
    return -1;
}

void setup() 
{
  digitalWrite(13,1);
  pinMode(13, OUTPUT);
  delay(500);//stabilizing CFSL
  digitalWrite(13,0);
  
  digitalWrite(RS485_DIR, RS485_RX);
  pinMode(RS485_DIR, OUTPUT);
  
  Serial.begin(9600);
  
  Serial2.begin(9600);
  Serial2.setTimeout(4);
/*
CFSL_set_packet(CFLS_CMD_LIQUID_LEVEL_READING_PERCENT, "01");
CFSL_write_packet(CFSL_packetWrite.packet);

delayMicroseconds(1200);
digitalWrite(RS485_DIR, RS485_RX);
*/
}

void loop() 
{
  float temp, fuelpercent;
  
  temp = CFSL_get_fuelpercent("01");
  
  if (temp != -1)
  {
        fuelpercent = temp;
        //
        Serial.println(fuelpercent,2);
        digitalWrite(13, !digitalRead(13));
        //
  }
  else
      Serial.println("e");

  //  delay(3);
 
}
void flushReceive(void)
{
  while(Serial2.available())
    Serial2.read();
}
char converttoascii(char c)
{
    if (c < 10)
    {
      return c+'0';
    } else {
      return (c-10) + 'A';
    }
}
