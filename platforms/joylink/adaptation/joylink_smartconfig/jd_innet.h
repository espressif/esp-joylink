#ifndef _JD_INNET_H_
#define _JD_INNET_H_

bool jd_innet_start();
void jd_innet_stop(void);
bool jd_innet_set_aes_key(const char *SecretKey);
const char *jd_innet_get_aes_key(void);

#endif
