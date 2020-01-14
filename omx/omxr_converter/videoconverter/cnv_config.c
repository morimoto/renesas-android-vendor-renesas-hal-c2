/*
 * Copyright(C) 2015-2018 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 */
 

/**
 * OMX Converter Configuration
 *
 * \file cnv_config.c
 * \attention
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "cnv_type.h"
#include "cnv_config.h"
#include <stdio.h>

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/
/*< Plugin Information*/
static CNV_PLUGIN_CONFIG_T sPluginConfig[] = {
    {CNV_PLUGIN_ID_FDP,"libomxr_cnvpfdp.so"},
};

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/
/**
* \breif get plugin index
*/
CNV_ERRORTYPE CnvConfig_GetPuluginIndex(
    CNV_U32 *nIndex,                /**< [in] exteinsion index */
    CNV_SUBERRORTYPE *pSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;
    
    /* Set PluginMaxIndex */
    *nIndex = (sizeof(sPluginConfig) / sizeof(CNV_PLUGIN_CONFIG_T));
    if( (*nIndex < 1) || (*nIndex > CNV_COUNT_MAX_PLUGINS) ){
        eResult = CNV_ERROR_INVALID_PARAMETER;
        if( pSubErrorCode != NULL ){
            *pSubErrorCode = CNV_ERROR_PARAM_CONFIG_PLUGINDATA;
        }
    }else{
        eResult = CNV_ERROR_NONE;
        if( pSubErrorCode !=  NULL ){
            *pSubErrorCode = CNV_SUBERROR_NONE;
        }
    }
    return eResult;
}

/**
* \brief get plugin information
*/
CNV_ERRORTYPE CnvConfig_GetPluginDllInfo(
    CNV_U32             nNum,            /**< [in] plugin number */
    CNV_PLUGIN_CONFIG_T *psPluginConfig, /**< [in] plugin configuration */
    CNV_SUBERRORTYPE    *pSubErrorCode   /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;
    CNV_U32 nPlugins = (sizeof(sPluginConfig) / sizeof(CNV_PLUGIN_CONFIG_T));
    
    if( nNum > (nPlugins-1) ){
        eResult = CNV_ERROR_INVALID_PARAMETER;
        if( pSubErrorCode != NULL ){
            *pSubErrorCode = CNV_ERROR_PARAM_CONFIG_PLUGINDATA;
        }
    }else{
        /** Set DLL load path */
        psPluginConfig->PluginId      = sPluginConfig[nNum].PluginId;
        psPluginConfig->PluginPath    = sPluginConfig[nNum].PluginPath;
        eResult = CNV_ERROR_NONE;
        if( pSubErrorCode != NULL ){
            *pSubErrorCode = CNV_SUBERROR_NONE;
        }
    }
    return eResult;
}
