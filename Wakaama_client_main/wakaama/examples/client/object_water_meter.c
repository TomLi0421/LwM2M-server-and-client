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

#define PRV_TLV_BUFFER_SIZE 64

// Resource IDs
#define CUMULATED_WATER_VOLUME 1
#define CUMULATED_WATER_METER_VALUE_RESET 2
#define TYPE_OF_METER 3
#define CUMULATED_PULSE_VALUE 4
#define CUMULATED_PULSE_VALUE_RESET 5
#define PULSE_RATIO 6
#define MINIMUM_FLOW_RATE 7
#define MAXIMUM_FLOW_RATE 8
#define LEAK_SUSPECTED 9
#define LEAK_DETECTED 10
#define BACK_FLOW_DETECT 11
#define BLOCKED_METER 12
#define FRAUD_DETECTED 13

#define PRV_CUMULATED_WATER_VOLUME 10.5
// #define PRV_CUMULATED_WATER_METER_VALUE_RESET
#define PRV_TYPE_OF_METER "ABC"
#define PRV_CUMULATED_PULSE_VALUE 15
// #define PRV_CUMULATED_PULSE_VALUE_RESET
#define PRV_PULSE_RATIO 10
#define PRV_MINIMUM_FLOW_RATE 11.5
#define PRV_MAXIMUM_FLOW_RATE 12.5
#define PRV_LEAK_SUSPECTED false
#define PRV_LEAK_DETECTED false
#define PRV_BACK_FLOW_DETECT false
#define PRV_BLOCKED_METER false
#define PRV_FRAUD_DETECTED false

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

    float cumulatedWaterVolume;
    // cumulatedWaterMeterValueReset
    char *typeOfMeter;
    int cumulatedPluseValue;
    // cumulatedPulseValueReset
    int pulseRatio;
    float minimumFlowRate;
    float maximumFlowRate;
    bool leakSuspected;
    bool leakDetected;
    bool backFlowDetected;
    bool blockedMeter;
    bool fraudDetected;
} prv_instance_t;

static uint8_t prv_delete(lwm2m_context_t *contextP, uint16_t id, lwm2m_object_t *objectP);
static uint8_t prv_create(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                          lwm2m_object_t *objectP);

// static uint8_t prv_set_value(lwm2m_data_t *dataP, prv_instance_t *targetP) {
//     switch (dataP->id) {
//     case CUMULATED_WATER_VOLUME:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_float(PRV_CUMULATED_WATER_VOLUME, dataP);
//         return COAP_205_CONTENT;
//     case CUMULATED_WATER_METER_VALUE_RESET:
//         return COAP_404_NOT_FOUND;
//     case TYPE_OF_METER:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         // lwm2m_data_encode_string(targetP->typeOfMeter, dataP);

//         if (targetP->typeOfMeter == NULL)
//             targetP->typeOfMeter = strdup(PRV_TYPE_OF_METER);
//         lwm2m_data_encode_string(targetP->typeOfMeter, dataP);
//         return COAP_205_CONTENT;
//     case CUMULATED_PULSE_VALUE:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_int(PRV_CUMULATED_PULSE_VALUE, dataP);
//         return COAP_205_CONTENT;
//     case CUMULATED_PULSE_VALUE_RESET:
//         return COAP_404_NOT_FOUND;
//     case PULSE_RATIO:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_int(targetP->pulseRatio, dataP);
//         return COAP_205_CONTENT;
//     case MINIMUM_FLOW_RATE:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_float(PRV_MINIMUM_FLOW_RATE, dataP);
//         return COAP_205_CONTENT;
//     case MAXIMUM_FLOW_RATE:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_float(PRV_MAXIMUM_FLOW_RATE, dataP);
//         return COAP_205_CONTENT;
//     case LEAK_SUSPECTED:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_bool(PRV_LEAK_SUSPECTED, dataP);
//         return COAP_205_CONTENT;
//     case LEAK_DETECTED:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_bool(PRV_LEAK_DETECTED, dataP);
//         return COAP_205_CONTENT;
//     case BACK_FLOW_DETECT:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_bool(PRV_BACK_FLOW_DETECT, dataP);
//         return COAP_205_CONTENT;
//     case BLOCKED_METER:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_bool(PRV_BLOCKED_METER, dataP);
//         return COAP_205_CONTENT;
//     case FRAUD_DETECTED:
//         if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
//             return COAP_404_NOT_FOUND;
//         lwm2m_data_encode_bool(PRV_FRAUD_DETECTED, dataP);
//         return COAP_205_CONTENT;
//     default:
//         return COAP_404_NOT_FOUND;
//     }
// }

static uint8_t prv_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t **dataArrayP,
                        lwm2m_object_t *objectP) {
    prv_instance_t *targetP;
    uint8_t result;
    int i;

    targetP = (prv_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);

    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {
            CUMULATED_WATER_VOLUME,
            // CUMULATED_WATER_METER_VALUE_RESET,
            TYPE_OF_METER,
            CUMULATED_PULSE_VALUE,
            // CUMULATED_PULSE_VALUE_RESET,
            PULSE_RATIO,
            MINIMUM_FLOW_RATE,
            MAXIMUM_FLOW_RATE,
            LEAK_SUSPECTED,
            LEAK_DETECTED,
            BACK_FLOW_DETECT,
            BLOCKED_METER,
            FRAUD_DETECTED,
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (int j = 0; j < nbRes; j++) {
            (*dataArrayP)[j].id = resList[j];
        }
    }

    i = 0;
    do {
        switch ((*dataArrayP)[i].id) {
        case CUMULATED_WATER_VOLUME:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_float(PRV_CUMULATED_WATER_VOLUME, *dataArrayP + i);
            return COAP_205_CONTENT;
        case CUMULATED_WATER_METER_VALUE_RESET:
            return COAP_404_NOT_FOUND;
        case TYPE_OF_METER:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            if (targetP->typeOfMeter == NULL)
                targetP->typeOfMeter = strdup(PRV_TYPE_OF_METER);
            lwm2m_data_encode_string(targetP->typeOfMeter, *dataArrayP + i);
            return COAP_205_CONTENT;
        case CUMULATED_PULSE_VALUE:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(PRV_CUMULATED_PULSE_VALUE, *dataArrayP + i);
            return COAP_205_CONTENT;
        case CUMULATED_PULSE_VALUE_RESET:
            return COAP_404_NOT_FOUND;
        case PULSE_RATIO:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(targetP->pulseRatio, *dataArrayP + i);
            return COAP_205_CONTENT;
        case MINIMUM_FLOW_RATE:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_float(PRV_MINIMUM_FLOW_RATE, *dataArrayP + i);
            return COAP_205_CONTENT;
        case MAXIMUM_FLOW_RATE:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_float(PRV_MAXIMUM_FLOW_RATE, *dataArrayP + i);
            return COAP_205_CONTENT;
        case LEAK_SUSPECTED:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_bool(PRV_LEAK_SUSPECTED, *dataArrayP + i);
            return COAP_205_CONTENT;
        case LEAK_DETECTED:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_bool(PRV_LEAK_DETECTED, *dataArrayP + i);
            return COAP_205_CONTENT;
        case BACK_FLOW_DETECT:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_bool(PRV_BACK_FLOW_DETECT, *dataArrayP + i);
            return COAP_205_CONTENT;
        case BLOCKED_METER:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_bool(PRV_BLOCKED_METER, *dataArrayP + i);
            return COAP_205_CONTENT;
        case FRAUD_DETECTED:
            if (dataArrayP[i]->type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_bool(PRV_FRAUD_DETECTED, *dataArrayP + i);
            return COAP_205_CONTENT;
        }
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return COAP_205_CONTENT;
}

/*
static uint8_t prv_discover(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP, lwm2m_data_t
**dataArrayP, lwm2m_object_t *objectP) { uint8_t result; int i;

    // this is a single instance object
    if (instanceId != 0) {
        return COAP_404_NOT_FOUND;
    }

    result = COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        uint16_t resList[] = {
            CUMULATED_WATER_VOLUME,
            // CUMULATED_WATER_METER_VALUE_RESET,
            TYPE_OF_METER,
            CUMULATED_PULSE_VALUE,
            // CUMULATED_PULSE_VALUE_RESET,
            PULSE_RATIO,
            MINIMUM_FLOW_RATE,
            MAXIMUM_FLOW_RATE,
            LEAK_SUSPECTED,
            LEAK_DETECTED,
            BACK_FLOW_DETECT,
            BLOCKED_METER,
            FRAUD_DETECTED,
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
            case CUMULATED_WATER_VOLUME:
            // case CUMULATED_WATER_METER_VALUE_RESET:
            case TYPE_OF_METER:
            case CUMULATED_PULSE_VALUE:
            // case CUMULATED_PULSE_VALUE_RESET:
            case PULSE_RATIO:
            case MINIMUM_FLOW_RATE:
            case MAXIMUM_FLOW_RATE:
            case LEAK_SUSPECTED:
            case LEAK_DETECTED:
            case BACK_FLOW_DETECT:
            case BLOCKED_METER:
            case FRAUD_DETECTED:
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
    uint8_t result;
    char *tmp;
    int i;

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
        /* No multiple instance resources */
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id) {
        case TYPE_OF_METER:
            if (dataArray[i].type == LWM2M_TYPE_STRING || dataArray[i].type == LWM2M_TYPE_OPAQUE) {
                tmp = targetP->typeOfMeter;
                targetP->typeOfMeter = lwm2m_malloc(dataArray[i].value.asBuffer.length + 1);
                strncpy(targetP->typeOfMeter, (char *)dataArray[i].value.asBuffer.buffer,
                        dataArray[i].value.asBuffer.length);
                lwm2m_free(tmp);
                result = COAP_204_CHANGED;
                break;
            } else {
                return COAP_400_BAD_REQUEST;
            }
        case PULSE_RATIO: {
            int value = 0;
            for (int j = 0; j < dataArray[i].value.asBuffer.length; j++) {
                value += (dataArray[i].value.asBuffer.buffer[j] << (8 * (dataArray[i].value.asBuffer.length - j - 1)));
            }
            targetP->pulseRatio = value;
            result = COAP_204_CHANGED;
            break;
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
}

void display_meter_object(lwm2m_object_t *object) {
    /*
    fprintf(stdout, "  /%u: Test object, instances:\r\n", object->objID);
    prv_instance_t *instance = (prv_instance_t *)object->instanceList;
    while (instance != NULL) {
        fprintf(stdout, "    /%u/%u: shortId: %u, test: %u\r\n", object->objID, instance->shortID,
    instance->shortID, instance->test); instance = (prv_instance_t *)instance->next;
    }
    */
}

lwm2m_object_t *get_meter_object(void) {
    lwm2m_object_t *meterObj;

    meterObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != meterObj) {
        prv_instance_t *targetP;
        memset(meterObj, 0, sizeof(lwm2m_object_t));

        meterObj->objID = METER_OBJECT_ID;
        targetP = (prv_instance_t *)lwm2m_malloc(sizeof(prv_instance_t));
        if (NULL == targetP)
            return NULL;
        memset(targetP, 0, sizeof(prv_instance_t));
        targetP->shortID = 0;
        meterObj->instanceList = LWM2M_LIST_ADD(meterObj->instanceList, targetP);
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID
         * is provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the
         * memory allocated to it)
         */
        meterObj->readFunc = prv_read;
        // meterObj->discoverFunc = prv_discover;
        meterObj->writeFunc = prv_write;
        meterObj->createFunc = prv_create;
        meterObj->deleteFunc = prv_delete;
        // meterObj->userData = lwm2m_malloc(sizeof(prv_instance_t));

        // /*
        //  * Also some user data can be stored in the object with a private structure containing the needed
        //  variables
        //  */
        // if (NULL != meterObj->userData) {
        //     ((prv_instance_t *)meterObj->userData)->typeOfMeter = PRV_TYPE_OF_METER;
        //     // strcpy(((device_data_t *)deviceObj->userData)->time_offset, "+01:00");
        // } else {
        //     lwm2m_free(meterObj->instanceList);
        //     lwm2m_free(meterObj);
        //     meterObj = NULL;
        // }
    }

    return meterObj;
}

void free_object_meter(lwm2m_object_t *object) {
    while (object->instanceList != NULL) {
        prv_instance_t *meterInstance = (prv_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        lwm2m_free(meterInstance);
    }
}
