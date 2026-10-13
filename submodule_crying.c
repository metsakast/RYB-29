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

int main(void)
{
    display_t display;
    FontxFile fx16G[2];

    int previous = 0;
    char message[16];
    char number_text[4];

    pynq_init();
    buttons_init();

    // UART0 transmitter on AR1
    switchbox_set_pin(IO_AR1, SWB_UART0_TX);
    uart_init(UART0);
    uart_reset_fifos(UART0);

    // LCD initialization
    display_init(&display);
    InitFontx(fx16G, "../../fonts/ILGH16XB.FNT", "");
    displaySetFontDirection(&display, TEXT_DIRECTION0);
    displayFillScreen(&display, RGB_BLACK);

    displayDrawString(&display, fx16G, 10, 30,
                      (uint8_t *)"CRYING", RGB_YELLOW);

    displayDrawString(&display, fx16G, 10, 70,
                      (uint8_t *)"PRESS BUTTON 1-4",
                      RGB_WHITE);

    while (1)
    {
        int current = get_pressed_button();

        if (current != 0 && current != previous)
        {
            snprintf(message, sizeof(message),
                     "C:%d\n", current);

            // Send number to Decision
            for (int i = 0; message[i] != '\0'; i++)
                uart_send(UART0, (uint8_t)message[i]);

            printf("Crying sent: %s", message);

            snprintf(number_text, sizeof(number_text),
                     "%d", current);

            displayFillScreen(&display, RGB_BLACK);

            displayDrawString(&display, fx16G, 10, 30,
                              (uint8_t *)"CRYING",
                              RGB_YELLOW);

            displayDrawString(&display, fx16G, 100, 100,
                              (uint8_t *)number_text,
                              RGB_YELLOW);

            displayDrawString(&display, fx16G, 10, 150,
                              (uint8_t *)"SENT TO DECISION",
                              RGB_WHITE);
        }

        previous = current;
        sleep_msec(20);
    }

    uart_destroy(UART0);
    buttons_destroy();
    display_destroy(&display);
    pynq_destroy();
    return 0;
}
