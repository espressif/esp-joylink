#include "joylink_light.h"
#include "joylink_led_rgb.h"

void light_init(void)
{
    joylink_light_init();
}

static void joylink_AmbientLight(void)
{
    joylink_light_set_hue(300);
}

void joylink_light_Control(user_dev_status_t user_dev)
{
    joylink_light_set_brightness(78);
    joylink_light_set_saturation(100);
    if (user_dev.Power == 1) {
        joylink_light_set_on(true);
        if (user_dev.AmbientLight == 1) {
            joylink_AmbientLight();
        } else if (user_dev.AmbientLight == 0) {
            joylink_light_set_saturation(0);
        }
    } else if (user_dev.Power == 0) {
        joylink_light_set_on(false);
        joylink_light_set_brightness(0);
    }
}
