// Tianchu Xie (tx675)
// Ruizheng Zhang (rz2378)
// Use I2C sda in PC_9 pin, with PullUp, 
// scl in PA_8 pin
#include <mbed.h>
#include "drivers/LCD_DISCO_F429ZI.h"
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

LCD_DISCO_F429ZI lcd;


#define SENSOR_ADDRESS 0b0011000

// wait period from sensor (us)
const int wait_period=250'000;
const int wait_1s = 1'000'000;
const float time_period = 0.25;

const int  Pmin = 0;
const int  Pmax = 300;
const int  Omax = 3774874;
const int  Omin = 419430;
DigitalInOut sdaDummy(PinName=PC_9,PinMode=PullUp);
DigitalIn sclDummy(PA_8,PullUp);
I2C Wire(PC_9,PA_8);
//the sensor's address for read transactions
static constexpr uint8_t read_addr=((SENSOR_ADDRESS<<1u)|1U); 
//the sensor's address for write transactions
static constexpr uint8_t write_addr=(SENSOR_ADDRESS<<1U);
static uint8_t read_buf[18];
static const uint8_t read_command[3]={0xAA,0x00,0x00};

float inPressure_arr[100];
float pressure_arr[1000];
float diff_arr[1000];
int pressure_index = 0;

// calcuation for mapping, diastolic, systolic BP
int getMaxIndex(){
  int index=0;
  float maxDiff=0;
  for (int i = 1; i < pressure_index; i++){
    if (pressure_arr[i] <50){
      break;
    }
    if (diff_arr[i] > maxDiff && pressure_arr[i] < 150) {
      index = i;
      maxDiff = diff_arr[i];
    }
  }
  return index;
}
int getFirst(){
  for(int i = 1;i<pressure_index;i++){
     if(diff_arr[i]>0 && pressure_arr[i] < 150)
      return i;
  }
  return -1;
}


float getHeartRate(int a, int b){
  int counter = 0;
  for (int i = a; i <= b; i++){
     if (diff_arr[i] > 0){
       counter++;
     }
  }
  return 60/((b - a) * time_period / counter);
}


void DisplayStartPage()
{
      lcd.Clear(LCD_COLOR_BLUE);
      lcd.SetBackColor(LCD_COLOR_BLUE);
      lcd.SetTextColor(LCD_COLOR_WHITE);
      lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"Hello", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"Press Blue Button", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"To Start", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(8), (uint8_t *)"", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(9), (uint8_t *)"By", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(10), (uint8_t *)"Tianchu Xie", CENTER_MODE);
      lcd.DisplayStringAt(0, LINE(11), (uint8_t *)"Ruizheng Zhang", CENTER_MODE);

      
}
void Displayfloat(int lineNum, float a){
    char display_buf[3][60];
    snprintf(display_buf[0],60,"%f",a);
    lcd.DisplayStringAt(0, LINE(lineNum), (uint8_t *)display_buf[0], LEFT_MODE);
}





int readPressure()
{
     uint8_t status=0;
     uint8_t data1;
     uint8_t data2;
     uint8_t data3;
      int pressure = 0;
      status = 0;
      data1 = 0;
      data2 = 0;
      data3 = 0;
      Wire.write(write_addr,(const char *) read_command,3,false);
      wait_us(5000);
      Wire.read(read_addr,(char*)&read_buf,4,false);
       status = (uint8_t)read_buf[0];
       data1 = (uint8_t)read_buf[1];
       data2 = (uint8_t)read_buf[2];
       data3 = (uint8_t)read_buf[3];
      pressure|=((uint8_t)read_buf[1]<<16);
      pressure|=((uint8_t)read_buf[2]<<8);
      pressure|=((uint8_t)read_buf[3]);
      if (status != 64){
        printf("Error in reading data Status is %u\n",status);
      }
  return pressure;
}
float getRealPressure(int rawPressure){
  return ((float)rawPressure - Omin) * (Pmax - Pmin) / (Omax - Omin) + Pmin;
}

float getPressure(){
  int rawPressure= 0;
  rawPressure = readPressure();
  return getRealPressure(rawPressure);
}


void inflationMeasure()
{
   printf("Start pumping pressure cuff till 150 mmHg\n");
   lcd.Clear(LCD_COLOR_BLUE);
   lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"Start pumping pressure", CENTER_MODE);
   lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"till 150mmHg", LEFT_MODE);
   wait_us(wait_1s);
   lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 3 second", CENTER_MODE);
   wait_us(wait_1s);
   lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 2 second", CENTER_MODE);
   wait_us(wait_1s);
   lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 1 second", CENTER_MODE);
  wait_us(wait_1s);
  lcd.Clear(LCD_COLOR_BLUE);
  lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Please Start pumping pressure", LEFT_MODE);
  lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"till 150mmHG", LEFT_MODE);


   while(true){
     float pressure = 0;
     pressure = getPressure();
     printf("%.3f\n", pressure);
     lcd.DisplayStringAt(0, LINE(4), (uint8_t *)"Current Pressure is:", LEFT_MODE);
     Displayfloat(7,pressure);

     
     if(pressure > 150){
        break;
     } 
       wait_us(wait_period);
   }
}

void deflationMeasure(){
  printf("Start deflation measure from 150 mmHg\n");
  wait_us(wait_1s);
  lcd.Clear(LCD_COLOR_BLUE);
  lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"Start release pressure", CENTER_MODE);
  lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"till 30mmHg", CENTER_MODE);
  wait_us(wait_1s);
  lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 3 second", CENTER_MODE);
  wait_us(wait_1s);
  lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 2 second", CENTER_MODE);
  wait_us(wait_1s);
  lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Start in 1 second", CENTER_MODE);
  lcd.Clear(LCD_COLOR_BLUE);

  while(true)
  {
     float pressure = 0;
     pressure = getPressure();
     if(pressure < 30)
       break ;
     printf("%.3f\n",pressure);
     float diff = pressure - pressure_arr[pressure_index-1];
     
     if(fabs(diff) > 4 * time_period){
       printf("drop so fast\n"); 
       lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Drop so fast Please slow", LEFT_MODE);
     } else if (fabs(diff) <  (1 * time_period)){
       printf("drop too slow\n"); 
       lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Drop so slow Please fast", LEFT_MODE);
     } else {
       lcd.Clear(LCD_COLOR_BLUE);
     }
     lcd.DisplayStringAt(0, LINE(4), (uint8_t *)"Diff is", LEFT_MODE);
     Displayfloat(5, diff);
     lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"Current Pressure is:", LEFT_MODE);
     Displayfloat(8, pressure);

    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Time is", CENTER_MODE);
     Displayfloat(8,pressure_index*(time_period));


    pressure_arr[pressure_index++]= pressure;
    diff_arr[pressure_index] = diff;
     wait_us(wait_period);
    
  }
}
DigitalIn button(BUTTON1);
DigitalOut led(LED3);
int main() {

  // put your setup code here, to run once:
  printf("initial code\n");
  int maxIndex = 0;
  int diaIndex = 0;
  int sysIndex = 0;
  int sysIndex1 = 0;
  float BP = 0;
  
  lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"Blood Pressure Measurer", CENTER_MODE);
  lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"Start pumping pressure", LEFT_MODE);
  DisplayStartPage();
  while(1) {

      if(button)
        {
            led = !led;
            inflationMeasure();
            deflationMeasure();
            maxIndex = getMaxIndex();
            sysIndex = getFirst();
            BP = pressure_arr[maxIndex];
            float diastolic = (3*BP - pressure_arr[sysIndex])/2;
            printf("The average BP is %f\n",BP);
            for (int i = 0; i < pressure_index; i++){
              printf("%f  ",pressure_arr[i]);
            }
            printf("\n");
            printf("\n");
            printf("The DiastolicDP is %f\n",diastolic);
            printf("The Systolic is %f\n",pressure_arr[sysIndex]);
            printf("heartRate is %f\n",getHeartRate(sysIndex,maxIndex));
            
            lcd.Clear(LCD_COLOR_BLUE);
            lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"DiastolicDP is", LEFT_MODE);
            Displayfloat(4,diastolic);
            lcd.DisplayStringAt(0, LINE(6), (uint8_t *)"Systolic is", LEFT_MODE);
            Displayfloat(7,pressure_arr[sysIndex]);
            lcd.DisplayStringAt(0, LINE(9), (uint8_t *)"heartRate is", LEFT_MODE);
            Displayfloat(10,getHeartRate(sysIndex,maxIndex));

        }
  }
}
