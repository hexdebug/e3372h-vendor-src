/* $Id: upnpdescgen.c,v 1.63 2011/05/27 21:36:50 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2011 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

   /**********************问题单修改记录******************************************
    日期              修改人         问题单号           修改内容

 2012.08.29        z00203875     2082304944    UPnP认证测试
 2014.11.21        n00202065     4110603649    禁用upnp wan侧连接断开拨号功能
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#ifdef ENABLE_EVENTS
#include "getifaddr.h"
#include "upnpredirect.h"
#endif
#include "upnpdescgen.h"
#include "miniupnpdpath.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"
#include "upnpurns.h"
#include "getconnstatus.h"


/* Event magical values codes */
#define CONNECTIONSTATUS_MAGICALVALUE (249)
#define FIREWALLENABLED_MAGICALVALUE (250)
#define INBOUNDPINHOLEALLOWED_MAGICALVALUE (251)
#define SYSTEMUPDATEID_MAGICALVALUE (252)
#define PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE (253)
#define EXTERNALIPADDRESS_MAGICALVALUE (254)
#define DEFAULTCONNECTIONSERVICE_MAGICALVALUE (255)


static const char * const upnptypes[] =
{
	"string",
	"boolean",
	"ui2",
	"ui4",
	"bin.base64"
};

static const char * const upnpdefaultvalues[] =
{
	0,
	"IP_Routed"/*"Unconfigured"*/, /* 1 default value for ConnectionType */
	"3600", /* 2 default value for PortMappingLeaseDuration */
};

static const char * const upnpallowedvalues[] =
{
	0,		/* 0 */
	"DSL",	/* 1 */
	"POTS",
	"Cable",
	"Ethernet",
	0,
	"Up",	/* 6 */
	"Down",
	"Initializing",
	"Unavailable",
	0,
	"TCP",	/* 11 */
	"UDP",
	0,
	"Unconfigured",	/* 14 */
	"IP_Routed",
	"IP_Bridged",
	0,
	"Unconfigured",	/* 18 */
	"Connecting",
	"Connected",
	"PendingDisconnect",
	"Disconnecting",
	"Disconnected",
	0,
	"ERROR_NONE",	/* 25 */
/* Optionals values :
 * ERROR_COMMAND_ABORTED
 * ERROR_NOT_ENABLED_FOR_INTERNET
 * ERROR_USER_DISCONNECT
 * ERROR_ISP_DISCONNECT
 * ERROR_IDLE_DISCONNECT
 * ERROR_FORCED_DISCONNECT
 * ERROR_NO_CARRIER
 * ERROR_IP_CONFIGURATION
 * ERROR_UNKNOWN */
	0,
	"",		/* 27 */
	0
};

static const int upnpallowedranges[] = {
	0,
	/* 1 PortMappingLeaseDuration */
	0,
	604800,
	/* 3 InternalPort */
	1,
	65535,
    /* 5 LeaseTime */
	1,
	86400,
	/* 7 OutboundPinholeTimeout */
	100,
	200,
};

static const char * magicargname[] = {
	0,
	"StartPort",
	"EndPort",
	"RemoteHost",
	"RemotePort",
	"InternalClient",
	"InternalPort",
	"IsWorking"
};

static const char xmlver[] = 
	"<?xml version=\"1.0\"?>\r\n";
static const char root_service[] =
	"scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"";

#ifdef FEATURE_HUAWEI_MBB_PNPX
static const char root_device[] = 
	"root xmlns:pnpx=\"http://schemas.microsoft.com/windows/pnpx/2005/11\" "
    "xmlns:df=\"http://schemas.microsoft.com/windows/2008/09/devicefoundation\" "
    "xmlns=\"urn:schemas-upnp-org:device-1-0\"";
#else
static const char root_device[] = 
	"root xmlns=\"urn:schemas-upnp-org:device-1-0\"";
#endif

/* root Description of the UPnP Device 
 * fixed to match UPnP_IGD_InternetGatewayDevice 1.0.pdf 
 * Needs to be checked with UPnP-gw-InternetGatewayDevice-v2-Device.pdf
 * presentationURL is only "recommended" but the router doesn't appears
 * in "Network connections" in Windows XP if it is not present. */

#ifdef FEATURE_HUAWEI_MBB_PNPX

#define PNPX_OFFSET 4
#define PNPX_LINE_NUM 4

#else

#define PNPX_OFFSET 0
#define PNPX_LINE_NUM 0

#endif
static const struct XMLElt rootDesc[] =
{
/* 0 */
	{root_device, INITHELPER(1,2)},
	{"specVersion", INITHELPER(3,2)},
#if defined(ENABLE_L3F_SERVICE) || defined(HAS_DUMMY_SERVICE) || defined(ENABLE_DP_SERVICE)
	{"device", INITHELPER(5,13 + PNPX_LINE_NUM)},
#else
	{"device", INITHELPER(5,12 + PNPX_LINE_NUM)},
#endif
	{"/major", "1"},
	{"/minor", "0"},
/* 5 */
#ifdef FEATURE_HUAWEI_MBB_PNPX
    {"/pnpx:X_hardwareId", hardware_id},
    {"/pnpx:X_compatibleId", "GenericUmPass"},
    {"/pnpx:X_deviceCategory", "NetworkInfrastructure.Router"},
    {"/df:X_deviceCategory", "Network.Router.Wireless"},
#endif 
	{"/deviceType", DEVICE_TYPE_IGD},
		/* urn:schemas-upnp-org:device:InternetGatewayDevice:1 or 2 */
	{"/friendlyName", upnp_cfg_st.rootdev_friendlyname},	/* required */
#ifdef FEATURE_HUAWEI_MBB_PNPX
	{"/manufacturer", "Vodafone(Huawei)"},		/* required */
#else
	{"/manufacturer", upnp_cfg_st.rootdev_manufacturer},		/* required */
#endif
/* 8 */
	{"/manufacturerURL", upnp_cfg_st.rootdev_manufacturerurl},	/* optional */
	{"/modelDescription", upnp_cfg_st.rootdev_modeldescription}, /* recommended */
	{"/modelName", upnp_cfg_st.rootdev_modelname},	/* required */
	{"/modelNumber", modelnumber},
	{"/modelURL", upnp_cfg_st.rootdev_modelurl},
	{"/serialNumber", serialnumber},
    /* BEGIN 2082304944 zhoujianchun 00203875 2012.8.29 modified */
	{"/UDN", uuidvalue_root},	/* required */
    /* END   2082304944 zhoujianchun 00203875 2012.8.29 modified */
	/* see if /UPC is needed. */
#ifdef ENABLE_6FC_SERVICE
#define SERVICES_OFFSET 63
#else
#define SERVICES_OFFSET 58
#endif
#if defined(ENABLE_L3F_SERVICE) || defined(HAS_DUMMY_SERVICE) || defined(ENABLE_DP_SERVICE)
	/* here we dening Services for the root device :
	 * L3F and DUMMY and DeviceProtection */
#ifdef ENABLE_L3F_SERVICE
#define NSERVICES1 1
#else
#define NSERVICES1 0
#endif
#ifdef HAS_DUMMY_SERVICE
#define NSERVICES2 1
#else
#define NSERVICES2 0
#endif
#ifdef ENABLE_DP_SERVICE
#define NSERVICES3 1
#else
#define NSERVICES3 0
#endif
#define NSERVICES (NSERVICES1+NSERVICES2+NSERVICES3)
	{"serviceList", INITHELPER(SERVICES_OFFSET + PNPX_OFFSET,NSERVICES)},
	{"deviceList", INITHELPER(18 + PNPX_OFFSET,1)},
#ifdef FEATURE_HUAWEI_MBB_PNPX
	{"/presentationURL", "http://vodafonemobile.wifi"},	/* recommended */
#else
	{"/presentationURL", presentationurl},	/* recommended */
#endif
#else
	{"deviceList", INITHELPER(18 + PNPX_OFFSET,1)},
	{"/presentationURL", presentationurl},	/* recommended */
	{0,0},
#endif
/* 18 */
	{"device", INITHELPER(19 + PNPX_OFFSET,13)},
/* 19 */
	{"/deviceType", DEVICE_TYPE_WAN}, /* required */
		/* urn:schemas-upnp-org:device:WANDevice:1 or 2 */
	{"/friendlyName", upnp_cfg_st.wandev_friendlyname},
	{"/manufacturer", upnp_cfg_st.wandev_manufacturer},
	{"/manufacturerURL", upnp_cfg_st.wandev_manufacturerurl},
	{"/modelDescription" , upnp_cfg_st.wandev_modeldescription},
	{"/modelName", upnp_cfg_st.wandev_modelname},
	{"/modelNumber", upnp_cfg_st.wandev_modelnumber},
	{"/modelURL", upnp_cfg_st.wandev_modelurl},
	{"/serialNumber", serialnumber},
    /* BEGIN 2082304944 zhoujianchun 00203875 2012.8.29 modified */
	{"/UDN", uuidvalue_wan},
    /* END   2082304944 zhoujianchun 00203875 2012.8.29 modified */
	{"/UPC", upnp_cfg_st.wandev_upc},
/* 30 */
	{"serviceList", INITHELPER(32 + PNPX_OFFSET,1)},
	{"deviceList", INITHELPER(38 + PNPX_OFFSET,1)},
/* 32 */
	{"service", INITHELPER(33 + PNPX_OFFSET,5)},
/* 33 */
	{"/serviceType",
			"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"},
	/*{"/serviceId", "urn:upnp-org:serviceId:WANCommonInterfaceConfig"}, */
	{"/serviceId", "urn:upnp-org:serviceId:WANCommonIFC1"}, /* required */
	{"/controlURL", WANCFG_CONTROLURL},
	{"/eventSubURL", WANCFG_EVENTURL},
	{"/SCPDURL", WANCFG_PATH},
/* 38 */
	{"device", INITHELPER(39 + PNPX_OFFSET,12)},
/* 39 */
	{"/deviceType", DEVICE_TYPE_WANC},
		/* urn:schemas-upnp-org:device:WANConnectionDevice:1 or 2 */
	{"/friendlyName", upnp_cfg_st.wancdev_friendlyname},
	{"/manufacturer", upnp_cfg_st.wancdev_manufacturer},
	{"/manufacturerURL", upnp_cfg_st.wancdev_manufacturerurl},
	{"/modelDescription", upnp_cfg_st.wancdev_modeldescription},
	{"/modelName", upnp_cfg_st.wancdev_modelname},
	{"/modelNumber", upnp_cfg_st.wancdev_modelnumber},
	{"/modelURL", upnp_cfg_st.wancdev_modelurl},
	{"/serialNumber", serialnumber},
    /* BEGIN 2082304944 zhoujianchun 00203875 2012.8.29 modified */
	{"/UDN", uuidvalue_wan_conn},
    /* END   2082304944 zhoujianchun 00203875 2012.8.29 modified */
	{"/UPC", upnp_cfg_st.wancdev_upc},
#ifdef ENABLE_6FC_SERVICE
	{"serviceList", INITHELPER(51 + PNPX_OFFSET,2)},
#else
	{"serviceList", INITHELPER(51 + PNPX_OFFSET,1)},
#endif
/* 51 */
	{"service", INITHELPER(53 + PNPX_OFFSET,5)},
	{"service", INITHELPER(58 + PNPX_OFFSET,5)},
/* 53 */
	{"/serviceType", SERVICE_TYPE_WANIPC},
		/* urn:schemas-upnp-org:service:WANIPConnection:2 for v2 */
	{"/serviceId", SERVICE_ID_WANIPC},
		/* urn:upnp-org:serviceId:WANIPConn1 or 2 */
	{"/controlURL", WANIPC_CONTROLURL},
	{"/eventSubURL", WANIPC_EVENTURL},
	{"/SCPDURL", WANIPC_PATH},
#ifdef ENABLE_6FC_SERVICE
/* 58 */
	{"/serviceType", "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1"},
	{"/serviceId", "urn:upnp-org:serviceId:WANIPv6FC1"},
	{"/controlURL", WANIP6FC_CONTROLURL},
	{"/eventSubURL", WANIP6FC_EVENTURL},
	{"/SCPDURL", WANIP6FC_PATH},
#endif
/* 58 / 63 = SERVICES_OFFSET*/
#if defined(HAS_DUMMY_SERVICE) || defined(ENABLE_L3F_SERVICE) || defined(ENABLE_DP_SERVICE)
	{"service", INITHELPER(SERVICES_OFFSET+2 + PNPX_OFFSET,5)},
	{"service", INITHELPER(SERVICES_OFFSET+7 + PNPX_OFFSET,5)},
#endif
#ifdef HAS_DUMMY_SERVICE
/* 60 / 65 = SERVICES_OFFSET+2 */
	{"/serviceType", "urn:schemas-dummy-com:service:Dummy:1"},
	{"/serviceId", "urn:dummy-com:serviceId:dummy1"},
	{"/controlURL", "/dummy"},
	{"/eventSubURL", "/dummy"},
	{"/SCPDURL", DUMMY_PATH},
#endif
#ifdef ENABLE_L3F_SERVICE
/* 60 / 65 = SERVICES_OFFSET+2 */
	{"/serviceType", "urn:schemas-upnp-org:service:Layer3Forwarding:1"},
	//{"/serviceId", "urn:upnp-org:serviceId:Layer3Forwarding1"},
	{"/serviceId", "urn:upnp-org:serviceId:L3Forwarding1"},
	{"/controlURL", L3F_CONTROLURL}, /* The Layer3Forwarding service is only */
	{"/eventSubURL", L3F_EVENTURL}, /* recommended, not mandatory */
	{"/SCPDURL", L3F_PATH},
#endif
#ifdef ENABLE_DP_SERVICE
/* InternetGatewayDevice v2 : 
 * it is RECOMMEDED that DeviceProtection service is implemented and applied.
 * If DeviceProtection is not implemented and applied, it is RECOMMENDED
 * that control points are able to access only actions and parameters defined
 * as Public role. */
/* 65 / 70 = SERVICES_OFFSET+7 */
	{"/serviceType", "urn:schemas-upnp-org:service:DeviceProtection:1"},
	{"/serviceId", "urn:upnp-org:serviceId:DeviceProtection1"},
	{"/controlURL", DP_CONTROLURL},
	{"/eventSubURL", DP_EVENTURL},
	{"/SCPDURL", DP_PATH},
#endif
	{0, 0}
};

/* WANIPCn.xml */
/* see UPnP_IGD_WANIPConnection 1.0.pdf
static struct XMLElt scpdWANIPCn[] =
{
	{root_service, {INITHELPER(1,2)}},
	{0, {0}}
};
*/
static const struct argument AddPortMappingArgs[] =
{
	{1, 11},	/* RemoteHost */
	{1, 12},	/* ExternalPort */
	{1, 14},	/* PortMappingProtocol */
	{1, 13},	/* InternalPort */
	{1, 15},	/* InternalClient */
	{1, 9},		/* PortMappingEnabled */
	{1, 16},	/* PortMappingDescription */
	{1, 10},	/* PortMappingLeaseDuration */
	{0, 0}
};

#ifdef IGD_V2
static const struct argument AddAnyPortMappingArgs[] =
{
	{1, 11},	/* RemoteHost */
	{1, 12},	/* ExternalPort */
	{1, 14},	/* PortMappingProtocol */
	{1, 13},	/* InternalPort */
	{1, 15},	/* InternalClient */
	{1, 9},		/* PortMappingEnabled */
	{1, 16},	/* PortMappingDescription */
	{1, 10},	/* PortMappingLeaseDuration */
	{2, 12},	/* NewReservedPort / ExternalPort */
	{0, 0}
};

static const struct argument DeletePortMappingRangeArgs[] =
{
	{1|(1<<2), 12},	/* NewStartPort / ExternalPort */
	{1|(2<<2), 12},	/* NewEndPort / ExternalPort */
	{1, 14},	/* NewProtocol / PortMappingProtocol */
	{1, 18},	/* NewManage / A_ARG_TYPE_Manage */
	{0, 0}
};

static const struct argument GetListOfPortMappingsArgs[] =
{
	{1|(1<<2), 12},	/* NewStartPort / ExternalPort */
	{1|(2<<2), 12},	/* NewEndPort / ExternalPort */
	{1, 14},	/* NewProtocol / PortMappingProtocol */
	{1, 18},	/* NewManage / A_ARG_TYPE_Manage */
	{1, 8},		/* NewNumberOfPorts / PortMappingNumberOfEntries */
	{2, 19},	/* NewPortListing / A_ARG_TYPE_PortListing */
	{0, 0}
};
#endif

static const struct argument GetExternalIPAddressArgs[] =
{
	{2, 7},
	{0, 0}
};

static const struct argument DeletePortMappingArgs[] = 
{
	{1, 11},
	{1, 12},
	{1, 14},
	{0, 0}
};

static const struct argument SetConnectionTypeArgs[] =
{
	{1, 0},
	{0, 0}
};

static const struct argument GetConnectionTypeInfoArgs[] =
{
	{2, 0},
	{2, 1},
	{0, 0}
};

static const struct argument GetStatusInfoArgs[] =
{
	{2, 2},
	{2, 4},
	{2, 3},
	{0, 0}
};

static const struct argument GetNATRSIPStatusArgs[] =
{
	{2, 5},
	{2, 6},
	{0, 0}
};

static const struct argument GetGenericPortMappingEntryArgs[] =
{
	{1, 8},
	{2, 11},
	{2, 12},
	{2, 14},
	{2, 13},
	{2, 15},
	{2, 9},
	{2, 16},
	{2, 10},
	{0, 0}
};

static const struct argument GetSpecificPortMappingEntryArgs[] =
{
	{1, 11},
	{1, 12},
	{1, 14},
	{2, 13},
	{2, 15},
	{2, 9},
	{2, 16},
	{2, 10},
	{0, 0}
};

static const struct action WANIPCnActions[] =
{
	{"SetConnectionType", SetConnectionTypeArgs}, /* R */
	{"GetConnectionTypeInfo", GetConnectionTypeInfoArgs}, /* R */
#ifdef MBB_FEATURE_UPNP_CERTIFICATION
	{"RequestConnection", 0}, /* R */
	{"ForceTermination", 0}, /* R */
#endif
	/*{"SetAutoDisconnectTime", 0},*/ /* O */
	/*{"SetIdleDisconnectTime", 0},*/ /* O */
	/*{"SetWarnDisconnectDelay", 0}, */ /* O */
	{"GetStatusInfo", GetStatusInfoArgs}, /* R */
	/*GetAutoDisconnectTime*/
	/*GetIdleDisconnectTime*/
	/*GetWarnDisconnectDelay*/
	{"GetNATRSIPStatus", GetNATRSIPStatusArgs}, /* R */
	{"GetGenericPortMappingEntry", GetGenericPortMappingEntryArgs}, /* R */
	{"GetSpecificPortMappingEntry", GetSpecificPortMappingEntryArgs}, /* R */
	{"AddPortMapping", AddPortMappingArgs}, /* R */
	{"DeletePortMapping", DeletePortMappingArgs}, /* R */
	{"GetExternalIPAddress", GetExternalIPAddressArgs}, /* R */
#ifdef IGD_V2
	{"DeletePortMappingRange", DeletePortMappingRangeArgs}, /* R, IGD v2 */
	{"GetListOfPortMappings", GetListOfPortMappingsArgs}, /* R, IGD v2 */
	{"AddAnyPortMapping", AddAnyPortMappingArgs}, /* R, IGD v2 */
#endif
#if 0
	{"AddPortMapping", AddPortMappingArgs}, /* R */
	{"GetExternalIPAddress", GetExternalIPAddressArgs}, /* R */
	{"DeletePortMapping", DeletePortMappingArgs}, /* R */
	{"SetConnectionType", SetConnectionTypeArgs}, /* R */
	{"GetConnectionTypeInfo", GetConnectionTypeInfoArgs}, /* R */
	{"RequestConnection", 0}, /* R */
	{"ForceTermination", 0}, /* R */
	{"GetStatusInfo", GetStatusInfoArgs}, /* R */
	{"GetNATRSIPStatus", GetNATRSIPStatusArgs}, /* R */
	{"GetGenericPortMappingEntry", GetGenericPortMappingEntryArgs}, /* R */
	{"GetSpecificPortMappingEntry", GetSpecificPortMappingEntryArgs}, /* R */
/* added in v2 UPnP-gw-WANIPConnection-v2-Service.pdf */
#ifdef IGD_V2
	{"AddAnyPortMapping", AddAnyPortMappingArgs},
	{"DeletePortMappingRange", DeletePortMappingRangeArgs},
	{"GetListOfPortMappings", GetListOfPortMappingsArgs},
#endif
#endif
	{0, 0}
};
/* R=Required, O=Optional */

static const struct stateVar WANIPCnVars[] =
{
/* 0 */
#if 0
	{"ConnectionType", 0, 0/*1*/}, /* required */
	{"PossibleConnectionTypes", 0|0x80, 0, 14, 15},
#endif
	{"ConnectionType", 0, 1, 14, 15}, /* required */
	{"PossibleConnectionTypes", 0|0x80, 0, 0, 15},
	 /* Required
	  * Allowed values : Unconfigured / IP_Routed / IP_Bridged */
	{"ConnectionStatus", 0|0x80, 0/*1*/, 18,
	 CONNECTIONSTATUS_MAGICALVALUE }, /* required */
	 /* Allowed Values : Unconfigured / Connecting(opt) / Connected
	  *                  PendingDisconnect(opt) / Disconnecting (opt)
	  *                  Disconnected */
	{"Uptime", 3, 0, 0, 0},	/* Required */
	{"LastConnectionError", 0, 0, 25, 0},	/* required : */
	 /* Allowed Values : ERROR_NONE(req) / ERROR_COMMAND_ABORTED(opt)
	  *                  ERROR_NOT_ENABLED_FOR_INTERNET(opt)
	  *                  ERROR_USER_DISCONNECT(opt)
	  *                  ERROR_ISP_DISCONNECT(opt)
	  *                  ERROR_IDLE_DISCONNECT(opt)
	  *                  ERROR_FORCED_DISCONNECT(opt)
	  *                  ERROR_NO_CARRIER(opt)
	  *                  ERROR_IP_CONFIGURATION(opt)
	  *                  ERROR_UNKNOWN(opt) */
	{"RSIPAvailable", 1, 0, 0, 0}, /* required */
	{"NATEnabled", 1, 0, 0, 0},    /* required */
	{"ExternalIPAddress", 0|0x80, 0, 0,
	 EXTERNALIPADDRESS_MAGICALVALUE}, /* required. Default : empty string */
	{"PortMappingNumberOfEntries", 2|0x80, 0, 0,
	 PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE}, /* required >= 0 */
	{"PortMappingEnabled", 1, 0, 0, 0}, /* Required */
/* 10 */
	{"PortMappingLeaseDuration", 3, 2, 1, 0}, /* required */
	/* TODO : for IGD v2 : 
	 * <stateVariable sendEvents="no">
	 *   <name>PortMappingLeaseDuration</name>
	 *   <dataType>ui4</dataType>
	 *   <defaultValue>Vendor-defined</defaultValue>
	 *   <allowedValueRange>
	 *      <minimum>0</minimum>
	 *      <maximum>604800</maximum>
	 *   </allowedValueRange>
	 * </stateVariable> */
	{"RemoteHost", 0, 0, 0, 0},   /* required. Default : empty string */
	{"ExternalPort", 2, 0, 0, 0}, /* required */
	{"InternalPort", 2, 0, 3, 0}, /* required */
	{"PortMappingProtocol", 0, 0, 11, 0}, /* required allowedValues: TCP/UDP */
	{"InternalClient", 0, 0, 0, 0}, /* required */
	{"PortMappingDescription", 0, 0, 0, 0}, /* required default: empty string */
/* added in v2 UPnP-gw-WANIPConnection-v2-Service.pdf */
#ifdef IGD_V2
	{"SystemUpdateID", 3|0x80, 0, 0, SYSTEMUPDATEID_MAGICALVALUE},
	{"A_ARG_TYPE_Manage", 1, 0, 0, 0},
	{"A_ARG_TYPE_PortListing", 0, 0, 0, 0},
#endif
	{0, 0, 0, 0, 0}
};

static const struct serviceDesc scpdWANIPCn =
{ WANIPCnActions, WANIPCnVars };

/* WANCfg.xml */
/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */

static const struct argument GetCommonLinkPropertiesArgs[] =
{
	{2, 0},
	{2, 1},
	{2, 2},
	{2, 3},
	{0, 0}
};

static const struct argument GetTotalBytesSentArgs[] =
{
	{2, 4},
	{0, 0}
};

static const struct argument GetTotalBytesReceivedArgs[] =
{
	{2, 5},
	{0, 0}
};

static const struct argument GetTotalPacketsSentArgs[] =
{
	{2, 6},
	{0, 0}
};

static const struct argument GetTotalPacketsReceivedArgs[] =
{
	{2, 7},
	{0, 0}
};

static const struct action WANCfgActions[] =
{
	{"GetCommonLinkProperties", GetCommonLinkPropertiesArgs}, /* Required */
	{"GetTotalBytesSent", GetTotalBytesSentArgs},             /* optional */
	{"GetTotalBytesReceived", GetTotalBytesReceivedArgs},     /* optional */
	{"GetTotalPacketsSent", GetTotalPacketsSentArgs},         /* optional */
	{"GetTotalPacketsReceived", GetTotalPacketsReceivedArgs}, /* optional */
	{0, 0}
};

/* See UPnP_IGD_WANCommonInterfaceConfig 1.0.pdf */
static const struct stateVar WANCfgVars[] =
{
	{"WANAccessType", 0, 0, 1, 0},
	/* Allowed Values : DSL / POTS / Cable / Ethernet 
	 * Default value : empty string */
	{"Layer1UpstreamMaxBitRate", 3, 0, 0, 0},
	{"Layer1DownstreamMaxBitRate", 3, 0, 0, 0},
	{"PhysicalLinkStatus", 0|0x80, 0, 6, 6},
	/*  allowed values : 
	 *      Up / Down / Initializing (optional) / Unavailable (optionnal)
	 *  no Default value 
	 *  Evented */
	{"TotalBytesSent", 3, 0, 0, 0},	   /* Optional */
	{"TotalBytesReceived", 3, 0, 0, 0},  /* Optional */
	{"TotalPacketsSent", 3, 0, 0, 0},    /* Optional */
	{"TotalPacketsReceived", 3, 0, 0, 0},/* Optional */
	/*{"MaximumActiveConnections", 2, 0},	// allowed Range value // OPTIONAL */
	/*{"WANAccessProvider", 0, 0},*/   /* Optional */
	{0, 0, 0, 0, 0}
};

static const struct serviceDesc scpdWANCfg =
{ WANCfgActions, WANCfgVars };

#ifdef ENABLE_L3F_SERVICE
/* Read UPnP_IGD_Layer3Forwarding_1.0.pdf */
static const struct argument SetDefaultConnectionServiceArgs[] =
{
	{1, 0}, /* in */
	{0, 0}
};

static const struct argument GetDefaultConnectionServiceArgs[] =
{
	{2, 0}, /* out */
	{0, 0}
};

static const struct action L3FActions[] =
{
	{"SetDefaultConnectionService", SetDefaultConnectionServiceArgs}, /* Req */
	{"GetDefaultConnectionService", GetDefaultConnectionServiceArgs}, /* Req */
	{0, 0}
};

static const struct stateVar L3FVars[] =
{
	{"DefaultConnectionService", 0|0x80, 0, 0,
	 DEFAULTCONNECTIONSERVICE_MAGICALVALUE}, /* Required */
	{0, 0, 0, 0, 0}
};

static const struct serviceDesc scpdL3F =
{ L3FActions, L3FVars };
#endif

#ifdef ENABLE_6FC_SERVICE
/* see UPnP-gw-WANIPv6FirewallControl-v1-Service.pdf */
static const struct argument GetFirewallStatusArgs[] =
{
	{2|0x80, 0}, /* OUT : FirewallEnabled */
	{2|0x80, 6}, /* OUT : InboundPinholeAllowed */
	{0, 0}
};

static const struct argument GetOutboundPinholeTimeoutArgs[] =
{
	{1|0x80|(3<<2), 1}, /* RemoteHost IN A_ARG_TYPE_IPv6Address */
	{1|0x80|(4<<2), 2}, /* RemotePort IN A_ARG_TYPE_Port */
	{1|0x80|(5<<2), 1}, /* InternalClient IN A_ARG_TYPE_IPv6Address */
	{1|0x80|(6<<2), 2}, /* InternalPort IN A_ARG_TYPE_Port */
	{1|0x80, 3}, /* Protocol IN A_ARG_TYPE_Protocol */
	{2|0x80, 7}, /* OutboundPinholeTimeout OUT A_ARG_TYPE_OutboundPinholeTimeout */
	{0, 0}
};

static const struct argument AddPinholeArgs[] =
{
	{1|0x80|(3<<2), 1}, /* RemoteHost IN A_ARG_TYPE_IPv6Address */
	{1|0x80|(4<<2), 2}, /* RemotePort IN A_ARG_TYPE_Port */
	{1|0x80|(5<<2), 1}, /* InternalClient IN A_ARG_TYPE_IPv6Address */
	{1|0x80|(6<<2), 2}, /* InternalPort IN A_ARG_TYPE_Port */
	{1|0x80, 3}, /* Protocol IN A_ARG_TYPE_Protocol */
	{1|0x80, 5}, /* LeaseTime IN A_ARG_TYPE_LeaseTime */
	{2|0x80, 4}, /* UniqueID OUT A_ARG_TYPE_UniqueID */
	{0, 0}
};

static const struct argument UpdatePinholeArgs[] =
{
	{1|0x80, 4}, /* UniqueID IN A_ARG_TYPE_UniqueID */
	{1, 5}, /* LeaseTime IN A_ARG_TYPE_LeaseTime */
	{0, 0}
};

static const struct argument DeletePinholeArgs[] =
{
	{1|0x80, 4}, /* UniqueID IN A_ARG_TYPE_UniqueID */
	{0, 0}
};

static const struct argument GetPinholePacketsArgs[] =
{
	{1|0x80, 4}, /* UniqueID IN A_ARG_TYPE_UniqueID */
	{2|0x80, 9}, /* PinholePackets OUT A_ARG_TYPE_PinholePackets */
	{0, 0}
};

static const struct argument CheckPinholeWorkingArgs[] =
{
	{1|0x80, 4}, /* UniqueID IN A_ARG_TYPE_UniqueID */
	{2|0x80|(7<<2), 8}, /* IsWorking OUT A_ARG_TYPE_Boolean */
	{0, 0}
};

static const struct action IPv6FCActions[] =
{
	{"GetFirewallStatus", GetFirewallStatusArgs}, /* Req */
	{"GetOutboundPinholeTimeout", GetOutboundPinholeTimeoutArgs}, /* Opt */
	{"AddPinhole", AddPinholeArgs}, /* Req */
	{"UpdatePinhole", UpdatePinholeArgs}, /* Req */
	{"DeletePinhole", DeletePinholeArgs}, /* Req */
	{"GetPinholePackets", GetPinholePacketsArgs}, /* Req */
	{"CheckPinholeWorking", CheckPinholeWorkingArgs}, /* Opt */
	{0, 0}
};

static const struct stateVar IPv6FCVars[] =
{
	{"FirewallEnabled", 1|0x80, 0, 0,
	 FIREWALLENABLED_MAGICALVALUE}, /* Required */
	{"A_ARG_TYPE_IPv6Address", 0, 0, 0, 0}, /* Required */
	{"A_ARG_TYPE_Port", 2, 0, 0, 0}, /* Required */
	{"A_ARG_TYPE_Protocol", 2, 0, 0, 0}, /* Required */
/* 4 */
	{"A_ARG_TYPE_UniqueID", 2, 0, 0, 0}, /* Required */
	{"A_ARG_TYPE_LeaseTime", 3, 0, 5, 0}, /* Required */
	{"InboundPinholeAllowed", 1|0x80, 0, 0,
	 INBOUNDPINHOLEALLOWED_MAGICALVALUE}, /* Required */
	{"A_ARG_TYPE_OutboundPinholeTimeout", 3, 0, 7, 0}, /* Optional */
/* 8 */
	{"A_ARG_TYPE_Boolean", 1, 0, 0, 0}, /* Optional */
	{"A_ARG_TYPE_PinholePackets", 3, 0, 0, 0}, /* Required */
	{0, 0, 0, 0, 0}
};

static const struct serviceDesc scpd6FC =
{ IPv6FCActions, IPv6FCVars };
#endif

#ifdef ENABLE_DP_SERVICE
/* UPnP-gw-DeviceProtection-v1-Service.pdf */
static const struct action DPActions[] =
{
	{"SendSetupMessage", 0},
	{"GetSupportedProtocols", 0},
	{"GetAssignedRoles", 0},
	{0, 0}
};

static const struct stateVar DPVars[] =
{
	{"SetupReady", 1|0x80, 0, 0, 0},
	{"SupportedProtocols", 0, 0, 0, 0},
	{"A_ARG_TYPE_ACL", 0, 0, 0, 0},
	{"A_ARG_TYPE_IdentityList", 0, 0, 0, 0},
	{"A_ARG_TYPE_Identity", 0, 0, 0, 0},
	{"A_ARG_TYPE_Base64", 4, 0, 0, 0},
	{"A_ARG_TYPE_String", 0, 0, 0, 0},
	{0, 0, 0, 0, 0}
};

static const struct serviceDesc scpdDP =
{ DPActions, DPVars };
#endif

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *
strcat_str(char * str, int * len, int * tmplen, const char * s2)
{
	int s2len;
	int newlen;
	char * p;

	s2len = (int)strlen(s2);
	if(*tmplen <= (*len + s2len))
	{
		if(s2len < 256)
			newlen = *tmplen + 256;
		else
			newlen = *tmplen + s2len + 1;
		p = (char *)realloc(str, newlen);
		if(p == NULL) /* handle a failure of realloc() */
			return str;
		str = p;
		*tmplen = newlen;
	}
	/*strcpy(str + *len, s2); */
	memcpy(str + *len, s2, s2len + 1);
	*len += s2len;
	return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *
strcat_char(char * str, int * len, int * tmplen, char c)
{
	char * p;

	if(*tmplen <= (*len + 1))
	{
		*tmplen += 256;
		p = (char *)realloc(str, *tmplen);
		if(p == NULL) /* handle a failure of realloc() */
		{
			*tmplen -= 256;
			return str;
		}
		str = p;
	}
	str[*len] = c;
	(*len)++;
	return str;
}

/* strcat_int()
 * concatenate the string representation of the integer.
 * call strcat_char() */
static char *
strcat_int(char * str, int * len, int * tmplen, int i)
{
	char buf[16];
	int j;

	if(i < 0) {
		str = strcat_char(str, len, tmplen, '-');
		i = -i;
	} else if(i == 0) {
		/* special case for 0 */
		str = strcat_char(str, len, tmplen, '0');
		return str;
	}
	j = 0;
	while(i && j < (int)(sizeof(buf))) {
		buf[j++] = '0' + (i % 10);
		i = i / 10;
	}
	while(j > 0) {
		str = strcat_char(str, len, tmplen, buf[--j]);
	}
	return str;
}

/* iterative subroutine using a small stack
 * This way, the progam stack usage is kept low */
static char *
genXML(char * str, int * len, int * tmplen,
                   const struct XMLElt * p)
{
	unsigned short i, j;
	unsigned long k;
	int top;
	const char * eltname, *s;
	char c;
	struct {
		unsigned short i;
		unsigned short j;
		const char * eltname;
	} pile[16]; /* stack */
	top = -1;
	i = 0;	/* current node */
	j = 1;	/* i + number of nodes*/
	for(;;)
	{
		eltname = p[i].eltname;
		if(!eltname)
			return str;
		if(eltname[0] == '/')
		{
			if(p[i].data && p[i].data[0])
			{
				/*printf("<%s>%s<%s>\n", eltname+1, p[i].data, eltname); */
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname+1);
				str = strcat_char(str, len, tmplen, '>');
				str = strcat_str(str, len, tmplen, p[i].data);
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname);
				str = strcat_char(str, len, tmplen, '>');
			}
			for(;;)
			{
				if(top < 0)
					return str;
				i = ++(pile[top].i);
				j = pile[top].j;
				/*printf("  pile[%d]\t%d %d\n", top, i, j); */
				if(i==j)
				{
					/*printf("</%s>\n", pile[top].eltname); */
					str = strcat_char(str, len, tmplen, '<');
					str = strcat_char(str, len, tmplen, '/');
					s = pile[top].eltname;
					for(c = *s; c > ' '; c = *(++s))
						str = strcat_char(str, len, tmplen, c);
					str = strcat_char(str, len, tmplen, '>');
					top--;
				}
				else
					break;
			}
		}
		else
		{
			/*printf("<%s>\n", eltname); */
			str = strcat_char(str, len, tmplen, '<');
			str = strcat_str(str, len, tmplen, eltname);
			str = strcat_char(str, len, tmplen, '>');
			k = (unsigned long)p[i].data;
			i = k & 0xffff;
			j = i + (k >> 16);
			top++;
			/*printf(" +pile[%d]\t%d %d\n", top, i, j); */
			pile[top].i = i;
			pile[top].j = j;
			pile[top].eltname = eltname;
		}
	}
}

/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string. 
 * - tmp_uuid argument is used to build the uuid string */
char *
genRootDesc(int * len)
{
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	* len = strlen(xmlver);
	/*strcpy(str, xmlver); */
	memcpy(str, xmlver, *len + 1);
	str = genXML(str, len, &tmplen, rootDesc);
	str[*len] = '\0';
	return str;
}

/* genServiceDesc() :
 * Generate service description with allowed methods and 
 * related variables. */
static char *
genServiceDesc(int * len, const struct serviceDesc * s)
{
	int i, j;
	const struct action * acts;
	const struct stateVar * vars;
	const struct argument * args;
	const char * p;
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	/*strcpy(str, xmlver); */
	*len = strlen(xmlver);
	memcpy(str, xmlver, *len + 1);
	
	acts = s->actionList;
	vars = s->serviceStateTable;

	str = strcat_char(str, len, &tmplen, '<');
	str = strcat_str(str, len, &tmplen, root_service);
	str = strcat_char(str, len, &tmplen, '>');

	str = strcat_str(str, len, &tmplen,
		"<specVersion><major>1</major><minor>0</minor></specVersion>");

	i = 0;
	str = strcat_str(str, len, &tmplen, "<actionList>");
	while(acts[i].name)
	{
		str = strcat_str(str, len, &tmplen, "<action><name>");
		str = strcat_str(str, len, &tmplen, acts[i].name);
		str = strcat_str(str, len, &tmplen, "</name>");
		/* argument List */
		args = acts[i].args;
		if(args)
		{
			str = strcat_str(str, len, &tmplen, "<argumentList>");
			j = 0;
			while(args[j].dir)
			{
				str = strcat_str(str, len, &tmplen, "<argument><name>");
				if((args[j].dir & 0x80) == 0) {
					str = strcat_str(str, len, &tmplen, "New");
				}
				p = vars[args[j].relatedVar].name;
				if(args[j].dir & 0x7c) {
					/* use magic values ... */
					str = strcat_str(str, len, &tmplen, magicargname[(args[j].dir & 0x7c) >> 2]);
				} else if(0 == memcmp(p, "PortMapping", 11)
				   && 0 != memcmp(p + 11, "Description", 11)) {
					if(0 == memcmp(p + 11, "NumberOfEntries", 15)) {
						/* PortMappingNumberOfEntries */
#ifdef IGD_V2
						if(0 == memcmp(acts[i].name, "GetListOfPortMappings", 22)) {
							str = strcat_str(str, len, &tmplen, "NumberOfPorts");
						} else {
							str = strcat_str(str, len, &tmplen, "PortMappingIndex");
						}
#else
						str = strcat_str(str, len, &tmplen, "PortMappingIndex");
#endif
					} else {
						/* PortMappingEnabled
						 * PortMappingLeaseDuration
						 * PortMappingProtocol */
						str = strcat_str(str, len, &tmplen, p + 11);
					}
#ifdef IGD_V2
				} else if(0 == memcmp(p, "A_ARG_TYPE_", 11)) {
					str = strcat_str(str, len, &tmplen, p + 11);
				} else if(0 == memcmp(p, "ExternalPort", 13)
				          && args[j].dir == 2
				          && 0 == memcmp(acts[i].name, "AddAnyPortMapping", 18)) {
					str = strcat_str(str, len, &tmplen, "ReservedPort");
#endif
				} else {
					str = strcat_str(str, len, &tmplen, p);
				}
				str = strcat_str(str, len, &tmplen, "</name><direction>");
				str = strcat_str(str, len, &tmplen, (args[j].dir&0x03)==1?"in":"out");
				str = strcat_str(str, len, &tmplen,
						"</direction><relatedStateVariable>");
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen,
						"</relatedStateVariable></argument>");
				j++;
			}
			str = strcat_str(str, len, &tmplen,"</argumentList>");
		}
		str = strcat_str(str, len, &tmplen, "</action>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
	i = 0;
	while(vars[i].name)
	{
		str = strcat_str(str, len, &tmplen,
				"<stateVariable sendEvents=\"");
#ifdef ENABLE_EVENTS
		str = strcat_str(str, len, &tmplen, (vars[i].itype & 0x80)?"yes":"no");
#else
		/* for the moment always send no. Wait for SUBSCRIBE implementation
		 * before setting it to yes */
		str = strcat_str(str, len, &tmplen, "no");
#endif
		str = strcat_str(str, len, &tmplen, "\"><name>");
		str = strcat_str(str, len, &tmplen, vars[i].name);
		str = strcat_str(str, len, &tmplen, "</name><dataType>");
		str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
		str = strcat_str(str, len, &tmplen, "</dataType>");
		if(vars[i].iallowedlist)
		{
		  if((vars[i].itype & 0x0f) == 0)
		  {
		    /* string */
		    str = strcat_str(str, len, &tmplen, "<allowedValueList>");
		    for(j=vars[i].iallowedlist; upnpallowedvalues[j]; j++)
		    {
		      str = strcat_str(str, len, &tmplen, "<allowedValue>");
		      str = strcat_str(str, len, &tmplen, upnpallowedvalues[j]);
		      str = strcat_str(str, len, &tmplen, "</allowedValue>");
		    }
		    str = strcat_str(str, len, &tmplen, "</allowedValueList>");
		  } else {
		    /* ui2 and ui4 */
		    str = strcat_str(str, len, &tmplen, "<allowedValueRange><minimum>");
			str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist]);
		    str = strcat_str(str, len, &tmplen, "</minimum><maximum>");
			str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist+1]);
		    str = strcat_str(str, len, &tmplen, "</maximum></allowedValueRange>");
		  }
		}
		/*if(vars[i].defaultValue) */
		if(vars[i].idefault)
		{
		  str = strcat_str(str, len, &tmplen, "<defaultValue>");
		  /*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
		  str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
		  str = strcat_str(str, len, &tmplen, "</defaultValue>");
		}
		str = strcat_str(str, len, &tmplen, "</stateVariable>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
	str[*len] = '\0';
	return str;
}

/* genWANIPCn() :
 * Generate the WANIPConnection xml description */
char *
genWANIPCn(int * len)
{
	return genServiceDesc(len, &scpdWANIPCn);
}

/* genWANCfg() :
 * Generate the WANInterfaceConfig xml description. */
char *
genWANCfg(int * len)
{
	return genServiceDesc(len, &scpdWANCfg);
}

#ifdef ENABLE_L3F_SERVICE
char *
genL3F(int * len)
{
	return genServiceDesc(len, &scpdL3F);
}
#endif

#ifdef ENABLE_6FC_SERVICE
char *
gen6FC(int * len)
{
	return genServiceDesc(len, &scpd6FC);
}
#endif

#ifdef ENABLE_DP_SERVICE
char *
genDP(int * len)
{
	return genServiceDesc(len, &scpdDP);
}
#endif

#ifdef ENABLE_EVENTS
static char *
genEventVars(int * len, const struct serviceDesc * s, const char * servns)
{
	char tmp[16];
	const struct stateVar * v;
	char * str;
	int tmplen;
	tmplen = 512;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	*len = 0;
	v = s->serviceStateTable;
    /* BEGIN   2082304944 zhoujianchun 00203875 2012.8.29 modified */
    str = strcat_str(str, len, &tmplen,
                "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\"");
    str = strcat_str(str, len, &tmplen, ">");

	while(v->name) {
		if(v->itype & 0x80) {
            str = strcat_str(str, len, &tmplen, "<e:property><");
    /* END   2082304944 zhoujianchun 00203875 2012.8.29 modified */
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, ">");
			//printf("<e:property><s:%s>", v->name);
			switch(v->ieventvalue) {
			case 0:
				break;
			case CONNECTIONSTATUS_MAGICALVALUE:
				/* or get_wan_connection_status_str(ext_if_name) */
				str = strcat_str(str, len, &tmplen,
				   upnpallowedvalues[18 + get_wan_connection_status(ext_if_name)]);
				break;
#ifdef ENABLE_6FC_SERVICE
			case FIREWALLENABLED_MAGICALVALUE:
				/* see 2.4.2 of UPnP-gw-WANIPv6FirewallControl-v1-Service.pdf */
				snprintf(tmp, sizeof(tmp), "%d",
				         ipv6fc_firewall_enabled);
				str = strcat_str(str, len, &tmplen, tmp);
				break;
			case INBOUNDPINHOLEALLOWED_MAGICALVALUE:
				/* see 2.4.3 of UPnP-gw-WANIPv6FirewallControl-v1-Service.pdf */
				snprintf(tmp, sizeof(tmp), "%d",
				         ipv6fc_inbound_pinhole_allowed);
				str = strcat_str(str, len, &tmplen, tmp);
				break;
#endif
#ifdef IGD_V2
			case SYSTEMUPDATEID_MAGICALVALUE:
				/* Please read section 2.3.23 SystemUpdateID
				 * of UPnP-gw-WANIPConnection-v2-Service.pdf */
				snprintf(tmp, sizeof(tmp), "%d",
				         1/* system update id */);
				str = strcat_str(str, len, &tmplen, tmp);
				break;
#endif
			case PORTMAPPINGNUMBEROFENTRIES_MAGICALVALUE:
				/* Port mapping number of entries magical value */
				snprintf(tmp, sizeof(tmp), "%d",
				         upnp_get_portmapping_number_of_entries());
				str = strcat_str(str, len, &tmplen, tmp);
				break;
			case EXTERNALIPADDRESS_MAGICALVALUE:
				/* External ip address magical value */
				if(use_ext_ip_addr)
					str = strcat_str(str, len, &tmplen, use_ext_ip_addr);
				else {
					char ext_ip_addr[INET_ADDRSTRLEN];
					if(getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN) < 0) {
						str = strcat_str(str, len, &tmplen, "0.0.0.0");
					} else {
						str = strcat_str(str, len, &tmplen, ext_ip_addr);
					}
				}
				break;
			case DEFAULTCONNECTIONSERVICE_MAGICALVALUE:
				/* DefaultConnectionService magical value */
				str = strcat_str(str, len, &tmplen, uuidvalue);
				str = strcat_str(str, len, &tmplen, ":WANConnectionDevice:1,urn:upnp-org:serviceId:WANIPConn1");
				break;
			default:
				str = strcat_str(str, len, &tmplen, upnpallowedvalues[v->ieventvalue]);
			}
            /* BEGIN 2082304944 zhoujianchun 00203875 2012.8.29 modified */
            str = strcat_str(str, len, &tmplen, "</");
            /* END   2082304944 zhoujianchun 00203875 2012.8.29 modified */
			str = strcat_str(str, len, &tmplen, v->name);
			str = strcat_str(str, len, &tmplen, "></e:property>");
			//printf("</s:%s></e:property>\n", v->name);
		}
		v++;
	}
	str = strcat_str(str, len, &tmplen, "</e:propertyset>");
	//printf("</e:propertyset>\n");
	//printf("\n");
	//printf("%d\n", tmplen);
	str[*len] = '\0';
	return str;
}

char *
getVarsWANIPCn(int * l)
{
	return genEventVars(l,
                        &scpdWANIPCn,
	                    SERVICE_TYPE_WANIPC);
}

char *
getVarsWANCfg(int * l)
{
	return genEventVars(l,
	                    &scpdWANCfg,
	                    "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1");
}

#ifdef ENABLE_L3F_SERVICE
char *
getVarsL3F(int * l)
{
	return genEventVars(l,
	                    &scpdL3F,
	                    "urn:schemas-upnp-org:service:Layer3Forwarding:1");
}
#endif

#ifdef ENABLE_6FC_SERVICE
char *
getVars6FC(int * l)
{
	return genEventVars(l,
	                    &scpd6FC,
	                    "urn:schemas-upnp-org:service:WANIPv6FirewallControl:1");
}
#endif

#ifdef ENABLE_DP_SERVICE
char *
getVarsDP(int * l)
{
	return genEventVars(l,
	                    &scpdDP,
	                    "urn:schemas-upnp-org:service:DeviceProtection:1");
}
#endif


#endif /* ENABLE_EVENTS */
