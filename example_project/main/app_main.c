// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include "esp_system.h"

void app_main(void)
{
    printf("================== ESPRESSIF ===================\n");
    printf("    ESP_IDF VERSION: %s\n",esp_get_idf_version());
    printf("    JOYLINK COMMIT: %s\n",ESP_JOYLINK_COMMIT_ID);
    printf("    Compile time: %s %s\n",__DATE__,__TIME__);
    printf("================================================\n");
}
