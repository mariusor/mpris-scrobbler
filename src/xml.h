/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_XML_H
#define MPRIS_SCROBBLER_XML_H

#include "api.h"

static void xml_attribute_free(struct xml_attribute *attr)
{
    if (NULL == attr) { return; }
    if (NULL != attr->name) {
        free(attr->name);
    }
    if (NULL != attr->value) {
        free(attr->value);
    }
    free(attr);
}

struct xml_attribute *xml_attribute_new(const char *name, size_t name_length, const char *value, size_t value_length)
{
    if (NULL == name) { return NULL; }
    if (NULL == value) { return NULL; }
    if (0 == name_length) { return NULL; }
    if (0 == value_length) { return NULL; }

    struct xml_attribute *attr = calloc(1, sizeof(struct xml_attribute));

    attr->name = calloc(1, sizeof(char) * (1 + name_length));
    strncpy (attr->name, name, name_length);

    attr->value = calloc(1, sizeof(char) * (1 + value_length));
    strncpy (attr->value, value, value_length);

    return attr;
}

static void xml_node_free(struct xml_node *node)
{
    if (NULL == node) { return; }

    _trace("xml::free_node(%p//%p):children: %u, attributes: %u", node, node->content, node->children_count, node->attributes_count);

    if (NULL != node->content) {
        free(node->content);
        node->content = NULL;
        node->content_length = 0;
    }
    if (NULL != node->name) {
        free(node->name);
        node->name_length = 0;
    }
    for (size_t i = 0; i < node->children_count; i++) {
        if (NULL != node->children[i]) {
            xml_node_free(node->children[i]);
            node->children_count--;
        }
    }
    for (size_t j = 0; j < node->attributes_count; j++) {
        if (NULL != node->attributes[j]) {
            xml_attribute_free(node->attributes[j]);
            node->attributes_count--;
        }
    }

    free(node);
}

struct xml_attribute *xml_node_get_attribute(struct xml_node *node, const char *key, size_t key_len)
{
    if (NULL == node) { return NULL; }
    if (node->attributes_count == 0) { return NULL; }
    for (size_t i = 0; i < node->attributes_count; i++) {
        struct xml_attribute *attr = node->attributes[i];
        if (strncmp(attr->name, key, key_len) == 0) {
            return attr;
        }
    }
    return NULL;
}

struct xml_node *xml_node_new(void)
{
    struct xml_node *node = malloc(sizeof(struct xml_node));
    node->parent = NULL;
    node->content = NULL;
    node->name = NULL;
    node->type = api_node_type_unknown;

    //*node->children = calloc(MAX_XML_NODES, sizeof(struct xml_node));
    //*node->attributes = calloc(MAX_XML_ATTRIBUTES, sizeof(struct xml_attribute));
#if 0
    for (size_t ni = 0; ni < MAX_XML_NODES; ni++) {
        node->children[ni] = malloc(sizeof(struct xml_node));
    }
    for (size_t ai = 0; ai < MAX_XML_ATTRIBUTES; ai++) {
        node->attributes[ai] = malloc(sizeof(struct xml_attribute));
    }
#endif

    node->attributes_count = 0;
    node->children_count = 0;
    node->content_length = 0;
    node->name_length = 0;

    return node;
}

struct xml_node *xml_document_new(void)
{
    const char *doc_type = "document";
    struct xml_node *doc = xml_node_new();
    doc->name = calloc(1, strlen(doc_type) + 1);
    strncpy(doc->name, doc_type, strlen(doc_type));
    doc->type = api_node_type_document;

    _trace("xml::doc_new(%p)", doc);
    return doc;
}

void xml_document_free(struct xml_node *doc)
{
    if (NULL == doc) { return; }
    xml_node_free(doc);
}

void xml_node_print (struct xml_node *node, short unsigned level)
{
    if (NULL == node) { return; }
    const char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    char *padding = calloc(1, sizeof(char) * (level + 1));
    strncpy(padding, tabs, level);

    _trace("%sxml::node<%s>(%p)::attr:%u,nodes:%u", padding, node->name, node, node->attributes_count, node->children_count);
    if (node->content_length > 0 && NULL != node->content) {
        _trace("%s\txml::node::content: \"%s\"", padding, node->content);
    }
    for (size_t ai = 0; ai < node->attributes_count; ai++) {
        struct xml_attribute *attr = node->attributes[ai];
        _trace("%s\txml::node::attribute(%p)[%u] %s = %s", padding, attr, ai, attr->name, attr->value);
    }
    for (size_t ni = 0; ni < node->children_count; ni++) {
        struct xml_node *n = node->children[ni];
        if (NULL != n) {
            xml_node_print(n, level+1);
        }
    }
    free(padding);
}

struct xml_node *xml_node_append_child(struct xml_node *to, struct xml_node *child)
{
    if (NULL == to) { return NULL; }
    if (NULL == child) { return NULL; }
    _trace("xml::append_child(%p)::to(%p)", child, to);

    if (to->children_count >= MAX_XML_NODES) { return NULL; }

    to->children[to->children_count] = child;
    to->children_count++;

    return child;
}

#endif // MPRIS_SCROBBLER_XML_H

