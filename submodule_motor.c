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

// Receive messages M:1 to M:4
int receive_motor_number(void)
{
    static int state = 0;
    static int saved_number = 0;

    while (uart_has_data(UART0))
    {
        char c = (char)uart_recv(UART0);

        if (state == 0)
        {
            if (c == 'M')
                state = 1;
        }
        else if (state == 1)
        {
            state = (c == ':') ? 2 : 0;
        }
        else if (state == 2)
        {
            if (c >= '1' && c <= '4')
            {
                saved_number = c - '0';
                state = 3;
            }
            else
            {
                state = 0;
            }
        }
        else if (state == 3)
        {
            state = 0;

            if (c == '\n')
                return saved_number;
        }
    }

    return 0;
}

void update_screen(display_t *display,
                   FontxFile fx16G[],
                   int number,
                   int from_uart)
{
    char text[16];

    displayFillScreen(display, RGB_BLACK);

    displayDrawString(display, fx16G, 10, 30,
                      (uint8_t *)"MOTOR MODULE",
                      RGB_PURPLE);

    snprintf(text, sizeof(text), "%d", number);

    displayDrawString(display, fx16G, 100, 100,
                      (uint8_t *)text,
                      RGB_PURPLE);

    if (from_uart)
    {
        displayDrawString(display, fx16G, 10, 150,
                          (uint8_t *)"RECEIVED FROM UART",
                          RGB_WHITE);
    }
    else
    {
        displayDrawString(display, fx16G, 10, 150,
                          (uint8_t *)"LOCAL BUTTON",
                          RGB_WHITE);
    }
}

int main(void)
{
    display_t display;
    FontxFile fx16G[2];

    int previous = 0;

    pynq_init();
    buttons_init();

    // UART0 receiver
    switchbox_set_pin(IO_AR0, SWB_UART0_RX);

    uart_init(UART0);
    uart_reset_fifos(UART0);

    display_init(&display);
    InitFontx(fx16G, "../../fonts/ILGH16XB.FNT", "");
    displaySetFontDirection(&display, TEXT_DIRECTION0);
    displayFillScreen(&display, RGB_BLACK);

    displayDrawString(&display, fx16G, 10, 30,
                      (uint8_t *)"MOTOR MODULE",
                      RGB_PURPLE);

    displayDrawString(&display, fx16G, 10, 70,
                      (uint8_t *)"WAITING...",
                      RGB_WHITE);

    while (1)
    {
        // Check UART communication
        int number = receive_motor_number();

        if (number > 0)
        {
            printf("Motor received: %d\n", number);

            update_screen(&display, fx16G,
                          number, 1);
        }

        // Check local buttons
        int current = get_pressed_button();

        if (current != 0 && current != previous)
        {
            printf("Motor button: %d\n", current);

            update_screen(&display, fx16G,
                          current, 0);
        }

        previous = current;

        sleep_msec(10);
    }

    uart_destroy(UART0);
    buttons_destroy();
    display_destroy(&display);
    pynq_destroy();

    return 0;
}
