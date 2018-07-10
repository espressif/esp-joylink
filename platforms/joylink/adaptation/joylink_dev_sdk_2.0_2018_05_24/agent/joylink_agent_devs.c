/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 *
 * @author: 
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joylink_agent.h"
#include "joylink_agent_devs.h"
#include "joylink_log.h"
#include "joylink.h"

AgentDev_t _g_adevs[AGENT_DEV_MAX];

pthread_mutex_t    devs_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef _SAVE_FILE_
    char  *devs_info_file = "devs_info.txt";
#endif

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_devs_load(void)
{
    /**TO DO
     *MUST load AgentDevs from flash when application run.
     */

#ifdef _SAVE_FILE_
    FILE *infile;
    infile = fopen(devs_info_file, "wr+");
    int i;
    for(i = 0; i< AGENT_DEV_MAX; i++){
        fread(&_g_adevs[i], sizeof(AgentDev_t), 1, infile);
        _g_adevs[i].fd = 0;
    }
    fclose(infile);
#endif

    return E_RET_OK;
}

E_JLRetCode_t
joylink_agent_devs_save_file(void)
{
    /**TO DO
     *MUST load AgentDevs from flash when application run.
     */

#ifdef _SAVE_FILE_
    log_info("sava to files");
    FILE *outfile;
    outfile = fopen(devs_info_file, "wb" );
    fwrite(_g_adevs, sizeof(_g_adevs), 1, outfile);
    fclose(outfile);
#endif

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: feedid
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_del(char *feedid)
{
    /**TO DO
     *MUST sync to flash
     */

    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(!strcmp(_g_adevs[i].feedid, feedid)){
            memset(&_g_adevs[i], 0, sizeof(AgentDev_t));
            /**
             * !!!!!!!!!!! sync to flash !!!!!!!!!!!!
             * */
            joylink_agent_devs_save_file();
            return E_RET_OK;
        }
    }
    log_error("agent devs no find:%s", feedid);
    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: pad
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_upd(char *feedid, AgentDev_t *pad)
{
    /**TO DO
     *MUST sync to flash
     */
    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(!strcmp(_g_adevs[i].feedid, feedid)){
            memcpy(&_g_adevs[i], pad, sizeof(AgentDev_t));
            /**
             * !!!!!!!!!!! sync to flash !!!!!!!!!!!!
             * */
            return E_RET_OK;
        }
    }

    log_error("agent devs no find:%s", pad->feedid);
    joylink_agent_devs_save_file();
    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: pad
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_upd_to_flash(char *feedid)
{
    /**TO DO
     *MUST sync to flash
     */
    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(!strcmp(_g_adevs[i].feedid, feedid)){
            /**
             * !!!!!!!!!!! sync to flash !!!!!!!!!!!!
             * */
            return E_RET_OK;
        }
    }

    log_error("agent devs no find:%s", feedid);
    joylink_agent_devs_save_file();
    return E_RET_ERROR;
}
/**
 * brief: 
 *
 * @Param: feedid
 * @Param: o_pad
 *
 * @Returns: 
 */
AgentDev_t *
joylink_agent_dev_get(char *feedid)
{
    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(!strcmp(_g_adevs[i].feedid, feedid)){
            return &_g_adevs[i];
        }
    }
    log_error("agent devs no find:%s", feedid);
    return NULL;
}

/**
 * brief: 
 *
 * @Param: pad
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_add(char feedid[JL_MAX_FEEDID_LEN], char ackey[JL_MAX_ACKEY_LEN])
{
    /**TO DO
     *MUST sync to flash
     */
    int32_t i;
    AgentDev_t *p = NULL; 
    p = joylink_agent_dev_get(feedid);
    if(NULL != p){
        memcpy(p->accesskey, ackey, JL_MAX_ACKEY_LEN);
        return E_RET_OK;
    }
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(!strcmp(_g_adevs[i].feedid, "")){
            memcpy(&_g_adevs[i].feedid, feedid, JL_MAX_FEEDID_LEN);
            memcpy(&_g_adevs[i].accesskey, ackey, JL_MAX_ACKEY_LEN);
            /**
             * !!!!!!!!!!! sync to flash !!!!!!!!!!!!
             * */
            joylink_agent_devs_save_file();
            return E_RET_OK;
        }
    }
    log_error("agent devs no space");

    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: feedid[JL_MAX_FEEDID_LEN]
 * @Param: ackey[JL_MAX_ACKEY_LEN]
 * @Param: sessionkey[JL_MAX_ACKEY_LEN]
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_get_keys(char feedid[JL_MAX_FEEDID_LEN], 
                      char ackey[JL_MAX_ACKEY_LEN],
                      char sessionkey[JL_MAX_SESSION_KEY_LEN])
{
    AgentDev_t * ad = joylink_agent_dev_get(feedid);
    if(NULL != ad){
        memcpy(ackey, ad->accesskey, JL_MAX_ACKEY_LEN);
        memcpy(sessionkey, ad->sessionkey, JL_MAX_SESSION_KEY_LEN);
        return E_RET_OK;
    }
    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: fds[AGENT_DEV_MAX]
 *
 * @Returns: 
 */
int
joylink_agent_devs_get_fds(int fds[AGENT_DEV_MAX])
{
    int32_t i;
    int32_t j = 0;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        /**
         * FIXME fd > 0 must adapter to SOC
         * */
        if(_g_adevs[i].fd > 0){
            fds[j++] = _g_adevs[i].fd;
        }
    }
    return j;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: feedid[JL_MAX_FEEDID_LEN]
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_get_feedid_by_fd(int fd, char feedid[JL_MAX_FEEDID_LEN])
{
    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(_g_adevs[i].fd == fd){
            memcpy(feedid,  _g_adevs[i].feedid, JL_MAX_FEEDID_LEN);
            return E_RET_OK;
        }
    }

    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
AgentDev_t *
joylink_agent_get_dev_by_fd(int fd)
{
    int32_t i;
    for(i = 0; i < AGENT_DEV_MAX; i++){
        if(_g_adevs[i].fd == fd){
            return &_g_adevs[i];
        }
    }

    return NULL;
}

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_clear_fd_by_fd(int fd)
{
    AgentDev_t * p = joylink_agent_get_dev_by_fd(fd);
    if(NULL != p){
        p->fd = 0;  
        return E_RET_OK;
    }

    return E_RET_ERROR;
}
