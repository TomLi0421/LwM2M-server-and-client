/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Achim Kraus, Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Ville Skyttä - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    Tuve Nordius, Husqvarna Group - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PRV_TLV_BUFFER_SIZE 64

// Resource IDs
#define GTIN_MODEL_NUMBER 1
#define MANUFACTURER_IDENTIFIER 2
#define USER_GIVEN_NAME 3
#define ASSET_IDENTIFIER 4
#define INSTALLATION_DATE 5
#define SOFTWARE_UPDATE 6
#define MAINTENANCE 7
#define CONFIGURATION_RESET 8
#define DEVICE_OPERATING_HOURS 9
#define ADDITIONAL_FIRMWARE_INFORMATION 10

#define PRV_GTINMODELNUMBER "123"
#define PRV_MANFACTURER_IDENTIFIER "abc"
#define PRV_USER_GIVEN_NAME "ABC"
#define PRV_ASSET_IDENTIFIER "ABC"
// #define PRV_INSTALLATION_DATE
#define PRV_SOFTWARE_UPDATE false
#define PRV_MAINTENANCE false
// #define PRV_CONFIGURATION_RESET
#define PRV_DEVICE_OPERATING_HOURS 1
#define PRV_ADDITION_FIRMWARE_INFORMATION "123"

/*
 * Multiple instance objects can use userdata to store data that will be shared between the different instances.
 * The lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_ {
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ *next; // matches lwm2m_list_t::next
    uint16_t shortID;            // matches lwm2m_list_t::id

    char *GTINModelNumber;
    char *manufacturerIdentifier;
    char *userGivenName;
    char *assetIdentifier;
    int64_t installationDate;
    bool softwareUpdate;
    bool maintenance;
    // configurationReset;
    int deviceOperatingHours;
    char *additionalFirmwareInformation;
} prv_instance_t;

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP);
static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                          lwm2m_object_t *objectP);

static uint8_t prv_set_value(lwm2m_data_t *dataP, prv_instance_t *targetP) {
    switch (dataP->id) {
    case GTIN_MODEL_NUMBER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_GTINMODELNUMBER, dataP);
        return COAP_205_CONTENT;
    case MANUFACTURER_IDENTIFIER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_MANFACTURER_IDENTIFIER, dataP);
        return COAP_205_CONTENT;
    case USER_GIVEN_NAME:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_USER_GIVEN_NAME, dataP);
        return COAP_205_CONTENT;
    case ASSET_IDENTIFIER:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_ASSET_IDENTIFIER, dataP);
        return COAP_205_CONTENT;
    case INSTALLATION_DATE:
        return COAP_404_NOT_FOUND;
    case SOFTWARE_UPDATE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_bool(PRV_SOFTWARE_UPDATE, dataP);
        return COAP_205_CONTENT;
    case MAINTENANCE:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_bool(targetP->maintenance, dataP);
        return COAP_205_CONTENT;
    case DEVICE_OPERATING_HOURS:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_int(PRV_DEVICE_OPERATING_HOURS, dataP);
        return COAP_205_CONTENT;
    case ADDITIONAL_FIRMWARE_INFORMATION:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
            return COAP_404_NOT_FOUND;
        lwm2m_data_encode_string(PRV_ADDITION_FIRMWARE_INFORMATION, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}
static uint8_t prv_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                        lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {
            GTIN_MODEL_NUMBER,
            MANUFACTURER_IDENTIFIER,
            USER_GIVEN_NAME,
            ASSET_IDENTIFIER,
            INSTALLATION_DATE,
            SOFTWARE_UPDATE,
            MAINTENANCE,
            CONFIGURATION_RESET,
            DEVICE_OPERATING_HOURS,
            ADDITIONAL_FIRMWARE_INFORMATION,
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do {
        result = prv_set_value((*dataArrayP) + i, (prv_instance_t *)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

/*
static uint8_t prv_discover(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                            lwm2m_object_t *objectP) {
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    result = COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {
            GTIN_MODEL_NUMBER,
            MANUFACTURER_IDENTIFIER,
            USER_GIVEN_NAME,
            ASSET_IDENTIFIER,
            INSTALLATION_DATE,
            SOFTWARE_UPDATE,
            MAINTENANCE,
            CONFIGURATION_RESET,
            DEVICE_OPERATING_HOURS,
            ADDITIONAL_FIRMWARE_INFORMATION,
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++) {
            (*dataArrayP)[i].id = resList[i];
        }
    } else {
        for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++) {
            switch ((*dataArrayP)[i].id) {
            case GTIN_MODEL_NUMBER:
            case MANUFACTURER_IDENTIFIER:
            case USER_GIVEN_NAME:
            case ASSET_IDENTIFIER:
            case INSTALLATION_DATE:
            case SOFTWARE_UPDATE:
            case MAINTENANCE:
            case CONFIGURATION_RESET:
            case DEVICE_OPERATING_HOURS:
            case ADDITIONAL_FIRMWARE_INFORMATION:
                break;
            default:
                result = COAP_404_NOT_FOUND;
            }
        }
    }

    return result;
}
*/

static uint8_t prv_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                         lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    prv_instance_t *targetP;
    int i;
    uint8_t result;
    char *tmp;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) {
        return COAP_404_NOT_FOUND;
    }

    if (writeType == LWM2M_WRITE_REPLACE_INSTANCE) {
        result = prv_delete(contextP, instanceId, objectP);
        if (result == COAP_202_DELETED) {
            result = prv_create(contextP, instanceId, numData, dataArray, objectP);
            if (result == COAP_201_CREATED) {
                result = COAP_204_CHANGED;
            }
        }
        return result;
    }

    i = 0;
    do {
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id) {
        case USER_GIVEN_NAME:
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                tmp = targetP->userGivenName;
                targetP->userGivenName = lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
                strncpy(targetP->userGivenName, (char *)dataArray[i].value.asBuffer.buffer,
                        dataArray[i].value.asBuffer.length);
                lwm2m_free(tmp);
                result = COAP_204_CHANGED;
                break;
            } else {
                return COAP_400_BAD_REQUEST;
            }
        case ASSET_IDENTIFIER:
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                tmp = targetP->assetIdentifier;
                targetP->assetIdentifier = lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
                strncpy(targetP->assetIdentifier, (char *)dataArray[i].value.asBuffer.buffer,
                        dataArray[i].value.asBuffer.length);
                lwm2m_free(tmp);
                result = COAP_204_CHANGED;
                break;
            } else {
                return COAP_400_BAD_REQUEST;
            }
            /*
            case INSTALLATION_DATE:
                if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                    tmp = targetP->assetIdentifier;
                    targetP->assetIdentifier = lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
                    strncpy(targetP->userGivenName, (char *)dataArray[i].value.asBuffer.buffer,
                            dataArray[i].value.asBuffer.length);
                    lwm2m_free(tmp);
                    result = COAP_204_CHANGED;

                    break;
                } else {
                    return COAP_400_BAD_REQUEST;
                }
            */
        case MAINTENANCE: {
            bool value;

            if (false == lwm2m_data_decode_bool(dataArray + i, &value)) {
                targetP->maintenance = value;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
        }
        default:
            return COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == COAP_204_CHANGED);
    return COAP_204_CHANGED;
}

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP) {
    prv_instance_t *targetP;

    objectP->instanceList = lwm2m_list_remove(objectP->instanceList, id, (lwm2m_list_t **)&targetP);
    if (NULL == targetP)
        return COAP_404_NOT_FOUND;

    lwm2m_free(targetP);

    return COAP_202_DELETED;
}

static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                          lwm2m_object_t *objectP) {
    prv_instance_t *targetP;
    uint8_t result;

    targetP = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
    if (NULL == targetP)
        return COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    objectP->instanceList = LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(contextP, instanceId, numData, dataArray, objectP, LWM2M_WRITE_REPLACE_RESOURCES);

    if (result != COAP_204_CHANGED) {
        prv_delete(contextP, instanceId, objectP);
    } else {
        result = COAP_201_CREATED;
    }

    return result;
}

/*
static uint8_t prv_exec(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId, uint8_t *buffer,
                        int length, lwm2m_object_t *objectP) {

    if (NULL == lwm2m_list_find(objectP->instanceList, instanceId))
        return COAP_404_NOT_FOUND;

    switch (resourceId) {
    case 1:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        fprintf(stdout,
                "\r\n-----------------\r\n"
                "Execute on %hu/%d/%d\r\n"
                " Parameter (%d bytes):\r\n",
                objectP->objID, instanceId, resourceId, length);
        prv_output_buffer((uint8_t *)buffer, length);
        fprintf(stdout, "-----------------\r\n\r\n");
        return COAP_204_CHANGED;
    case 3:
        return COAP_405_METHOD_NOT_ALLOWED;
    case 5:
        return COAP_405_METHOD_NOT_ALLOWED;
    default:
        return COAP_404_NOT_FOUND;
    }
}
*/

void display_extension_object(lwm2m_object_t *object) {
    /*
    fprintf(stdout, "  /%u: Test object, instances:\r\n", object->objID);
    prv_instance_t *instance = (prv_instance_t *)object->instanceList;
    while (instance != NULL) {
        fprintf(stdout, "    /%u/%u: shortId: %u, test: %u\r\n", object->objID);
        // instance->shortID, instance->shortID, instance->test
        instance = (prv_instance_t *)instance->next;
    }
    */
}

lwm2m_object_t *get_extension_object(void) {
    lwm2m_object_t *extensionObj;

    extensionObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != extensionObj) {
        prv_instance_t *targetP;
        memset(extensionObj, 0, sizeof(lwm2m_object_t));

        extensionObj->objID = EXTENSION_OBJECT_ID;
        targetP = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
        if (NULL == targetP)
            return NULL;
        memset(targetP, 0, sizeof(prv_instance_t));
        targetP->shortID = 0;
        extensionObj->instanceList = LWM2M_LIST_ADD(extensionObj->instanceList, targetP);
        extensionObj->readFunc = prv_read;
        // extensionObj->discoverFunc = prv_discover;
        extensionObj->writeFunc = prv_write;
        // extensionObj->executeFunc = prv_exec;
        extensionObj->createFunc = prv_create;
        extensionObj->deleteFunc = prv_delete;
        extensionObj->userData = lwm2m_malloc(sizeof(prv_instance_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != extensionObj->userData) {
            ((prv_instance_t *)extensionObj->userData)->maintenance = PRV_MAINTENANCE;
            // strcpy(((device_data_t *)deviceObj->userData)->time_offset, "+01:00");
        } else {
            lwm2m_free(extensionObj->instanceList);
            lwm2m_free(extensionObj);
            extensionObj = NULL;
        }
    }
    return extensionObj;
}

void free_extension_object(lwm2m_object_t *object) {
    while (object->instanceList != NULL) {
        prv_instance_t *extensionInstance = (prv_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        lwm2m_free(extensionInstance);
    }
}