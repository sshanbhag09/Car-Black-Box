#include "main.h"

unsigned char clock_reg[3];
char time[7];              //"HHMMSS"
char log[11];              //HHMMSS EVNT SP
char pos = -1, log_count = 0;
long int sec = 0;
extern int return_time, press;
extern char pre_key, pre_control_flag;
char *menu[] = {"View log", "Clear log", "Download log", "Set time", "Change passwd"};
              //   0            1              2             3             4
char saved_time[7], saved_speed[3], saved_event[3];
extern unsigned char new_sec, new_min, new_hr;
void display_time(void)
{

    
     // HH -> 
    time[0] = ((clock_reg[0] >> 4) & 0x03) + '0';
    time[1] = (clock_reg[0] & 0x0F) + '0';
    
   
    // MM 
    time[2] = ((clock_reg[1] >> 4) & 0x07) + '0';
    time[3] = (clock_reg[1] & 0x0F) + '0';
    
    
    // SS
    time[4] = ((clock_reg[2] >> 4) & 0x07) + '0';
    time[5] = (clock_reg[2] & 0x0F) + '0';
    time[6] = '\0';
     clcd_putch(time[0], LINE2(0));  //HH
    clcd_putch(time[1], LINE2(1));
    clcd_putch(':', LINE2(2));
    clcd_putch(time[2], LINE2(3));    //MM
    clcd_putch(time[3], LINE2(4));
    clcd_putch(':', LINE2(5));
    clcd_putch(time[4], LINE2(6));    //SS
    clcd_putch(time[5], LINE2(7));
    
   
    
}
void get_time(void)
{
    clock_reg[0] = read_ds1307(HOUR_ADDR); // HH -> BCD 
    clock_reg[1] = read_ds1307(MIN_ADDR); // MM -> BCD 
    clock_reg[2] = read_ds1307(SEC_ADDR); // SS -> BCD 
    
    
    
}
void display_default_screen(char *event, unsigned char speed)
{
    clcd_print("  TIME    EV  SP", LINE1(0));
    /*if (pre_control_flag == LOGIN_MENU_FLAG)
    {
        pre_key = press = 0;
    }*/
    get_time();
    display_time();
    clcd_print(event, LINE2(10));
    //To display speed 
    //Speed 98
    //9  on 14th pos
    //8 on 15th pos
    clcd_putch(speed / 10 + '0', LINE2(14));
    clcd_putch(speed % 10 + '0', LINE2(15));
}

void log_event(void)
{
    char addr;
    if (++pos == 10)
        pos = 0;
    addr = pos * 10 + 5;    //5, 15, 25, 35
    ext_eeprom_24C02_str_write(addr, log);
 
    if (log_count < 10)
        log_count++;
}
//HHMMSS EVNT SP
void log_car_event(char * event[], unsigned char speed,int gr)
{
    get_time();
    strncpy(log, time, 6);
    strncpy(&log[6], event[gr], 2);          //or log + 6
    log[8] = speed / 10 + '0';
    log[9] = speed % 10 + '0';
    log[10] = '\0';
    log_event();
}

void clear_screen(void)
{
    clcd_write(CLEAR_DISP_SCREEN, INST_MODE);
    __delay_us(500);
}

char login(unsigned char reset_flag, unsigned char key)
{
    char saved_passwd[4];
    static char new_passwd[4], i;
    static unsigned char attempt_rem;
    
    if (reset_flag == RESET_PASSWORD)
    {
        i = 0;
        new_passwd[0] = '\0';
        new_passwd[1] = '\0';
        new_passwd[2] = '\0';
        new_passwd[3] = '\0';
        attempt_rem = 3;
        return_time = 5;
    }
    
    if (return_time == 0)
    {
        return RETURN_BACK;
    }
    
    if (key == ALL_RELEASED)
    {
        //Read new password from user
        if (pre_key == SW4 && i < 4)     //1
        {
            new_passwd[i] = '1';
            clcd_putch('*', LINE2(i + 6));
            i++;
            return_time = 5;
            pre_key = press = 0;
        }
        else if (pre_key == SW5 && i < 4)     //0
        {
            new_passwd[i] = '0';
            clcd_putch('*', LINE2(i + 6));
            i++;
            return_time = 5;
            pre_key = press = 0;
        }
    }
    
    if (i == 4)
    {
        for (int j = 0 ; j < 4 ; j++)
        {
            saved_passwd[j] = ext_eeprom_24C02_read(j);
        }
        if (!strncmp(saved_passwd, new_passwd, 4))   //password is matching
        {
            return LOGIN_SUCCESS;
        }
        else                                        //Incorrect password
        {
            clear_screen();
            attempt_rem--;
            if (attempt_rem == 0)     //Block the system for 15min
            {
                clear_screen();
                clcd_print ("You are blocked", LINE1(0));
                clcd_print ("for 180 sec..", LINE2(0));
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
                sec = 180;
                while(sec)    //After 1min condition will fail
                {
                    clcd_putch(sec / 100 + '0', LINE2(13));
                    clcd_putch(sec % 100 / 10 + '0', LINE2(14));
                    clcd_putch(sec % 100 % 10 + '0', LINE2(15));
                }
                attempt_rem = 3;
            }
            else
            {
                clear_screen();
                clcd_print(" WRONG PASSWORD", LINE1(0));
                clcd_putch(attempt_rem + '0', LINE2(0));
                clcd_print("attempt remain", LINE2(2));
                __delay_ms(3000);
            }
            return_time = 5;
            clear_screen();
            clcd_print(" ENTER PASSWORD ", LINE1(0));
            clcd_putch(' ', LINE2(5));
            clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
            __delay_us(100);
            i = 0;
        }
    }
}

char login_menu(unsigned char reset_flag, unsigned char key)
{
    static char menu_pos;
    if (reset_flag == RESET_LOGIN_MENU)
    {
        clear_screen();
        return_time = 5;
        menu_pos = 0;
        pre_key = press = 0;
    }
    
    if (key == ALL_RELEASED && press < 13)
    {
        if (pre_key == SW5)
        {
            if (menu_pos < 4)
            {
                menu_pos++;
                return_time = 5;
                clear_screen();
            }
            pre_key = press = 0;
        }
        else if (pre_key == SW4)
        {
            if (menu_pos > 0)
            {
                menu_pos--;
                return_time = 5;
                clear_screen();
            }
            pre_key = press = 0;
        }
    }
    
    if (menu_pos < 4)
    {
        clcd_putch('*', LINE1(0));
        clcd_print(menu[menu_pos], LINE1(2));
        clcd_print(menu[menu_pos + 1], LINE2(2));
    }
    else if (menu_pos == 4)
    {
        clcd_putch('*', LINE2(0));
        clcd_print(menu[menu_pos - 1], LINE1(2));
        clcd_print(menu[menu_pos], LINE2(2));
    }
    
    return menu_pos;
}

void get_saved_log(char log_pos)
{
    unsigned char addr, i;
    addr = (log_pos - 1) * 10 + 5;    //5, 15, 25, 35
    
    for (i = 0 ; i < 6 ; i++)
    {
        saved_time[i] = ext_eeprom_24C02_read(addr + i);
    }
    saved_time[i] = '\0';
    
    saved_event[0] = ext_eeprom_24C02_read(addr + 6);
    saved_event[1] = ext_eeprom_24C02_read(addr + 7);
    saved_event[2] = '\0';
    
    saved_speed[0] = ext_eeprom_24C02_read(addr + 8);
    saved_speed[1] = ext_eeprom_24C02_read(addr + 9);
    saved_speed[2] = '\0';
    
}
void print_log(char log_pos)
{
    clcd_putch('#', LINE1(0));
    clcd_putch(log_pos -  1 + '0', LINE2(0));
    clcd_putch(saved_time[0], LINE2(2));  //HH
    clcd_putch(saved_time[1], LINE2(3));
    clcd_putch(':', LINE2(4));
    clcd_putch(saved_time[2], LINE2(5));    //MM
    clcd_putch(saved_time[3], LINE2(6));
    clcd_putch(':', LINE2(7));
    clcd_putch(saved_time[4], LINE2(8));    //SS
    clcd_putch(saved_time[5], LINE2(9));
    
    clcd_putch(saved_event[0], LINE2(11));
    clcd_putch(saved_event[1], LINE2(12));
    
    clcd_putch(saved_speed[0], LINE2(14));
    clcd_putch(saved_speed[1], LINE2(15));
    
}
char view_log(unsigned char reset_flag, unsigned char key)
{
    static char log_pos;
    
    if (reset_flag == RESET_VIEW_LOG_POS)
    {
        clear_screen();
        return_time = 5;
        if (log_count > 0)
        {
            clcd_print("   TIME    EV SP", LINE1(0));
            log_pos = 1;
            get_saved_log(log_pos);
            print_log(log_pos);
        }
    }
    
    if (log_count > 0)
    {
        if (key == ALL_RELEASED && press < 13)
        {
            if (pre_key == SW4)
            {
                if (--log_pos < 1)
                    log_pos = log_count;
                get_saved_log(log_pos);
                print_log(log_pos);
                pre_key = press = 0;
                return_time = 5;
            }
            else if (pre_key == SW5)
            {
                if (++log_pos > log_count)
                    log_pos = 1;
                get_saved_log(log_pos);
                print_log(log_pos);
                pre_key = press = 0;
                return_time = 5;
            }
        }
    }
    
    else
    {
        clcd_print("     NO LOGS    ", LINE1(0));
        clcd_print("  ARE AVAILABLE ", LINE2(0));
        __delay_ms(3000);
        return TASK_FAIL;
    }
    __delay_ms(81);
    return TASK_SUCCESS;
}

void clear_log(unsigned char reset_flag)
{
    if (reset_flag == RESET_MEMORY)
    {
        log_count = 0;
        pos = -1;
        clcd_print("LOGS ARE CLEARED", LINE1(0));
        clcd_print("  SUCCESSFULLY  ", LINE2(0));
        __delay_ms(3000);
    }
}

void display_log_in_uart(char log_pos)
{
    putchar(log_pos - 1 + '0');
    putchar(' ');
    putchar(saved_time[0]);  //HH
    putchar(saved_time[1]);
    putchar(':');
    putchar(saved_time[2]);    //MM
    putchar(saved_time[3]);
    putchar(':');
    putchar(saved_time[4]);    //SS
    putchar(saved_time[5]);
    putchar(' ');
    putchar(saved_event[0]);
    putchar(saved_event[1]);
    putchar(' ');
    putchar(saved_speed[0]);
    putchar(saved_speed[1]);
    putchar('\n');
}

void download_log(void)
{
    char i = 1;
    
    if (log_count > 0)
    {
        while (i <= log_count)
        {
            get_saved_log(i);
            display_log_in_uart(i);
            i++;
        }
        clcd_print(" LOGS DISPLAYED ", LINE1(0));
        clcd_print("     IN UART    ", LINE2(0));
    }
    else
    {
        clcd_print("     NO LOGS    ", LINE1(0));
        clcd_print("  ARE AVAILABLE ", LINE2(0));
    }
    __delay_ms(3000);
}

void edit_time(unsigned char reset_flag, unsigned char key)
{
    static char new_time[8];
    static int i, j, blink_delay, blink_flag;
    if (reset_flag == RESET_TIME)
    {
        clear_screen();
        return_time = 5;
        blink_delay = blink_flag = 0;
        clcd_print("HH MM SS", LINE1(4));
        get_time();
        strcpy(new_time, time);
        new_sec = (new_time[4] - '0') * 10 + new_time[5] - '0';
        new_min = (new_time[2] - '0') * 10 + new_time[3] - '0';
        new_hr = (new_time[0] - '0') * 10 + new_time[1] - '0';
        i = 2;
        j = 10;
    }
  
    if (key == ALL_RELEASED && press < 13)
    {
        if (pre_key == SW4)
        {
            if (i == 2)
            {
                if (++new_sec > 59)
                    new_sec = 0;
            }
            else if (i == 1)
            {
                if (++new_min > 59)
                    new_min = 0;
            }
            else
            {
                if (++new_hr > 23)
                    new_hr = 0;
            }
            return_time = 5;
            pre_key = press = 0;
        }
        else if (pre_key == SW5)
        {
            if (--i < 0)
                i = 2;
            
            if (i == 2)
                j = 10;
            else if (i == 1)
                j = 7;
            else if (i == 0)
                j = 4;
            return_time = 5;
            pre_key = press = 0;
        }
    }
    
    if (++blink_delay > 10)
    {
        blink_flag = !blink_flag;
        blink_delay = 0;
    }
    
    if (!blink_flag)
    {
        clcd_putch(new_hr / 10 + '0', LINE2(4));     //HH
        clcd_putch(new_hr % 10 + '0', LINE2(5));
        clcd_putch(':', LINE2(6));
        clcd_putch(new_min / 10 + '0', LINE2(7));     //MM
        clcd_putch(new_min % 10 + '0', LINE2(8));
        clcd_putch(':', LINE2(9));
        clcd_putch(new_sec / 10 + '0', LINE2(10));    //SS
        clcd_putch(new_sec % 10 + '0', LINE2(11));
    }
    else
    {
        clcd_putch(0xFF, LINE2(j));    
        clcd_putch(0xFF, LINE2(j + 1));
    }
}

char change_password(unsigned char reset_flag, unsigned char key)
{
    char saved_passwd[4];
    static char new_passwd_1[4], new_passwd_2[4];
    static int i, j;
    if (reset_flag == RESET_PASSWORD)
    {
        clear_screen();
        return_time = 5;
        clcd_print(" ENTER PASSWORD ", LINE1(0));
        clcd_putch(' ', LINE2(5));
        clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
        __delay_us(100);
        i = j = 0;
        new_passwd_1[0] = '\0';
        new_passwd_1[1] = '\0';
        new_passwd_1[2] = '\0';
        new_passwd_1[3] = '\0';
        new_passwd_2[0] = '\0';
        new_passwd_2[1] = '\0';
        new_passwd_2[2] = '\0';
        new_passwd_2[3] = '\0';
    }
  
    if (key == ALL_RELEASED && press < 13)
    {
         //Read new password from user
        if (pre_key == SW4)     //1
        {
            if (i < 4)
            {
                new_passwd_1[i] = '1';
                clcd_putch('*', LINE2(i + 6));
                i++;
                pre_key = press = 0;
            }
            else
            {
                new_passwd_2[j] = '1';
                clcd_putch('*', LINE2(j + 6));
                j++;
            }
            return_time = 5;
            pre_key = press = 0;
        }
        else if (pre_key == SW5)     //0
        {
            if (i < 4)
            {
                new_passwd_1[i] = '0';
                clcd_putch('*', LINE2(i + 6));
                i++;
                pre_key = press = 0;
            }
            else
            {
                new_passwd_2[j] = '0';
                clcd_putch('*', LINE2(j + 6));
                j++;
            }
            return_time = 5;
            pre_key = press = 0;
        }
    }
    
    if (i == 4)
    {
        i++;
        clear_screen();
        clcd_print("RE-ENTER", LINE1(4));
        clcd_putch(' ', LINE2(5));
    }
    else if (j == 4)
    {
        if (!strncmp(new_passwd_1, new_passwd_2, 4))   //password is matching
        {
            clear_screen();
            for (int i = 0 ; i < 4 ; i++)
            {
                ext_eeprom_24C02_byte_write(i, new_passwd_1[i]);
            }
            clcd_print("PASSWORD CHANGED", LINE1(0));
            clcd_print("  SUCCESSFULLY  ", LINE2(0));
             __delay_ms(3000);
            return TASK_SUCCESS;
        }
        else                                        //Incorrect password
        {
            i = j = 0;
            clcd_print("PASSWORD DIDN'T ", LINE1(0));
            clcd_print("MATCH, TRY AGAIN", LINE2(0));
             __delay_ms(3000);
            clear_screen();
            clcd_print(" ENTER PASSWORD ", LINE1(0));
            clcd_putch(' ', LINE2(5));
            new_passwd_1[0] = '\0';
            new_passwd_1[1] = '\0';
            new_passwd_1[2] = '\0';
            new_passwd_1[3] = '\0';
            new_passwd_2[0] = '\0';
            new_passwd_2[1] = '\0';
            new_passwd_2[2] = '\0';
            new_passwd_2[3] = '\0';
            return_time = 5;
        }
    }
    __delay_ms(81);
}

