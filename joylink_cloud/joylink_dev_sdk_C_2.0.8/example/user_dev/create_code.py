#!/usr/bin/python
# -*- coding: UTF-8 -*-
# 文件名：joylink_sdk.py

import shutil
import json

snapshot_json = "snapshot.json"

#json index
user_dev = "user_dev"
stream_id = "stream_id"
value_type = "value_type"
value_init = "value_init"

#c mode model file
joylink_model_h_txt = "joylink_model_extern.h"
joylink_model_c_txt = "joylink_model_extern.c"
joylink_model_json_c_txt = "joylink_model_extern_json.c"

#target c mode file
joylink_extern_h = "./../joylink_extern.h"
joylink_extern_c = "./../joylink_extern.c"
joylink_extern_json_c = "./../joylink_extern_json.c"

#user struct data typedef
user_define_place = "#ifdef __cplusplus\n}\n#endif\n"

user_dev_status_def =  "typedef struct _user_dev_status_t {\n"
user_dev_status_index = ""
user_dev_status_finish = "} user_dev_status_t;\n"

#user struct data init
user_data_place = "/*E_JLDEV_TYPE_GW*/\n#ifdef _SAVE_FILE_\n"

user_dev_status_init = "user_dev_status_t user_dev = \n{\n"
user_dev_status_data = ""
user_dev_status_end = "};\n"

#user json data parse
user_dev_parse_ctrl_place = "			char *dout = cJSON_Print(pSub);"

#user data package into json
user_dev_package_info_place = "	out=cJSON_Print(root);"

fun_get_snap_shot_retcode = "joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)"
fun_get_snap_shot = "joylink_dev_get_snap_shot(char *out_snap, int32_t out_max)"
fun_get_snap_shot_json = "joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid)"

fun_ctrl_lan_json = "joylink_dev_lan_json_ctrl(const char *json_cmd)"
fun_ctrl_script = "joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)"


"""
---------------------------------------------------------------
"""

shutil.copy(joylink_model_h_txt, joylink_extern_h)
shutil.copy(joylink_model_c_txt, joylink_extern_c)
shutil.copy(joylink_model_json_c_txt, joylink_extern_json_c)

file = open(snapshot_json, 'r')
json_data = file.read()
file.close()

text = json.loads(json_data)

user_data_index = ""
for index in text[user_dev]:
    string = index[stream_id];
    user_data_index += "#define USER_DATA_" + string.upper() + "   \"" + index[stream_id] + "\"\n";
print user_data_index

file = open(joylink_extern_h,'r')
content = file.read()
post = content.find(user_define_place)
if post != -1:
    content = content[:post] + user_data_index + "\n" + content[post:]
    file = open(joylink_extern_h,'w')
    file.write(content)
else:
    print "Can't find:", user_define_place;
file.close()

user_dev_status_index = ""
for index in text[user_dev]:
    user_dev_status_index += "    "
    if (index[value_type] == "string"):
        user_dev_status_index += "char"
    else:
        user_dev_status_index += index[value_type]
    user_dev_status_index += " "
    user_dev_status_index += index[stream_id]
    if (index[value_type] == "string"):
        user_dev_status_index += "[64]"
    user_dev_status_index += ";\n"
user_struct = user_dev_status_def + user_dev_status_index + user_dev_status_finish
print user_struct

file = open(joylink_extern_h,'r')
content = file.read()
post = content.find(user_define_place)
if post != -1:
    content = content[:post] + user_struct + "\n" + content[post:]
    file = open(joylink_extern_h,'w')
    file.write(content)
else:
    print "Can't find:", user_define_place;
file.close()

user_dev_status_data = ""
for index in text[user_dev]:
    user_dev_status_data += "    ."
    user_dev_status_data += index[stream_id]
    
    if (index[value_type] == "string"):
        user_dev_status_data += " = \""
        user_dev_status_data += index[value_init]
        user_dev_status_data += "\""
    else:
        user_dev_status_data += " = "
        user_dev_status_data += index[value_init]
    user_dev_status_data += ",\n"
user_data = user_dev_status_init + user_dev_status_data[0:len(user_dev_status_data)-2] + "\n" + user_dev_status_end
print user_data

file = open(joylink_extern_c,'r')
content = file.read()
post = content.find(user_data_place)
if post != -1:
    content = content[:post] + user_data + "\n" + content[post:]
    file = open(joylink_extern_c,'w')
    file.write(content)
else:
    print "Can't find:", user_data_place;
file.close()

user_dev_parse_ctrl_data = ""
for index in text[user_dev]:
    user_dev_parse_ctrl_data += "			if(!strcmp("
    user_dev_parse_ctrl_data += "USER_DATA_" + index[stream_id].upper() + ", pSId->valuestring)){\n"
    if (index[value_type] == "string"):
        user_dev_parse_ctrl_data += "				memset(userDev->"
        user_dev_parse_ctrl_data += index[stream_id] + ", 0, sizeof(userDev->" + index[stream_id] + "));\n"
        user_dev_parse_ctrl_data += "				strcpy(userDev->"
        user_dev_parse_ctrl_data += index[stream_id] + ", pV->valuestring);\n"
        user_dev_parse_ctrl_data += "			}\n"
    else:
        user_dev_parse_ctrl_data += "				memset(tmp_str, 0, sizeof(tmp_str));\n"
        user_dev_parse_ctrl_data += "				strcpy(tmp_str, pV->valuestring);\n"
        user_dev_parse_ctrl_data += "				userDev->"
        user_dev_parse_ctrl_data += index[stream_id]
        if(index[value_type] == "float"):
             user_dev_parse_ctrl_data += " = atof(tmp_str);\n"
        else:
            user_dev_parse_ctrl_data += " = atoi(tmp_str);\n"
        user_dev_parse_ctrl_data += "			}\n"
print user_dev_parse_ctrl_data

file = open(joylink_extern_json_c,'r')
content = file.read()
post = content.find(user_dev_parse_ctrl_place)
if post != -1:
    content = content[:post] + user_dev_parse_ctrl_data + "\n" + content[post:]
    file = open(joylink_extern_json_c,'w')
    file.write(content)
else:
    print "Can't find:", user_dev_parse_ctrl_place;
file.close()

"""
------------------------------------------------------------------------------
"""
user_dev_package_info_data = "	char i2str[32];\n"
for index in text[user_dev]:
    user_dev_package_info_data += "	cJSON *data_" + index[stream_id] + " = cJSON_CreateObject();\n"
    user_dev_package_info_data += "	cJSON_AddItemToArray(arrary, data_" + index[stream_id] + ");\n"
    user_dev_package_info_data += "	cJSON_AddStringToObject(data_" + index[stream_id] + ", \"stream_id\", \""
    user_dev_package_info_data += index[stream_id] + "\");\n"
    if (index[value_type] == "string"):
        user_dev_package_info_data += "	cJSON_AddStringToObject(data_" + index[stream_id] + ", \"current_value\", userDev->"
        user_dev_package_info_data += index[stream_id] + ");\n\n"
    else:
        user_dev_package_info_data += "	memset(i2str, 0, sizeof(i2str));\n"
        if(index[value_type] == "float"):
            user_dev_package_info_data += "	sprintf(i2str, \"%f\", userDev->"
        else:
            user_dev_package_info_data += "	sprintf(i2str, \"%d\", userDev->"
        user_dev_package_info_data += index[stream_id] + ");\n"
        user_dev_package_info_data += "	cJSON_AddStringToObject(data_" + index[stream_id] + ", \"current_value\", i2str);\n\n"
print user_dev_package_info_data

file = open(joylink_extern_json_c,'r')
content = file.read()
post = content.find(user_dev_package_info_place)
if post != -1:
    content = content[:post] + user_dev_package_info_data + "\n" + content[post:]
    file = open(joylink_extern_json_c,'w')
    file.write(content)
else:
    print "Can't find:", user_dev_package_info_place;
file.close()

print "end\n"

