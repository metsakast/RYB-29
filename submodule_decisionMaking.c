#include <stdio.h>
#include <stdint.h>
#include <libpynq.h>

int get_pressed_button(void)
{
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if (get_button_state(i) == BUTTON_PUSHED)
            return i + 1;
    }
    return 0;
}

// Send a numbered message to Motor
void send_motor(int number)
{
    char message[16];

    snprintf(message, sizeof(message),
             "M:%d\n", number);

    for (int i = 0; message[i] != '\0'; i++)
        uart_send(UART0, (uint8_t)message[i]);

    printf("Motor sent: %s", message);
}

// Receive a message such as H:3 or C:2
// Returns 0 when no complete message is ready.
int receive_number(int uart, char expected)
{
    static int state[2] = {0, 0};
    static int saved_number[2] = {0, 0};

    while (uart_has_data(uart))
    {
        char c = (char)uart_recv(uart);

        if (state[uart] == 0)
        {
            if (c == expected)
                state[uart] = 1;
        }
        else if (state[uart] == 1)
        {
            state[uart] = (c == ':') ? 2 : 0;
        }
        else if (state[uart] == 2)
        {
            if (c >= '1' && c <= '4')
            {
                saved_number[uart] = c - '0';
                state[uart] = 3;
            }
            else
            {
                state[uart] = 0;
            }
        }
        else if (state[uart] == 3)
        {
            state[uart] = 0;

            if (c == '\n')
                return saved_number[uart];
        }
    }

    return 0;
}

void update_screen(display_t *display,
                   FontxFile fx16G[],
                   int heartbeat,
                   int crying,
                   int motor)
{
    char text[32];

    displayFillScreen(display, RGB_BLACK);

    displayDrawString(display, fx16G, 10, 30,
                      (uint8_t *)"DECISION",
                      RGB_BLUE);

    snprintf(text, sizeof(text),
             "HEARTBEAT: %d", heartbeat);

    displayDrawString(display, fx16G, 10, 70,
                      (uint8_t *)text,
                      RGB_RED);

    snprintf(text, sizeof(text),
             "CRYING: %d", crying);

    displayDrawString(display, fx16G, 10, 110,
                      (uint8_t *)text,
                      RGB_YELLOW);

    snprintf(text, sizeof(text),
             "MOTOR: %d", motor);

    displayDrawString(display, fx16G, 10, 150,
                      (uint8_t *)text,
                      RGB_BLUE);
}

int main(void)
{
    display_t display;
    FontxFile fx16G[2];

    int heartbeat = 0;
    int crying = 0;
    int motor = 0;
    int previous = 0;

    pynq_init();
    buttons_init();

    // Heartbeat RX + Motor TX
    switchbox_set_pin(IO_AR0, SWB_UART0_RX);
    switchbox_set_pin(IO_AR1, SWB_UART0_TX);

    // Crying RX
    switchbox_set_pin(IO_AR2, SWB_UART1_RX);

    uart_init(UART0);
    uart_init(UART1);

    uart_reset_fifos(UART0);
    uart_reset_fifos(UART1);

    display_init(&display);
    InitFontx(fx16G, "../../fonts/ILGH16XB.FNT", "");
    displaySetFontDirection(&display, TEXT_DIRECTION0);

    update_screen(&display, fx16G, 0, 0, 0);

    while (1)
    {
        int updated = 0;

        // Receive Heartbeat
        int h = receive_number(UART0, 'H');

        if (h > 0)
        {
            heartbeat = h;
            motor = h;

            printf("Heartbeat received: %d\n", h);

            send_motor(h);
            updated = 1;
        }

        // Receive Crying
        int c = receive_number(UART1, 'C');

        if (c > 0)
        {
            crying = c;
            motor = c;

            printf("Crying received: %d\n", c);

            send_motor(c);
            updated = 1;
        }

        // Decision's own buttons
        int current = get_pressed_button();

        if (current != 0 && current != previous)
        {
            motor = current;

            send_motor(current);
            updated = 1;
        }

        previous = current;

        if (updated)
        {
            update_screen(&display, fx16G,
                          heartbeat, crying, motor);
        }

        sleep_msec(10);
    }

    uart_destroy(UART0);
    uart_destroy(UART1);
    buttons_destroy();
    display_destroy(&display);
    pynq_destroy();

    return 0;
}
