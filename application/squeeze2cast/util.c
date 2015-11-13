/*
 *  Squeeze2cast - LMS to Cast gateway
 *
 *  Squeezelite : (c) Adrian Smith 2012-2014, triode1@btinternet.com
 *  Additions & gateway : (c) Philippe 2014, philippe_44@outlook.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdarg.h>

#include "squeezedefs.h"
#include "util.h"
#include "util_common.h"
#include "upnptools.h"

/*----------------------------------------------------------------------------*/
/* globals */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* locals */
/*----------------------------------------------------------------------------*/
static log_level loglevel = lWARN;

/*----------------------------------------------------------------------------*/
void UtilInit(log_level level)
{
	loglevel = level;
}

/*----------------------------------------------------------------------------*/
void ExtractIP(const char *URL, in_addr_t *IP)
{
	int i;
	char*p1 = malloc(strlen(URL) + 1);
	char *p2;

	strcpy(p1, URL);
	p2 = strtok(p1,"/");
	p2 = strtok(NULL,"/");
	strtok(p2, ".");
	for (i = 0; i < 3; i++) {
		*((u8_t*) IP + i) = p2 ? atoi(p2) : 0;
		p2 = strtok(NULL, ".");
	}
	strtok(p2, ":");
	*((u8_t*) IP + 3) = p2 ? atoi(p2) : 0;
	free(p1);
}


/*----------------------------------------------------------------------------*/
const char *GetAppIdItem(json_t *root, char* appId, char *item)
{
	json_t *elm;
	int i;

	if ((elm = json_object_get(root, "status")) == NULL) return NULL;
	if ((elm = json_object_get(elm, "applications")) == NULL) return NULL;
	for (i = 0; i < json_array_size(elm); i++) {
		json_t *id, *data = json_array_get(elm, i);
		id = json_object_get(data, "appId");
		if (strcasecmp(json_string_value(id), appId)) continue;
		id = json_object_get(data, item);
		return json_string_value(id);
	}

	return NULL;
}


/*----------------------------------------------------------------------------*/
int GetMediaItem_I(json_t *root, int n, char *item)
{
	json_t *elm;

	if ((elm = json_object_get(root, "status")) == NULL) return 0;
	if ((elm = json_array_get(elm, n)) == NULL) return 0;
	if ((elm = json_object_get(elm, item)) == NULL) return 0;
	return json_integer_value(elm);
}


/*----------------------------------------------------------------------------*/
double GetMediaItem_F(json_t *root, int n, char *item)
{
	json_t *elm;

	if ((elm = json_object_get(root, "status")) == NULL) return 0;
	if ((elm = json_array_get(elm, n)) == NULL) return 0;
	if ((elm = json_object_get(elm, item)) == NULL) return 0;
	return json_real_value(elm);
}


/*----------------------------------------------------------------------------*/
const char *GetMediaItem_S(json_t *root, int n, char *item)
{
	json_t *elm;
	const char *str;

	if ((elm = json_object_get(root, "status")) == NULL) return NULL;
	elm = json_array_get(elm, n);
	elm = json_object_get(elm, item);
	str = json_string_value(elm);
	return str;
}


/*----------------------------------------------------------------------------*/
void QueueInit(tQueue *queue)
{
	queue->item = NULL;
}

/*----------------------------------------------------------------------------*/
void QueueInsert(tQueue *queue, void *item)
{
	while (queue->item)	queue = queue->next;
	queue->item = item;
	queue->next = malloc(sizeof(tQueue));
	queue->next->item = NULL;
}


/*----------------------------------------------------------------------------*/
void *QueueExtract(tQueue *queue)
{
	void *item = queue->item;
	tQueue *next = queue->next;

	if (item) {
		queue->item = next->item;
		if (next->item) queue->next = next->next;
		NFREE(next);
	}

	return item;
}

/*----------------------------------------------------------------------------*/
void QueueFlush(tQueue *queue)
{
	void *item = queue->item;
	tQueue *next = queue->next;

	queue->item = NULL;

	while (item) {
		next = queue->next;
		item = next->item;
		if (next->item) queue->next = next->next;
		NFREE(next);
	}
}


/*----------------------------------------------------------------------------*/
char *XMLGetFirstDocumentItem(IXML_Document *doc, const char *item)
{
	IXML_NodeList *nodeList = NULL;
	IXML_Node *textNode = NULL;
	IXML_Node *tmpNode = NULL;
	char *ret = NULL;

	nodeList = ixmlDocument_getElementsByTagName(doc, (char *)item);
	if (nodeList) {
		tmpNode = ixmlNodeList_item(nodeList, 0);
		if (tmpNode) {
			textNode = ixmlNode_getFirstChild(tmpNode);
			if (!textNode) {
				LOG_WARN("(BUG) ixmlNode_getFirstChild(tmpNode) returned NULL", NULL);
				ret = strdup("");
			}
			else {
				ret = strdup(ixmlNode_getNodeValue(textNode));
				if (!ret) {
					LOG_WARN("ixmlNode_getNodeValue returned NULL", NULL);
					ret = strdup("");
				}
			}
		} else
			LOG_WARN("ixmlNodeList_item(nodeList, 0) returned NULL", NULL);
	} else
		LOG_SDEBUG("Error finding %s in XML Node", item);

	if (nodeList) ixmlNodeList_free(nodeList);

	return ret;
}

/*----------------------------------------------------------------------------*/
char *XMLGetFirstElementItem(IXML_Element *element, const char *item)
{
	IXML_NodeList *nodeList = NULL;
	IXML_Node *textNode = NULL;
	IXML_Node *tmpNode = NULL;
	char *ret = NULL;

	nodeList = ixmlElement_getElementsByTagName(element, (char *)item);
	if (nodeList == NULL) {
		LOG_WARN("Error finding %s in XML Node", item);
		return NULL;
	}
	tmpNode = ixmlNodeList_item(nodeList, 0);
	if (!tmpNode) {
		LOG_WARN("Error finding %s value in XML Node", item);
		ixmlNodeList_free(nodeList);
		return NULL;
	}
	textNode = ixmlNode_getFirstChild(tmpNode);
	ret = strdup(ixmlNode_getNodeValue(textNode));
	if (!ret) {
		LOG_ERROR("Error allocating memory for %s in XML Node",item);
		ixmlNodeList_free(nodeList);
		return NULL;
	}
	ixmlNodeList_free(nodeList);

	return ret;
}


/*----------------------------------------------------------------------------*/
IXML_Node *XMLAddNode(IXML_Document *doc, IXML_Node *parent, char *name, char *fmt, ...)
{
	IXML_Node *node, *elm;
//	IXML_Element *elm;

	char buf[256];
	va_list args;

	elm = (IXML_Node*) ixmlDocument_createElement(doc, name);
	if (parent) ixmlNode_appendChild(parent, elm);
	else ixmlNode_appendChild((IXML_Node*) doc, elm);

	if (fmt) {
		va_start(args, fmt);

		vsprintf(buf, fmt, args);
		node = ixmlDocument_createTextNode(doc, buf);
		ixmlNode_appendChild(elm, node);

		va_end(args);
	}

	return elm;
}


/*----------------------------------------------------------------------------*/
int XMLAddAttribute(IXML_Document *doc, IXML_Node *parent, char *name, char *fmt, ...)
{
	char buf[256];
	int ret;
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, 256, fmt, args);
	ret = ixmlElement_setAttribute((IXML_Element*) parent, name, buf);
	va_end(args);

	return ret;
}


/*----------------------------------------------------------------------------*/
unsigned Time2Int(char *Time)
{
	char *p;
	unsigned ret;

	p = strrchr(Time, ':');

	if (!p) return 0;

	*p = '\0';
	ret = atol(p + 1);

	p = strrchr(Time, ':');
	if (p) {
		*p = '\0';
		ret += atol(p + 1)*60;
	}

	p = strrchr(Time, ':');
	if (p) {
		*p = '\0';
		ret += atol(p + 1)*3600;
	}

	return ret;
}


/*---------------------------------------------------------------------------*/
static char *format2ext(u8_t format)
{
	switch(format) {
		case 'p': return "wav";
		case 'm': return "mp3";
		case 'f': return "flac";
		case 'w': return "wma";
		case 'o': return "ogg";
		case 'a':
		case 'l': return "m4a";
		default: return "xxx";
	}
}



{
	if (!ext) return ' ';

	if (strstr(ext, "wav")) return 'w';
	if (strstr(ext, "flac") || strstr(ext, "flc")) return 'f';
	if (strstr(ext, "mp3")) return 'm';
	if (strstr(ext, "wma")) return 'a';
	if (strstr(ext, "ogg")) return 'o';
	if (strstr(ext, "m4a")) return '4';
	if (strstr(ext, "mp4")) return '4';
	if (strstr(ext, "aif")) return 'i';

	return ' ';
}



bool SetContentType(struct sMR *Device, sq_seturi_t *uri)
{
	strcpy(uri->ext, format2ext(uri->codec));

	switch (uri->codec) {
	case 'm': return  strcpy(uri->content_type, "audio/mpeg");
	case 'f': return  strcpy(uri->content_type, "audio/flac");
	case 'w': return  strcpy(uri->content_type, "audio/wma");
	case 'o': return  strcpy(uri->content_type, "audio/ogg");
	case 'a': return  strcpy(uri->content_type, "audio/aac");
	case 'l': return  strcpy(uri->content_type, "audio/m4a");
	//case 'p': return  sprintf(uri->content_type, "audio/L%d;channels=%d;rate=%d", uri->sample_size, uri->channels, uri->sample_rate);
	case 'p': return  strcpy(uri->content_type, "audio/wav");;
	default:
		strcpy(uri->content_type, "unknown");
		strcpy(uri->proto_info, "");
		return false;
	}
}



/*----------------------------------------------------------------------------*/
void SaveConfig(char *name, void *ref, bool full)
{
	struct sMR *p;
	IXML_Document *doc = ixmlDocument_createDocument();
	IXML_Document *old_doc = ref;
	IXML_Node	 *root, *common;
	IXML_NodeList *list;
	IXML_Element *old_root;
	char *s;
	FILE *file;
	int i;

	old_root = ixmlDocument_getElementById(old_doc, "squeeze2cast");

	if (!full && old_doc) {
		root = ixmlNode_cloneNode((IXML_Node*) old_root, true);
		ixmlNode_appendChild((IXML_Node*) doc, root);

		list = ixmlDocument_getElementsByTagName((IXML_Document*) root, "device");
		for (i = 0; i < (int) ixmlNodeList_length(list); i++) {
			IXML_Node *device;

			device = ixmlNodeList_item(list, i);
			ixmlNode_removeChild(root, device, &device);
			ixmlNode_free(device);
		}
		if (list) ixmlNodeList_free(list);
	}
	else {
		root = XMLAddNode(doc, NULL, "squeeze2cast", NULL);

		XMLAddNode(doc, root, "server", glSQServer);
		XMLAddNode(doc, root, "upnp_socket", gluPNPSocket);
		XMLAddNode(doc, root, "base_mac", "%02x:%02x:%02x:%02x:%02x:%02x", glMac[0],
					glMac[1], glMac[2], glMac[3], glMac[4], glMac[5]);
		XMLAddNode(doc, root, "slimproto_log", level2debug(glLog.slimproto));
		XMLAddNode(doc, root, "stream_log", level2debug(glLog.stream));
		XMLAddNode(doc, root, "output_log", level2debug(glLog.output));
		XMLAddNode(doc, root, "decode_log", level2debug(glLog.decode));
		XMLAddNode(doc, root, "web_log", level2debug(glLog.web));
		XMLAddNode(doc, root, "upnp_log", level2debug(glLog.upnp));
		XMLAddNode(doc, root, "main_log",level2debug(glLog.main));
		XMLAddNode(doc, root, "sq2mr_log", level2debug(glLog.sq2mr));
		XMLAddNode(doc, root, "upnp_scan_interval", "%d", (u32_t) gluPNPScanInterval);
		XMLAddNode(doc, root, "upnp_scan_timeout", "%d", (u32_t) gluPNPScanTimeout);
		XMLAddNode(doc, root, "log_limit", "%d", (s32_t) glLogLimit);

		common = XMLAddNode(doc, root, "common", NULL);
		XMLAddNode(doc, common, "streambuf_size", "%d", (u32_t) glDeviceParam.stream_buf_size);
		XMLAddNode(doc, common, "output_size", "%d", (u32_t) glDeviceParam.output_buf_size);
		XMLAddNode(doc, common, "buffer_dir", glDeviceParam.buffer_dir);
		XMLAddNode(doc, common, "buffer_limit", "%d", (u32_t) glDeviceParam.buffer_limit);
		XMLAddNode(doc, common, "max_read_wait", "%d", (int) glDeviceParam.max_read_wait);
		XMLAddNode(doc, common, "max_GET_bytes", "%d", (s32_t) glDeviceParam.max_get_bytes);
		XMLAddNode(doc, common, "keep_buffer_file", "%d", (int) glDeviceParam.keep_buffer_file);
		XMLAddNode(doc, common, "enabled", "%d", (int) glMRConfig.Enabled);
		XMLAddNode(doc, common, "codecs", glDeviceParam.codecs);
		XMLAddNode(doc, common, "sample_rate", "%d", (int) glDeviceParam.sample_rate);
		XMLAddNode(doc, common, "L24_format", "%d", (int) glDeviceParam.L24_format);
		XMLAddNode(doc, common, "flac_header", "%d", (int) glDeviceParam.flac_header);
		XMLAddNode(doc, common, "send_icy", "%d", (int) glDeviceParam.send_icy);
		XMLAddNode(doc, common, "volume_on_play", "%d", (int) glMRConfig.VolumeOnPlay);
		XMLAddNode(doc, common, "send_metadata", "%d", (int) glMRConfig.SendMetaData);
		XMLAddNode(doc, common, "send_coverart", "%d", (int) glMRConfig.SendCoverArt);
		XMLAddNode(doc, common, "upnp_remove_count", "%d", (u32_t) glMRConfig.UPnPRemoveCount);
		XMLAddNode(doc, common, "pause_volume", "%d", (int) glMRConfig.PauseVolume);
		XMLAddNode(doc, common, "auto_play", "%d", (int) glMRConfig.AutoPlay);
	}

	for (i = 0; i < MAX_RENDERERS; i++) {
		IXML_Node *dev_node;

		if (!glMRDevices[i].InUse) continue;
		else p = &glMRDevices[i];

		if (old_doc && ((dev_node = (IXML_Node*) FindMRConfig(old_doc, p->UDN)) != NULL)) {
			dev_node = ixmlNode_cloneNode(dev_node, true);
			ixmlNode_appendChild((IXML_Node*) doc, root);
		}
		else {
			dev_node = XMLAddNode(doc, root, "device", NULL);
			XMLAddNode(doc, dev_node, "udn", p->UDN);
			XMLAddNode(doc, dev_node, "name", *(p->Config.Name) ? p->Config.Name : p->FriendlyName);
			XMLAddNode(doc, dev_node, "mac", "%02x:%02x:%02x:%02x:%02x:%02x", p->sq_config.mac[0],
						p->sq_config.mac[1], p->sq_config.mac[2], p->sq_config.mac[3], p->sq_config.mac[4], p->sq_config.mac[5]);
			XMLAddNode(doc, dev_node, "enabled", "%d", (int) p->Config.Enabled);
		}
	}

	// add devices in old XML file that has not been discovered
	list = ixmlDocument_getElementsByTagName((IXML_Document*) old_root, "device");
	for (i = 0; i < (int) ixmlNodeList_length(list); i++) {
		char *udn;
		IXML_Node *device, *node;

		device = ixmlNodeList_item(list, i);
		node = (IXML_Node*) ixmlDocument_getElementById((IXML_Document*) device, "udn");
		node = ixmlNode_getFirstChild(node);
		udn = (char*) ixmlNode_getNodeValue(node);
		if (!FindMRConfig(doc, udn)) {
			device = ixmlNode_cloneNode(device, true);
			ixmlNode_appendChild((IXML_Node*) root, device);
		}
	}
	if (list) ixmlNodeList_free(list);

	file = fopen(name, "wb");
	s = ixmlDocumenttoString(doc);
	fwrite(s, 1, strlen(s), file);
	fclose(file);
	free(s);

	ixmlDocument_free(doc);
}

/*----------------------------------------------------------------------------*/
static void LoadConfigItem(tMRConfig *Conf, sq_dev_param_t *sq_conf, char *name, char *val)
{
	if (!strcmp(name, "streambuf_size")) sq_conf->stream_buf_size = atol(val);
	if (!strcmp(name, "output_size")) sq_conf->output_buf_size = atol(val);
	if (!strcmp(name, "buffer_dir")) strcpy(sq_conf->buffer_dir, val);
	if (!strcmp(name, "buffer_limit")) sq_conf->buffer_limit = atol(val);
	if (!strcmp(name, "stream_length")) Conf->StreamLength = atol(val);
	if (!strcmp(name, "max_read_wait")) sq_conf->max_read_wait = atol(val);
	if (!strcmp(name, "max_GET_bytes")) sq_conf->max_get_bytes = atol(val);
	if (!strcmp(name, "send_icy")) sq_conf->send_icy = atol(val);
	if (!strcmp(name, "enabled")) Conf->Enabled = atol(val);
	if (!strcmp(name, "codecs")) strcpy(sq_conf->codecs, val);
	if (!strcmp(name, "sample_rate"))sq_conf->sample_rate = atol(val);
	if (!strcmp(name, "L24_format"))sq_conf->L24_format = atol(val);
	if (!strcmp(name, "flac_header"))sq_conf->flac_header = atol(val);
	if (!strcmp(name, "keep_buffer_file"))sq_conf->keep_buffer_file = atol(val);
	if (!strcmp(name, "upnp_remove_count"))Conf->UPnPRemoveCount = atol(val);
	if (!strcmp(name, "volume_on_play")) Conf->VolumeOnPlay = atol(val);
	if (!strcmp(name, "pause_volume")) Conf->PauseVolume = atol(val);
	if (!strcmp(name, "auto_play")) Conf->AutoPlay = atol(val);
	if (!strcmp(name, "send_metadata")) Conf->SendMetaData = atol(val);
	if (!strcmp(name, "send_coverart")) Conf->SendCoverArt = atol(val);
	if (!strcmp(name, "name")) strcpy(Conf->Name, val);
	if (!strcmp(name, "mac"))  sscanf(val,"%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
								   &sq_conf->mac[0],&sq_conf->mac[1],&sq_conf->mac[2],
								   &sq_conf->mac[3],&sq_conf->mac[4],&sq_conf->mac[5]);
}

/*----------------------------------------------------------------------------*/
static void LoadGlobalItem(char *name, char *val)
{
	if (!val) return;

	if (!strcmp(name, "server")) strcpy(glSQServer, val);
	if (!strcmp(name, "upnp_socket")) strcpy(gluPNPSocket, val);
	if (!strcmp(name, "slimproto_log")) glLog.slimproto = debug2level(val);
	if (!strcmp(name, "stream_log")) glLog.stream = debug2level(val);
	if (!strcmp(name, "output_log")) glLog.output = debug2level(val);
	if (!strcmp(name, "decode_log")) glLog.decode = debug2level(val);
	if (!strcmp(name, "web_log")) glLog.web = debug2level(val);
	if (!strcmp(name, "upnp_log")) glLog.upnp = debug2level(val);
	if (!strcmp(name, "main_log")) glLog.main = debug2level(val);
	if (!strcmp(name, "sq2mr_log")) glLog.sq2mr = debug2level(val);
	if (!strcmp(name, "base_mac"))  sscanf(val,"%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
								   &glMac[0],&glMac[1],&glMac[2],&glMac[3],&glMac[4],&glMac[5]);
	if (!strcmp(name, "upnp_scan_interval")) gluPNPScanInterval = atol(val);
	if (!strcmp(name, "upnp_scan_timeout")) gluPNPScanTimeout = atol(val);
	if (!strcmp(name, "log_limit")) glLogLimit = atol(val);
 }


/*----------------------------------------------------------------------------*/
void *FindMRConfig(void *ref, char *UDN)
{
	IXML_Element *elm;
	IXML_Node	*device = NULL;
	IXML_NodeList *l1_node_list;
	IXML_Document *doc = (IXML_Document*) ref;
	char *v;
	unsigned i;

	elm = ixmlDocument_getElementById(doc, "squeeze2cast");
	l1_node_list = ixmlDocument_getElementsByTagName((IXML_Document*) elm, "udn");
	for (i = 0; i < ixmlNodeList_length(l1_node_list); i++) {
		IXML_Node *l1_node, *l1_1_node;
		l1_node = ixmlNodeList_item(l1_node_list, i);
		l1_1_node = ixmlNode_getFirstChild(l1_node);
		v = (char*) ixmlNode_getNodeValue(l1_1_node);
		if (v && !strcmp(v, UDN)) {
			device = ixmlNode_getParentNode(l1_node);
			break;
		}
	}
	if (l1_node_list) ixmlNodeList_free(l1_node_list);
	return device;
}

/*----------------------------------------------------------------------------*/
void *LoadMRConfig(void *ref, char *UDN, tMRConfig *Conf, sq_dev_param_t *sq_conf)
{
	IXML_NodeList *node_list;
	IXML_Document *doc = (IXML_Document*) ref;
	IXML_Node *node;
	char *n, *v;
	unsigned i;

	node = (IXML_Node*) FindMRConfig(doc, UDN);
	if (node) {
		node_list = ixmlNode_getChildNodes(node);
		for (i = 0; i < ixmlNodeList_length(node_list); i++) {
			IXML_Node *l1_node, *l1_1_node;
			l1_node = ixmlNodeList_item(node_list, i);
			n = (char*) ixmlNode_getNodeName(l1_node);
			l1_1_node = ixmlNode_getFirstChild(l1_node);
			v = (char*) ixmlNode_getNodeValue(l1_1_node);
			LoadConfigItem(Conf, sq_conf, n, v);
		}
		if (node_list) ixmlNodeList_free(node_list);
	}

	return node;
}

/*----------------------------------------------------------------------------*/
void *LoadConfig(char *name, tMRConfig *Conf, sq_dev_param_t *sq_conf)
{
	IXML_Element *elm;
	IXML_Document	*doc;

	doc = ixmlLoadDocument(name);
	if (!doc) return NULL;

	elm = ixmlDocument_getElementById(doc, "squeeze2cast");
	if (elm) {
		unsigned i;
		char *n, *v;
		IXML_NodeList *l1_node_list;
		l1_node_list = ixmlNode_getChildNodes((IXML_Node*) elm);
		for (i = 0; i < ixmlNodeList_length(l1_node_list); i++) {
			IXML_Node *l1_node, *l1_1_node;
			l1_node = ixmlNodeList_item(l1_node_list, i);
			n = (char*) ixmlNode_getNodeName(l1_node);
			l1_1_node = ixmlNode_getFirstChild(l1_node);
			v = (char*) ixmlNode_getNodeValue(l1_1_node);
			LoadGlobalItem(n, v);
		}
		if (l1_node_list) ixmlNodeList_free(l1_node_list);
	}

	elm = ixmlDocument_getElementById((IXML_Document	*)elm, "common");
	if (elm) {
		char *n, *v;
		IXML_NodeList *l1_node_list;
		unsigned i;
		l1_node_list = ixmlNode_getChildNodes((IXML_Node*) elm);
		for (i = 0; i < ixmlNodeList_length(l1_node_list); i++) {
			IXML_Node *l1_node, *l1_1_node;
			l1_node = ixmlNodeList_item(l1_node_list, i);
			n = (char*) ixmlNode_getNodeName(l1_node);
			l1_1_node = ixmlNode_getFirstChild(l1_node);
			v = (char*) ixmlNode_getNodeValue(l1_1_node);
			LoadConfigItem(&glMRConfig, &glDeviceParam, n, v);
		}
		if (l1_node_list) ixmlNodeList_free(l1_node_list);
	}

	return doc;
 }


/*----------------------------------------------------------------------------*/
char *uPNPEvent2String(Upnp_EventType S)
{
	switch (S) {
	/* Discovery */
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
		return "UPNP_DISCOVERY_ADVERTISEMENT_ALIVE";
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		return "UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE";
	case UPNP_DISCOVERY_SEARCH_RESULT:
		return "UPNP_DISCOVERY_SEARCH_RESULT";
	case UPNP_DISCOVERY_SEARCH_TIMEOUT:
		return "UPNP_DISCOVERY_SEARCH_TIMEOUT";
	/* SOAP */
	case UPNP_CONTROL_ACTION_REQUEST:
		return "UPNP_CONTROL_ACTION_REQUEST";
	case UPNP_CONTROL_ACTION_COMPLETE:
		return "UPNP_CONTROL_ACTION_COMPLETE";
	case UPNP_CONTROL_GET_VAR_REQUEST:
		return "UPNP_CONTROL_GET_VAR_REQUEST";
	case UPNP_CONTROL_GET_VAR_COMPLETE:
		return "UPNP_CONTROL_GET_VAR_COMPLETE";
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		return "UPNP_EVENT_SUBSCRIPTION_REQUEST";
	case UPNP_EVENT_RECEIVED:
		return "UPNP_EVENT_RECEIVED";
	case UPNP_EVENT_RENEWAL_COMPLETE:
		return "UPNP_EVENT_RENEWAL_COMPLETE";
	case UPNP_EVENT_SUBSCRIBE_COMPLETE:
		return "UPNP_EVENT_SUBSCRIBE_COMPLETE";
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		return "UPNP_EVENT_UNSUBSCRIBE_COMPLETE";
	case UPNP_EVENT_AUTORENEWAL_FAILED:
		return "UPNP_EVENT_AUTORENEWAL_FAILED";
	case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
		return "UPNP_EVENT_SUBSCRIPTION_EXPIRED";
	}

	return "";
}


