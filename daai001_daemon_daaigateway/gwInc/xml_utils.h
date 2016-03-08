#ifndef XML_UTILES_H
#define XML_UTILES_H

#include <stdbool.h>
#include "libroxml/inc/roxml.h"

# ifdef __cplusplus
extern "C" {
# endif

#ifdef WITH_LIBXML2
typedef XmlDoc xml_doc;
typedef XmlNode xml_node;
#else
typedef node_t xml_doc;
typedef node_t xml_node;
#endif

xml_node* xml_add_node(xml_node* n, int position, int type, char* name, char* value);
void xml_del_node(xml_node* n);

xml_node* xml_new_attr(xml_node* n, char* name, char* value);
xml_node* xml_has_attr(xml_node* n, char* name);

int xml_set_attr(xml_node* n, char* name, char* value);
int xml_set_content(xml_node* n, char* value);
int xml_set_name(xml_node* n, char* value);

char* xml_get_attr(xml_node* n, const char* name, char* buf, int len, int* size);
int xml_getAttributeFromRoot(xml_node* root, char* nodeXPath, char* attributeName, char* result, int resultSize);

int xml_get_path(xml_node* n, char* node_path);
xml_node** xml_get_chlds(xml_node* n, int* nb, int extended);
char* xml_get_content(xml_node* n, char* buffer, int size, int* cnt);

int xml_save_doc(xml_doc* root, char* file);

xml_node* xml_copy_node(xml_node* n, int extended);

void xml_add_chld(xml_node* parent, xml_node* child, int extended);

void xml_register_ns(xml_node* ctx, char* name, char* ns);

xml_node** xml_xpath(xml_node* n, char* path, int* count);

int xml_get_child_nb(xml_node* n, int extended);

xml_node* xml_load_doc(char* dirpath, char* filename);

xml_node* xml_load_buf(char* buffer);

char* xml_get_name(xml_node* n, char* buffer, int size);

xml_node* xml_get_chld(xml_node* n, char* name, int nth);

int xml_get_type(xml_node* n);

int xml_get_chld_nb(xml_node* n);
int xml_get_attr_nb(xml_node* n);

xml_node* xml_get_parent(xml_node* n);
xml_node* xml_get_next_sibling(xml_node* n);
xml_node* xml_getSibling(xml_node* currentNode, const char* siblingName);

void xml_close(xml_node* n);

void xml_release(void* data);

xml_node* xml_get_attrNode(xml_node* n, char* name, int nth);

# ifdef __cplusplus
}
#endif

#endif // XML_UTILS_H
