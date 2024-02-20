// /*
//  * Name : Sushant Shanbhag
//  * Date : 05-12-2023
//  * Project : CAR BLACK BOX
// */


#include "main.h"

#pragma config WDTE = OFF     // Watchdog Timer Enable bit (WDT disabled)

char *gears[] = {"GN", "GR", "G1", "G2", "G3", "G4","G5","C","ON"}; 
       
char pre_key, pre_control_flag = RESET_NOTHING;
int press = 0;
extern char time[7];
unsigned char new_sec, new_min, new_hr;

static void init_config(void) {
    //Initialize CLCD
    init_clcd();
    //Initialize ADC
    init_adc();
    //Initialize DKP
    init_digital_keypad();
    //Initialize I2c
    init_i2c(100000); //100k
    //Initialize RTC
    init_ds1307();
    //Initialize TIMER2
    init_timer2();
    //Initialize UART
    init_uart(9600);
    
    puts("UART Test Code\n\r");
    
    /* Peripheral Interrupt Enable Bit (For Timer2) */
    PEIE = 1;
    
    /* Enable all the Global Interrupts */
    GIE = 1;
    
}

void main(void) {
    unsigned char control_flag = DASHBOARD_FLAG, reset_flag;    //Default screen
    char event[3] = "ON";
    unsigned char key, speed = 0, gr = 0;    //0 to 99
    char menu_pos, return_val;
    extern int return_time;
    
    init_config();
    
    /* To log initial state of car event and the speed in the EEprom*/
    log_car_event(gears, speed,8);
    
    /*To store the password in the field location of the ext EEprom 24c02*/
    
    ext_eeprom_24C02_str_write(0x00, "1010");
    
    while (1) {
        /*Read speed from POT1*/
        speed = (unsigned char)(read_adc() / 10);   //0 - 1023
        if (speed > 99)
        {
            speed = 99;
        }
        
        //read SW press to print event
        key = read_digital_keypad(LEVEL);
        if ((key != ALL_RELEASED) && (key != SW6))
        {
            if (pre_control_flag != LOGIN_MENU_FLAG)
            {
                press++;
                pre_key = key;
            }
        }
        else if (key == ALL_RELEASED)
            press = pre_control_flag = RESET_NOTHING;
        
        /*To remove key debouncing effect*/
        for (int i = 2000 ; i-- ; );
        
        if (key == ALL_RELEASED)
        {
            if (control_flag == DASHBOARD_FLAG)
            {
                if (pre_key == SW1 && strcmp(event, "C "))     //For collision
                {
                    strcpy(event, "C ");
                    log_car_event(gears, speed,7);
                    pre_key = press = 0;
                }
                
                else if (pre_key == SW2 && gr < 7)   //For increasing gears
                {
                    strcpy(event, gears[gr]);
                    gr++;
                    log_car_event(gears, speed,gr);
                    pre_key = press = 0;
                }
                    
                else if (pre_key == SW3 && gr > 0)   //For decreasing gears
                {
                    gr--;
                    strcpy(event, gears[gr]);
                    log_car_event(gears, speed,gr);
                    pre_key = press = 0;
                }
        
                /*To enter the password*/
                else if (pre_key == SW4 || pre_key == SW5)
                {
                    clear_screen();
                    clcd_print(" ENTER PASSWORD ", LINE1(0));
                    clcd_putch(' ', LINE2(5));
                    clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
                    __delay_us(100);
                    control_flag = LOGIN_FLAG;
                    reset_flag = RESET_PASSWORD;
                    TMR2ON = 1;
                    pre_key = press = 0;
                }
            }
        }
            
            
        if (press > 13)
        {
            if (pre_key == SW4)       //SW4 pressed more than 2 sec
            {
                if (control_flag == LOGIN_MENU_FLAG)
                {
                    pre_control_flag = LOGIN_MENU_FLAG;
                    switch(menu_pos)
                    {
                        case 0:
                            //view log
                            control_flag = VIEW_LOG_FLAG;
                            reset_flag = RESET_VIEW_LOG_POS;
                            break;
                        case 1:
                            //clear log
                            control_flag = CLEAR_LOG_FLAG;
                            reset_flag = RESET_MEMORY;
                            break;
                        case 2:
                            //download log
                            control_flag = DOWNLOAD_LOG_FLAG;
                            break;
                        case 3:
                            //set time
                            control_flag = SET_TIME_FLAG;
                            reset_flag = RESET_TIME;
                            break;
                        case 4:
                            //change password
                            control_flag = CHANGE_PASSWORD_FLAG;
                            reset_flag = RESET_PASSWORD;
                            break;
                    }
                }
                else if (control_flag == VIEW_LOG_FLAG || control_flag == CHANGE_PASSWORD_FLAG)
                {
                    control_flag = LOGIN_MENU_FLAG;
                    reset_flag = RESET_LOGIN_MENU;
                    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                    __delay_us(100);
                }
                else if (control_flag == SET_TIME_FLAG)
                {
                    control_flag = LOGIN_MENU_FLAG;
                    reset_flag = RESET_LOGIN_MENU;
                    write_ds1307(SEC_ADDR, (new_sec / 10 << 4) | (new_sec % 10));    //59 / 10 = 5,  59 % 10 = 9
                    write_ds1307(MIN_ADDR, (new_min / 10 << 4) | (new_min % 10));
                    write_ds1307(HOUR_ADDR, (new_hr / 10 << 4) | (new_hr % 10));
                }
                pre_key = press = 0;
            }
                
            else if (pre_key == SW5)      //SW5 pressed more than 2 sec
            {
                if (control_flag == LOGIN_MENU_FLAG || control_flag == VIEW_LOG_FLAG || control_flag == SET_TIME_FLAG || control_flag == CHANGE_PASSWORD_FLAG)
                {
                    clear_screen();
                    pre_control_flag = LOGIN_MENU_FLAG;
                    control_flag = DASHBOARD_FLAG;       //display Default screen
                    pre_key = press = 0;
                    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                    __delay_us(100);
                    TMR2ON = 0;
                }
            }
        }
        
        if (pre_control_flag == LOGIN_MENU_FLAG)
            pre_key = press = 0;
        
        switch(control_flag)
        {
            case DASHBOARD_FLAG:   //default case
                display_default_screen(event, speed);
                break;
            case LOGIN_FLAG:   //login case
                switch(login(reset_flag, key))
                {
                    case RETURN_BACK:
                        clear_screen();
                        control_flag = DASHBOARD_FLAG;
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);
                        TMR2ON = 0;
                        break;
                    case LOGIN_SUCCESS:
                        control_flag = LOGIN_MENU_FLAG;
                        reset_flag = RESET_LOGIN_MENU;
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);
                        continue;
                        break;
                }
                break;
            case LOGIN_MENU_FLAG:
                menu_pos = login_menu(reset_flag,key);
                if (return_time == 0)
                {
                    clear_screen();
                    control_flag = DASHBOARD_FLAG;
                    TMR2ON = 0;
                }
                break;
            case VIEW_LOG_FLAG:
                return_val = view_log(reset_flag, key);
                
                if (return_val == TASK_FAIL)
                {
                    control_flag = LOGIN_MENU_FLAG;
                    reset_flag = RESET_LOGIN_MENU;
                    continue;
                }
                else if (return_time == 0)
                {
                    clear_screen();
                    control_flag = DASHBOARD_FLAG;
                    TMR2ON = 0;
                }
                break;
            case CLEAR_LOG_FLAG:
                clear_log(reset_flag);
                control_flag = LOGIN_MENU_FLAG;
                reset_flag = RESET_LOGIN_MENU;
                continue;
                break;
            case DOWNLOAD_LOG_FLAG:
                download_log();
                control_flag = LOGIN_MENU_FLAG;
                reset_flag = RESET_LOGIN_MENU;
                continue;
                break;
            case SET_TIME_FLAG:
                edit_time(reset_flag, key);
                if (return_time == 0)
                {
                    clear_screen();
                    control_flag = DASHBOARD_FLAG;
                    TMR2ON = 0;
                }
                break;
            case CHANGE_PASSWORD_FLAG:
                return_val = change_password(reset_flag, key);
                if (return_val == TASK_SUCCESS)
                {
                    control_flag = LOGIN_MENU_FLAG;
                    reset_flag = RESET_LOGIN_MENU;
                    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                    __delay_us(100);
                    continue;
                }
                else if (return_time == 0)
                {
                    clear_screen();
                    control_flag = DASHBOARD_FLAG;
                    clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                    __delay_us(100);
                    TMR2ON = 0;
                }
                break;
                    
        }
        
        reset_flag = RESET_NOTHING;
    }
    return;
}
